#!/usr/bin/env python3
"""Вшивает файлы карт в исходник, чтобы игра работала без внешних данных.

Нужно для сборки APK: внутри пакета каталога data/maps нет. Сгенерированный
файл коммитится, поэтому на устройстве Python не требуется — только при
изменении карт: make embed
"""
import pathlib
import sys

SRC = pathlib.Path("data/maps")
OUT = pathlib.Path("src/embedded_maps.cpp")


def delimiter(text: str) -> str:
    """Подбирает разделитель, которого нет внутри текста карты."""
    for d in ("MAP", "MAPX", "MAPXX", "MAPXXX"):
        if f'){d}"' not in text:
            return d
    raise SystemExit("не удалось подобрать разделитель для сырого литерала")


def main() -> int:
    maps = sorted(SRC.glob("*.map"))
    if not maps:
        print(f"нет карт в {SRC}", file=sys.stderr)
        return 1

    parts = [
        "// СГЕНЕРИРОВАНО tools/embed_maps.py — не править вручную.",
        "// Пересобрать после изменения карт:  make embed",
        '#include "embedded_maps.h"',
        "",
        "#include <cstring>",
        "",
        "namespace {",
        "",
    ]
    names = []
    for path in maps:
        text = path.read_text(encoding="utf-8")
        ident = "k_" + path.stem.replace("-", "_")
        d = delimiter(text)
        names.append((path.stem, ident))
        # Открывающий разделитель и содержимое — одним куском: перевод строки
        # сразу после R"…( попал бы внутрь литерала и копия перестала бы
        # совпадать с файлом.
        parts.append(f'const char {ident}[] = R"{d}(' + text.rstrip("\n") + f'){d}";')
        parts.append("")
    parts.append("} // namespace")
    parts.append("")
    parts.append("const char* embedded_map(const char* id) {")
    parts.append("    if (!id) return nullptr;")
    for stem, ident in names:
        parts.append(f'    if (std::strcmp(id, "{stem}") == 0) return {ident};')
    parts.append("    return nullptr;")
    parts.append("}")
    parts.append("")

    OUT.write_text("\n".join(parts), encoding="utf-8")
    total = sum(p.stat().st_size for p in maps)
    print(f"вшито карт: {len(maps)} ({total} байт) -> {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
