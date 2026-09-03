#include "draw.h"
#include "font.h"

#include <SDL2/SDL.h>

namespace gfx {

namespace {

// Глифы в атласе лежат в 16 столбцов: 179 глифов дают 12 строк.
const int ATLAS_COLS = 16;

int atlas_rows() { return (FONT_GLYPHS + ATLAS_COLS - 1) / ATLAS_COLS; }

// Масштаб подбирается по короткой стороне окна: на телефоне поперёк экрана
// должно помещаться около полусотни знаков, иначе текст нечитаем.
int pick_scale(int w, int h) {
    const int shortest = w < h ? w : h;
    int sc = shortest / (48 * FONT_W);
    if (sc < 1) sc = 1;
    if (sc > 6) sc = 6;
    return sc;
}

} // namespace

SDL_Surface* new_surface32(int w, int h) {
    Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xFF000000; gmask = 0x00FF0000; bmask = 0x0000FF00; amask = 0x000000FF;
#else
    rmask = 0x000000FF; gmask = 0x0000FF00; bmask = 0x00FF0000; amask = 0xFF000000;
#endif
    return SDL_CreateRGBSurface(0, w, h, 32, rmask, gmask, bmask, amask);
}

Canvas::Canvas() : win_(0), ren_(0), atlas_(0), w_(0), h_(0), scale_(1) {}

Canvas::~Canvas() { close(); }

int Canvas::cell_w() const { return FONT_W * scale_; }
int Canvas::cell_h() const { return FONT_H * scale_; }

int Canvas::touch_unit() const {
    // Примерно 7 мм на обычном телефоне: меньше палец не берёт.
    const int shortest = w_ < h_ ? w_ : h_;
    int u = shortest / 12;
    const int min_u = cell_h() * 2;
    return u < min_u ? min_u : u;
}

bool Canvas::open(const std::string& title, int want_w, int want_h) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        err_ = std::string("SDL_Init: ") + SDL_GetError();
        return false;
    }

    // На телефоне окно всегда во весь экран; на настольной машине — заданного
    // размера и с возможностью растянуть.
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
#if defined(__ANDROID__)
    flags |= SDL_WINDOW_FULLSCREEN;
#endif
    win_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, want_w, want_h, flags);
    if (!win_) {
        err_ = std::string("SDL_CreateWindow: ") + SDL_GetError();
        return false;
    }

    ren_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren_) ren_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_SOFTWARE);
    if (!ren_) {
        err_ = std::string("SDL_CreateRenderer: ") + SDL_GetError();
        return false;
    }
    SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_BLEND);

    SDL_GetRendererOutputSize(ren_, &w_, &h_);
    scale_ = pick_scale(w_, h_);

    if (!build_atlas()) return false;
    return true;
}

bool Canvas::build_atlas() {
    const int aw = ATLAS_COLS * FONT_W;
    const int ah = atlas_rows() * FONT_H;

    SDL_Surface* surf = new_surface32(aw, ah);
    if (!surf) {
        err_ = std::string("SDL_CreateRGBSurface: ") + SDL_GetError();
        return false;
    }
    SDL_LockSurface(surf);
    Uint32* px = static_cast<Uint32*>(surf->pixels);
    const int pitch = surf->pitch / 4;
    for (int i = 0; i < aw * ah; ++i) px[i] = 0;

    // Глифы белые и непрозрачные только там, где стоит бит: цвет потом
    // задаётся модуляцией, один атлас на все надписи.
    for (int g = 0; g < FONT_GLYPHS; ++g) {
        const int gx = (g % ATLAS_COLS) * FONT_W;
        const int gy = (g / ATLAS_COLS) * FONT_H;
        const unsigned char* bits = FONT_BITS + static_cast<std::size_t>(g) * static_cast<std::size_t>(FONT_H);
        for (int y = 0; y < FONT_H; ++y)
            for (int x = 0; x < FONT_W; ++x)
                if ((bits[y] >> (FONT_W - 1 - x)) & 1)
                    px[(gy + y) * pitch + gx + x] = 0xFFFFFFFFu;
    }
    SDL_UnlockSurface(surf);

    atlas_ = SDL_CreateTextureFromSurface(ren_, surf);
    SDL_FreeSurface(surf);
    if (!atlas_) {
        err_ = std::string("SDL_CreateTexture: ") + SDL_GetError();
        return false;
    }
    SDL_SetTextureBlendMode(atlas_, SDL_BLENDMODE_BLEND);
    return true;
}

