#include "tiles.h"

#include "font.h"
#include "../paths.h"
#include "../platform.h"

namespace gfx {

namespace {

const unsigned char MARKER_R = 1, MARKER_G = 2, MARKER_B = 3, MARKER_A = 255;

void fail(std::string* err, const std::string& what) { if (err) *err = what; }

// Наложение цвета с альфой поверх готового пикселя — правило source-over.
// Нужно там, где вид по умолчанию кладёт полупрозрачный узор на заливку.
void blend_over(unsigned char* dst, Rgba src) {
    if (src.a == 0) return;
    if (src.a == 255) {
        dst[0] = src.r; dst[1] = src.g; dst[2] = src.b; dst[3] = 255;
        return;
    }
    const int a = src.a;
    const int inv = 255 - a;
    // Фон тайлов местности непрозрачен, и общая формула здесь свелась бы к
    // этой же: смешиваем цвета, прозрачность остаётся максимальной из двух.
    dst[0] = static_cast<unsigned char>((src.r * a + dst[0] * inv) / 255);
    dst[1] = static_cast<unsigned char>((src.g * a + dst[1] * inv) / 255);
    dst[2] = static_cast<unsigned char>((src.b * a + dst[2] * inv) / 255);
    dst[3] = static_cast<unsigned char>(dst[3] > src.a ? dst[3] : src.a);
}

// Знак шрифта в картинку: тот же растр, что рисует Canvas::glyph_at, только
// пиксели ложатся в память, а не в видеокарту.
void draw_glyph(Image* img, int x0, int y0, unsigned cp, Rgba color, int sc) {
    if (!img || sc < 1) return;
    const unsigned char* bits = glyph(cp);
    for (int row = 0; row < FONT_H; ++row) {
        const unsigned char line = bits[row];
        for (int col = 0; col < FONT_W; ++col) {
            if (!((line >> (FONT_W - 1 - col)) & 1)) continue;
            for (int dy = 0; dy < sc; ++dy)
                for (int dx = 0; dx < sc; ++dx) {
                    const int x = x0 + col * sc + dx;
                    const int y = y0 + row * sc + dy;
                    if (x < 0 || y < 0 || x >= img->w || y >= img->h) continue;
                    blend_over(img->at(x, y), color);
                }
        }
    }
}

} // namespace

// ------------------------------------------------------------------ формат

bool tjtm_is_marker(const unsigned char* rgba) {
    return rgba[0] == MARKER_R && rgba[1] == MARKER_G &&
           rgba[2] == MARKER_B && rgba[3] == MARKER_A;
}

bool tjtm_slice(const Image& sheet, std::vector<Image>* out, std::string* err) {
    if (!out) return false;
    const int cw = sheet.w / TJTM_COLS;
    const int ch = sheet.h / TJTM_ROWS;
    if (sheet.empty() || cw < 1 || ch < 1) {
        fail(err, "не похоже на лист тайлов: картинка должна делиться на 8 ячеек по каждой стороне");
        return false;
    }

    out->assign(static_cast<std::size_t>(TJTM_SLOTS), Image());
    for (int i = 0; i < TJTM_ROWS; ++i) {
        for (int j = 0; j < TJTM_COLS; ++j) {
            const int ox = j * cw, oy = i * ch;

            // Край тайла ищется по маркеру: вправо по верхней строке ячейки
            // и вниз по левому столбцу — ровно как это делает редактор.
            int tw = 0;
            while (tw < cw && !tjtm_is_marker(sheet.at(ox + tw, oy))) ++tw;
            int th = 0;
            while (th < ch && !tjtm_is_marker(sheet.at(ox, oy + th))) ++th;

            Image tile;
            if (tw >= 1 && th >= 1) {
                tile = Image(tw, th);
                for (int y = 0; y < th; ++y)
                    for (int x = 0; x < tw; ++x) {
                        const unsigned char* s = sheet.at(ox + x, oy + y);
                        unsigned char* d = tile.at(x, y);
                        d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
                    }
            }
            (*out)[static_cast<std::size_t>(i * TJTM_COLS + j)] = tile;
        }
    }
    return true;
}

bool tjtm_compose(const std::vector<Image>& tiles, Image* out, std::string* err) {
    if (!out) return false;
    if (tiles.size() > static_cast<std::size_t>(TJTM_SLOTS)) {
        fail(err, "в лист не влезает больше 64 тайлов");
        return false;
    }

    int cw = 1, ch = 1;
    for (std::size_t k = 0; k < tiles.size(); ++k) {
        if (tiles[k].w > cw) cw = tiles[k].w;
        if (tiles[k].h > ch) ch = tiles[k].h;
    }
    if (cw > TJTM_MAX_TILE || ch > TJTM_MAX_TILE) {
        fail(err, "тайл больше, чем открывает редактор: не больше 63 пикселей по стороне");
        return false;
    }
    // Запас в пиксель гарантирует разделитель даже у тайла во всю ячейку.
    ++cw; ++ch;

    Image sheet(TJTM_COLS * cw, TJTM_ROWS * ch);
    sheet.clear(MARKER_R, MARKER_G, MARKER_B, MARKER_A);
    for (std::size_t k = 0; k < tiles.size(); ++k) {
        const Image& t = tiles[k];
        const int ox = static_cast<int>(k % TJTM_COLS) * cw;
        const int oy = static_cast<int>(k / TJTM_COLS) * ch;
        for (int y = 0; y < t.h; ++y)
            for (int x = 0; x < t.w; ++x) {
                const unsigned char* s = t.at(x, y);
                unsigned char* d = sheet.at(ox + x, oy + y);
                d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
            }
    }
    *out = sheet;
    return true;
}

// ---------------------------------------------------------------- раскладка

int slot_of_tile(Tile t) {
    switch (t) {
        case Tile::Floor:     return SLOT_FLOOR;
        case Tile::Wall:      return SLOT_WALL;
        case Tile::Water:     return SLOT_WATER;
        case Tile::Tree:      return SLOT_TREE;
        case Tile::Grass:     return SLOT_GRASS;
        case Tile::Road:      return SLOT_ROAD;
        case Tile::DeadWater: return SLOT_DEADWATER;
        default:              return -1;
    }
}

int slot_of_glyph(unsigned cp) {
    switch (cp) {
        case glyph::PLAYER: return SLOT_PLAYER;
        case glyph::NPC:    return SLOT_NPC;
        case glyph::MOB:    return SLOT_MOB;
        case glyph::EXIT:   return SLOT_EXIT;
        case glyph::SIGN:   return SLOT_SIGN;
        case glyph::ITEM:   return SLOT_ITEM;
        case glyph::BED:    return SLOT_BED;
        case glyph::CHEST:  return SLOT_CHEST;
        case glyph::PORTAL: return SLOT_PORTAL;
        case glyph::NOTE:   return SLOT_NOTE;
        default:            return -1;
    }
}

const char* slot_name(int slot) {
    switch (slot) {
        case SLOT_FLOOR:     return "пол";
        case SLOT_WALL:      return "стена";
        case SLOT_WATER:     return "вода";
        case SLOT_TREE:      return "дерево";
        case SLOT_GRASS:     return "трава";
        case SLOT_ROAD:      return "дорога";
        case SLOT_DEADWATER: return "стоячая вода";
        case SLOT_PLAYER:    return "герой";
        case SLOT_NPC:       return "житель";
        case SLOT_MOB:       return "враг";
        case SLOT_EXIT:      return "переход";
        case SLOT_SIGN:      return "табличка";
        case SLOT_ITEM:      return "предмет";
        case SLOT_BED:       return "лежанка";
        case SLOT_CHEST:     return "сундук";
        case SLOT_PORTAL:    return "портал";
        case SLOT_NOTE:      return "записка";
        default:             return "свободный";
    }
}

const char* slot_file(int slot) {
    switch (slot) {
        case SLOT_FLOOR:     return "floor";
        case SLOT_WALL:      return "wall";
        case SLOT_WATER:     return "water";
        case SLOT_TREE:      return "tree";
        case SLOT_GRASS:     return "grass";
        case SLOT_ROAD:      return "road";
        case SLOT_DEADWATER: return "deadwater";
        case SLOT_PLAYER:    return "player";
        case SLOT_NPC:       return "npc";
        case SLOT_MOB:       return "mob";
        case SLOT_EXIT:      return "exit";
        case SLOT_SIGN:      return "sign";
        case SLOT_ITEM:      return "item";
        case SLOT_BED:       return "bed";
        case SLOT_CHEST:     return "chest";
        case SLOT_PORTAL:    return "portal";
        case SLOT_NOTE:      return "note";
        default:             return 0;
    }
}

// ------------------------------------------------------------ графика по файлам

int load_slot_files(const std::string& dir, std::vector<Image>* tiles, std::string* err) {
    if (!tiles || dir.empty()) return 0;
    if (tiles->size() != static_cast<std::size_t>(TJTM_SLOTS))
        tiles->assign(static_cast<std::size_t>(TJTM_SLOTS), Image());

    int loaded = 0;
    for (int i = 0; i < TJTM_SLOTS; ++i) {
        const char* base = slot_file(i);
        if (!base) continue;
        const std::string path = dir + "/" + base + ".png";
        if (!paths::file_exists(path)) continue;

        Image img;
        std::string one;
        if (!png_read(path, &img, &one)) {
            // Про нечитаемый файл надо сказать: художник правит картинку и
            // ждёт, что в игре что-то изменится. Но остальные тайлы это не
            // отменяет — читаем дальше.
            if (err && err->empty()) *err = path + ": " + one;
            continue;
        }
        (*tiles)[static_cast<std::size_t>(i)] = img;
        ++loaded;
    }
    return loaded;
}

bool save_slot_files(const std::string& dir, int size, std::string* err) {
    if (dir.empty()) { fail(err, "не задан каталог"); return false; }
    if (!platform::make_dir(dir)) { fail(err, "не создать каталог " + dir); return false; }

    for (int i = 0; i < TJTM_SLOTS; ++i) {
        const char* base = slot_file(i);
        if (!base) continue;
        const Image art = default_slot_art(i, size);
        if (art.empty()) continue;
        const std::string path = dir + "/" + base + ".png";
        std::string one;
        if (!png_write(path, art, &one)) { fail(err, path + ": " + one); return false; }
    }
    return true;
}

// ------------------------------------------------------------ вид по умолчанию

Rgba default_player_backing() {
    const Rgba c = { 120, 110, 60, 140 };
    return c;
}

Rgba default_tile_color(Tile t) {
    Rgba c = { 20, 20, 20, 255 };
    switch (t) {
        case Tile::Floor:     c.r =  52; c.g =  50; c.b =  46; break;
        case Tile::Wall:      c.r =  78; c.g =  74; c.b =  68; break;
        case Tile::Water:     c.r =  34; c.g =  62; c.b = 104; break;
        case Tile::Tree:      c.r =  30; c.g =  62; c.b =  38; break;
        case Tile::Grass:     c.r =  38; c.g =  58; c.b =  34; break;
        case Tile::Road:      c.r =  72; c.g =  62; c.b =  44; break;
        case Tile::DeadWater: c.r =  44; c.g =  58; c.b =  58; break;
        default: break;
    }
    return c;
}

// Мелкая рябь поверх заливки: без неё поле выглядит плоской клеёнкой.
unsigned default_tile_texture(Tile t) {
    switch (t) {
        case Tile::Wall:      return 0x2592;   // ▒
        case Tile::Tree:      return 'T';
        case Tile::Grass:     return ',';
        case Tile::Water:     return '~';
        case Tile::DeadWater: return ':';
        default:              return 0;
    }
}

Rgba default_object_color(unsigned cp) {
    Rgba c = { 200, 200, 200, 255 };
    switch (cp) {
        case glyph::PLAYER: c.r = 255; c.g = 248; c.b = 220; break;
        case glyph::NPC:    c.r = 120; c.g = 200; c.b = 255; break;
        case glyph::MOB:    c.r = 232; c.g =  96; c.b =  88; break;
        case glyph::EXIT:   c.r = 240; c.g = 216; c.b = 120; break;
        case glyph::SIGN:   c.r = 200; c.g = 190; c.b = 150; break;
        case glyph::ITEM:   c.r = 140; c.g = 230; c.b = 150; break;
        case glyph::BED:    c.r = 210; c.g = 160; c.b = 220; break;
        case glyph::CHEST:  c.r = 230; c.g = 180; c.b = 100; break;
        case glyph::PORTAL: c.r = 180; c.g = 140; c.b = 255; break;
        case glyph::NOTE:   c.r = 230; c.g = 230; c.b = 160; break;
        default: break;
    }
    return c;
}

namespace {

// Слот местности -> тайл. Обратная сторона slot_of_tile: она нужна только
// экспорту, поэтому наружу не торчит.
bool tile_of_slot(int slot, Tile* out) {
    for (int i = 0; i < static_cast<int>(Tile::Count); ++i) {
        const Tile t = static_cast<Tile>(i);
        if (slot_of_tile(t) == slot) { *out = t; return true; }
    }
    return false;
}

// Слот объекта -> знак. Так же только для экспорта.
bool glyph_of_slot(int slot, unsigned* out) {
    static const char SIGNS[] = {
        glyph::PLAYER, glyph::NPC, glyph::MOB, glyph::EXIT, glyph::SIGN,
        glyph::ITEM, glyph::BED, glyph::CHEST, glyph::PORTAL, glyph::NOTE
    };
    for (std::size_t i = 0; i < sizeof SIGNS; ++i) {
        const unsigned cp = static_cast<unsigned char>(SIGNS[i]);
        if (slot_of_glyph(cp) == slot) { *out = cp; return true; }
    }
    return false;
}

} // namespace

Image default_slot_art(int slot, int size) {
    if (size < FONT_H) return Image();

    // Масштаб знака берётся по высоте клетки: на экране он тоже занимает
    // её целиком, и экспорт должен показывать ровно то, что видно в игре.
    const int sc = size / FONT_H;
    const int gx = (size - FONT_W * sc) / 2;
    const int gy = (size - FONT_H * sc) / 2;

    Tile t;
    if (tile_of_slot(slot, &t)) {
        Image img(size, size);
        const Rgba fill = default_tile_color(t);
        img.clear(fill.r, fill.g, fill.b, 255);
        const unsigned tex = default_tile_texture(t);
        if (tex) {
            const Rgba shade = { 0, 0, 0, 70 };
            draw_glyph(&img, gx, gy, tex, shade, sc);
        }
        return img;
    }

    unsigned cp;
    if (glyph_of_slot(slot, &cp)) {
        // Объект рисуется по прозрачному фону: под ним должна быть видна
        // местность, на которой он стоит. Исключение — герой: под ним лежит
        // полупрозрачная подложка, чтобы его было видно везде.
        Image img(size, size);
        if (slot == SLOT_PLAYER) {
            const Rgba b = default_player_backing();
            img.clear(b.r, b.g, b.b, b.a);
        }
        draw_glyph(&img, gx, gy, cp, default_object_color(cp), sc);
        return img;
    }

    return Image();
}

Image default_sheet(int size) {
    std::vector<Image> tiles(static_cast<std::size_t>(TJTM_SLOTS));
    for (int i = 0; i < TJTM_SLOTS; ++i)
        tiles[static_cast<std::size_t>(i)] = default_slot_art(i, size);

    Image sheet;
    if (!tjtm_compose(tiles, &sheet, 0)) return Image();
    return sheet;
}

} // namespace gfx
