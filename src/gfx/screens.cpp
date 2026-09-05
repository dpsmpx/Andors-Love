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
            "  держать палец — идти, пока держишь\n"
            "  держать дольше — перейти на бег\n"
            "  вести пальцем — цель едет за ним\n"
            "  тап по себе или в стену — остановиться\n"
            "  тап по пункту — выбрать его\n"
            "  держать на строке продажи — продать пачкой\n"
            "  свайп в окне — прокрутка\n"
            "  тап мимо окна — закрыть окно\n"
            "\n"
            "Клавиатура\n"
            "  WASD или стрелки — шаг\n"
            "  C герой   I сумка   Q задания   K навыки\n"
            "  P порталы  B книги   L журнал   M меню\n"
            "  1 2 3 — стойка, Enter — удар в бою\n"
            "\n"
            "Знаки карты\n"
            "  @ ты      N житель    X враг\n"
            "  > переход ! табличка  * предмет\n"
            "  & лежанка C сундук    O портал   ? записка\n"
            "\n"
            "Благодарности\n"
            "  Группе vk.com/programmer_ne_lamer\n"
            "  за сообщество и поддержку.";
    }
    if (k == Modal::Log) {
        // Открывается на последних записях: журнал читают с конца, как ленту,
        // а к началу листают, когда ищут, где что случилось. Предел зависит
        // от ширины окна и станет известен только при отрисовке — она и
        // подрежет это число до настоящего.
        m.scroll = 1 << 28;
    }
    push_modal(m);
}

void App::push_text_input(const std::string& title, const std::string& book_id,
                          int index, const std::string& initial, std::size_t max_len) {
    Modal m(Modal::TextInput);
    m.title   = title;
    m.arg     = book_id;
    m.index   = index;
    m.buffer  = initial;
    m.max_len = max_len;
    push_modal(m);   // клавиатуру поднимет он же: см. takes_text
}

// Подтверждение. Число в поле уже обрезано — его обрезали на каждом нажатии,
// — поэтому здесь остаётся только перечитать пустое поле как единицу и продать.
void App::commit_amount(Modal& m) {
    amount_from_text(m);
    clamp_amount(m);

    const ShopDef* sh = Content::get().shop(m.arg);
    if (sh) g_.sell(*sh, m.item, m.amount);
    pop();
}

void App::commit_text_input(Modal& m) {
    const std::string book_id = m.arg;
    const int index = m.index;
    const std::string text = m.buffer;
    pop();           // клавиатуру погасит он же

    if (index < 0 && index != BOOK_APPEND) { g_.book_set_title(book_id, text); return; }
    if (index == BOOK_APPEND) {
        const Book* b = g_.book(book_id);
        g_.book_insert_line(book_id, b ? static_cast<int>(b->lines.size()) : 0, text);
        return;
    }
    g_.book_set_line(book_id, index, text);
}

void App::push_message(const std::string& title, const std::string& body) {
    Modal m(Modal::Message);
    m.title = title;
    m.body = body;
    push_modal(m);
}

// Окна, в которых что-то набирают. На Android им нужна экранная клавиатура,
// и поднимается она вместе с окном, а опускается вместе с ним — каким бы
// путём оно ни закрылось.
static bool takes_text(Modal::Kind k) {
    return k == Modal::TextInput || k == Modal::Amount;
}

void App::push_modal(const Modal& m) {
    Modal copy = m;
    copy.born_press = ptr_.press_id();
    stack_.push_back(copy);
    if (takes_text(copy.kind)) SDL_StartTextInput();
}

void App::pop() {
    if (stack_.empty()) return;
    // Клавиатура гасится здесь, а не на каждом выходе порознь: выходов у окна
    // четыре — кнопка, Escape, тап мимо и подтверждение, — и «Отмена» раньше
    // оставляла её поднятой, потому что о ней там никто не вспомнил.
    if (takes_text(stack_.back().kind)) SDL_StopTextInput();
    stack_.pop_back();
}

void App::close_top() {
    if (stack_.empty()) return;
    const bool was_death = (stack_.back().kind == Modal::Message &&
                            stack_.back().arg == "death");
    pop();
    if (was_death) {
        mode_ = MODE_MENU;
        menu_list_ = ListView();
        refresh_save_summary();
    }
}
void App::close_all() { stack_.clear(); }
Modal* App::top() { return stack_.empty() ? 0 : &stack_.back(); }

void App::refresh_save_summary() {
    summary_ = SaveSummary();
    Game probe(data_root_);
    if (!probe.load_from(save_path_)) { has_save_ = false; return; }

    has_save_ = true;
    const Player& p = probe.player();
    summary_.ok = true;
    summary_.name = p.name;
    summary_.level = p.level;
    summary_.gold = p.gold;
    summary_.turn = probe.turn();
    for (std::map<std::string, int>::const_iterator it = p.quests.begin();
         it != p.quests.end(); ++it) {
        if (it->second == QUEST_DONE) ++summary_.done;
        else if (it->second > 0)      ++summary_.open_q;
    }
    const RaceDef* rd = Content::get().race(p.race);
    const SpecDef* sd = Content::get().spec(p.spec);
    summary_.kind = std::string(rd ? rd->name : p.race) + " · " + (sd ? sd->name : p.spec);
    const Location* lc = probe.here();
    summary_.place = lc ? lc->name : std::string("?");
}

