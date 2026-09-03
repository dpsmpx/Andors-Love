#!/usr/bin/env python3
"""Генератор вшитого растрового шрифта.

Игра целиком на русском, а таскать с собой TTF и SDL_ttf ради этого не стоит:
шрифт растеризуется здесь один раз и ложится в исходник как массив битов.
Ширина фиксированная — весь текстовый слой игры (wrap, pad, trunc) считает
символы, а не пиксели, и на этом держится вся вёрстка.

Запуск: python3 tools/gen_font.py   (нужны python3-pil и DejaVu Sans Mono)
"""
import sys

from PIL import Image, ImageDraw, ImageFont

W, H = 8, 16                     # клетка глифа в пикселях
TTF = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
PT = 15                          # кегль, подобран под клетку 8x16

# Набор: печатная латиница, кириллица с Ё, и та пунктуация, что реально
# встречается в текстах игры и в картах.
CHARS = [chr(c) for c in range(0x20, 0x7F)]
CHARS += [chr(c) for c in range(0x410, 0x450)]          # А-я
CHARS += ["Ё", "ё"]
CHARS += ["«", "»", "—", "–", "…", "·", "№", "→", "↑", "↓", "←",
          "░", "▒", "█", "•", "×", "‹", "›"]

seen, chars = set(), []
for ch in CHARS:
    if ch not in seen:
        seen.add(ch)
        chars.append(ch)


def render(font, ch):
    """Один глиф как список строк битов, выровненный по базовой линии."""
    img = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(img)
    # Смещение подобрано так, чтобы буквы стояли на общей базовой линии
    # и не срезались сверху у прописных.
    d.text((0, -2), ch, font=font, fill=255)
    rows = []
    px = img.load()
    for y in range(H):
        bits = 0
        for x in range(W):
            if px[x, y] > 110:
                bits |= 1 << (W - 1 - x)
        rows.append(bits)
    return rows


def main():
    try:
        font = ImageFont.truetype(TTF, PT)
    except OSError:
        sys.exit("не найден шрифт " + TTF)

    glyphs = [(ch, render(font, ch)) for ch in chars]

    # Пробел обязан быть пустым, а не «почти пустым».
    glyphs[0] = (" ", [0] * H)

    empty = [ch for ch, rows in glyphs[1:] if not any(rows)]
    if empty:
        sys.exit("пустые глифы: " + " ".join(empty))

    out = ["// Сгенерировано tools/gen_font.py. Руками не править.",
           "// Растровый шрифт %dx%d, %d глифов: латиница, кириллица, пунктуация."
           % (W, H, len(glyphs)),
           '#include "font.h"',
           "",
           "namespace gfx {",
           "",
           "const int FONT_W = %d;" % W,
           "const int FONT_H = %d;" % H,
           "const int FONT_GLYPHS = %d;" % len(glyphs),
           "",
           "// Кодовые точки глифов, по возрастанию не обязано быть — поиск двоичный",
           "// по отсортированной копии не нужен, таблица мала.",
           "const unsigned short FONT_CODEPOINTS[%d] = {" % len(glyphs)]

    line = "   "
    for ch, _ in glyphs:
        piece = " %d," % ord(ch)
        if len(line) + len(piece) > 78:
            out.append(line)
            line = "   "
        line += piece
    out.append(line)
    out.append("};")
    out.append("")
    out.append("// По %d байт на глиф: строка сверху вниз, бит слева направо." % H)
    out.append("const unsigned char FONT_BITS[%d] = {" % (len(glyphs) * H))
    for ch, rows in glyphs:
        # Обратная косая в конце комментария склеила бы строки: подписываем кодом.
        label = ch if ch not in ("\\",) else "U+005C"
        out.append("    " + "".join("0x%02X," % r for r in rows) + "   // " + label)
    out.append("};")
    out.append("")
    out.append("} // namespace gfx")
    out.append("")

    with open("src/gfx/font_data.cpp", "w", encoding="utf-8") as fh:
        fh.write("\n".join(out))
    print("глифов: %d, байт растра: %d" % (len(glyphs), len(glyphs) * H))


main()
