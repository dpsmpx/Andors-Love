#!/usr/bin/env python3
"""Проверка карт игры.

Карты — это файлы `data/maps/*.map`, и они первичны: их правят руками или
редактором True Joint Tile Maker, а скрипт их читает и проверяет. Раньше он
их генерировал, и тогда всякая ручная правка терялась при следующем запуске;
теперь ни одного файла он не пишет.

Проверяется то, что нельзя увидеть глазом на сетке 48x18:

  * сетка прямоугольна и совпадает с заявленным `size`;
  * точка входа в локацию проходима;
  * из точки входа достижима каждая проходимая клетка — иначе получается
    угол карты, куда игрок не попадёт никогда;
  * объекты стоят в границах, на проходимых и достижимых клетках;
  * переходы ведут в существующую локацию, на проходимую клетку, и эта
    клетка достижима из точки входа той локации.

Разбор нарочно строг: файл, который игра прочитает иначе, чем задумано, —
это и есть ошибка, которую скрипт должен поймать до запуска.

    python3 tools/check_maps.py [каталог]

Код возврата: 0 — всё сошлось, 1 — есть ошибки.
"""
import os
import sys
from collections import deque

# Проходимые символы. Тот же набор, что у tile_walkable в src/types.cpp:
# пол, трава, дорога и мёртвая вода, которая стоит и держит вес.
WALKABLE = set(".,=:")
# Известные символы сетки. Игра неизвестный символ считает стеной молча,
# но в файле он почти наверняка опечатка, и сказать о нём стоит.
KNOWN_TILES = set(".#~T,=:")

# Виды объектов и сколько полей у каждого не считая x, y. None — хвост
# произвольной длины (текст таблички, содержимое сундука, условие перехода).
OBJECTS = {
    "npc":   1,     # npc x y id
    "item":  2,     # item x y id count
    "exit":  None,  # exit x y target tx ty [условие]
    "spawn": 3,     # spawn x y enemy min max
    "chest": None,  # chest x y золото ключ содержимое
    "note":  1,     # note x y id
    "bed":   0,     # bed x y
    "sign":  None,  # sign x y текст до конца строки
}

# Где игрок оказывается, впервые входя в локацию. Таблица не выводится из
# файлов: точка входа — это замысел, а не свойство сетки, и проверять
# достижимость надо именно от неё.
ENTRIES = {
    "village": (5, 8), "forest": (2, 8), "cave": (3, 9),
    "ruins": (24, 3), "sanctum": (3, 9), "vault": (3, 9),
    "graveyard": (45, 9), "swamp": (44, 9), "goatpath": (24, 15),
    "glassfield": (3, 9), "mill": (24, 15), "market": (3, 9),
    "bridge": (24, 3), "saltmines": (24, 3), "caravanserai": (24, 15),
    "doubled": (3, 9), "halfcity": (24, 2), "endless": (3, 9),
    "foundry": (24, 3), "canal": (24, 2), "counter": (24, 15),
    "archive": (24, 15), "ordergate": (24, 15), "gatehouse": (24, 15),
    "library": (24, 15), "drafting": (24, 15), "cells": (24, 15),
    "furnace": (24, 15), "refusalhall": (24, 15), "node2": (24, 15),
    "node3": (24, 15), "grave": (24, 15), "meadow": (24, 15),
    "farhouse": (45, 9), "well": (45, 5), "grove": (45, 9),
    "battlefield": (45, 12), "otherhalf": (44, 13), "upstair": (4, 14),
    "edge": (24, 15), "emptyalder": (24, 15), "homepath": (45, 9),
    "firstseam": (24, 15), "gallery": (44, 9), "measures": (24, 15),
    "meeting": (24, 15), "node1": (24, 15), "cinchheart": (44, 9),
    "zeropoint": (24, 15), "finale": (24, 15),
}


class Problems(object):
    """Собирает ошибки, чтобы за один прогон показать все, а не первую."""

    def __init__(self):
        self.items = []

    def add(self, where, text):
        self.items.append("%s: %s" % (where, text))

    def __len__(self):
        return len(self.items)


