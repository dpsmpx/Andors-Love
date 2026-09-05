#include "app.h"
#include "../content.h"
#include "../paths.h"
#include "../platform.h"
#include "font.h"
#include "tileset.h"

#include <SDL2/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace gfx {

namespace {

// Имена тайлов существ: по картинке на жителя и на врага. Список берётся из
// содержимого игры, поэтому новый житель или враг получает свой тайл сам
// собой — второго списка, который пришлось бы держать в согласии, нет.
std::vector<std::string> creature_tile_names() {
    const Content& c = Content::get();
    std::vector<std::string> names;
    for (std::map<std::string, NpcDef>::const_iterator it = c.npcs().begin();
         it != c.npcs().end(); ++it)
        names.push_back(creature_tile_name("npc", it->first));
    for (std::map<std::string, EnemyDef>::const_iterator it = c.enemies().begin();
         it != c.enemies().end(); ++it)
        names.push_back(creature_tile_name("mob", it->first));
    return names;
}

// Вид по умолчанию — тот, которым карта рисуется без листа тайлов. Цвета и
// знаки для него лежат в tiles.cpp: оттуда их берёт и эта отрисовка, и
// экспорт листа. Две копии рано или поздно разошлись бы, и в файле оказалось
// бы не то, что на экране.
Color rgba_to_color(Rgba c) { return Color(c.r, c.g, c.b, c.a); }
Color tile_color(Tile t) { return rgba_to_color(default_tile_color(t)); }
Color object_color(unsigned cp) { return rgba_to_color(default_object_color(cp)); }

} // namespace

App::App()
    : log_lines_src_(0), log_lines_w_(0), log_lines_ep_(0),
      mode_(MODE_MENU), quit_(false), has_save_(false),
      create_step_(0), new_race_("human"), new_spec_("swordsman"),
      now_ms_(0), died_(false) {}

const std::vector<std::string>& App::log_lines(int cols) const {
    const std::vector<std::string>& lg = g_.log();
    const unsigned long ep = g_.log_epoch();
    if (cols < 4) cols = 4;
    if (log_lines_w_ == cols && log_lines_ep_ == ep && log_lines_src_ == lg.size())
        return log_lines_;

    // Ширина сменилась, или у журнала срезали начало (предел записей, новая
    // партия, загрузка) — раскладывать заново. Одной длины для этого мало:
    // после срезания журнал дорастает до прежней длины другими записями,
    // поэтому спрашивается счётчик срезаний. В обычном же случае журнал
    // только дописывается, и трогать разложенное незачем.
    if (log_lines_w_ != cols || log_lines_ep_ != ep || log_lines_src_ > lg.size()) {
        log_lines_.clear();
        log_lines_src_ = 0;
        log_lines_w_ = cols;
        log_lines_ep_ = ep;
    }
    for (std::size_t i = log_lines_src_; i < lg.size(); ++i) {
        const std::vector<std::string> part = wrap(lg[i], static_cast<std::size_t>(cols));
        for (std::size_t k = 0; k < part.size(); ++k) log_lines_.push_back(part[k]);
    }
    log_lines_src_ = lg.size();
    return log_lines_;
}

// ------------------------------------------------------------------- запуск

bool App::start(int argc, char** argv, int win_w, int win_h) {
    // Пути ищутся тем же кодом, что и в терминальной сборке: иначе игрок
    // сохранится в одной оболочке и не найдёт сохранения в другой.
    const char* argv0 = argc > 0 ? argv[0] : 0;
    save_dir_  = paths::save_dir(argv0);
    save_path_ = save_dir_ + "/hero.sav";
    data_root_ = paths::data_root(argv0);
    g_ = Game(data_root_);

    if (!c_.open("Любовь Эндора", win_w, win_h)) {
        SDL_Log("%s", c_.error().c_str());
        return false;
    }
    ptr_.configure(c_.touch_unit() / 3);

    // Лист тайлов необязателен: без него игра рисует как рисовала. Поэтому
    // отсутствие файла молчит, а вот испорченный файл — говорит: иначе
    // художник правил бы картинку и гадал, почему ничего не меняется.
    tiles_path_ = paths::tiles_dir(argv0);
    std::string terr;
    tiles_.load(c_.renderer(), tiles_path_, tiles_path_ + "/tiles.png",
                creature_tile_names(), &terr);
    // Пустой каталог молчит: графика необязательна. А вот файл, который есть,
    // но не читается, надо назвать — иначе художник правит картинку и гадает,
    // почему в игре ничего не меняется.
    if (!terr.empty()) SDL_Log("графика: %s", terr.c_str());

    refresh_save_summary();
    return true;
}

