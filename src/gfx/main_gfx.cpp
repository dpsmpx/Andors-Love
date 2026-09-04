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
    // Экспорт листа тайлов: сохраняет то, чем игра рисует карту сейчас, в
    // картинку для редактора. С неё удобно начинать рисовать свою графику —
    // размеры, раскладка и разделители уже такие, какие нужны.
    // Окно для этого не нужно: пиксели считает тот же код, что и рисование.
    if (argc >= 3 && std::strcmp(argv[1], "--export-tiles") == 0) {
        int size = gfx::TILE_PX;
        if (argc >= 4) {
            size = std::atoi(argv[3]);
            if (size < gfx::FONT_H || size > gfx::TJTM_MAX_TILE) {
                std::cerr << "размер тайла должен быть от " << gfx::FONT_H
                          << " до " << gfx::TJTM_MAX_TILE << "\n";
                return 2;
            }
        }
        const gfx::Image sheet = gfx::default_sheet(size);
        std::string err;
        if (!gfx::png_write(argv[2], sheet, &err)) {
            std::cerr << "не сохранить " << argv[2] << ": " << err << "\n";
            return 1;
        }
        std::cout << "лист тайлов " << sheet.w << "x" << sheet.h
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
