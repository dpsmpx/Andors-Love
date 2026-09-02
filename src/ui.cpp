#include "ui.h"

#include "platform.h"

#include <algorithm>
#include <iostream>

namespace ui {
namespace {

int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Раскладка экрана считается заново перед каждой отрисовкой: на настольном
// терминале панель героя стоит справа от карты, на узком экране телефона она
// уезжает под карту, а окно обзора сжимается. Иначе 88 колонок игрового
// экрана сворачивались бы переносами в нечитаемую кашу.
struct Layout {
    int  cols = 80;
    int  rows = 24;
    int  view_w = 48;
    int  view_h = 18;
    bool side = true;    // панель сбоку, а не снизу
    int  rule = 60;      // ширина разделительных линий
    int  bar  = 12;      // длина полосок HP/AP
};

Layout layout() {
    Layout L;
    platform::term_size(&L.cols, &L.rows);
    // Панель сбоку требует 48 карты + рамка + ~30 панели.
    L.side = (L.cols >= 80 && L.rows >= 24);
    if (L.side) {
        L.view_w = 48;
        L.view_h = 18;
        L.bar    = 12;
    } else {
        L.view_w = clampi(L.cols - 2, 20, 48);
        // Место под заголовок, рамки, компактную панель и журнал.
        L.view_h = clampi(L.rows - 12, 8, 18);
        L.bar    = clampi(L.cols / 4, 6, 12);
    }
    L.rule = clampi(L.cols - 1, 20, 64);
    return L;
}

std::string bar(int cur, int max, int width, char full = '#', char empty = '.') {
    if (width < 1) width = 1;
    if (max <= 0) return std::string(static_cast<std::size_t>(width), empty);
    cur = clampi(cur, 0, max);
    int n = cur * width / max;
    if (n == 0 && cur > 0) n = 1;               // остаток здоровья всегда виден
    return std::string(static_cast<std::size_t>(n), full) +
           std::string(static_cast<std::size_t>(width - n), empty);
}

void out(const std::string& s) { std::cout << s << '\n'; }
void rule(int w) { out(std::string(static_cast<std::size_t>(w), '-')); }

// Кадр карты. Слои пишутся в порядке из дизайн-документа: тайл, койка,
// переход, табличка, предмет, моб, NPC, игрок.
std::vector<std::string> render_map(Game& g, const Location& loc, int vw, int vh) {
    int ox = g.player().pos.x - vw / 2;
    int oy = g.player().pos.y - vh / 2;
    ox = std::max(0, std::min(ox, std::max(0, loc.w - vw)));
    oy = std::max(0, std::min(oy, std::max(0, loc.h - vh)));

    std::vector<std::string> rows;
    for (int y = 0; y < vh; ++y) {
        std::string row;
        for (int x = 0; x < vw; ++x) {
            Vec2 p{ox + x, oy + y};
            char ch = ' ';
            if (loc.in_bounds(p)) {
                ch = tile_glyph(loc.at(p));
                if (loc.bed_at(p))  ch = glyph::BED;
                int ci = loc.chest_index_at(p);
                if (ci >= 0 && !g.chest_opened(loc.id, ci)) ch = glyph::CHEST;
                if (g.portal_at(p, g.player().loc))         ch = glyph::PORTAL;
                if (loc.exit_at(p)) ch = glyph::EXIT;
                if (loc.sign_at(p)) ch = glyph::SIGN;
                int ii = loc.item_index_at(p);
                if (ii >= 0 && !g.item_taken(loc.id, ii)) ch = glyph::ITEM;
                if (g.mob_at(p, g.player().loc)) ch = glyph::MOB;
                if (loc.npc_at(p))               ch = glyph::NPC;
                if (p == g.player().pos)         ch = glyph::PLAYER;
            }
            row += ch;
        }
        rows.push_back(row);
    }
    return rows;
}

// Полная панель — для широкого экрана, столбцом справа от карты.
std::vector<std::string> panel_wide(Game& g, const Layout& L) {
    const Player& p = g.player();
    Stats t = g.total();
    std::vector<std::string> V;

    V.push_back(p.name + ", ур." + to_str(p.level));
    V.push_back("HP " + pad(to_str(p.hp) + "/" + to_str(t.max_hp), 8) + bar(p.hp, t.max_hp, L.bar));
    V.push_back("AP " + pad(to_str(p.ap) + "/" + to_str(t.max_ap), 8) + bar(p.ap, t.max_ap, L.bar));
    V.push_back("Опыт " + to_str(p.exp) + "/" + to_str(g.exp_to_next()));
    V.push_back("Золото " + to_str(p.gold));
    V.push_back("");
    V.push_back("Стойка: " + std::string(stance_name(p.stance)));
    V.push_back("Кураж:  " + bar(p.momentum, MOMENTUM_MAX, MOMENTUM_MAX, '*', '-') +
                " " + to_str(p.momentum) + "/" + to_str(MOMENTUM_MAX));
    if (p.skill_points > 0) V.push_back("Очков навыка: " + to_str(p.skill_points) + " (K)");
    if (!p.effects.empty())
        V.push_back("Эффекты: " + trunc(Game::effects_line(p.effects), 20));
    V.push_back("");
    V.push_back("Урон " + to_str(t.dmg_min) + "-" + to_str(t.dmg_max) +
                "  меткость " + to_str(t.attack) + "%");
    V.push_back("Блок " + to_str(t.block) + "%  броня " + to_str(t.armor) +
                "  крит " + to_str(t.crit) + "%");
    V.push_back("");
    V.push_back("--- снаряжение ---");
    for (int i = 0; i < static_cast<int>(Slot::Count); ++i) {
        const std::string& id = p.equipped[static_cast<std::size_t>(i)];
        std::string nm = "—";
        if (!id.empty())
            if (const ItemDef* d = Content::get().item(id)) nm = d->name;
        V.push_back(pad(slot_name(static_cast<Slot>(i)), 8) + trunc(nm, 20));
    }
    return V;
}

// Сжатая панель — для телефона, тремя строками под картой.
std::vector<std::string> panel_narrow(Game& g, const Layout& L) {
    const Player& p = g.player();
    Stats t = g.total();
    std::vector<std::string> V;

    V.push_back(trunc(p.name, 12) + " ур." + to_str(p.level) +
                "  HP " + to_str(p.hp) + "/" + to_str(t.max_hp) +
                " " + bar(p.hp, t.max_hp, L.bar));
    V.push_back("AP " + to_str(p.ap) + "/" + to_str(t.max_ap) +
                "  Оп " + to_str(p.exp) + "/" + to_str(g.exp_to_next()) +
                "  Зол " + to_str(p.gold) +
                (p.skill_points > 0 ? "  +" + to_str(p.skill_points) + " нав.(K)" : ""));
    V.push_back(std::string(stance_name(p.stance)) +
                "  Кураж " + bar(p.momentum, MOMENTUM_MAX, MOMENTUM_MAX, '*', '-') +
                "  Ур " + to_str(t.dmg_min) + "-" + to_str(t.dmg_max) +
                " Мет " + to_str(t.attack) + "% Бр " + to_str(t.armor));
    if (!p.effects.empty()) V.push_back("Эфф: " + Game::effects_line(p.effects));
    return V;
}

} // namespace

void draw_world(Game& g) {
    const Layout L = layout();
    const Location* loc = g.here();
    platform::clear_screen();
    if (!loc) {
        out("Локация не загружена: " + g.world().last_error());
        return;
    }

    std::vector<std::string> map = render_map(g, *loc, L.view_w, L.view_h);
    const std::string border = "+" + std::string(static_cast<std::size_t>(L.view_w), '-') + "+";

    if (L.side) {
        std::string head = loc->name + "  ·  ход " + to_str(g.turn()) + "  ·  ";
        head += (L.cols >= 92) ? "[?] справка  [C] герой  [I] сумка  [Q] квесты  [K] навыки"
                               : "[?]спр [C]герой [I]сумка [Q]квест [K]нав";
        out(trunc(head, static_cast<std::size_t>(L.cols)));
        std::vector<std::string> pan = panel_wide(g, L);
        out(border);
        for (std::size_t i = 0; i < map.size(); ++i)
            out("|" + map[i] + "|  " + (i < pan.size() ? pan[i] : std::string()));
        out(border);
    } else {
        out(trunc(loc->name, static_cast<std::size_t>(L.cols) - 12) + " · ход " + to_str(g.turn()));
        out(border);
        for (const std::string& r : map) out("|" + r + "|");
        out(border);
        for (const std::string& r : panel_narrow(g, L)) out(trunc(r, static_cast<std::size_t>(L.cols)));
        out(trunc(L.cols >= 42 ? "[?]спр [C]герой [I]сумка [Q]квест [K]нав"
                               : "[?] [C]герой [I]сумка [Q]квест [K]нав",
                  static_cast<std::size_t>(L.cols)));
    }

    const std::vector<std::string>& lg = g.log();
    const std::size_t show = L.side ? 4 : 3;
    std::size_t from = lg.size() > show ? lg.size() - show : 0;
    for (std::size_t i = from; i < lg.size(); ++i)
        out("  " + trunc(lg[i], static_cast<std::size_t>(L.cols) - 2));
    for (std::size_t i = lg.size() - from; i < show; ++i) out("");
    std::cout.flush();
}

int choose(const std::string& title, const std::vector<std::string>& items,
           const std::string& footer, const std::vector<int>& hotkeys, int* hotkey_out) {
    if (items.empty()) return CHOOSE_CANCEL;
    int sel = 0;
    for (;;) {
        const Layout L = layout();
        platform::clear_screen();
        for (const std::string& t : wrap(title, static_cast<std::size_t>(L.cols))) out(t);
        rule(L.rule);
        for (std::size_t i = 0; i < items.size(); ++i)
            out((static_cast<int>(i) == sel ? " > " : "   ") +
                trunc(items[i], static_cast<std::size_t>(L.cols) - 4));
        rule(L.rule);
        out(trunc(footer.empty() ? "  ^v выбор · Enter принять · Esc назад" : footer,
                  static_cast<std::size_t>(L.cols)));
        std::cout.flush();

        int k = platform::read_key();
        for (int h : hotkeys) {
            if (k != h) continue;
            if (hotkey_out) *hotkey_out = k;
            return CHOOSE_HOTKEY;
        }
        switch (k) {
            case platform::KEY_UP:   case 'w': case 'W':
                sel = (sel + static_cast<int>(items.size()) - 1) % static_cast<int>(items.size()); break;
            case platform::KEY_DOWN: case 's': case 'S':
                sel = (sel + 1) % static_cast<int>(items.size()); break;
            case platform::KEY_ENTER: case '\r': case platform::KEY_SPACE: return sel;
            case platform::KEY_EOF:
            case platform::KEY_ESC: case 'q': case 'Q': return CHOOSE_CANCEL;
            default:
                if (k >= '1' && k <= '9') {
                    int idx = k - '1';
                    if (idx < static_cast<int>(items.size())) return idx;
                }
                break;
        }
    }
}

std::string read_line(const std::string& prompt, const std::string& def) {
    std::string buf;
    for (;;) {
        const Layout L = layout();
        const std::size_t w = static_cast<std::size_t>(L.cols);
        platform::clear_screen();
        for (const std::string& t : wrap(prompt, w)) out(t);
        out("");
        out(trunc("  " + (buf.empty() ? def + "  (по умолчанию)" : buf) + "_", w));
        out("");
        out(trunc(L.cols >= 40 ? "  Enter — принять, Esc — по умолчанию"
                               : "  Enter — ок, Esc — по умолч.", w));
        std::cout.flush();

        int k = platform::read_key();
        if (k == platform::KEY_ENTER || k == '\r') break;
        if (k == platform::KEY_ESC || k == platform::KEY_EOF) { buf.clear(); break; }
        if (k == 127 || k == 8) {
            // Срезаем целый UTF-8 символ, а не байт.
            while (!buf.empty() && (static_cast<unsigned char>(buf.back()) & 0xC0) == 0x80)
                buf.pop_back();
            if (!buf.empty()) buf.pop_back();
            continue;
        }
        if (k >= 32 && k < 1000 && utf8_len(buf) < 20) buf += static_cast<char>(k);
    }
    return buf.empty() ? def : buf;
}

bool screen_create_hero(std::string* name, std::string* race, std::string* spec) {
    const Content& c = Content::get();

    *name = read_line("Как тебя звать?", "Странник");

    std::vector<std::string> rows;
    for (const RaceDef& r : c.races())
        rows.push_back(pad(r.name, 10) + r.desc);
    int sel = choose("Выбери расу\n", rows, "  ^v выбор · Enter принять · Esc отмена");
    if (sel < 0) return false;
    *race = c.races()[static_cast<std::size_t>(sel)].id;

    rows.clear();
    for (const SpecDef& sp : c.specs()) {
        std::string start;
        if (const ItemDef* d = c.item(sp.start_item)) start = " [" + d->name + "]";
        rows.push_back(pad(sp.name, 10) + sp.desc + start);
    }
    sel = choose("Выбери путь\n", rows, "  ^v выбор · Enter принять · Esc отмена");
    if (sel < 0) return false;
    *spec = c.specs()[static_cast<std::size_t>(sel)].id;
    return true;
}

void message_box(const std::string& title, const std::string& body) {
    const Layout L = layout();
    platform::clear_screen();
    for (const std::string& t : wrap(title, static_cast<std::size_t>(L.cols))) out(t);
    rule(L.rule);
    for (const std::string& t : wrap(body, static_cast<std::size_t>(L.cols))) out(t);
    rule(L.rule);
    out("  Любая клавиша — далее");
    std::cout.flush();
    platform::read_key();
}

void help_screen() {
    message_box("Управление",
        "  Стрелки или WASD — идти\n"
        "  C — герой        I — сумка       Q — квесты      K — навыки\n"
        "  F — эффекты      P — порталы\n"
        "  1 2 3            — стойка: осторожная / ровная / яростная\n"
        "  ?                — эта справка\n"
        "  Esc              — пауза: сохранить, загрузить, выйти\n"
        "\n"
        "В бою:\n"
        "  A — атака        P — мощный удар (кураж " + to_str(MOMENTUM_COST) + ")\n"
        "  U — предмет      E — закончить ход              F — бежать\n"
        "  1 2 3            — сменить стойку (раз за раунд)\n"
        "\n"
        "Знаки карты:\n"
        "  @ ты        N житель — шаг к нему заводит разговор\n"
        "  X враг      шаг к нему начинает бой\n"
        "  > переход   ! табличка   * предмет   & лежанка\n"
        "  C сундук    O портал (поставленный тобой)\n"
        "  # стена     T дерево     ~ вода      . , = земля\n"
        "\n"
        "Все враги показаны одним знаком, все жители — другим:\n"
        "в текстовом режиме символов на всех не хватит. Кто перед\n"
        "тобой, видно по имени в диалоге и в окне боя.");
}

// ------------------------------------------------------------------ герой

void screen_character(Game& g) {
    const Player& p = g.player();
    Stats t  = g.total();
    Stats nb = g.total_no_stance();

    std::string s;
    s += "  " + p.name + ", уровень " + to_str(p.level) + "\n";
    s += "  Опыт: " + to_str(p.exp) + " / " + to_str(g.exp_to_next()) + "\n";
    s += "  Золото: " + to_str(p.gold) + "\n\n";
    s += "  Здоровье      " + to_str(p.hp) + " / " + to_str(t.max_hp) + "\n";
    s += "  Очки действия " + to_str(p.ap) + " / " + to_str(t.max_ap) + "\n";
    s += "  Урон          " + to_str(t.dmg_min) + " - " + to_str(t.dmg_max) + "\n";
    s += "  Меткость      " + to_str(t.attack) + "%\n";
    s += "  Блок          " + to_str(t.block) + "%\n";
    s += "  Броня         " + to_str(t.armor) + "\n";
    s += "  Крит          " + to_str(t.crit) + "%\n";
    s += "  Атака стоит   " + to_str(g.attack_cost()) + " AP\n\n";
    s += "  Стойка: " + std::string(stance_name(p.stance)) + "\n";
    s += "  (" + std::string(stance_hint(p.stance)) + ")\n";
    s += "  Без стойки: мет " + to_str(nb.attack) + "%, блок " +
         to_str(nb.block) + "%, броня " + to_str(nb.armor) + "\n\n";
    s += "  Навыки:\n";
    bool any = false;
    for (const auto& kv : p.skills) {
        if (kv.second <= 0) continue;
        any = true;
        const SkillDef* sd = Content::get().skill(kv.first);
        s += "    " + std::string(sd ? sd->name : kv.first) + " — ранг " + to_str(kv.second) + "\n";
    }
    if (!any) s += "    пока никаких\n";

    message_box("Герой", s);
}

void screen_inventory(Game& g) {
    for (;;) {
        const Layout L = layout();
        const Player& p = g.player();
        const std::size_t namew = L.side ? 24 : 16;

        std::vector<std::string> rows;
        std::vector<std::string> ids;

        for (int i = 0; i < static_cast<int>(Slot::Count); ++i) {
            const std::string& id = p.equipped[static_cast<std::size_t>(i)];
            if (id.empty()) continue;
            const ItemDef* d = Content::get().item(id);
            rows.push_back("[надето] " + (d ? d->name : id));
            ids.push_back("!" + to_str(i));           // '!' — маркер снятия
        }
        for (const ItemStack& st : p.inv) {
            const ItemDef* d = Content::get().item(st.id);
            rows.push_back(pad(d ? d->name : st.id, namew) + " x" + to_str(st.count));
            ids.push_back(st.id);
        }
        if (rows.empty()) { message_box("Сумка", "  Пусто."); return; }

        int sel = choose("Сумка · золото: " + to_str(p.gold), rows,
                         "  Enter — действие, Esc — назад");
        if (sel < 0) return;

        const std::string& key = ids[static_cast<std::size_t>(sel)];
        if (key[0] == '!') { g.unequip(static_cast<Slot>(to_int(key.substr(1)))); continue; }

        const ItemDef* d = Content::get().item(key);
        if (!d) continue;

        std::vector<std::string> acts;
        std::vector<int> codes;
        if (d->kind == ItemKind::Consumable) { acts.push_back("Применить"); codes.push_back(0); }
        if (slot_for(d->kind) != Slot::Count) { acts.push_back("Надеть");   codes.push_back(1); }
        acts.push_back("Осмотреть"); codes.push_back(2);
        acts.push_back("Выбросить"); codes.push_back(3);
        acts.push_back("Назад");     codes.push_back(4);

        int a = choose(d->name, acts);
        if (a < 0) continue;
        switch (codes[static_cast<std::size_t>(a)]) {
            case 0: g.use_item(key); break;
            case 1: g.equip(key); break;
            case 2: {
                Stats b = d->bonus;
                std::string s = "  " + d->name + " (" + kind_name(d->kind) + ")\n  " +
                                d->desc + "\n\n  Цена: " + to_str(d->price) + "\n";
                if (b.max_hp)  s += "  +" + to_str(b.max_hp)  + " к здоровью\n";
                if (b.max_ap)  s += "  +" + to_str(b.max_ap)  + " к очкам действия\n";
                if (b.attack)  s += "  " + to_str(b.attack)   + "% к меткости\n";
                if (b.dmg_min || b.dmg_max)
                    s += "  урон +" + to_str(b.dmg_min) + "/" + to_str(b.dmg_max) + "\n";
                if (b.block)   s += "  " + to_str(b.block)    + "% к блоку\n";
                if (b.armor)   s += "  " + to_str(b.armor)    + " к броне\n";
                if (b.crit)    s += "  " + to_str(b.crit)     + "% к криту\n";
                if (b.ap_atk)  s += "  " + to_str(b.ap_atk)   + " AP к стоимости атаки\n";
                if (d->heal_hp) s += "  восстанавливает " + to_str(d->heal_hp) + " HP\n";
                if (d->heal_ap) s += "  восстанавливает " + to_str(d->heal_ap) + " AP\n";
                message_box("Предмет", s);
                break;
            }
            case 3: g.drop_item(key); break;
            default: break;
        }
    }
}

void screen_quests(Game& g) {
    const Content& c = Content::get();
    std::string s;
    bool any = false;

    for (const QuestDef& q : c.quests()) {
        auto it = g.player().quests.find(q.id);
        int st = (it == g.player().quests.end()) ? QUEST_NONE : it->second;
        if (st == QUEST_NONE) continue;
        any = true;
        s += (st == QUEST_DONE ? "  [x] " : "  [ ] ") + q.name + "\n";
        std::string txt = c.quest_stage_text(q.id, st);
        if (!txt.empty()) s += "      " + txt + "\n";
        if (q.id == "wolves" && st != QUEST_DONE) {
            auto k = g.player().counters.find("kill_wolf");
            int killed = (k == g.player().counters.end()) ? 0 : k->second;
            s += "      Волков убито: " + to_str(killed) + " / 5\n";
        }
        if (q.id == "pelts" && st != QUEST_DONE)
            s += "      Шкур в сумке: " + to_str(g.count_item("wolf_pelt")) + " / 3\n";
        s += "\n";
    }
    if (!any) s = "  Пока ни одного задания.\n  Поговори с жителями Ольховки.";
    message_box("Задания", s);
}

void screen_skills(Game& g) {
    for (;;) {
        const Layout L = layout();
        const Content& c = Content::get();
        std::vector<std::string> rows;
        std::vector<std::string> ids;
        for (const SkillDef& s : c.skills()) {
            auto it = g.player().skills.find(s.id);
            int rank = (it == g.player().skills.end()) ? 0 : it->second;
            std::string row = pad(s.name, 10) + to_str(rank) + "/" + to_str(s.max_rank);
            if (L.side) row += "  " + s.desc;
            rows.push_back(row);
            ids.push_back(s.id);
        }
        int pts = g.player().skill_points;
        int sel = choose("Навыки · очков: " + to_str(pts), rows,
                         pts > 0 ? "  Enter — вложить очко, Esc — назад"
                                 : "  Очков нет. Esc — назад");
        if (sel < 0) return;
        if (pts <= 0) continue;
        g.learn_skill(ids[static_cast<std::size_t>(sel)]);
    }
}

void screen_effects(Game& g) {
    const Content& c = Content::get();
    const std::vector<ActiveEffect>& list = g.player().effects;
    std::string s;
    if (list.empty()) {
        s = "  Ничего не действует.";
    } else {
        for (const ActiveEffect& a : list) {
            const EffectDef* d = c.effect(a.id);
            if (!d) continue;
            s += std::string(d->harmful ? "  [-] " : "  [+] ") + d->name;
            if (a.power > 1) s += " (сила " + to_str(a.power) + ")";
            s += ", ходов: " + to_str(a.turns) + "\n      " + d->desc + "\n\n";
        }
    }
    s += "\n  Вредные эффекты снимает противоядие и отдых на лежанке.";
    message_box("Что на тебе действует", s);
}

void screen_enchant(Game& g) {
    const Content& c = Content::get();
    for (;;) {
        // Зачаровать можно надетое и лежащее в сумке снаряжение.
        std::vector<std::string> rows, ids;
        for (int i = 0; i < static_cast<int>(Slot::Count); ++i) {
            const std::string& id = g.player().equipped[static_cast<std::size_t>(i)];
            if (id.empty()) continue;
            const ItemDef* d = c.item(id);
            auto e = g.player().enchants.find(id);
            std::string mark = (e == g.player().enchants.end()) ? "—" : "уже зачарован";
            if (e != g.player().enchants.end())
                if (const EnchantDef* ed = c.enchant(e->second)) mark = "«" + ed->name + "»";
            rows.push_back(pad(d ? d->name : id, 20) + "[надето] " + mark);
            ids.push_back(id);
        }
        for (const ItemStack& st : g.player().inv) {
            const ItemDef* d = c.item(st.id);
            if (!d || slot_for(d->kind) == Slot::Count) continue;
            auto e = g.player().enchants.find(st.id);
            std::string mark = "—";
            if (e != g.player().enchants.end())
                if (const EnchantDef* ed = c.enchant(e->second)) mark = "«" + ed->name + "»";
            rows.push_back(pad(d->name, 20) + "          " + mark);
            ids.push_back(st.id);
        }
        if (rows.empty()) { message_box("Зачарование", "  Зачаровывать нечего."); return; }

        int sel = choose("Что зачаровать? · золото: " + to_str(g.player().gold), rows,
                         "  Enter — выбрать вещь, Esc — уйти");
        if (sel < 0) return;
        const std::string item = ids[static_cast<std::size_t>(sel)];

        if (!g.can_enchant(item)) {
            message_box("Зачарование", "  На этой вещи уже есть зачарование.\n"
                                       "  Снять его нельзя.");
            continue;
        }

        std::vector<std::string> erows, eids;
        for (const EnchantDef& e : c.enchants()) {
            std::string req = to_str(e.price) + " зол.";
            if (!e.reagent.empty()) {
                const ItemDef* rd = c.item(e.reagent);
                req += " + " + std::string(rd ? rd->name : e.reagent) +
                       " (" + to_str(g.count_item(e.reagent)) + "/" +
                       to_str(e.reagent_count) + ")";
            }
            erows.push_back(pad(e.name, 10) + pad(req, 34) + e.desc);
            eids.push_back(e.id);
        }
        int es = choose("Какое зачарование?", erows, "  Enter — наложить, Esc — назад");
        if (es < 0) continue;
        g.enchant_item(item, eids[static_cast<std::size_t>(es)]);
    }
}

void screen_portals(Game& g) {
    for (;;) {
        if (!g.player().portal_master) {
            message_box("Порталы", "  Ты пока не умеешь их ставить.\n"
                                   "  Об этом знает отшельник в лесу.");
            return;
        }
        std::vector<std::string> rows;
        rows.push_back("Поставить портал здесь (нужен портальный камень: " +
                       to_str(g.count_item("portal_stone")) + ")");
        rows.push_back("Снять портал под ногами");
        rows.push_back("Назад");

        std::string list;
        for (std::size_t i = 0; i < g.player().portals.size(); ++i) {
            const Portal& pt = g.player().portals[i];
            const Location* l = g.world().location(pt.loc);
            list += "  " + to_str(static_cast<int>(i) + 1) + ". " +
                    std::string(l ? l->name : pt.loc) +
                    " (" + to_str(pt.pos.x) + "," + to_str(pt.pos.y) + ")\n";
        }
        if (list.empty()) list = "  (порталов нет)\n";

        int sel = choose("Порталы · поставлено " +
                         to_str(static_cast<int>(g.player().portals.size())) + " из " +
                         to_str(PORTAL_LIMIT) + "\n" + list, rows);
        if (sel < 0 || sel == 2) return;
        if (sel == 0) g.place_portal();
        if (sel == 1) g.remove_portal_here();
    }
}

// ---------------------------------------------------------------- диалоги

void run_shop(Game& g, const std::string& shop_id) {
    const ShopDef* shop = Content::get().shop(shop_id);
    if (!shop) return;
    bool buying = true;

    for (;;) {
        const Layout L = layout();
        const Content& c = Content::get();
        const std::size_t namew = L.side ? 24 : 16;

        std::vector<std::string> rows;
        std::vector<std::string> ids;

        if (buying) {
            for (const std::string& gid : shop->goods) {
                const ItemDef* d = c.item(gid);
                if (!d) continue;
                rows.push_back(pad(d->name, namew) + pad(to_str(g.buy_price(*shop, *d)), 5) + "зол.");
                ids.push_back(gid);
            }
        } else {
            for (const ItemStack& st : g.player().inv) {
                const ItemDef* d = c.item(st.id);
                if (!d || d->price <= 0) continue;   // квестовое не продаётся
                rows.push_back(pad(d->name, namew) + pad("x" + to_str(st.count), 5) +
                               to_str(g.sell_price(*shop, *d)) + "зол.");
                ids.push_back(st.id);
            }
        }
        if (rows.empty()) rows.push_back(buying ? "(товар кончился)" : "(продавать нечего)");

        std::string title = (L.side ? shop->name + " · " : "") +
                            std::string(buying ? "ПОКУПКА" : "ПРОДАЖА") +
                            " · золото: " + to_str(g.player().gold);
        int hk = 0;
        int sel = choose(title, rows,
                         L.side ? "  ^v выбор · Enter сделка · <> или T — купить/продать · Esc уйти"
                                : "  Enter сделка · T купить/продать · Esc",
                         {'t', 'T', platform::KEY_LEFT, platform::KEY_RIGHT}, &hk);
        if (sel == CHOOSE_HOTKEY) { buying = !buying; continue; }
        if (sel < 0) return;
        if (ids.empty() || sel >= static_cast<int>(ids.size())) continue;

        if (buying) g.buy(*shop, ids[static_cast<std::size_t>(sel)]);
        else        g.sell(*shop, ids[static_cast<std::size_t>(sel)]);
    }
}

void run_dialogue(Game& g, const std::string& npc_id) {
    const Content& c = Content::get();
    const NpcDef* npc = c.npc(npc_id);
    if (!npc) return;

    std::string node_id = npc->root;
    for (int guard = 0; guard < 64 && !node_id.empty(); ++guard) {
        const DlgNode* node = c.node(node_id);
        if (!node) return;

        std::vector<std::string> rows;
        std::vector<const DlgOption*> opts;
        for (const DlgOption& o : node->options) {
            if (!g.option_available(o)) continue;
            rows.push_back(o.text);
            opts.push_back(&o);
        }
        if (rows.empty()) return;

        int sel = choose(npc->name + "\n\n" + node->text + "\n", rows,
                         "  ^v выбор · Enter сказать · Esc уйти");
        if (sel < 0) return;

        const DlgOption* o = opts[static_cast<std::size_t>(sel)];
        std::string open_shop;
        bool open_ench = false;
        g.apply_option(*o, npc->shop, &open_shop, &open_ench);
        if (!open_shop.empty()) { run_shop(g, open_shop); return; }
        if (open_ench)          { screen_enchant(g); return; }
        node_id = o->next;
    }
}

// -------------------------------------------------------------------- бой

void run_combat(Game& g) {
    while (g.combat().active) {
        const Layout L = layout();
        const Mob* m = g.mob_by_uid(g.combat().mob_uid);
        const EnemyDef* e = m ? Content::get().enemy(m->enemy_id) : nullptr;
        if (!e) break;

        const Player& p = g.player();
        Stats t = g.total();
        const int hb = L.side ? 20 : L.bar;

        platform::clear_screen();
        out("=== БОЙ ===");
        if (L.side) {
            out("");
            out("  " + pad(e->name, 24) + "HP " +
                pad(to_str(g.combat().enemy_hp) + "/" + to_str(e->stats.max_hp), 10) +
                bar(g.combat().enemy_hp, e->stats.max_hp, hb));
            out("  " + pad("меткость " + to_str(e->stats.attack) + "%", 24) +
                "блок " + to_str(e->stats.block) + "%  броня " + to_str(e->stats.armor));
            out("");
            rule(L.rule);
            out("  " + pad(p.name, 24) + "HP " +
                pad(to_str(p.hp) + "/" + to_str(t.max_hp), 10) + bar(p.hp, t.max_hp, hb));
            out("  " + pad("AP " + to_str(p.ap) + "/" + to_str(t.max_ap), 24) +
                "Кураж " + bar(p.momentum, MOMENTUM_MAX, MOMENTUM_MAX, '*', '-') +
                " " + to_str(p.momentum) + "/" + to_str(MOMENTUM_MAX));
            out("  " + pad("Стойка: " + std::string(stance_name(p.stance)), 24) +
                stance_hint(p.stance));
            if (!p.effects.empty())
                out("  " + pad("На тебе:", 24) + trunc(Game::effects_line(p.effects), 36));
            if (m && !m->effects.empty())
                out("  " + pad("На противнике:", 24) + trunc(Game::effects_line(m->effects), 36));
        } else {
            out(trunc(e->name, static_cast<std::size_t>(L.cols)));
            out("HP " + pad(to_str(g.combat().enemy_hp) + "/" + to_str(e->stats.max_hp), 8) +
                bar(g.combat().enemy_hp, e->stats.max_hp, hb));
            out("мет " + to_str(e->stats.attack) + "% бл " + to_str(e->stats.block) +
                "% бр " + to_str(e->stats.armor));
            rule(L.rule);
            out(trunc(p.name, static_cast<std::size_t>(L.cols)));
            out("HP " + pad(to_str(p.hp) + "/" + to_str(t.max_hp), 8) + bar(p.hp, t.max_hp, hb));
            out("AP " + to_str(p.ap) + "/" + to_str(t.max_ap) +
                "  Кураж " + bar(p.momentum, MOMENTUM_MAX, MOMENTUM_MAX, '*', '-'));
            out("Стойка: " + std::string(stance_name(p.stance)));
            if (!p.effects.empty())
                out(trunc("Ты: " + Game::effects_line(p.effects), static_cast<std::size_t>(L.cols)));
            if (m && !m->effects.empty())
                out(trunc("Враг: " + Game::effects_line(m->effects), static_cast<std::size_t>(L.cols)));
        }
        rule(L.rule);

        const std::vector<std::string>& lg = g.combat().log;
        const std::size_t show = L.side ? 8 : 5;
        std::size_t from = lg.size() > show ? lg.size() - show : 0;
        for (std::size_t i = from; i < lg.size(); ++i)
            out("  " + trunc(lg[i], static_cast<std::size_t>(L.cols) - 2));
        for (std::size_t i = lg.size() - from; i < show; ++i) out("");

        out("");
        if (L.side) {
            out("  [A] атака (" + to_str(g.attack_cost()) + " AP)   [P] мощный удар (кураж " +
                to_str(MOMENTUM_COST) + ")   [U] предмет (" + to_str(AP_ITEM_COST) + " AP)");
            out("  [1/2/3] стойка   [E] закончить ход   [F] бежать");
        } else {
            out("[A]атака " + to_str(g.attack_cost()) + "AP  [P]мощный " + to_str(MOMENTUM_COST));
            out("[U]предмет [E]ход [F]бежать [1/2/3]стойка");
        }
        std::cout.flush();

        int k = platform::read_key();
        switch (k) {
            case 'a': case 'A': g.combat_attack(false); break;
            case 'p': case 'P': g.combat_attack(true);  break;
            case 'e': case 'E': g.combat_end_turn();    break;
            case 'f': case 'F': g.combat_flee();        break;
            case '1': g.combat_set_stance(Stance::Cautious); break;
            case '2': g.combat_set_stance(Stance::Balanced); break;
            case '3': g.combat_set_stance(Stance::Fierce);   break;
            case 'u': case 'U': {
                std::vector<std::string> rows, ids;
                for (const ItemStack& st : g.player().inv) {
                    const ItemDef* d = Content::get().item(st.id);
                    if (!d || d->kind != ItemKind::Consumable) continue;
                    rows.push_back(pad(d->name, 20) + "x" + to_str(st.count));
                    ids.push_back(st.id);
                }
                if (rows.empty()) { message_box("Бой", "  Расходников нет."); break; }
                int s = choose("Что применить?", rows);
                if (s >= 0) g.combat_use_item(ids[static_cast<std::size_t>(s)]);
                break;
            }
            case platform::KEY_ESC: break;   // из боя просто так не выйти
            case platform::KEY_EOF: return;  // ввод закончился — не крутимся вхолостую
            default: break;
        }

        if (g.player_dead()) break;
    }

    // Цикл выше обрывается на кадре ДО добивающего удара, поэтому итог боя
    // показываем отдельно — иначе исход виден только в журнале мира.
    if (!g.player_dead() && !g.combat().log.empty()) {
        const std::vector<std::string>& lg = g.combat().log;
        std::size_t from = lg.size() > 6 ? lg.size() - 6 : 0;
        std::string body;
        for (std::size_t i = from; i < lg.size(); ++i) body += "  " + lg[i] + "\n";
        message_box("Бой окончен", body);
    }
}

} // namespace ui