int App::run(int argc, char** argv) {
    if (!start(argc, argv, 900, 600)) return 1;
    while (!quit_) {
        now_ms_ = SDL_GetTicks();
        pump_events();
        step(now_ms_);
        draw();
        c_.present();
    }
    c_.close();
    return 0;
}

void App::step(unsigned now_ms) {
    now_ms_ = now_ms;

    Gesture gs;
    while (ptr_.poll(&gs)) {
        if (gs.kind == G_TAP) on_tap(gs.x, gs.y, gs.press);
        else if (gs.kind == G_SWIPE) on_swipe(gs.dx, gs.dy);
    }

    // Палец на карте — герой идёт к нему. Это состояние, а не событие:
    // держишь — идёт, ведёшь пальцем — цель едет следом.
    follow_finger();

    if (mode_ == MODE_PLAY && !modal_open() && !g_.combat().active) {
        if (walk_.update(g_, now_ms_)) {
            g_.world_turn();
            // Шаг привёл к чему-то: доводим до того же, к чему привёл бы
            // ручной шаг, иначе ходьба «упирается» в жителя и молчит.
            if (walk_.last_stop() == WS_EVENT) interact_after_walk();
        }
        if (g_.combat().active) walk_.stop();
    }

    // Смерть разбирается здесь, а не в бою: бой кончается, а герой мёртв,
    // и без этого игра просто продолжалась бы с нулём здоровья.
    if (mode_ == MODE_PLAY && !g_.combat().active && g_.player_dead() && !died_) {
        died_ = true;
        walk_.stop();
        close_all();
        push_message("Смерть",
                     "Твой путь обрывается здесь.\n\n"
                     "Последнее сохранение осталось нетронутым — "
                     "загрузи его из главного меню.");
        stack_.back().arg = "death";
    }

    if (!g_.log().empty()) status_ = g_.log().back();
}

void App::draw() {
    c_.begin(theme().bg);
    if (mode_ == MODE_MENU)        draw_main_menu();
    else if (mode_ == MODE_CREATE) draw_create_hero();
    else {
        draw_log();
        draw_world();
        draw_hud();
        if (g_.combat().active) draw_combat();
    }
    // Окна рисуются в любом режиме: справка из главного меню — такое же
    // окно, и если рисовать их только в игре, она просто не появится.
    // Затемнение кладётся перед каждым окном, а не только перед верхним:
    // два полупрозрачных окна подряд иначе просвечивают друг сквозь друга.
    for (std::size_t i = 0; i < stack_.size(); ++i) {
        dim_screen(c_, 140);
        draw_modal(stack_[i]);
    }
}

// -------------------------------------------------------------------- ввод

