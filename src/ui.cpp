#include "ui.h"

#include "platform.h"

#include <algorithm>
#include <iostream>

namespace ui {
namespace {

constexpr int VIEW_W = 48;
constexpr int VIEW_H = 18;
constexpr int PANEL  = 30;

std::string bar(int cur, int max, int width, char full = '#', char empty = '.') {
    if (max <= 0) return std::string(static_cast<std::size_t>(width), empty);
    if (cur < 0) cur = 0;
    if (cur > max) cur = max;
    int n = cur * width / max;
    if (n == 0 && cur > 0) n = 1;               // остаток здоровья всегда виден
    return std::string(static_cast<std::size_t>(n), full) +
           std::string(static_cast<std::size_t>(width - n), empty);
}

void out(const std::string& s) { std::cout << s << '\n'; }

std::string frame_top(int w)    { return "+" + std::string(static_cast<std::size_t>(w), '-') + "+"; }

// Строит один кадр карты: слои пишутся по порядку из README —
// тайл, койка, переход, табличка, предмет, моб, NPC, игрок.
std::vector<std::string> render_map(Game& g, const Location& loc) {
    int ox = g.player().pos.x - VIEW_W / 2;
    int oy = g.player().pos.y - VIEW_H / 2;
    ox = std::max(0, std::min(ox, std::max(0, loc.w - VIEW_W)));
    oy = std::max(0, std::min(oy, std::max(0, loc.h - VIEW_H)));

    std::vector<std::string> rows;
    for (int y = 0; y < VIEW_H; ++y) {
        std::string row;
        for (int x = 0; x < VIEW_W; ++x) {
            Vec2 p{ox + x, oy + y};
            char ch = ' ';
            if (loc.in_bounds(p)) {
                ch = tile_glyph(loc.at(p));
                if (loc.bed_at(p))                       ch = '&';
                if (loc.exit_at(p))                      ch = '>';
                if (loc.sign_at(p))                      ch = '!';
                int ii = loc.item_index_at(p);
                if (ii >= 0 && !g.item_taken(loc.id, ii)) ch = '*';
                if (const Mob* m = g.mob_at(p, g.player().loc)) {
                    if (const EnemyDef* e = Content::get().enemy(m->enemy_id)) ch = e->glyph;
                }
                if (const MapNpc* n = loc.npc_at(p)) {
                    if (const NpcDef* d = Content::get().npc(n->npc_id)) ch = d->glyph;
                }
                if (p == g.player().pos) ch = '@';
            }
            row += ch;
        }
        rows.push_back(row);
    }
    return rows;
}

std::vector<std::string> panel_lines(Game& g) {
    const Player& p = g.player();
    Stats t = g.total();
    std::vector<std::string> L;

    L.push_back(p.name + ", ур." + to_str(p.level));
    L.push_back("HP " + pad(to_str(p.hp) + "/" + to_str(t.max_hp), 8) + bar(p.hp, t.max_hp, 12));
    L.push_back("AP " + pad(to_str(p.ap) + "/" + to_str(t.max_ap), 8) + bar(p.ap, t.max_ap, 12));
    L.push_back("Опыт " + to_str(p.exp) + "/" + to_str(g.exp_to_next()));
    L.push_back("Золото " + to_str(p.gold));
    L.push_back("");
    L.push_back("Стойка: " + std::string(stance_name(p.stance)));
    L.push_back("Кураж:  " + bar(p.momentum, MOMENTUM_MAX, MOMENTUM_MAX, '*', '-') +
                " " + to_str(p.momentum) + "/" + to_str(MOMENTUM_MAX));
    if (p.skill_points > 0)
        L.push_back("Очков навыка: " + to_str(p.skill_points) + " (K)");
    L.push_back("");
    L.push_back("Урон " + to_str(t.dmg_min) + "-" + to_str(t.dmg_max) +
                "  меткость " + to_str(t.attack) + "%");
    L.push_back("Блок " + to_str(t.block) + "%  броня " + to_str(t.armor) +
                "  крит " + to_str(t.crit) + "%");
    L.push_back("");
    L.push_back("--- снаряжение ---");
    for (int i = 0; i < static_cast<int>(Slot::Count); ++i) {
        const std::string& id = p.equipped[static_cast<std::size_t>(i)];
        std::string nm = "—";
        if (!id.empty())
            if (const ItemDef* d = Content::get().item(id)) nm = d->name;
        L.push_back(pad(slot_name(static_cast<Slot>(i)), 8) + trunc(nm, PANEL - 9));
    }
    return L;
}

} // namespace

void draw_world(Game& g) {
    const Location* loc = g.here();
    platform::clear_screen();
    if (!loc) {
        out("Локация не загружена: " + g.world().last_error());
        return;
    }

    std::vector<std::string> map = render_map(g, *loc);
    std::vector<std::string> pan = panel_lines(g);

    out(loc->name + "  ·  ход " + to_str(g.turn()) +
        "  ·  [?] справка  [C] герой  [I] сумка  [Q] квесты  [K] навыки");
    out(frame_top(VIEW_W));
    for (std::size_t i = 0; i < map.size(); ++i) {
        std::string right = (i < pan.size()) ? pan[i] : std::string();
        out("|" + map[i] + "|  " + right);
    }
    out(frame_top(VIEW_W));

    const std::vector<std::string>& lg = g.log();
    std::size_t from = lg.size() > 4 ? lg.size() - 4 : 0;
    for (std::size_t i = from; i < lg.size(); ++i) out("  " + lg[i]);
    for (std::size_t i = lg.size() - from; i < 4; ++i) out("");
    std::cout.flush();
}

int choose(const std::string& title, const std::vector<std::string>& items,
           const std::string& footer, const std::vector<int>& hotkeys, int* hotkey_out) {
    if (items.empty()) return -1;
    int sel = 0;
    for (;;) {
        platform::clear_screen();
        out(title);
        out(std::string(60, '-'));
        for (std::size_t i = 0; i < items.size(); ++i)
            out((static_cast<int>(i) == sel ? " > " : "   ") + items[i]);
        out(std::string(60, '-'));
        out(footer.empty() ? "  Стрелки/WS — выбор, Enter/пробел — принять, Esc — назад"
                           : footer);
        std::cout.flush();

        int k = platform::read_key();
        for (int h : hotkeys) {
            if (k != h) continue;
            if (hotkey_out) *hotkey_out = k;
            return CHOOSE_HOTKEY;
        }
        switch (k) {
            case platform::KEY_UP:   case 'w': case 'W': sel = (sel + static_cast<int>(items.size()) - 1) % static_cast<int>(items.size()); break;
            case platform::KEY_DOWN: case 's': case 'S': sel = (sel + 1) % static_cast<int>(items.size()); break;
            case platform::KEY_ENTER: case '\r': case platform::KEY_SPACE: return sel;
            case platform::KEY_EOF:
            case platform::KEY_ESC: case 'q': case 'Q': return -1;
            default:
                // Быстрый выбор цифрой 1..9.
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
        platform::clear_screen();
        out(prompt);
        out("");
        out("  " + (buf.empty() ? def + "  (по умолчанию)" : buf) + "_");
        out("");
        out("  Enter — принять, Esc — по умолчанию");
        std::cout.flush();

        int k = platform::read_key();
        if (k == platform::KEY_ENTER || k == '\r') break;
        if (k == platform::KEY_ESC || k == platform::KEY_EOF) { buf.clear(); break; }
        if (k == 127 || k == 8) {                       // backspace
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

void message_box(const std::string& title, const std::string& body) {
    platform::clear_screen();
    out(title);
    out(std::string(60, '-'));
    out(body);
    out(std::string(60, '-'));
    out("  Любая клавиша — далее");
    std::cout.flush();
    platform::read_key();
}

void help_screen() {
    message_box("Управление",
        "  Стрелки или WASD — идти\n"
        "  C — герой        I — сумка       Q — квесты      K — навыки\n"
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
        "  @ ты   > переход   ! табличка   * предмет   & лежанка\n"
        "  # стена   T дерево   ~ вода   . , = земля\n"
        "  M Мирон  L Лада  B Бран  G Гурий  H отшельник\n"
        "  r крыса  w волк  W вожак  b разбойник");
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
    s += "  Здоровье     " + to_str(p.hp) + " / " + to_str(t.max_hp) + "\n";
    s += "  Очки действия " + to_str(p.ap) + " / " + to_str(t.max_ap) + "\n";
    s += "  Урон          " + to_str(t.dmg_min) + " - " + to_str(t.dmg_max) + "\n";
    s += "  Меткость      " + to_str(t.attack) + "%\n";
    s += "  Блок          " + to_str(t.block) + "%\n";
    s += "  Броня         " + to_str(t.armor) + "\n";
    s += "  Крит          " + to_str(t.crit) + "%\n";
    s += "  Атака стоит   " + to_str(g.attack_cost()) + " AP\n\n";
    s += "  Стойка: " + std::string(stance_name(p.stance)) + " (" +
         stance_hint(p.stance) + ")\n";
    s += "  Без учёта стойки: меткость " + to_str(nb.attack) + "%, блок " +
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
        const Player& p = g.player();
        std::vector<std::string> rows;
        std::vector<std::string> ids;

        for (int i = 0; i < static_cast<int>(Slot::Count); ++i) {
            const std::string& id = p.equipped[static_cast<std::size_t>(i)];
            if (id.empty()) continue;
            const ItemDef* d = Content::get().item(id);
            rows.push_back("[надето] " + pad(slot_name(static_cast<Slot>(i)), 8) +
                           (d ? d->name : id));
            ids.push_back("!" + to_str(i));           // '!' — маркер снятия
        }
        for (const ItemStack& st : p.inv) {
            const ItemDef* d = Content::get().item(st.id);
            std::string nm = d ? d->name : st.id;
            rows.push_back(pad(nm, 24) + " x" + pad(to_str(st.count), 4) +
                           (d ? std::string(kind_name(d->kind)) : ""));
            ids.push_back(st.id);
        }
        if (rows.empty()) { message_box("Сумка", "  Пусто."); return; }

        int sel = choose("Сумка · золото: " + to_str(p.gold), rows,
                         "  Enter — действие, Esc — назад");
        if (sel < 0) return;

        const std::string& key = ids[static_cast<std::size_t>(sel)];
        if (key[0] == '!') {
            g.unequip(static_cast<Slot>(to_int(key.substr(1))));
            continue;
        }

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
        const Content& c = Content::get();
        std::vector<std::string> rows;
        std::vector<std::string> ids;
        for (const SkillDef& s : c.skills()) {
            auto it = g.player().skills.find(s.id);
            int rank = (it == g.player().skills.end()) ? 0 : it->second;
            rows.push_back(pad(s.name, 12) + pad(to_str(rank) + "/" + to_str(s.max_rank), 8) +
                           s.desc);
            ids.push_back(s.id);
        }
        int pts = g.player().skill_points;
        int sel = choose("Навыки · свободных очков: " + to_str(pts), rows,
                         pts > 0 ? "  Enter — вложить очко, Esc — назад"
                                 : "  Очков нет. Esc — назад");
        if (sel < 0) return;
        if (pts <= 0) continue;
        g.learn_skill(ids[static_cast<std::size_t>(sel)]);
    }
}

// ---------------------------------------------------------------- диалоги

void run_shop(Game& g, const std::string& shop_id) {
    const ShopDef* shop = Content::get().shop(shop_id);
    if (!shop) return;
    bool buying = true;

    for (;;) {
        const Content& c = Content::get();
        std::vector<std::string> rows;
        std::vector<std::string> ids;

        if (buying) {
            for (const std::string& gid : shop->goods) {
                const ItemDef* d = c.item(gid);
                if (!d) continue;
                rows.push_back(pad(d->name, 24) + pad(kind_name(d->kind), 12) +
                               to_str(g.buy_price(*shop, *d)) + " зол.");
                ids.push_back(gid);
            }
        } else {
            for (const ItemStack& st : g.player().inv) {
                const ItemDef* d = c.item(st.id);
                if (!d || d->price <= 0) continue;   // квестовое не продаётся
                rows.push_back(pad(d->name, 24) + pad("x" + to_str(st.count), 12) +
                               to_str(g.sell_price(*shop, *d)) + " зол.");
                ids.push_back(st.id);
            }
        }
        if (rows.empty()) rows.push_back(buying ? "(товар кончился)" : "(продавать нечего)");

        std::string title = shop->name + " · " + (buying ? "ПОКУПКА" : "ПРОДАЖА") +
                            " · золото: " + to_str(g.player().gold);
        int hk = 0;
        int sel = choose(title, rows,
                         "  ↑↓ выбор · Enter — сделка · ←→ или T — купить/продать · Esc — уйти",
                         {'t', 'T', platform::KEY_LEFT, platform::KEY_RIGHT}, &hk);
        if (sel == CHOOSE_HOTKEY) { buying = !buying; continue; }
        if (sel < 0) return;
        if (ids.empty()) continue;
        if (sel >= static_cast<int>(ids.size())) continue;

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
                         "  Стрелки — выбор, Enter — сказать, Esc — уйти");
        if (sel < 0) return;

        const DlgOption* o = opts[static_cast<std::size_t>(sel)];
        std::string open_shop;
        g.apply_option(*o, npc->shop, &open_shop);
        if (!open_shop.empty()) { run_shop(g, open_shop); return; }
        node_id = o->next;
    }
}

// -------------------------------------------------------------------- бой

void run_combat(Game& g) {
    while (g.combat().active) {
        const Mob* m = g.mob_by_uid(g.combat().mob_uid);
        const EnemyDef* e = m ? Content::get().enemy(m->enemy_id) : nullptr;
        if (!e) break;

        Stats t = g.total();
        platform::clear_screen();
        out("=== БОЙ ===");
        out("");
        out("  " + pad(e->name, 24) + "HP " + pad(to_str(g.combat().enemy_hp) + "/" +
            to_str(e->stats.max_hp), 10) + bar(g.combat().enemy_hp, e->stats.max_hp, 20));
        out("  " + pad("меткость " + to_str(e->stats.attack) + "%", 24) +
            "блок " + to_str(e->stats.block) + "%  броня " + to_str(e->stats.armor));
        out("");
        out(std::string(64, '-'));
        out("  " + pad(g.player().name, 24) + "HP " + pad(to_str(g.player().hp) + "/" +
            to_str(t.max_hp), 10) + bar(g.player().hp, t.max_hp, 20));
        out("  " + pad("AP " + to_str(g.player().ap) + "/" + to_str(t.max_ap), 24) +
            "Кураж " + bar(g.player().momentum, MOMENTUM_MAX, MOMENTUM_MAX, '*', '-') +
            " " + to_str(g.player().momentum) + "/" + to_str(MOMENTUM_MAX));
        out("  " + pad("Стойка: " + std::string(stance_name(g.player().stance)), 24) +
            stance_hint(g.player().stance));
        out(std::string(64, '-'));

        const std::vector<std::string>& lg = g.combat().log;
        std::size_t from = lg.size() > 8 ? lg.size() - 8 : 0;
        for (std::size_t i = from; i < lg.size(); ++i) out("  " + lg[i]);
        for (std::size_t i = lg.size() - from; i < 8; ++i) out("");

        out("");
        out("  [A] атака (" + to_str(g.attack_cost()) + " AP)   [P] мощный удар (кураж " +
            to_str(MOMENTUM_COST) + ")   [U] предмет (" + to_str(AP_ITEM_COST) + " AP)");
        out("  [1/2/3] стойка   [E] закончить ход   [F] бежать");
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
                    rows.push_back(pad(d->name, 24) + "x" + to_str(st.count));
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
