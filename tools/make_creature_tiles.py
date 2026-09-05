#!/usr/bin/env python3
"""Заводит по картинке на жителя и на врага.

Игра ищет тайл существа по имени файла: data/tiles/npc_elder.png,
mob_rat.png. Здесь такие файлы создаются заготовками — копией общего тайла
жителя или врага, — чтобы дальше их можно было рисовать по одному в
редакторе, а не начинать с пустого места.

Уже существующий файл не трогается никогда. Это главное свойство: рисунки
делаются руками и долго, а список существ растёт, поэтому прогон должен
добавлять недостающее и не иметь ни одного способа стереть сделанное.

Список берётся из src/content.cpp — того же места, откуда его берёт игра.
"""
import os
import re
import shutil
import sys

CONTENT = "src/content.cpp"
TILES = "data/tiles"


def ids_between(src, start, end, pattern):
    """Ищет id в куске файла между двумя определениями функций."""
    try:
        body = src[src.index(start):src.index(end)]
    except ValueError:
        raise SystemExit("не найден раздел %s в %s" % (start, CONTENT))
    return re.findall(pattern, body)


def main():
    if not os.path.isfile(CONTENT):
        raise SystemExit("запускать из корня репозитория: нет %s" % CONTENT)

    src = open(CONTENT, encoding="utf-8").read()
    npcs = ids_between(src, "void Content::build_npcs()",
                       "void Content::build_endings()",
                       r'add\(\s*"([a-z0-9_]+)"\s*,')
    mobs = ids_between(src, "void Content::build_enemies()",
                       "void Content::build_skills()",
                       r'\.id\s*=\s*"([a-z0-9_]+)"\s*;')

    made = kept = 0
    missing_base = []
    for kind, ids in (("npc", npcs), ("mob", mobs)):
        base = os.path.join(TILES, kind + ".png")
        if not os.path.isfile(base):
            missing_base.append(base)
            continue
        for cid in ids:
            path = os.path.join(TILES, "%s_%s.png" % (kind, cid))
            if os.path.exists(path):
                kept += 1
                continue
            shutil.copyfile(base, path)
            made += 1

    if missing_base:
        raise SystemExit("нет общего тайла: " + ", ".join(missing_base))

    print("жителей %d, врагов %d" % (len(npcs), len(mobs)))
    print("создано заготовок: %d, уже было: %d" % (made, kept))
    return 0


if __name__ == "__main__":
    sys.exit(main())
