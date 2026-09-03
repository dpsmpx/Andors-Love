#pragma once
#include <string>

// Вшитый растровый шрифт фиксированной ширины. Игра целиком на русском,
// и таскать TTF со шрифтовой библиотекой ради этого не нужно: растр
// генерируется один раз (tools/gen_font.py) и лежит в исходнике.
//
// Ширина фиксированная не из любви к терминалам: весь текстовый слой игры —
// wrap, pad, trunc — считает символы, а не пиксели. Моноширинный шрифт
// оставляет всю эту вёрстку верной без единой правки.

namespace gfx {

extern const int FONT_W;
extern const int FONT_H;
extern const int FONT_GLYPHS;
extern const unsigned short FONT_CODEPOINTS[];
extern const unsigned char  FONT_BITS[];

// Растр глифа: FONT_H байт, бит слева направо. Для неизвестного символа
// возвращается растр вопросительного знака — рисовать нечего лучше, чем
// молча оставлять дыру.
const unsigned char* glyph(unsigned cp);

// Есть ли такой символ в шрифте на самом деле.
bool has_glyph(unsigned cp);

// Разбор UTF-8: возвращает кодовую точку и двигает i за конец символа.
// Битая последовательность даёт U+FFFD и сдвиг на один байт, чтобы разбор
// не зациклился на мусоре.
unsigned utf8_next(const std::string& s, std::size_t& i);

} // namespace gfx
