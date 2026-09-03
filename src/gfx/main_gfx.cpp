// Точка входа графической сборки. Терминальная (src/main.cpp) остаётся
// на месте: логика у них общая, различается только оболочка.
#include "app.h"

// SDL2 на Android подменяет точку входа макросом (main -> SDL_main), и
// заголовок обязан быть виден именно в файле с main(). Без него APK
// собирается, но не запускается: системе нечего вызывать.
#include <SDL2/SDL.h>

#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
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