void App::pump_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                quit_ = true;
                break;

            // Android может убить приложение, пока оно в фоне, и не спросить.
            // Сохраняемся сами, иначе игрок теряет прогресс просто потому,
            // что переключился на другое окно.
            case SDL_APP_WILLENTERBACKGROUND:
            case SDL_APP_TERMINATING:
                if (mode_ == MODE_PLAY && g_.here()) save_game();
                break;

            case SDL_FINGERDOWN:
                ptr_.down(static_cast<int>(e.tfinger.fingerId),
                          static_cast<int>(e.tfinger.x * c_.width()),
                          static_cast<int>(e.tfinger.y * c_.height()), now_ms_);
                break;
            case SDL_FINGERMOTION:
                ptr_.move(static_cast<int>(e.tfinger.fingerId),
                          static_cast<int>(e.tfinger.x * c_.width()),
                          static_cast<int>(e.tfinger.y * c_.height()), now_ms_);
                break;
            case SDL_FINGERUP:
                ptr_.up(static_cast<int>(e.tfinger.fingerId),
                        static_cast<int>(e.tfinger.x * c_.width()),
                        static_cast<int>(e.tfinger.y * c_.height()), now_ms_);
                break;

            // Мышь на настольной машине идёт тем же путём, что палец:
            // одна ветка логики, а не две расходящиеся.
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.which != SDL_TOUCH_MOUSEID)
                    ptr_.down(-1, e.button.x, e.button.y, now_ms_);
                break;
            case SDL_MOUSEMOTION:
                if (e.motion.which != SDL_TOUCH_MOUSEID && (e.motion.state & SDL_BUTTON_LMASK))
                    ptr_.move(-1, e.motion.x, e.motion.y, now_ms_);
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.which != SDL_TOUCH_MOUSEID)
                    ptr_.up(-1, e.button.x, e.button.y, now_ms_);
                break;
            case SDL_MOUSEWHEEL:
                if (Modal* m = top()) scroll_modal(*m, -e.wheel.y);
                break;

            case SDL_KEYDOWN: {
                const SDL_Keycode k = e.key.keysym.sym;
                int code = 0;
                switch (k) {
                    case SDLK_UP:     code = platform::KEY_UP; break;
                    case SDLK_DOWN:   code = platform::KEY_DOWN; break;
                    case SDLK_LEFT:   code = platform::KEY_LEFT; break;
                    case SDLK_RIGHT:  code = platform::KEY_RIGHT; break;
                    case SDLK_ESCAPE:
                    case SDLK_AC_BACK: code = platform::KEY_ESC; break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER: code = '\r'; break;
                    case SDLK_BACKSPACE: code = 8; break;
                    default:
                        if (k >= 32 && k < 127) code = static_cast<int>(k);
                        break;
                }
                if (code) on_key(code);
                break;
            }

            case SDL_TEXTINPUT:
                on_text(e.text.text);
                break;

            default: break;
        }
    }
}

void App::on_tap(int x, int y, unsigned press) {
    // Открытое окно перехватывает касание в любом режиме — иначе окно поверх
    // главного меню было бы видно, но недоступно.
    if (Modal* m = top()) {
        // Кроме одного случая: окно открылось этим же касанием. Так бывает,
        // когда палец в упор к жителю — шаг упирается в него и заводит
        // разговор ещё до того, как палец убрали. Отпускание тогда пришлось
        // бы «мимо окна», и разговор закрывался бы сам собой.
        if (m->born_press != press) modal_tap(*m, x, y);
        return;
    }
    if (mode_ == MODE_MENU) { main_menu_tap(x, y); return; }
    if (mode_ == MODE_CREATE) { create_hero_tap(x, y); return; }
    if (g_.combat().active) { combat_tap(x, y); return; }
    world_tap(x, y);
}

void App::scroll_modal(Modal& m, int rows) {
    // Текстовые окна листаются своим счётчиком строк, списки — своим.
    // Крутить оба сразу — путаница: видно одно, а едет другое.
    if (is_text_modal(m.kind)) {
        m.scroll += rows;
        if (m.scroll < 0) m.scroll = 0;
        return;
    }
    m.list.scroll += rows;
    if (m.list.scroll < 0) m.list.scroll = 0;
}

void App::on_text(const char* utf8) {
    if (!utf8 || !*utf8) return;
    if (Modal* m = top()) {
        if (m->kind == Modal::TextInput && utf8_len(m->buffer) < m->max_len)
            m->buffer += utf8;
        return;
    }
    if (mode_ == MODE_CREATE && create_step_ == 0 && utf8_len(new_name_) < 16)
        new_name_ += utf8;
}

// Есть ли смысл идти в эту клетку. Стена — нет; всё остальное, вплоть до
// таблички и врага, — да: до них ходьба доводит и делает то же, что шаг
// вручную. Предикат один на удержание и на тап, иначе палец и касание
// понимали бы карту по-разному.
bool App::reachable_cell(const Location& loc, Vec2 cell) {
    return loc.walkable(cell) || loc.exit_at(cell) || loc.npc_at(cell) ||
           loc.sign_at(cell) || g_.mob_at(cell, loc.id);
}

void App::follow_finger() {
    if (mode_ != MODE_PLAY || modal_open() || g_.combat().active) return;
    if (!ptr_.pressed()) return;

    Vec2 cell;
    if (!cell_at(ptr_.press_x(), ptr_.press_y(), &cell)) return;

    // В стену идти незачем: палец на непроходимой клетке — это или промах,
    // или намерение открыть меню, и оба разбираются на отпускании.
    const Location* loc = g_.here();
    if (!loc || !reachable_cell(*loc, cell)) return;

    // Держишь дольше — герой переходит на бег. Отдельного жеста для бега
    // нет: удержание и есть движение, а долгое удержание — быстрое.
    const bool run = ptr_.held_ms(now_ms_) >= RUN_AFTER_MS;
    walk_.retarget(cell, run);
}

