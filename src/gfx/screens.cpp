#include "app.h"
#include "../content.h"
#include "../platform.h"

#include <SDL2/SDL.h>

namespace gfx {

namespace {

} // namespace

// ------------------------------------------------------------------ стек

void App::push(Modal::Kind k) {
    Modal m(k);
    if (k == Modal::Help) {
        m.body =
            "Касание\n"
            "  тап по полю — идти к точке\n"
            "  удержание — бежать за пальцем\n"
            "  свайп — шаг в сторону\n"
            "  тап по себе или в стену — это меню\n"
            "  тап по пункту — выбрать его\n"
            "  тап мимо окна — закрыть окно\n"
            "\n"
            "Клавиатура\n"
            "  WASD или стрелки — шаг\n"
            "  C герой   I сумка   Q задания   K навыки\n"
            "  F эффекты  P порталы  B книги   M меню\n"
            "  1 2 3 — стойка, Enter — удар в бою\n"
            "\n"
            "Знаки карты\n"
            "  @ ты      N житель    X враг\n"
            "  > переход ! табличка  * предмет\n"
            "  & лежанка C сундук    O портал   ? записка";
    }
    stack_.push_back(m);
}

void App::push_message(const std::string& title, const std::string& body) {
    Modal m(Modal::Message);
    m.title = title;
    m.body = body;
    stack_.push_back(m);
}

void App::pop() { if (!stack_.empty()) stack_.pop_back(); }
void App::close_all() { stack_.clear(); }
Modal* App::top() { return stack_.empty() ? 0 : &stack_.back(); }

void App::save_game() {
    platform::make_dir(save_path_.substr(0, save_path_.find_last_of('/')));
    if (g_.save_to(save_path_)) { has_save_ = true; g_.msg("Игра сохранена."); }
    else push_message("Ошибка сохранения", g_.error());
}

void App::load_game() {
    if (g_.load_from(save_path_)) { close_all(); walk_.stop(); }
    else push_message("Ошибка загрузки", g_.error());
}

// -------------------------------------------------------------------- HUD

void App::hud_buttons(std::vector<Rect>* out) const {
    const Rect hud = hud_area();
    row_of(Rect(hud.x + 4, hud.y + hud.h - c_.touch_unit() - 4, hud.w - 8, c_.touch_unit()),
           5, 6, out);
}

void App::draw_hud() {
    const Theme& th = theme();
    const Rect hud = hud_area();
    c_.fill(hud, Color(20, 23, 29, 240));
    c_.fill(Rect(hud.x, hud.y, hud.w, 1), th.border);

    const Player& p = g_.player();
    const Stats t = g_.total();
    const int sc = c_.scale();
    const int ch = c_.cell_h();
    int y = hud.y + 4;

    const Location* loc = g_.here();
    std::string head = (loc ? loc->name : std::string("?"));
    head += "  ур." + to_str(p.level) + "  зол." + to_str(p.gold);
    c_.text(6, y, head, th.accent, sc);
    y += ch;

    // Полосы здоровья и очков действия: цифры мелкие, полосу видно сразу.
    const int bw = hud.w - 12;
    const int bh = ch / 2 + 2;
    const int hp_w = t.max_hp > 0 ? bw * (p.hp < 0 ? 0 : p.hp) / t.max_hp : 0;
    c_.fill(Rect(6, y, bw, bh), Color(60, 30, 30, 220));
    c_.fill(Rect(6, y, hp_w > bw ? bw : hp_w, bh), Color(180, 60, 56));
    c_.text(10, y - 1, to_str(p.hp) + "/" + to_str(t.max_hp) + " HP", th.text, sc > 1 ? sc - 1 : 1);
    y += bh + 2;
    const int ap_w = t.max_ap > 0 ? bw * (p.ap < 0 ? 0 : p.ap) / t.max_ap : 0;
    c_.fill(Rect(6, y, bw, bh), Color(30, 40, 60, 220));
    c_.fill(Rect(6, y, ap_w > bw ? bw : ap_w, bh), Color(70, 120, 190));
    c_.text(10, y - 1, to_str(p.ap) + "/" + to_str(t.max_ap) + " AP", th.text, sc > 1 ? sc - 1 : 1);
    y += bh + 3;

    if (!status_.empty())
        c_.text(6, y, trunc(status_, static_cast<std::size_t>((hud.w - 12) / c_.cell_w())),
                th.faint, sc);

    // Журнал в промежутке между картой и панелью: место всё равно пустует,
    // а видеть последние строки полезнее, чем чёрный фон.
    const int gap_top = map_bottom();
    const int gap_h = hud.y - gap_top;
    if (gap_h > ch * 2) {
        const std::vector<std::string>& lg = g_.log();
        const int lines = (gap_h - ch / 2) / ch;
        int from = static_cast<int>(lg.size()) - lines;
        if (from < 0) from = 0;
        int ly = gap_top + ch / 2;
        const std::size_t fit = static_cast<std::size_t>((c_.width() - 12) / c_.cell_w());
        for (int i = from; i < static_cast<int>(lg.size()); ++i) {
            c_.text(6, ly, trunc(lg[static_cast<std::size_t>(i)], fit), th.faint, sc);
            ly += ch;
        }
    }

    // Пять сенсорных кнопок: те же, что клавиши C, I, Q, K и меню.
    std::vector<Rect> b;
    hud_buttons(&b);
    static const char* labels[5] = {"Герой", "Сумка", "Квесты", "Навыки", "Меню"};
    for (int i = 0; i < 5; ++i)
        button(c_, b[static_cast<std::size_t>(i)], labels[i], true, false);
}

// ------------------------------------------------------------- главное меню

void App::menu_layout(std::vector<std::string>* items, std::vector<Rect>* out) const {
    items->clear();
    items->push_back("Новая игра");
    if (has_save_) items->push_back("Продолжить");
    items->push_back("Управление");
    items->push_back("Выход");

    const int bh = c_.touch_unit();
    const int gap = 8;
    const int bw = c_.width() * 2 / 3;
    const int total = static_cast<int>(items->size()) * (bh + gap) - gap;
    // Кнопки прижаты к низу: туда дотягивается большой палец.
    const int top = c_.height() - total - c_.cell_h() * 3;
    column_of(Rect((c_.width() - bw) / 2, top, bw, total),
              static_cast<int>(items->size()), gap, out);
}

void App::draw_main_menu() {
    const Theme& th = theme();
    const int sc = c_.scale();
    const int ch = c_.cell_h();

    c_.fill(Rect(0, 0, c_.width(), c_.height()), Color(16, 18, 24));

    int y = c_.height() / 10;
    c_.text_centered(Rect(0, y, c_.width(), ch * 2), "ЛЮБОВЬ ЭНДОРА", th.accent, sc + 1);
    y += ch * 3;
    c_.text_centered(Rect(0, y, c_.width(), ch), "роглайк в духе «Следа Эндора»", th.faint, sc);
    y += ch * 3;

    // Главное меню — единственный экран без наложений: только прогресс
    // выбранного героя и пункты выбора.
    if (has_save_) {
        Game probe;
        if (probe.load_from(save_path_)) {
            const Player& p = probe.player();
            int done = 0, open_q = 0;
            for (const auto& kv : p.quests) {
                if (kv.second == QUEST_DONE) ++done;
                else if (kv.second > 0) ++open_q;
            }
            const RaceDef* rd = Content::get().race(p.race);
            const SpecDef* sd = Content::get().spec(p.spec);
            const Location* lc = probe.here();

            const int pw = c_.width() * 5 / 6;
            Rect card((c_.width() - pw) / 2, y, pw, ch * 7);
            c_.fill(card, Color(26, 30, 38, 235));
            c_.frame(card, th.border, 1);
            int cy = card.y + ch / 2;
            const int tx = card.x + ch / 2;
            c_.text(tx, cy, p.name + ", уровень " + to_str(p.level), th.text, sc);
            cy += ch;
            c_.text(tx, cy, std::string(rd ? rd->name : p.race) + " · " +
                    (sd ? sd->name : p.spec), th.faint, sc);
            cy += ch;
            c_.text(tx, cy, "Место: " + (lc ? lc->name : std::string("?")), th.faint, sc);
            cy += ch;
            c_.text(tx, cy, "Заданий пройдено: " + to_str(done), th.faint, sc);
            cy += ch;
            c_.text(tx, cy, "В работе: " + to_str(open_q) +
                    "   золото: " + to_str(p.gold), th.faint, sc);
            cy += ch;
            c_.text(tx, cy, "Ход: " + to_str(probe.turn()), th.faint, sc);
        }
    }

    std::vector<std::string> items;
    std::vector<Rect> bs;
    menu_layout(&items, &bs);
    for (std::size_t i = 0; i < items.size(); ++i)
        button(c_, bs[i], items[i], true, static_cast<int>(i) == menu_list_.cursor);
}

void App::main_menu_tap(int x, int y) {
    std::vector<std::string> items;
    std::vector<Rect> bs;
    menu_layout(&items, &bs);

    int pick = -1;
    if (x < 0) pick = -y - 1;                     // выбор с клавиатуры
    else
        for (std::size_t i = 0; i < bs.size(); ++i)
            if (bs[i].contains(x, y)) { pick = static_cast<int>(i); break; }
    if (pick < 0 || pick >= static_cast<int>(items.size())) return;

    const std::string& what = items[static_cast<std::size_t>(pick)];
    if (what == "Новая игра") {
        mode_ = MODE_CREATE;
        create_step_ = 0;
        new_name_ = "";
        create_list_ = ListView();
        SDL_StartTextInput();
    } else if (what == "Продолжить") {
        if (g_.load_from(save_path_)) { mode_ = MODE_PLAY; close_all(); walk_.stop(); }
    } else if (what == "Управление") {
        Modal m(Modal::Help);
        push(Modal::Help);
        m = stack_.back();
        stack_.pop_back();
        push_message("Управление", m.body);
    } else if (what == "Выход") {
        quit_ = true;
    }
}

void App::create_layout(Rect* list_area, Rect* next_btn) const {
    const int ch = c_.cell_h();
    const int bw = c_.width() * 5 / 6;
    const int bx = (c_.width() - bw) / 2;
    const int bh = c_.touch_unit();
    *next_btn = Rect(bx, c_.height() - bh - ch, bw, bh);
    const int top = ch * 2 + ch * 3 + ch * 2;
    *list_area = Rect(bx, top, bw, next_btn->y - top - ch);
}

void App::draw_create_hero() {
    const Theme& th = theme();
    const int sc = c_.scale();
    const int ch = c_.cell_h();
    c_.fill(Rect(0, 0, c_.width(), c_.height()), Color(16, 18, 24));

    Rect area, next;
    create_layout(&area, &next);

    c_.text_centered(Rect(0, ch * 2, c_.width(), ch), "СОЗДАНИЕ ГЕРОЯ", th.accent, sc + 1);

    const Content& cont = Content::get();
    if (create_step_ == 0) {
        c_.text(area.x, area.y - ch * 2, "Как тебя зовут?", th.text, sc);
        Rect field(area.x, area.y, area.w, c_.touch_unit());
        c_.fill(field, Color(30, 34, 42, 240));
        c_.frame(field, th.accent, 2);
        const std::string shown = new_name_.empty() ? std::string("Герой") : new_name_;
        c_.text(field.x + ch / 2, field.y + (field.h - ch) / 2, shown + "_",
                new_name_.empty() ? th.faint : th.text, sc);
        c_.text(area.x, field.y + field.h + ch / 2,
                "Наберите имя и нажмите «Дальше».", th.faint, sc);
        button(c_, next, "Дальше", true, true);
        return;
    }

    const bool races = (create_step_ == 1);
    c_.text(area.x, area.y - ch * 2, races ? "Кто ты?" : "Чем занимался?", th.text, sc);

    std::vector<Row> rows;
    if (races)
        for (const RaceDef& r : cont.races()) rows.push_back(Row(r.name + " — " + r.desc));
    else
        for (const SpecDef& sp : cont.specs()) rows.push_back(Row(sp.name + " — " + sp.desc));

    create_list_.draw(c_, area, rows);
    button(c_, next, races ? "Выбрать расу" : "Начать игру", true, true);
}

void App::create_hero_tap(int x, int y) {
    const Content& cont = Content::get();
    Rect area, next;
    create_layout(&area, &next);

    if (create_step_ == 0) {
        if (x < 0 || next.contains(x, y)) {
            create_step_ = 1;
            create_list_ = ListView();
            SDL_StopTextInput();
        }
        return;
    }

    const bool races = (create_step_ == 1);
    const int n = races ? static_cast<int>(cont.races().size())
                        : static_cast<int>(cont.specs().size());

    int pick = -1;
    if (x < 0) pick = -y - 1;
    else {
        const int hit = create_list_.hit(c_, area, x, y, n);
        if (hit >= 0) pick = hit;                       // тап по строке — выбор
        else if (next.contains(x, y)) pick = create_list_.cursor < 0 ? 0 : create_list_.cursor;
    }
    if (pick < 0 || pick >= n) return;

    if (races) {
        new_race_ = cont.races()[static_cast<std::size_t>(pick)].id;
        create_step_ = 2;
        create_list_ = ListView();
        return;
    }
    new_spec_ = cont.specs()[static_cast<std::size_t>(pick)].id;
    g_.new_game(new_name_.empty() ? "Герой" : new_name_, new_race_, new_spec_);
    if (!g_.here()) {
        push_message("Не удалось начать игру", g_.world().last_error());
        mode_ = MODE_MENU;
        return;
    }
    mode_ = MODE_PLAY;
    close_all();
    walk_.stop();
}

// -------------------------------------------------------------------- бой

void App::combat_layout(std::vector<Rect>* top_row, std::vector<Rect>* bottom_row,
                        Rect* body) const {
    Rect frame;
    const Rect area = panel_rect(c_, true, 40, 16, &frame);
    if (body) *body = area;
    const int bh = c_.touch_unit();
    row_of(Rect(area.x, area.y + area.h - bh * 2 - 6, area.w, bh), 3, 6, top_row);
    row_of(Rect(area.x, area.y + area.h - bh, area.w, bh), 3, 6, bottom_row);
}

void App::draw_combat() {
    const Theme& th = theme();
    const Combat& cb = g_.combat();
    const Mob* m = g_.mob_by_uid(cb.mob_uid);
    const EnemyDef* e = m ? Content::get().enemy(m->enemy_id) : 0;
    if (!e) return;

    dim_screen(c_, 120);
    const int ch = c_.cell_h();
    const int sc = c_.scale();
    Rect frame;
    const Rect area = panel(c_, "Бой", 40, 16, &frame);
    (void)frame;

    int y = area.y;
    const int bw = area.w;

    c_.text(area.x, y, e->name, th.warn, sc);
    y += ch;
    const int ehp = cb.enemy_hp < 0 ? 0 : cb.enemy_hp;
    c_.fill(Rect(area.x, y, bw, ch), Color(60, 30, 30, 220));
    c_.fill(Rect(area.x, y, e->stats.max_hp > 0 ? bw * ehp / e->stats.max_hp : 0, ch),
            Color(190, 70, 62));
    c_.text(area.x + 4, y, to_str(ehp) + " / " + to_str(e->stats.max_hp), th.text, sc);
    y += ch * 2;

    const Player& p = g_.player();
    const Stats t = g_.total();
    c_.text(area.x, y, p.name + "  кураж " + to_str(p.momentum) + "/" + to_str(MOMENTUM_COST),
            th.text, sc);
    y += ch;
    const int php = p.hp < 0 ? 0 : p.hp;
    c_.fill(Rect(area.x, y, bw, ch), Color(30, 50, 34, 220));
    c_.fill(Rect(area.x, y, t.max_hp > 0 ? bw * php / t.max_hp : 0, ch), Color(90, 170, 96));
    c_.text(area.x + 4, y, to_str(php) + " / " + to_str(t.max_hp) + " HP   " +
            to_str(p.ap) + "/" + to_str(t.max_ap) + " AP", th.text, sc);
    y += ch * 2;

    // Журнал боя: последние строки, больше на экране и не нужно.
    std::vector<Rect> r1, r2;
    combat_layout(&r1, &r2, 0);
    const std::vector<std::string>& lg = cb.log;
    const int log_h = r1[0].y - 6 - y;
    const int lines = log_h > 0 ? log_h / ch : 0;
    int from = static_cast<int>(lg.size()) - lines;
    if (from < 0) from = 0;
    for (int i = from; i < static_cast<int>(lg.size()); ++i) {
        c_.text(area.x, y, trunc(lg[static_cast<std::size_t>(i)],
                                 static_cast<std::size_t>(bw / c_.cell_w())), th.faint, sc);
        y += ch;
    }

    const bool can_hit = p.ap >= g_.attack_cost();
    button(c_, r1[0], "Удар", can_hit, false);
    button(c_, r1[1], "Мощный", can_hit && p.momentum >= MOMENTUM_COST, false);
    button(c_, r1[2], "Сумка", true, false);
    button(c_, r2[0], std::string("Стойка: ") + stance_name(p.stance), true, false);
    button(c_, r2[1], "Конец хода", true, false);
    button(c_, r2[2], "Бежать", true, false);
}

void App::combat_tap(int x, int y) {
    std::vector<Rect> r1, r2;
    combat_layout(&r1, &r2, 0);

    if (r1[0].contains(x, y)) { g_.combat_attack(false); return; }
    if (r1[1].contains(x, y)) { g_.combat_attack(true);  return; }
    if (r1[2].contains(x, y)) { push(Modal::Inventory);  return; }
    if (r2[0].contains(x, y)) {
        const Stance s = g_.player().stance;
        g_.combat_set_stance(s == Stance::Cautious ? Stance::Balanced
                             : (s == Stance::Balanced ? Stance::Fierce : Stance::Cautious));
        return;
    }
    if (r2[1].contains(x, y)) { g_.combat_end_turn(); return; }
    if (r2[2].contains(x, y)) { g_.combat_flee();     return; }
}

} // namespace gfx
