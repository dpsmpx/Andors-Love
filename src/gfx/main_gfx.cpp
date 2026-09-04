// Точка входа графической сборки. Терминальная (src/main.cpp) остаётся
// на месте: логика у них общая, различается только оболочка.
#include "app.h"
#include "font.h"
#include "tiles.h"

// SDL2 на Android подменяет точку входа макросом (main -> SDL_main), и
// заголовок обязан быть виден именно в файле с main(). Без него APK
// собирается, но не запускается: системе нечего вызывать.
#include <SDL2/SDL.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    // Экспорт графики: сохраняет то, чем игра рисует карту сейчас, картинками
    // по одной на тайл. Их и открывает редактор — между «нарисовал» и «увидел
    // в игре» не остаётся ни одного преобразования. Окно для этого не нужно:
    // пиксели считает тот же код, что и рисование.
    const bool to_dir   = (argc >= 3 && std::strcmp(argv[1], "--export-tiles") == 0);
    const bool to_sheet = (argc >= 3 && std::strcmp(argv[1], "--export-sheet") == 0);
    if (to_dir || to_sheet) {
        const int cap = to_sheet ? gfx::TJTM_MAX_TILE : gfx::TILE_MAX_PX;
        int size = gfx::TILE_PX;
        if (argc >= 4) {
            size = std::atoi(argv[3]);
            if (size < gfx::FONT_H || size > cap) {
                std::cerr << "размер тайла должен быть от " << gfx::FONT_H
                          << " до " << cap << "\n";
                return 2;
            }
        }

        std::string err;
        if (to_dir) {
            if (!gfx::save_slot_files(argv[2], size, &err)) {
                std::cerr << "не сохранить в " << argv[2] << ": " << err << "\n";
                return 1;
            }
            std::cout << "тайлы " << size << "x" << size << " по одному файлу на слот"
                      << " сохранены в " << argv[2] << "\n";
            return 0;
        }

        // Лист одним файлом: редактор его больше не открывает, но так удобно
        // увезти весь набор целиком.
        const gfx::Image sheet = gfx::default_sheet(size);
        if (!gfx::png_write(argv[2], sheet, &err)) {
            std::cerr << "не сохранить " << argv[2] << ": " << err << "\n";
            return 1;
        }
        std::cout << "лист " << sheet.w << "x" << sheet.h
                  << " (тайл " << size << "x" << size << ") сохранён в "
                  << argv[2] << "\n";
        return 0;
    }

    gfx::App app;

    // Прогон по сценарию: нужен, чтобы графику можно было проверить без
    // экрана и без пальцев — на сборочной машине и в тестах.
    if (argc >= 4 && std::strcmp(argv[1], "--script") == 0) {
        std::ifstream in(argv[2]);
        if (!in) { std::cerr << "не открыть сценарий: " << argv[2] << "\n"; return 2; }
        std::vector<std::string> lines;
        std::string l;
        while (std::getline(in, l)) {
            if (!l.empty() && l[l.size() - 1] == '\r') l.erase(l.size() - 1);
            if (!l.empty() && l[0] != '#') lines.push_back(l);
        }
        return app.run_script(lines, argv[3], argc, argv);
    }

    return app.run(argc, argv);
}