class Map(object):
    def __init__(self, mid, path):
        self.id = mid
        self.path = path
        self.name = ""
        self.w = 0
        self.h = 0
        self.g = []           # список строк сетки
        self.objects = []     # (номер строки в файле, разобранные поля)

    def tile(self, x, y):
        return self.g[y][x]

    def in_bounds(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h


def parse(path, pr):
    """Читает один файл карты. Возвращает Map или None, если разбор не удался."""
    mid = os.path.splitext(os.path.basename(path))[0]
    m = Map(mid, path)
    where = os.path.basename(path)

    try:
        with open(path, encoding="utf-8") as fh:
            lines = fh.read().split("\n")
    except OSError as e:
        pr.add(where, "не прочитать файл (%s)" % e)
        return None

    # Хвостовой перевод строки даёт пустой элемент — он не строка файла.
    if lines and lines[-1] == "":
        lines.pop()

    i = 0
    seen_grid = False
    seen_objects = False
    while i < len(lines):
        raw = lines[i]
        line = raw.rstrip("\r")
        n = i + 1
        i += 1
        if not line.strip():
            continue
        key = line.split(" ", 1)[0]

        if key == "name":
            m.name = line[len("name"):].strip()
        elif key == "size":
            p = line.split()
            if len(p) != 3:
                pr.add("%s:%d" % (where, n), "size ждёт два числа, получено %r" % line)
                return None
            try:
                m.w, m.h = int(p[1]), int(p[2])
            except ValueError:
                pr.add("%s:%d" % (where, n), "size с нечисловым размером: %r" % line)
                return None
            if m.w < 3 or m.h < 3:
                pr.add("%s:%d" % (where, n), "размер %dx%d слишком мал" % (m.w, m.h))
                return None
        elif key == "grid":
            if m.w == 0 or m.h == 0:
                pr.add("%s:%d" % (where, n), "grid раньше size")
                return None
            seen_grid = True
            for row in range(m.h):
                if i >= len(lines):
                    pr.add("%s:%d" % (where, n), "сетка оборвалась: строк %d из %d" % (row, m.h))
                    return None
                g = lines[i].rstrip("\r")
                i += 1
                if len(g) != m.w:
                    pr.add("%s:%d" % (where, i),
                           "ширина строки %d, а size обещал %d" % (len(g), m.w))
                    return None
                bad = sorted(set(g) - KNOWN_TILES)
                if bad:
                    pr.add("%s:%d" % (where, i),
                           "неизвестные символы %s — игра сочтёт их стеной"
                           % " ".join("%r" % b for b in bad))
                m.g.append(g)
        elif key == "objects":
            seen_objects = True
        elif key == "end":
            break
        elif key in OBJECTS:
            fields = parse_object(line, "%s:%d" % (where, n), m, pr)
            if fields is not None:
                m.objects.append((n, fields))
        else:
            pr.add("%s:%d" % (where, n), "непонятная строка %r" % line)

    if not seen_grid:
        pr.add(where, "нет сетки")
        return None
    if len(m.g) != m.h:
        pr.add(where, "строк сетки %d, а size обещал %d" % (len(m.g), m.h))
        return None
    if not seen_objects and m.objects:
        pr.add(where, "объекты есть, а строки objects нет")
    return m


def parse_object(line, where, m, pr):
    """Разбирает строку объекта. Возвращает список полей или None."""
    p = line.split()
    kind = p[0]
    want = OBJECTS[kind]
    if len(p) < 3:
        pr.add(where, "у объекта %s нет координат" % kind)
        return None
    try:
        x, y = int(p[1]), int(p[2])
    except ValueError:
        pr.add(where, "координаты не числа: %r" % line)
        return None
    if want is not None and len(p) != 3 + want:
        pr.add(where, "%s ждёт %d полей после координат, а их %d"
               % (kind, want, len(p) - 3))
        return None
    if kind == "exit":
        # exit x y цель tx ty [условие] — цель и точка прибытия обязательны.
        if len(p) < 6:
            pr.add(where, "переходу нужны цель и точка прибытия: %r" % line)
            return None
        try:
            int(p[4]), int(p[5])
        except ValueError:
            pr.add(where, "точка прибытия не числа: %r" % line)
            return None
    if kind == "item" or kind == "spawn":
        for f in p[4:]:
            try:
                int(f)
            except ValueError:
                pr.add(where, "%s ждёт числа, а получил %r" % (kind, f))
                return None
    if not m.in_bounds(x, y):
        pr.add(where, "объект за границей карты: (%d, %d)" % (x, y))
        return None
    return p


def reachable(m, start):
    """Заливка от точки входа. Возвращает (дошли ли вообще, карта достижимости)."""
    seen = [[False] * m.w for _ in range(m.h)]
    sx, sy = start
    if not m.in_bounds(sx, sy) or m.tile(sx, sy) not in WALKABLE:
        return False, seen
    q = deque([(sx, sy)])
    seen[sy][sx] = True
    while q:
        x, y = q.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if m.in_bounds(nx, ny) and not seen[ny][nx] and m.tile(nx, ny) in WALKABLE:
                seen[ny][nx] = True
                q.append((nx, ny))
    return True, seen


def check(maps, pr, seen_files=None, quiet=False):
    by_id = dict((m.id, m) for m in maps)
    reach = {}
    if seen_files is None:
        seen_files = set(by_id)

    # Таблица входов и набор файлов должны совпадать: карта без входа не
    # проверяется вовсе, а вход без карты — след переименования. Файл, который
    # не разобрался, — это не пропавший файл, и путать их не надо: об ошибке
    # разбора уже сказано выше, а второе сообщение про «нет файла» уводило бы
    # искать не там.
    for mid in sorted(set(ENTRIES) - set(by_id)):
        if mid in seen_files:
            pr.add("точки входа", "%s не проверен: файл не разобрался" % mid)
        else:
            pr.add("точки входа", "для %s нет файла %s.map" % (mid, mid))
    for mid in sorted(set(by_id) - set(ENTRIES)):
        pr.add(mid, "нет точки входа в таблице ENTRIES скрипта")

    for m in sorted(maps, key=lambda x: x.id):
        if m.id not in ENTRIES:
            continue
        start = ENTRIES[m.id]
        ok, seen = reachable(m, start)
        reach[m.id] = seen
        if not ok:
            pr.add(m.id, "точка входа %s непроходима" % (start,))
            continue

        total = sum(1 for row in m.g for c in row if c in WALKABLE)
        got = sum(r.count(True) for r in seen)
        if got != total:
            pr.add(m.id, "отрезано %d проходимых клеток из %d" % (total - got, total))

        for n, p in m.objects:
            x, y = int(p[1]), int(p[2])
            if m.tile(x, y) not in WALKABLE:
                pr.add("%s:%d" % (m.id, n), "объект в стене -> %s" % " ".join(p))
            elif not seen[y][x]:
                pr.add("%s:%d" % (m.id, n), "объект недостижим -> %s" % " ".join(p))

        if not quiet:
            print("%-12s %-28s объектов %2d, проходимых клеток %3d, все достижимы: %s"
                  % (m.id, m.name[:28], len(m.objects), total,
                     "да" if got == total else "НЕТ"))

    # Переходы. Проверяются после всех карт: пункт назначения — другая карта,
    # и её достижимость должна быть уже посчитана.
    for m in sorted(maps, key=lambda x: x.id):
        for n, p in m.objects:
            if p[0] != "exit":
                continue
            tgt, tx, ty = p[3], int(p[4]), int(p[5])
            if tgt not in by_id:
                pr.add("%s:%d" % (m.id, n), "переход в несуществующую локацию %s" % tgt)
                continue
            dm = by_id[tgt]
            if not dm.in_bounds(tx, ty):
                pr.add("%s:%d" % (m.id, n),
                       "точка прибытия (%d, %d) вне карты %s" % (tx, ty, tgt))
                continue
            if dm.tile(tx, ty) not in WALKABLE:
                pr.add("%s:%d" % (m.id, n),
                       "точка прибытия (%d, %d) в %s непроходима" % (tx, ty, tgt))
                continue
            dseen = reach.get(tgt)
            if dseen is not None and not dseen[ty][tx]:
                pr.add("%s:%d" % (m.id, n),
                       "точка прибытия (%d, %d) отрезана от %s" % (tx, ty, tgt))


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("-")]
    quiet = "-q" in argv or "--quiet" in argv
    root = args[0] if args else "data/maps"
    if not os.path.isdir(root):
        print("нет каталога с картами: %s" % root)
        return 1

    paths = sorted(os.path.join(root, f) for f in os.listdir(root) if f.endswith(".map"))
    if not paths:
        print("в %s нет ни одной карты" % root)
        return 1

    pr = Problems()
    maps = []
    seen_files = set()
    for path in paths:
        seen_files.add(os.path.splitext(os.path.basename(path))[0])
        m = parse(path, pr)
        if m is not None:
            maps.append(m)

    check(maps, pr, seen_files, quiet)

    print("")
    if pr:
        print("ОШИБОК: %d" % len(pr))
        for line in pr.items:
            print("  %s" % line)
        return 1
    print("карт проверено: %d, всё сходится" % len(maps))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
