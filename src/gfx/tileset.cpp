#include "tileset.h"

#include "../paths.h"

#include <SDL2/SDL.h>

namespace gfx {

Tileset::Tileset() : tex_(0), cw_(0), ch_(0), tw_(0), th_(0), cols_(TJTM_COLS) {}

Tileset::~Tileset() { unload(); }

void Tileset::unload() {
    if (tex_) { SDL_DestroyTexture(tex_); tex_ = 0; }
    cw_ = ch_ = tw_ = th_ = 0;
    cols_ = TJTM_COLS;
    src_.clear();
    named_.clear();
}

bool Tileset::has(int slot) const {
    if (!tex_ || slot < 0 || slot >= static_cast<int>(src_.size())) return false;
    return src_[static_cast<std::size_t>(slot)].w > 0 &&
           src_[static_cast<std::size_t>(slot)].h > 0;
}

bool Tileset::has_named(const std::string& name) const {
    std::map<std::string, int>::const_iterator it = named_.find(name);
    if (it == named_.end()) return false;
    return has(it->second);
}

bool Tileset::load(SDL_Renderer* ren, const std::string& dir, const std::string& sheet,
                   std::string* err) {
    return load(ren, dir, sheet, std::vector<std::string>(), err);
}

bool Tileset::load(SDL_Renderer* ren, const std::string& dir, const std::string& sheet,
                   const std::vector<std::string>& named, std::string* err) {
    unload();

    std::vector<Image> tiles(static_cast<std::size_t>(TJTM_SLOTS));

    // Лист кладётся первым слоем: он мог остаться от прежних версий редактора,
    // где набор хранился одной картинкой.
    if (!sheet.empty() && paths::file_exists(sheet)) {
        Image img;
        std::string one;
        if (png_read(sheet, &img, &one) && tjtm_slice(img, &tiles, &one)) {
            // порядок важен: отдельные файлы лягут поверх
        } else if (err && err->empty()) {
            *err = sheet + ": " + one;
        }
    }

    // Отдельные файлы — то, что сохраняет редактор, и они главнее.
    load_slot_files(dir, &tiles, err);

    std::vector<Image> extra;
    load_named_files(dir, named, &extra, err);

    int found = 0;
    for (int i = 0; i < TJTM_SLOTS; ++i)
        if (!tiles[static_cast<std::size_t>(i)].empty()) ++found;
    for (std::size_t i = 0; i < extra.size(); ++i)
        if (!extra[i].empty()) ++found;
    if (!found) {
        if (err && err->empty()) *err = "не найдено ни одного тайла";
        return false;
    }
    return build(ren, tiles, named, extra, err);
}

bool Tileset::build(SDL_Renderer* ren, const std::vector<Image>& tiles, std::string* err) {
    return build(ren, tiles, std::vector<std::string>(), std::vector<Image>(), err);
}

bool Tileset::build(SDL_Renderer* ren, const std::vector<Image>& tiles,
                    const std::vector<std::string>& names,
                    const std::vector<Image>& named, std::string* err) {
    unload();
    if (!ren) { if (err) *err = "нет отрисовщика"; return false; }
    if (tiles.size() != static_cast<std::size_t>(TJTM_SLOTS)) {
        if (err) *err = "нужно ровно 64 слота";
        return false;
    }
    if (names.size() != named.size()) {
        if (err) *err = "имён и картинок именованных тайлов разное число";
        return false;
    }

    // Ячейки: сперва слоты, потом именованные тайлы в порядке names.
    std::vector<Image> all(tiles);
    all.insert(all.end(), named.begin(), named.end());
    for (std::size_t i = 0; i < names.size(); ++i)
        if (!names[i].empty())
            named_[names[i]] = TJTM_SLOTS + static_cast<int>(i);

    for (std::size_t i = 0; i < all.size(); ++i) {
        if (all[i].w > tw_) tw_ = all[i].w;
        if (all[i].h > th_) th_ = all[i].h;
    }
    if (tw_ < 1 || th_ < 1) {
        if (err) *err = "в листе нет ни одного тайла";
        return false;
    }

    // Атлас складывается вплотную, без разделителей: они нужны только файлу,
    // чтобы редактор нашёл границы, а видеокарте достаточно координат.
    cw_ = tw_;
    ch_ = th_;

    // Сетка атласа. Пока ячеек не больше шестидесяти четырёх, она остаётся
    // прежней 8x8 — так раскладка совпадает с листом и с редактором. Когда
    // добавляются именованные тайлы, берётся квадратная: у неё наименьшая
    // большая сторона, а в предел текстуры упирается именно сторона.
    const int total = static_cast<int>(all.size());
    cols_ = TJTM_COLS;
    if (total > TJTM_SLOTS) {
        cols_ = 1;
        while (cols_ * cols_ < total) ++cols_;
    }
    const int rows = (total + cols_ - 1) / cols_;

    // Слишком крупный атлас не влезет в текстуру, и вместо картинки вышел бы
    // чёрный экран без единого слова. Предел у старых телефонов бывает 2048,
    // поэтому спрашиваем его у самого отрисовщика, а не выдумываем.
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(ren, &info) == 0 && info.max_texture_width > 0) {
        const int need_w = cols_ * cw_, need_h = rows * ch_;
        if (need_w > info.max_texture_width || need_h > info.max_texture_height) {
            if (err) {
                char buf[224];
                SDL_snprintf(buf, sizeof buf,
                             "атлас %dx%d из %d тайлов не помещается в текстуру "
                             "%dx%d: уменьшите тайлы (ячейка берётся по самому "
                             "крупному, сейчас %dx%d)",
                             need_w, need_h, total, info.max_texture_width,
                             info.max_texture_height, cw_, ch_);
                *err = buf;
            }
            unload();
            return false;
        }
    }

    SDL_Surface* surf = new_surface32(cols_ * cw_, rows * ch_);
    if (!surf) {
        if (err) *err = std::string("SDL_CreateRGBSurface: ") + SDL_GetError();
        return false;
    }

    SDL_LockSurface(surf);
    unsigned char* base = static_cast<unsigned char*>(surf->pixels);
    for (int y = 0; y < surf->h; ++y)
        SDL_memset(base + static_cast<std::size_t>(y) * static_cast<std::size_t>(surf->pitch),
                   0, static_cast<std::size_t>(surf->w) * 4);

    src_.assign(all.size(), Rect());
    for (std::size_t i = 0; i < all.size(); ++i) {
        const Image& t = all[i];
        if (t.empty()) continue;
        const int ox = (static_cast<int>(i) % cols_) * cw_;
        const int oy = (static_cast<int>(i) / cols_) * ch_;
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
    return draw_index(ren, slot, dst);
}

bool Tileset::draw_named(SDL_Renderer* ren, const std::string& name, const Rect& dst) const {
    std::map<std::string, int>::const_iterator it = named_.find(name);
    if (it == named_.end()) return false;
    return draw_index(ren, it->second, dst);
}

bool Tileset::draw_index(SDL_Renderer* ren, int idx, const Rect& dst) const {
    if (!ren || !has(idx) || dst.w <= 0 || dst.h <= 0) return false;
    const Rect& s = src_[static_cast<std::size_t>(idx)];
    SDL_Rect ss = { s.x, s.y, s.w, s.h };
    SDL_Rect dd = { dst.x, dst.y, dst.w, dst.h };
    return SDL_RenderCopy(ren, tex_, &ss, &dd) == 0;
}

} // namespace gfx