void App::save_game() {
    platform::make_dir(save_dir_);
    if (g_.save_to(save_path_)) { g_.msg("Игра сохранена."); refresh_save_summary(); }
    else push_message("Ошибка сохранения", g_.error());
}

void App::load_game() {
    if (g_.load_from(save_path_)) { close_all(); walk_.stop(); died_ = false; }
    else push_message("Ошибка загрузки", g_.error());
}

// -------------------------------------------------------------------- HUD

void App::hud_buttons(std::vector<Rect>* out) const {
    const Rect hud = hud_area();
    row_of(Rect(hud.x + 4, hud.y + hud.h - c_.touch_unit() - 4, hud.w - 8, c_.touch_unit()),
           5, 6, out);
}

// Журнал занимает то, что осталось над картой. Место там всё равно
// пустует, а видеть последние строки полезнее, чем чёрный фон.
//
// Раньше журнал был между картой и панелью, а карта — сверху. Теперь
// наоборот: по карте тыкают, и она внизу, под большим пальцем, а журнал
// читают — ему верх.
Color App::tone_color(unsigned char tone) const {
    // Одно место, где важность превращается в цвет: и лента внизу, и окно
    // журнала спрашивают здесь, поэтому одно и то же событие в них не может
    // оказаться разного цвета.
    const Theme& th = theme();
    switch (static_cast<MsgTone>(tone)) {
        case MsgTone::Good: return th.good;
        case MsgTone::Bad:  return th.warn;
        case MsgTone::Loud: return th.accent;
        default:            return th.faint;
    }
}

void App::draw_log() {
    const Rect lr = log_area();
    const int ch = c_.cell_h();
    if (lr.h <= ch) return;

    const int lines = (lr.h - ch / 2) / ch;
    if (lines < 1) return;
    const std::size_t fit = static_cast<std::size_t>((lr.w - 12) / c_.cell_w());
    std::vector<std::size_t> src;
    const std::vector<std::string> tail = log_tail(g_.log(), fit, lines, &src);
    const std::vector<unsigned char>& tones = g_.log_tones();

    // Строки прижаты книзу, к самой карте. Пока журнал не заполнил отведённое,
    // свежее сообщение всё равно оказывается рядом с картой, а не в другом
    // конце экрана, и пустое место собирается сверху, где оно читается полем,
    // а не дырой посреди.
    int ly = lr.y + lr.h - static_cast<int>(tail.size()) * ch - ch / 2;
    if (ly < lr.y) ly = lr.y;
    for (std::size_t i = 0; i < tail.size(); ++i) {
        // Обычные сообщения остаются приглушёнными: лента под картой — это
        // фон, и если выделить всё, не выделено ничего.
        unsigned char tone = 0;
        if (i < src.size() && src[i] < tones.size()) tone = tones[src[i]];
        c_.text(lr.x + 6, ly, tail[i], tone_color(tone), c_.scale());
        ly += ch;
    }
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

    // Последнее сообщение целиком, с переносом по словам. Раньше хвост
    // отрезался, а прочитать его было негде: журнал показывал ту же строку
    // и так же обрезанной.
    if (!status_.empty()) {
        const int room = (hud.y + hud.h - c_.touch_unit() - 8) - y;
        // Одна строка была здесь и раньше, впритык к кнопкам; больше —
        // только если панель выросла и место действительно есть.
        int rows = room > 0 ? room / ch : 0;
        if (rows < 1) rows = 1;
        const std::size_t cols = static_cast<std::size_t>((hud.w - 12) / c_.cell_w());
        const std::vector<std::string> ls = wrap(status_, cols);
        // Здесь берётся начало, а не конец: у одного сообщения важнее его
        // начало, тогда как у журнала важнее последнее сообщение.
        for (int i = 0; i < rows && i < static_cast<int>(ls.size()); ++i) {
            c_.text(6, y, ls[static_cast<std::size_t>(i)], th.faint, sc);
            y += ch;
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
    if (summary_.ok) {
        const int pw = c_.width() * 5 / 6;
        Rect card((c_.width() - pw) / 2, y, pw, ch * 7);
        c_.fill(card, Color(26, 30, 38, 235));
        c_.frame(card, th.border, 1);
        int cy = card.y + ch / 2;
        const int tx = card.x + ch / 2;
        c_.text(tx, cy, summary_.name + ", уровень " + to_str(summary_.level), th.text, sc);
        cy += ch;
        c_.text(tx, cy, summary_.kind, th.faint, sc);
        cy += ch;
        c_.text(tx, cy, "Место: " + summary_.place, th.faint, sc);
        cy += ch;
        c_.text(tx, cy, "Заданий пройдено: " + to_str(summary_.done), th.faint, sc);
        cy += ch;
        c_.text(tx, cy, "В работе: " + to_str(summary_.open_q) +
                "   золото: " + to_str(summary_.gold), th.faint, sc);
        cy += ch;
        c_.text(tx, cy, "Ход: " + to_str(summary_.turn), th.faint, sc);
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
        if (g_.load_from(save_path_)) {
            mode_ = MODE_PLAY;
            close_all();
            walk_.stop();
            died_ = false;
        }
    } else if (what == "Управление") {
        push(Modal::Help);
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
    died_ = false;
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
    const int log_h = r1[0].y - 6 - y;
    const int lines = log_h > 0 ? log_h / ch : 0;
    const std::size_t cols = static_cast<std::size_t>(bw / c_.cell_w());
    for (const std::string& s : log_tail(cb.log, cols, lines)) {
        c_.text(area.x, y, s, th.faint, sc);
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
