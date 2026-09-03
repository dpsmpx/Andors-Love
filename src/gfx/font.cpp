#include "font.h"

namespace gfx {

namespace {

// Прямая таблица «кодовая точка -> номер глифа». Все символы шрифта лежат
// ниже U+0500, так что таблица маленькая и строится один раз.
const unsigned MAP_SIZE = 0x500;

const short* index_table() {
    static short table[MAP_SIZE];
    static bool built = false;
    if (!built) {
        built = true;
        for (unsigned i = 0; i < MAP_SIZE; ++i) table[i] = -1;
        for (int i = 0; i < FONT_GLYPHS; ++i) {
            const unsigned cp = FONT_CODEPOINTS[i];
            if (cp < MAP_SIZE) table[cp] = static_cast<short>(i);
        }
    }
    return table;
}

} // namespace

int glyph_index(unsigned cp) {
    if (cp < MAP_SIZE) return index_table()[cp];
    // Единичные знаки выше таблицы (тире, многоточие) ищутся перебором:
    // их считанные штуки, и в горячем пути они почти не встречаются.
    for (int i = 0; i < FONT_GLYPHS; ++i)
        if (FONT_CODEPOINTS[i] == cp) return i;
    return -1;
}

bool has_glyph(unsigned cp) { return glyph_index(cp) >= 0; }

const unsigned char* glyph(unsigned cp) {
    int i = glyph_index(cp);
    if (i < 0) i = glyph_index('?');
    if (i < 0) i = 0;
    return FONT_BITS + static_cast<std::size_t>(i) * static_cast<std::size_t>(FONT_H);
}

unsigned utf8_next(const std::string& s, std::size_t& i) {
    if (i >= s.size()) return 0;
    const unsigned char c = static_cast<unsigned char>(s[i]);

    int extra;
    unsigned cp;
    if (c < 0x80)        { ++i; return c; }
    else if ((c & 0xE0) == 0xC0) { extra = 1; cp = c & 0x1Fu; }
    else if ((c & 0xF0) == 0xE0) { extra = 2; cp = c & 0x0Fu; }
    else if ((c & 0xF8) == 0xF0) { extra = 3; cp = c & 0x07u; }
    else { ++i; return 0xFFFD; }          // одинокий байт продолжения

    if (i + static_cast<std::size_t>(extra) >= s.size()) { ++i; return 0xFFFD; }
    for (int k = 1; k <= extra; ++k) {
        const unsigned char n = static_cast<unsigned char>(s[i + static_cast<std::size_t>(k)]);
        if ((n & 0xC0) != 0x80) { ++i; return 0xFFFD; }
        cp = (cp << 6) | (n & 0x3Fu);
    }
    i += static_cast<std::size_t>(extra) + 1;
    return cp;
}

} // namespace gfx