void Canvas::close() {
    if (atlas_) { SDL_DestroyTexture(atlas_); atlas_ = 0; }
    if (ren_)   { SDL_DestroyRenderer(ren_);  ren_ = 0; }
    if (win_)   { SDL_DestroyWindow(win_);    win_ = 0; }
    if (SDL_WasInit(SDL_INIT_VIDEO)) SDL_Quit();
}

void Canvas::begin(Color bg) {
    // Размер окна может измениться поворотом экрана — пересчитываем каждый кадр.
    SDL_GetRendererOutputSize(ren_, &w_, &h_);
    scale_ = pick_scale(w_, h_);
    SDL_SetRenderDrawColor(ren_, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(ren_);
}

void Canvas::present() { SDL_RenderPresent(ren_); }

void Canvas::fill(const Rect& r, Color c) {
    if (r.w <= 0 || r.h <= 0) return;
    SDL_Rect s = {r.x, r.y, r.w, r.h};
    SDL_SetRenderDrawColor(ren_, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(ren_, &s);
}

void Canvas::frame(const Rect& r, Color c, int thick) {
    if (thick < 1) thick = 1;
    fill(Rect(r.x, r.y, r.w, thick), c);
    fill(Rect(r.x, r.y + r.h - thick, r.w, thick), c);
    fill(Rect(r.x, r.y, thick, r.h), c);
    fill(Rect(r.x + r.w - thick, r.y, thick, r.h), c);
}

void Canvas::glyph_at(int x, int y, unsigned cp, Color c, int sc) {
    if (cp == ' ' || cp == 0) return;
    int g = glyph_index(cp);
    if (g < 0) g = glyph_index('?');
    if (g < 0) return;

    SDL_Rect src = {(g % ATLAS_COLS) * FONT_W, (g / ATLAS_COLS) * FONT_H, FONT_W, FONT_H};
    SDL_Rect dst = {x, y, FONT_W * sc, FONT_H * sc};
    SDL_SetTextureColorMod(atlas_, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(atlas_, c.a);
    SDL_RenderCopy(ren_, atlas_, &src, &dst);
}

void Canvas::text(int x, int y, const std::string& s, Color c, int sc) {
    std::size_t i = 0;
    int cx = x;
    while (i < s.size()) {
        const unsigned cp = utf8_next(s, i);
        if (cp == '\n') break;             // строки рисуются по одной
        glyph_at(cx, y, cp, c, sc);
        cx += FONT_W * sc;
    }
}

int Canvas::text_width(const std::string& s, int sc) const {
    std::size_t i = 0;
    int n = 0;
    while (i < s.size()) {
        const unsigned cp = utf8_next(s, i);
        if (cp == '\n') break;
        ++n;
    }
    return n * FONT_W * sc;
}

int Canvas::text_height(int sc) const { return FONT_H * sc; }

void Canvas::text_centered(const Rect& r, const std::string& s, Color c, int sc) {
    const int tw = text_width(s, sc);
    const int th = text_height(sc);
    text(r.x + (r.w - tw) / 2, r.y + (r.h - th) / 2, s, c, sc);
}

void Canvas::clip(const Rect& r) {
    SDL_Rect s = {r.x, r.y, r.w, r.h};
    SDL_RenderSetClipRect(ren_, &s);
}

void Canvas::clip_off() { SDL_RenderSetClipRect(ren_, 0); }

} // namespace gfx
