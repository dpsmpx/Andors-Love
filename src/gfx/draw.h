#pragma once
#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Surface;

// Тонкий слой поверх SDL2: прямоугольники, рамки и текст вшитым шрифтом.
// Ничего не знает про игру — только про то, как положить пиксели на экран.

namespace gfx {

struct Color {
    unsigned char r, g, b, a;
    Color() : r(0), g(0), b(0), a(255) {}
    Color(int rr, int gg, int bb, int aa = 255)
        : r(static_cast<unsigned char>(rr)), g(static_cast<unsigned char>(gg)),
          b(static_cast<unsigned char>(bb)), a(static_cast<unsigned char>(aa)) {}
};

struct Rect {
    int x, y, w, h;
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int xx, int yy, int ww, int hh) : x(xx), y(yy), w(ww), h(hh) {}
    bool contains(int px, int py) const {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};

// Поверхность 32 бита на пиксель, RGBA в порядке байтов памяти.
// Намеренно через SDL_CreateRGBSurface с явными масками: обёртка
// SDL_CreateRGBSurfaceWithFormat появилась только в SDL 2.0.5, а в C4Droid
// лежит SDL2 постарше, и на ней сборка просто не компилируется.
SDL_Surface* new_surface32(int w, int h);

class Canvas {
public:
    Canvas();
    ~Canvas();

    // Окно во весь экран на Android и в окне на настольной машине.
    bool open(const std::string& title, int want_w, int want_h);
    void close();
    const std::string& error() const { return err_; }

    int  width()  const { return w_; }
    int  height() const { return h_; }
    // Целый масштаб шрифта: подбирается по размеру окна так, чтобы текст
    // читался и на телефоне, и на настольном экране.
    int  scale()  const { return scale_; }
    int  cell_w() const;
    int  cell_h() const;
    // Палец — не мышь: минимальная сторона нажимаемого элемента.
    int  touch_unit() const;

    void begin(Color bg);
    void present();

    void fill(const Rect& r, Color c);
    void frame(const Rect& r, Color c, int thick);
    // Текст рисуется от левого верхнего угла; scale — множитель к базовым 8x16.
    void text(int x, int y, const std::string& s, Color c, int sc);
    void text_centered(const Rect& r, const std::string& s, Color c, int sc);
    // Ширина строки в пикселях при данном масштабе.
    int  text_width(const std::string& s, int sc) const;
    int  text_height(int sc) const;

    // Один символ по кодовой точке: карта рисуется именно так.
    void glyph_at(int x, int y, unsigned cp, Color c, int sc);

    // Ограничение вывода прямоугольником (списки со скроллом).
    void clip(const Rect& r);
    void clip_off();

    SDL_Renderer* renderer() const { return ren_; }

private:
    bool build_atlas();

    SDL_Window*   win_;
    SDL_Renderer* ren_;
    SDL_Texture*  atlas_;
    int           w_, h_, scale_;
    std::string   err_;

    Canvas(const Canvas&);
    Canvas& operator=(const Canvas&);
};

} // namespace gfx
