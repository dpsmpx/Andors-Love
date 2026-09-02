#include "game.h"
#include "platform.h"
#include "ui.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace {

// Каталог сохранений выбирается один раз при старте: внутри APK рабочий
// каталог может оказаться недоступным для записи, и молча терять прогресс
// нельзя.
std::string g_save_dir  = "saves";
std::string g_save_file = "saves/hero.sav";

// Каталог годится, только если в него удаётся создать и записать файл.
bool dir_writable(const std::string& dir) {
    if (dir.empty()) return false;
    if (!platform::make_dir(dir)) return false;
    const std::string probe = dir + "/.write-test";
    {
        std::ofstream out(probe);
        if (!out) return false;
        out << 'x';
        if (!out) return false;
    }
    std::remove(probe.c_str());
    return true;
}

std::string find_save_dir() {
    if (const char* env = std::getenv("ANDORS_LOVE_SAVE"))
        if (dir_writable(env)) return env;

    if (dir_writable("saves")) return "saves";

    if (const char* home = std::getenv("HOME")) {
        const std::string d = std::string(home) + "/.andors-love";
        if (dir_writable(d)) return d;
    }
    if (const char* tmp = std::getenv("TMPDIR")) {
        const std::string d = std::string(tmp) + "/andors-love";
        if (dir_writable(d)) return d;
    }
    return "saves";   // не нашли — оставим привычный путь ради понятной ошибки
}

bool has_maps(const std::string& dir) {
    if (dir.empty()) return false;
    std::ifstream probe(dir + "/village.map");
    return static_cast<bool>(probe);
}

// Каталог карт ищем по очереди: переменная окружения, рабочий каталог, рядом
// с исполняемым файлом. На Android каталог проекта часто лежит на /sdcard,
// откуда запускать бинарники нельзя, поэтому «рядом с бинарником» и явная
// переменная — не роскошь, а рабочий сценарий.
std::string find_data_root(const char* argv0) {
    if (const char* env = std::getenv("ANDORS_LOVE_DATA"))
        if (has_maps(env)) return env;

    if (has_maps("data/maps")) return "data/maps";

    const std::string exe = platform::exe_dir(argv0);
    if (!exe.empty() && has_maps(exe + "/data/maps")) return exe + "/data/maps";

    return "data/maps";     // вернём привычный путь, чтобы текст ошибки был понятным
}

// Возвращает false, если игрок вышел в главное меню.
void play(Game& g) {
    for (;;) {
        if (g.combat().active) {
            ui::run_combat(g);
            if (g.player_dead()) {
                ui::message_box("Смерть",
                    "  Твой путь обрывается здесь.\n\n"
                    "  Последнее сохранение осталось нетронутым —\n"
                    "  загрузи его из главного меню.");
                return;
            }
            g.world_turn();
            continue;
        }

        ui::draw_world(g);
        int k = platform::read_key();
        if (k == platform::KEY_EOF) return;

        int dx = 0, dy = 0;
        switch (k) {
            case platform::KEY_UP:    case 'w': case 'W': dy = -1; break;
            case platform::KEY_DOWN:  case 's': case 'S': dy =  1; break;
            case platform::KEY_LEFT:  case 'a': case 'A': dx = -1; break;
            case platform::KEY_RIGHT: case 'd': case 'D': dx =  1; break;

            case 'c': case 'C': ui::screen_character(g); continue;
            case 'i': case 'I': ui::screen_inventory(g); continue;
            case 'q': case 'Q': ui::screen_quests(g);    continue;
            case 'k': case 'K': ui::screen_skills(g);    continue;
            case '?':           ui::help_screen();       continue;

            case '1': g.combat_set_stance(Stance::Cautious);
                      g.msg("Стойка: осторожная."); continue;
            case '2': g.combat_set_stance(Stance::Balanced);
                      g.msg("Стойка: ровная.");     continue;
            case '3': g.combat_set_stance(Stance::Fierce);
                      g.msg("Стойка: яростная.");   continue;

            case platform::KEY_ESC: {
                std::vector<std::string> opts{"Вернуться в игру", "Сохранить игру",
                                              "Загрузить сохранение",
                                              "Выйти в главное меню"};
                int s = ui::choose("Пауза", opts);
                if (s == 1) {
                    platform::make_dir(g_save_dir);
                    if (g.save_to(g_save_file)) g.msg("Игра сохранена: " + g_save_file);
                    else ui::message_box("Ошибка сохранения", "  " + g.error());
                } else if (s == 2) {
                    if (!g.load_from(g_save_file))
                        ui::message_box("Ошибка загрузки", "  " + g.error());
                } else if (s == 3) {
                    return;
                }
                continue;
            }
            default: continue;
        }

        if (dx == 0 && dy == 0) continue;

        Bump b = g.try_move(dx, dy);
        switch (b) {
            case Bump::Npc: {
                const Location* loc = g.here();
                Vec2 p{g.player().pos.x + dx, g.player().pos.y + dy};
                if (loc)
                    if (const MapNpc* n = loc->npc_at(p)) ui::run_dialogue(g, n->npc_id);
                continue;                       // разговор не тратит ход мира
            }
            case Bump::Sign: {
                const Location* loc = g.here();
                Vec2 p{g.player().pos.x + dx, g.player().pos.y + dy};
                if (loc)
                    if (const MapSign* s = loc->sign_at(p))
                        ui::message_box("Табличка", "  " + s->text);
                continue;
            }
            case Bump::Blocked:
                continue;
            case Bump::Combat:
                continue;                       // бой обработается в начале цикла
            default:
                break;
        }
        g.world_turn();
    }
}

} // namespace

int main(int argc, char** argv) {
    platform::RawMode raw;
    platform::hide_cursor();

    g_save_dir  = find_save_dir();
    g_save_file = g_save_dir + "/hero.sav";

    Game g(find_data_root(argc > 0 ? argv[0] : nullptr));
    while (!platform::input_closed()) {
        std::vector<std::string> items{
            "Новая игра", "Продолжить (загрузить сохранение)", "Справка", "Выход"
        };
        int sel = ui::choose("ЛЮБОВЬ ЭНДОРА\n  роглайк в духе «Следа Эндора»\n", items,
                             "  Стрелки — выбор, Enter — принять");

        // Каждый пункт обрабатывается ровно один раз: «Справка» показывает
        // справку, «Выход» выходит.
        if (sel == 0) {
            std::string name = ui::read_line("Как тебя звать?", "Странник");
            g.new_game(name);
            if (!g.here()) {
                ui::message_box("Не удалось начать игру",
                                "  " + g.world().last_error() + "\n\n"
                                "  Карты ищутся в data/maps рядом с рабочим\n"
                                "  каталогом или с самим бинарником. Запусти\n"
                                "  игру из корня проекта либо укажи путь явно:\n"
                                "  ANDORS_LOVE_DATA=/путь/к/data/maps ./andors-love");
                continue;
            }
            play(g);
        } else if (sel == 1) {
            if (!g.load_from(g_save_file)) {
                ui::message_box("Загрузка не удалась", "  " + g.error());
                continue;
            }
            play(g);
        } else if (sel == 2) {
            ui::help_screen();
        } else {
            break;                              // -1 (Esc) или «Выход»
        }
    }

    platform::show_cursor();
    platform::clear_screen();
    std::cout << "До встречи в Ольховке.\n";
    return 0;
}