void App::on_swipe(int dx, int dy) {
    // В открытом окне свайп листает — список или текст, смотря что показано.
    if (Modal* m = top()) { scroll_modal(*m, dy); return; }
    // На карте свайп ничего не делает сам: он лишь ведёт палец, а за героем
    // следит follow_finger. Раньше свайп выдавал по шагу на каждый пройденный
    // порог, и одно движение пальца уносило героя на десяток клеток разом.
    (void)dx; (void)dy;
}

void App::world_tap(int x, int y) {
    // Тап по HUD-кнопкам разбирается там же, где они нарисованы.
    const Rect hud = hud_area();
    if (hud.contains(x, y)) {
        std::vector<Rect> b;
        hud_buttons(&b);
        static const Modal::Kind kinds[5] = {Modal::Character, Modal::Inventory,
                                             Modal::Quests, Modal::Skills, Modal::GameMenu};
        for (int i = 0; i < 5; ++i)
            if (b[static_cast<std::size_t>(i)].contains(x, y)) { push(kinds[i]); return; }
        return;
    }

    Vec2 cell;
    if (!cell_at(x, y, &cell)) return;

    const Location* loc = g_.here();
    if (!loc) return;

    // Тап по себе или в стену только останавливает ход. Меню открывается
    // кнопкой внизу окна — она всегда на месте, а окно, выскакивающее от
    // промаха по стене, мешало: целишься в проход, попадаешь в угол дома,
    // и вместо шага получаешь меню.
    const bool on_self = (cell.x == g_.player().pos.x && cell.y == g_.player().pos.y);
    if (on_self || !reachable_cell(*loc, cell)) {
        walk_.stop();
        return;
    }
    // По остальному тап задаёт цель и отпускает: короткое касание отправляет
    // героя туда и после того, как палец убрали, — «тапнул и пошёл».
    walk_.go(cell, false);
}

