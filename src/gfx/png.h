#pragma once
#include <cstddef>
#include <string>
#include <vector>

// Чтение и запись PNG без внешних библиотек.
//
// Своё, а не SDL_image, по той же причине, по которой шрифт вшит в исходник:
// игра должна собираться там, где есть только сам SDL2 — в C4Droid, в Termux,
// в MSYS2 — и не тянуть за собой ни одной дополнительной зависимости. Здесь
// нужен ровно один формат и ровно один его подвид, а не универсальный
// загрузчик картинок.
//
// Читается: глубина 1, 2, 4, 8 и 16 бит, все пять типов цвета (серый,
// RGB, палитра, серый с альфой, RGBA), прозрачность через tRNS. Не читается
// чересстрочный PNG (Adam7) — он сообщает об этом внятной ошибкой, а не
// молчит и не рисует мусор.
//
// Пишется: 8 бит, RGBA, без чересстрочности. Сжатие — deflate с фиксированными
// кодами Хаффмана и жадным поиском совпадений: для пиксельной графики с
// большими одноцветными пятнами этого более чем достаточно.

namespace gfx {

// Растр в памяти: RGBA по байту на канал, строки сверху вниз, без выравнивания.
struct Image {
    int w, h;
    std::vector<unsigned char> px;

    Image() : w(0), h(0) {}
    Image(int ww, int hh)
        : w(ww < 0 ? 0 : ww), h(hh < 0 ? 0 : hh),
          px(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4, 0) {}

    bool empty() const { return w <= 0 || h <= 0; }

    unsigned char* at(int x, int y) {
        return &px[(static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                    static_cast<std::size_t>(x)) * 4];
    }
    const unsigned char* at(int x, int y) const {
        return &px[(static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
                    static_cast<std::size_t>(x)) * 4];
    }

    void set(int x, int y, unsigned char r, unsigned char g, unsigned char b,
             unsigned char a = 255) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        unsigned char* p = at(x, y);
        p[0] = r; p[1] = g; p[2] = b; p[3] = a;
    }

    // Заливка целиком: пустая картинка — не ошибка, просто нечего заливать.
    void clear(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);
};

// Всё возвращает true при успехе; при неудаче пишет причину в err, если он дан.
bool png_read(const std::string& path, Image* out, std::string* err);
bool png_read_mem(const unsigned char* data, std::size_t n, Image* out, std::string* err);
bool png_write(const std::string& path, const Image& img, std::string* err);
// В память — тем же кодом, что и в файл: так проверяется запись в тестах.
bool png_write_mem(const Image& img, std::vector<unsigned char>* out, std::string* err);

} // namespace gfx
