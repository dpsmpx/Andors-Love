#include "font.h"

namespace gfx {

namespace {

int index_of(unsigned cp) {
    for (int i = 0; i < FONT_GLYPHS; ++i)
        if (FONT_CODEPOINTS[i] == cp) return i;
    return -1;
}

} // namespace

bool has_glyph(unsigned cp) { return index_of(cp) >= 0; }

const unsigned char* glyph(unsigned cp) {
    int i = index_of(cp);
    if (i < 0) i = index_of('?');
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