void App::on_key(int key) {
    // Окно перехватывает клавиши так же, как касания.
    if (Modal* m = top()) {
        if (m->kind == Modal::TextInput) {
            if (key == 8) {
                // Backspace убирает символ, а не байт: строка может быть русской.
                std::size_t i = m->buffer.size();
                while (i > 0 && (static_cast<unsigned char>(m->buffer[i - 1]) & 0xC0) == 0x80) --i;
                if (i > 0) m->buffer.erase(i - 1);
                return;
            }
            if (key == '\r') { commit_text_input(*m); return; }
            if (key == platform::KEY_ESC) { SDL_StopTextInput(); pop(); return; }
            return;
        }
        const int n = 1 << 20;
        if (key == platform::KEY_UP) {
            m->list.set_cursor(m->list.cursor <= 0 ? 0 : m->list.cursor - 1, n, 1);
            scroll_modal(*m, -1);
            return;
        }
        if (key == platform::KEY_DOWN) {
            m->list.set_cursor(m->list.cursor < 0 ? 0 : m->list.cursor + 1, n, 1);
            scroll_modal(*m, 1);
            return;
        }
        if (key == platform::KEY_ESC || key == 'm' || key == 'M') { close_top(); return; }
        if (key == '\r' || key == ' ') { activate_row(*m, m->list.cursor < 0 ? 0 : m->list.cursor); return; }
        return;
    }

    if (mode_ == MODE_MENU) {
        const int n = has_save_ ? 4 : 3;
        if (key == platform::KEY_UP)   menu_list_.set_cursor(menu_list_.cursor - 1, n, n);
        if (key == platform::KEY_DOWN) menu_list_.set_cursor(menu_list_.cursor + 1, n, n);
        if (key == '\r' || key == ' ')
            main_menu_tap(-1, -(menu_list_.cursor < 0 ? 0 : menu_list_.cursor) - 1);
        if (key == platform::KEY_ESC) quit_ = true;
        return;
    }

    if (mode_ == MODE_CREATE) {
        if (key == platform::KEY_ESC) { mode_ = MODE_MENU; return; }
        if (create_step_ == 0) {
            if (key == 8 && !new_name_.empty()) {
                // Backspace должен убирать символ, а не байт: имя может быть русским.
                std::size_t i = new_name_.size();
                while (i > 0 && (static_cast<unsigned char>(new_name_[i - 1]) & 0xC0) == 0x80) --i;
                if (i > 0) new_name_.erase(i - 1);
                return;
            }
            if (key == '\r') { create_step_ = 1; create_list_ = ListView(); SDL_StopTextInput(); return; }
            return;
        }
        const Content& c = Content::get();
        const int n = create_step_ == 1 ? static_cast<int>(c.races().size())
                                        : static_cast<int>(c.specs().size());
        if (key == platform::KEY_UP)
            create_list_.set_cursor(create_list_.cursor <= 0 ? 0 : create_list_.cursor - 1, n, n);
        if (key == platform::KEY_DOWN)
            create_list_.set_cursor(create_list_.cursor < 0 ? 0 : create_list_.cursor + 1, n, n);
        if (key == '\r' || key == ' ')
            create_hero_tap(-1, -(create_list_.cursor < 0 ? 0 : create_list_.cursor) - 1);
        return;
    }

    // --- в игре ---
    if (g_.combat().active) {
        switch (key) {
            case '\r': case ' ': g_.combat_attack(false); break;
            case '1': g_.combat_set_stance(Stance::Cautious); break;
            case '2': g_.combat_set_stance(Stance::Balanced); break;
            case '3': g_.combat_set_stance(Stance::Fierce); break;
            case 'e': case 'E': g_.combat_end_turn(); break;
            case 'r': case 'R': g_.combat_flee(); break;
            case 'i': case 'I': push(Modal::Inventory); break;
            default: break;
        }
        return;
    }

    switch (key) {
        case platform::KEY_UP:    case 'w': case 'W': walk_.stop(); step_player(0, -1); return;
        case platform::KEY_DOWN:  case 's': case 'S': walk_.stop(); step_player(0,  1); return;
        case platform::KEY_LEFT:  case 'a': case 'A': walk_.stop(); step_player(-1, 0); return;
        case platform::KEY_RIGHT: case 'd': case 'D': walk_.stop(); step_player( 1, 0); return;
        case 'c': case 'C': push(Modal::Character);  return;
        case 'i': case 'I': push(Modal::Inventory);  return;
        case 'q': case 'Q': push(Modal::Quests);     return;
        case 'k': case 'K': push(Modal::Skills);     return;
        case 'p': case 'P': push(Modal::Portals);    return;
        case 'b': case 'B': push(Modal::Library);    return;
        case 'l': case 'L': push(Modal::Log);        return;
        case '?':           push(Modal::Help);       return;
        case 'm': case 'M':
        case platform::KEY_ESC: push(Modal::GameMenu); return;
        default: return;
    }
}

// ---------------------------------------------------------------- мир: шаг

void App::step_player(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    const Bump b = g_.try_move(dx, dy);
    handle_bump(b, dx, dy);
}

void App::handle_bump(Bump b, int dx, int dy) {
    const Location* loc = g_.here();
    switch (b) {
        case Bump::Npc: {
            if (loc) {
                Vec2 p(g_.player().pos.x + dx, g_.player().pos.y + dy);
                if (const MapNpc* n = loc->npc_at(p)) begin_dialogue(n->npc_id);
            }
            return;                      // разговор не тратит ход мира
        }
        case Bump::Sign: {
            if (loc) {
                Vec2 p(g_.player().pos.x + dx, g_.player().pos.y + dy);
                if (const MapSign* s = loc->sign_at(p)) push_message("Табличка", s->text);
            }
            return;
        }
        case Bump::Blocked:
        case Bump::Chest:
            return;
        case Bump::Combat:
            walk_.stop();
            return;
        default:
            break;
    }
    g_.world_turn();
}

void App::interact_after_walk() {
    const Location* loc = g_.here();
    if (!loc) return;
    const Vec2 p(g_.player().pos.x + walk_.last_dx(), g_.player().pos.y + walk_.last_dy());
    switch (walk_.last_bump()) {
        case Bump::Npc:
            if (const MapNpc* n = loc->npc_at(p)) begin_dialogue(n->npc_id);
            return;
        case Bump::Sign:
            if (const MapSign* s = loc->sign_at(p)) push_message("Табличка", s->text);
            return;
        default:
            return;
    }
}

