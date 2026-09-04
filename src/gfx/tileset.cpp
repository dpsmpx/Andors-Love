#include "tileset.h"

#include <SDL2/SDL.h>

namespace gfx {

Tileset::Tileset() : tex_(0), cw_(0), ch_(0), tw_(0), th_(0) {}

Tileset::~Tileset() { unload(); }

void Tileset::unload() {
    if (tex_) { SDL_DestroyTexture(tex_); tex_ = 0; }
    cw_ = ch_ = tw_ = th_ = 0;
    for (int i = 0; i < TJTM_SLOTS; ++i) src_[i] = Rect();
}

bool Tileset::has(int slot) const {
    if (!tex_ || slot < 0 || slot >= TJTM_SLOTS) return false;
    return src_[slot].w > 0 && src_[slot].h > 0;
}

bool Tileset::load(SDL_Renderer* ren, const std::string& path, std::string* err) {
    unload();
    if (!ren) { if (err) *err = "нет отрисовщика"; return false; }

    Image sheet;
    if (!png_read(path, &sheet, err)) return false;

    std::vector<Image> tiles;
    if (!tjtm_slice(sheet, &tiles, err)) return false;

    for (int i = 0; i < TJTM_SLOTS; ++i) {
        const Image& t = tiles[static_cast<std::size_t>(i)];
        if (t.w > tw_) tw_ = t.w;
        if (t.h > th_) th_ = t.h;
    }
    if (tw_ < 1 || th_ < 1) {
        if (err) *err = "в листе нет ни одного тайла";
        return false;
    }

    // Атлас складывается вплотную, без разделителей: они нужны только файлу,
    // чтобы редактор нашёл границы, а видеокарте достаточно координат.
    cw_ = tw_;
    ch_ = th_;

    // Слишком крупный лист не влезет в текстуру, и вместо картинки вышел бы
    // чёрный экран без единого слова. Предел у старых телефонов бывает 2048,
    // поэтому спрашиваем его у самого отрисовщика, а не выдумываем.
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(ren, &info) == 0 && info.max_texture_width > 0) {
        const int need_w = TJTM_COLS * cw_, need_h = TJTM_ROWS * ch_;
        if (need_w > info.max_texture_width || need_h > info.max_texture_height) {
            if (err) {
                char buf[192];
                SDL_snprintf(buf, sizeof buf,
                             "лист %dx%d не помещается в текстуру %dx%d: "
                             "уменьшите тайлы (в редакторе они и так не больше %d)",
                             need_w, need_h, info.max_texture_width,
                             info.max_texture_height, TJTM_MAX_TILE);
                *err = buf;
            }
            unload();
            return false;
        }
    }

    SDL_Surface* surf = new_surface32(TJTM_COLS * cw_, TJTM_ROWS * ch_);
    if (!surf) {
        if (err) *err = std::string("SDL_CreateRGBSurface: ") + SDL_GetError();
        return false;
    }

    SDL_LockSurface(surf);
    unsigned char* base = static_cast<unsigned char*>(surf->pixels);
    for (int y = 0; y < surf->h; ++y)
        SDL_memset(base + static_cast<std::size_t>(y) * static_cast<std::size_t>(surf->pitch),
                   0, static_cast<std::size_t>(surf->w) * 4);

    for (int i = 0; i < TJTM_SLOTS; ++i) {
        const Image& t = tiles[static_cast<std::size_t>(i)];
        if (t.empty()) continue;
        const int ox = (i % TJTM_COLS) * cw_;
        const int oy = (i / TJTM_COLS) * ch_;
        for (int y = 0; y < t.h; ++y) {
            unsigned char* dst = base +
                static_cast<std::size_t>(oy + y) * static_cast<std::size_t>(surf->pitch) +
                static_cast<std::size_t>(ox) * 4;
            SDL_memcpy(dst, t.at(0, y), static_cast<std::size_t>(t.w) * 4);
        }
        src_[i] = Rect(ox, oy, t.w, t.h);
    }
    SDL_UnlockSurface(surf);

    tex_ = SDL_CreateTextureFromSurface(ren, surf);
    SDL_FreeSurface(surf);
    if (!tex_) {
        if (err) *err = std::string("SDL_CreateTexture: ") + SDL_GetError();
        unload();
        return false;
    }
    SDL_SetTextureBlendMode(tex_, SDL_BLENDMODE_BLEND);
    return true;
}

bool Tileset::draw(SDL_Renderer* ren, int slot, const Rect& dst) const {
    if (!ren || !has(slot) || dst.w <= 0 || dst.h <= 0) return false;
    const Rect& s = src_[slot];
    SDL_Rect ss = { s.x, s.y, s.w, s.h };
    SDL_Rect dd = { dst.x, dst.y, dst.w, dst.h };
    return SDL_RenderCopy(ren, tex_, &ss, &dd) == 0;
}

} // namespace gfx
