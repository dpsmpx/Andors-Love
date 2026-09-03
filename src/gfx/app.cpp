#include "app.h"
#include "../content.h"
#include "../paths.h"
#include "../platform.h"
#include "font.h"

#include <SDL2/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace gfx {

namespace {

// Цвет клетки по типу местности. Мир рисуется цветом, а не только знаком:
// с этого и начинается «переход на графику».
Color tile_color(Tile t) {
    switch (t) {
        case Tile::Floor:     return Color(52, 50, 46);
        case Tile::Wall:      return Color(78, 74, 68);
        case Tile::Water:     return Color(34, 62, 104);
        case Tile::Tree:      return Color(30, 62, 38);
        case Tile::Grass:     return Color(38, 58, 34);
        case Tile::Road:      return Color(72, 62, 44);
        case Tile::DeadWater: return Color(44, 58, 58);
        default:              return Color(20, 20, 20);
    }
}

// Мелкая рябь поверх заливки: без неё поле выглядит плоской клеёнкой.
unsigned tile_texture(Tile t) {
    switch (t) {
        case Tile::Wall:      return 0x2592;   // ▒
        case Tile::Tree:      return 'T';
        case Tile::Grass:     return ',';
        case Tile::Water:     return '~';
        case Tile::DeadWater: return ':';
        default:              return 0;
    }
}

Color object_color(unsigned cp) {
    switch (cp) {
        case glyph::PLAYER: return Color(255, 248, 220);
        case glyph::NPC:    return Color(120, 200, 255);
        case glyph::MOB:    return Color(232, 96, 88);
        case glyph::EXIT:   return Color(240, 216, 120);
        case glyph::SIGN:   return Color(200, 190, 150);
        case glyph::ITEM:   return Color(140, 230, 150);
        case glyph::BED:    return Color(210, 160, 220);
        case glyph::CHEST:  return Color(230, 180, 100);
        case glyph::PORTAL: return Color(180, 140, 255);
        case glyph::NOTE:   return Color(230, 230, 160);
        default:            return Color(200, 200, 200);
    }
}

} // namespace

App::App()
    : mode_(MODE_MENU), quit_(false), has_save_(false),
      create_step_(0), new_race_("human"), new_spec_("swordsman"),
      now_ms_(0), died_(false) {}

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
        if (gs.kind == G_TAP) on_tap(gs.x, gs.y);
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

void App::on_tap(int x, int y) {
    // Открытое окно перехватывает касание в любом режиме — иначе окно поверх
    // главного меню было бы видно, но недоступно.
    if (Modal* m = top()) { modal_tap(*m, x, y); return; }
    if (mode_ == MODE_MENU) { main_menu_tap(x, y); return; }
    if (mode_ == MODE_CREATE) { create_hero_tap(x, y); return; }
    if (g_.combat().active) { combat_tap(x, y); return; }
    world_tap(x, y);
}

void App::scroll_modal(Modal& m, int rows) {
    // Текстовые окна листаются своим счётчиком строк, списки — своим.
    // Крутить оба сразу — путаница: видно одно, а едет другое.
    if (m.kind == Modal::Message || m.kind == Modal::Help ||
        m.kind == Modal::Character || m.kind == Modal::Book || m.kind == Modal::Ending) {
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

    // Тап по себе или в стену — меню, но только если герой стоит. На ходу
    // то же касание просто останавливает его: меню, выскочившее посреди
    // дороги от промаха по стене, — последнее, что нужно бегущему.
    const bool on_self = (cell.x == g_.player().pos.x && cell.y == g_.player().pos.y);
    if (on_self || !reachable_cell(*loc, cell)) {
        if (walk_.active()) { walk_.stop(); return; }
        push(Modal::GameMenu);
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
        case 'f': case 'F': push(Modal::Effects);    return;
        case 'p': case 'P': push(Modal::Portals);    return;
        case 'b': case 'B': push(Modal::Library);    return;
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
    stack_.push_back(m);
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

int App::map_bottom() const {
    int ox, oy, cols, rows;
    camera(&ox, &oy, &cols, &rows);
    return map_area().y + rows * map_cell();
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
    const Rect area = map_area();
    if (!area.contains(x, y)) return false;
    int ox, oy, cols, rows;
    camera(&ox, &oy, &cols, &rows);
    const int cw = map_cell(), chh = map_cell();
    const int left = area.x + (area.w - cols * cw) / 2;
    const int top  = area.y;
    const int gx = (x - left) / cw, gy = (y - top) / chh;
    if (x < left || y < top || gx >= cols || gy >= rows) return false;
    if (out) *out = Vec2(ox + gx, oy + gy);
    return true;
}

void App::draw_world() {
    const Location* loc = g_.here();
    const Rect area = map_area();
    if (!loc) {
        c_.text(c_.cell_w(), c_.cell_h(), "Локация не загружена", theme().warn, c_.scale());
        return;
    }

    int ox, oy, cols, rows;
    camera(&ox, &oy, &cols, &rows);
    const int sc = map_scale();
    const int cw = map_cell(), chh = map_cell();
    const int left = area.x + (area.w - cols * cw) / 2;
    const int top  = area.y;
    // Глиф уже клетки: центрируем, иначе знаки липнут к левому краю.
    const int gpad = (cw - FONT_W * sc) / 2;

    for (int gy = 0; gy < rows; ++gy) {
        for (int gx = 0; gx < cols; ++gx) {
            const Vec2 p(ox + gx, oy + gy);
            const Tile t = loc->at(p);
            const Rect cell(left + gx * cw, top + gy * chh, cw, chh);
            c_.fill(cell, tile_color(t));
            const unsigned tg = tile_texture(t);
            if (tg) c_.glyph_at(cell.x + gpad, cell.y, tg, Color(0, 0, 0, 70), sc);
        }
    }

    // Объекты поверх местности, в том же порядке, что и в терминале.
    struct Obj { Vec2 p; unsigned cp; };
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
    for (const MapNpc& n : loc->npcs)   { Obj o; o.p = n.pos; o.cp = glyph::NPC;  objs.push_back(o); }
    for (const Mob& m : g_.mobs())
        if (m.loc == loc->id) { Obj o; o.p = m.pos; o.cp = glyph::MOB; objs.push_back(o); }

    for (const Obj& o : objs) {
        const int gx = o.p.x - ox, gy = o.p.y - oy;
        if (gx < 0 || gy < 0 || gx >= cols || gy >= rows) continue;
        c_.glyph_at(left + gx * cw + gpad, top + gy * chh, o.cp, object_color(o.cp), sc);
    }

    // Герой рисуется последним, с подложкой: его видно всегда.
    const int px = g_.player().pos.x - ox, py = g_.player().pos.y - oy;
    if (px >= 0 && py >= 0 && px < cols && py < rows) {
        const Rect cell(left + px * cw, top + py * chh, cw, chh);
        c_.fill(cell, Color(120, 110, 60, 140));
        c_.glyph_at(cell.x + gpad, cell.y, glyph::PLAYER, object_color(glyph::PLAYER), sc);
    }

    // Цель ходьбы: видно, куда герой идёт и что приказ принят.
    if (walk_.active()) {
        const int tx = walk_.target().x - ox, ty = walk_.target().y - oy;
        if (tx >= 0 && ty >= 0 && tx < cols && ty < rows)
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