void App::begin_dialogue(const std::string& npc_id) {
    const NpcDef* n = Content::get().npc(npc_id);
    if (!n) return;
    Modal m(Modal::Dialogue);
    m.arg  = npc_id;
    m.node = n->root;
    push_modal(m);
}

// -------------------------------------------------------------- камера/карта

Rect App::hud_area() const {
    const int h = c_.touch_unit() + c_.cell_h() * 4;
    return Rect(0, c_.height() - h, c_.width(), h);
}

Rect App::map_area() const {
    const Rect hud = hud_area();
    return Rect(0, 0, c_.width(), hud.y);
}

int App::map_scale() const {
    const Rect area = map_area();
    const int want_cols = 15;
    int sc = area.w / (want_cols * FONT_H);
    const int by_h = area.h / (want_cols * FONT_H);
    if (by_h < sc) sc = by_h;
    if (sc < 1) sc = 1;
    if (sc > 8) sc = 8;
    return sc;
}

int App::map_cell() const { return FONT_H * map_scale(); }

Rect App::map_block() const {
    const Rect area = map_area();
    int ox, oy, cols, rows;
    camera(&ox, &oy, &cols, &rows);
    const int cw = map_cell(), chh = map_cell();
    const int w = cols * cw, h = rows * chh;
    // Карта прижата книзу, вплотную к панели. На телефоне в вертикальном
    // режиме по ней и тыкают чаще всего, а большой палец достаёт снизу;
    // журнал же читают, но не трогают, и ему место наверху.
    return Rect(area.x + (area.w - w) / 2, area.y + area.h - h, w, h);
}

Rect App::log_area() const {
    const Rect area = map_area();
    const Rect blk = map_block();
    return Rect(area.x, area.y, area.w, blk.y - area.y);
}

void App::camera(int* cx, int* cy, int* cols, int* rows) const {
    const Location* loc = g_.here();
    const Rect area = map_area();
    const int cw = map_cell(), chh = map_cell();
    int vw = area.w / cw, vh = area.h / chh;
    if (!loc) { *cx = *cy = 0; *cols = vw; *rows = vh; return; }
    if (vw > loc->w) vw = loc->w;
    if (vh > loc->h) vh = loc->h;

    int ox = g_.player().pos.x - vw / 2;
    int oy = g_.player().pos.y - vh / 2;
    if (ox + vw > loc->w) ox = loc->w - vw;
    if (oy + vh > loc->h) oy = loc->h - vh;
    if (ox < 0) ox = 0;
    if (oy < 0) oy = 0;
    *cx = ox; *cy = oy; *cols = vw; *rows = vh;
}

bool App::cell_at(int x, int y, Vec2* out) const {
    const Rect blk = map_block();
    if (!blk.contains(x, y)) return false;
    int ox, oy, cols, rows;
    camera(&ox, &oy, &cols, &rows);
    const int gx = (x - blk.x) / map_cell(), gy = (y - blk.y) / map_cell();
    if (gx < 0 || gy < 0 || gx >= cols || gy >= rows) return false;
    if (out) *out = Vec2(ox + gx, oy + gy);
    return true;
}

void App::draw_world() {
    const Location* loc = g_.here();
    if (!loc) {
        c_.text(c_.cell_w(), c_.cell_h(), "Локация не загружена", theme().warn, c_.scale());
        return;
    }

    int ox, oy, cols, rows;
    camera(&ox, &oy, &cols, &rows);
    const int sc = map_scale();
    const int cw = map_cell(), chh = map_cell();
    const Rect blk = map_block();
    const int left = blk.x;
    const int top  = blk.y;
    // Глиф уже клетки: центрируем, иначе знаки липнут к левому краю.
    const int gpad = (cw - FONT_W * sc) / 2;

    for (int gy = 0; gy < rows; ++gy) {
        for (int gx = 0; gx < cols; ++gx) {
            const Vec2 p(ox + gx, oy + gy);
            const Rect cell(left + gx * cw, top + gy * chh, cw, chh);
            // В темноте клетка не рисуется вовсе — не затемняется, а именно
            // остаётся чёрной: игрок не должен угадывать карту по силуэту.
            if (!g_.cell_lit(p)) { c_.fill(cell, Color(0, 0, 0, 255)); continue; }
            const Tile t = loc->at(p);
            // Нарисованный тайл заменяет вид по умолчанию, ненарисованный —
            // нет. Поэтому набор можно рисовать по одному тайлу: готовое
            // появляется в игре сразу, остальное выглядит как раньше.
            if (tiles_.draw(c_.renderer(), slot_of_tile(t), cell)) continue;
            c_.fill(cell, tile_color(t));
            const unsigned tg = default_tile_texture(t);
            if (tg) c_.glyph_at(cell.x + gpad, cell.y, tg, Color(0, 0, 0, 70), sc);
        }
    }

    // Объекты поверх местности, в том же порядке, что и в терминале.
    // art — имя собственного тайла существа, если он у него есть. У прочих
    // объектов пусто: им хватает слота своего вида.
    struct Obj { Vec2 p; unsigned cp; std::string art; };
    std::vector<Obj> objs;
    for (std::size_t i = 0; i < loc->items.size(); ++i)
        if (!g_.item_taken(loc->id, static_cast<int>(i)))
            { Obj o; o.p = loc->items[i].pos; o.cp = glyph::ITEM; objs.push_back(o); }
    for (std::size_t i = 0; i < loc->notes.size(); ++i)
        if (!g_.note_taken(loc->id, static_cast<int>(i)))
            { Obj o; o.p = loc->notes[i].pos; o.cp = glyph::NOTE; objs.push_back(o); }
    for (std::size_t i = 0; i < loc->chests.size(); ++i)
        if (!g_.chest_opened(loc->id, static_cast<int>(i)))
            { Obj o; o.p = loc->chests[i].pos; o.cp = glyph::CHEST; objs.push_back(o); }
    for (const MapSign& s : loc->signs) { Obj o; o.p = s.pos; o.cp = glyph::SIGN; objs.push_back(o); }
    for (const MapExit& e : loc->exits) { Obj o; o.p = e.pos; o.cp = glyph::EXIT; objs.push_back(o); }
    for (const Vec2& b : loc->beds)    { Obj o; o.p = b;     o.cp = glyph::BED;  objs.push_back(o); }
    for (const Portal& pt : g_.player().portals)
        if (pt.loc == loc->id) { Obj o; o.p = pt.pos; o.cp = glyph::PORTAL; objs.push_back(o); }
    for (const MapNpc& n : loc->npcs) {
        Obj o; o.p = n.pos; o.cp = glyph::NPC;
        o.art = creature_tile_name("npc", n.npc_id);
        objs.push_back(o);
    }
    for (const Mob& m : g_.mobs())
        if (m.loc == loc->id) {
            Obj o; o.p = m.pos; o.cp = glyph::MOB;
            o.art = creature_tile_name("mob", m.enemy_id);
            objs.push_back(o);
        }

    for (const Obj& o : objs) {
        const int gx = o.p.x - ox, gy = o.p.y - oy;
        if (gx < 0 || gy < 0 || gx >= cols || gy >= rows) continue;
        if (!g_.cell_lit(o.p)) continue;
        const Rect cell(left + gx * cw, top + gy * chh, cw, chh);
        // Свой тайл существа важнее общего: нарисованный житель выглядит
        // собой, а ненарисованный — как все жители, и только потом уже
        // знаком шрифта. Три ступени, и каждая работает сама по себе.
        if (!o.art.empty() && tiles_.draw_named(c_.renderer(), o.art, cell)) continue;
        if (tiles_.draw(c_.renderer(), slot_of_glyph(o.cp), cell)) continue;
        c_.glyph_at(cell.x + gpad, cell.y, o.cp, object_color(o.cp), sc);
    }

    // Герой рисуется последним: его видно всегда.
    const int px = g_.player().pos.x - ox, py = g_.player().pos.y - oy;
    if (px >= 0 && py >= 0 && px < cols && py < rows) {
        const Rect cell(left + px * cw, top + py * chh, cw, chh);
        if (!tiles_.draw(c_.renderer(), SLOT_PLAYER, cell)) {
            c_.fill(cell, rgba_to_color(default_player_backing()));
            c_.glyph_at(cell.x + gpad, cell.y, glyph::PLAYER,
                        object_color(glyph::PLAYER), sc);
        }
    }

    // Цель ходьбы: видно, куда герой идёт и что приказ принят.
    if (walk_.active()) {
        const int tx = walk_.target().x - ox, ty = walk_.target().y - oy;
        if (tx >= 0 && ty >= 0 && tx < cols && ty < rows && g_.cell_lit(walk_.target()))
            c_.frame(Rect(left + tx * cw, top + ty * chh, cw, chh),
                     walk_.running() ? theme().accent : theme().good, 2);
    }
}

// ------------------------------------------------- прогон по сценарию

int App::run_script(const std::vector<std::string>& script, const std::string& out_dir,
                    int argc, char** argv) {
    if (!start(argc, argv, 480, 900)) return 1;   // портрет, как на телефоне
    if (!out_dir.empty()) {
        // Сценарий не должен трогать настоящее сохранение игрока.
        save_dir_  = out_dir;
        save_path_ = out_dir + "/hero.sav";
        refresh_save_summary();
    }

    unsigned clock = 0;
    int fake_id = 100;
    const unsigned FRAME_MS = 16;

    for (std::size_t i = 0; i < script.size() && !quit_; ++i) {
        std::istringstream ls(script[i]);
        std::string cmd;
        ls >> cmd;

        if (cmd == "tap") {
            int x = 0, y = 0; ls >> x >> y;
            ++fake_id;
            ptr_.down(fake_id, x, y, clock);
            clock += 40;
            ptr_.up(fake_id, x, y, clock);
        } else if (cmd == "hold") {
            // Палец опускается и остаётся: дальше его двигают drag-ом,
            // а отпускают release-ом. Ходьбу ведёт именно это состояние.
            int x = 0, y = 0; ls >> x >> y;
            ++fake_id;
            ptr_.down(fake_id, x, y, clock);
        } else if (cmd == "drag") {
            int x = 0, y = 0; ls >> x >> y;
            ptr_.move(fake_id, x, y, clock);
        } else if (cmd == "release") {
            ptr_.up(fake_id, ptr_.press_x(), ptr_.press_y(), clock);
        } else if (cmd == "swipe") {
            int dx = 0, dy = 0; ls >> dx >> dy;
            ++fake_id;
            ptr_.down(fake_id, 200, 300, clock);
            clock += 20;
            ptr_.move(fake_id, 200 + dx * 200, 300 + dy * 200, clock);
            ptr_.up(fake_id, 200 + dx * 200, 300 + dy * 200, clock);
        } else if (cmd == "key") {
            std::string k; ls >> k;
            if (k == "esc") on_key(platform::KEY_ESC);
            else if (k == "up") on_key(platform::KEY_UP);
            else if (k == "down") on_key(platform::KEY_DOWN);
            else if (k == "enter") on_key('\r');
            else if (!k.empty()) on_key(static_cast<unsigned char>(k[0]));
        } else if (cmd == "wait") {
            // Ожидание идёт кадрами, а не одним прыжком часов: удержание
            // пальца проверяется ровно тем же способом, каким работает
            // живой цикл, иначе за секунду ожидания вышел бы один шаг.
            int ms = 0; ls >> ms;
            for (int left = ms; left > 0; left -= static_cast<int>(FRAME_MS)) {
                clock += FRAME_MS;
                step(clock);
            }
        } else if (cmd == "type") {
            std::string rest;
            std::getline(ls, rest);
            if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
            on_text(rest.c_str());
        } else if (cmd == "where") {
            // Печать положения героя: по ней сценарий проверяется снаружи,
            // как снимки экрана проверяют рисование.
            std::printf("where %d %d %s %s\n", g_.player().pos.x, g_.player().pos.y,
                        walk_.active() ? (walk_.running() ? "run" : "walk") : "stand",
                        g_.here() ? g_.here()->id.c_str() : "-");
            std::fflush(stdout);
        } else if (cmd == "quit") {
            quit_ = true;
        }

        // Дальше — ровно тот же шаг и то же рисование, что в живом цикле:
        // сценарий проверяет игру, а не свою копию игры.
        step(clock);
        draw();

        if (cmd == "shot") {
            std::string name; ls >> name;
            SDL_Surface* surf = new_surface32(c_.width(), c_.height());
            if (surf && SDL_RenderReadPixels(c_.renderer(), 0, surf->format->format,
                                             surf->pixels, surf->pitch) == 0)
                SDL_SaveBMP(surf, (out_dir + "/" + name + ".bmp").c_str());
            if (surf) SDL_FreeSurface(surf);
        }
        c_.present();
        clock += FRAME_MS;
    }

    c_.close();
    return 0;
}

} // namespace gfx
