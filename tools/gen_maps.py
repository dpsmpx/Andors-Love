#!/usr/bin/env python3
"""Генератор карт с проверками.

Карты пишутся рельефом (прямоугольники), а не строками вручную: ширина строки
тогда верна по построению. После генерации скрипт проверяет, что вся карта
связна, объекты стоят на проходимых клетках, а переходы взаимно согласованы —
иначе легко получить локацию, из которой не выбраться.
"""
import sys
from collections import deque

W, H = 48, 18
WALKABLE = set(".,=")


class Map:
    def __init__(self, mid, name, fill="#"):
        self.id, self.name = mid, name
        self.g = [[fill] * W for _ in range(H)]
        self.objects = []
        self.border()

    def border(self):
        for x in range(W):
            self.g[0][x] = self.g[H - 1][x] = "#"
        for y in range(H):
            self.g[y][0] = self.g[y][W - 1] = "#"

    def rect(self, x0, y0, x1, y1, ch):
        for y in range(max(1, y0), min(H - 1, y1 + 1)):
            for x in range(max(1, x0), min(W - 1, x1 + 1)):
                self.g[y][x] = ch
        return self

    def fill_all(self, ch):
        return self.rect(1, 1, W - 2, H - 2, ch)

    def put(self, x, y, ch):
        self.g[y][x] = ch
        return self

    def obj(self, line):
        self.objects.append(line)
        return self

    def rows(self):
        return ["".join(r) for r in self.g]

    def write(self, path):
        with open(path, "w", encoding="utf-8") as fh:
            fh.write("name %s\nsize %d %d\ngrid\n" % (self.name, W, H))
            for r in self.rows():
                fh.write(r + "\n")
            fh.write("objects\n")
            for o in self.objects:
                fh.write(o + "\n")
            fh.write("end\n")


# ------------------------------------------------------------- ДЕРЕВНЯ
village = Map("village", "Деревня Ольховка", ",")
for y in (2, 3, 4):
    village.rect(3, y, 9, y, "#").rect(35, y, 41, y, "#")
village.rect(1, 7, W - 2, 8, "=")
village.rect(17, 10, 21, 12, "~")
for y in (11, 12, 13):
    village.rect(3, y, 9, y, "#").rect(35, y, 41, y, "#")
village.rect(7, 15, 9, 15, "T").rect(31, 15, 33, 15, "T")
village.objects = [
    "npc 6 5 elder",
    "npc 38 5 smith",
    "npc 6 10 trader",
    "npc 38 10 herbalist",
    "npc 24 5 enchanter",
    "bed 10 3",
    "sign 12 6 Ольховка. Восточные ворота ведут в Ольховый лес.",
    "sign 44 9 За воротами начинается лес. Волки выходят к самой тропе.",
    "item 11 16 bread 2",
    "item 30 16 torch 1",
    "chest 44 3 40 - bread:2 herb_potion:1",
    "spawn 20 15 rat 2 4",
    "spawn 44 15 rat 1 3",
    "exit 46 8 forest 2 8",
]

# ------------------------------------------------------------------ ЛЕС
forest = Map("forest", "Ольховый лес", ",")
for (x, y, n) in [(1,1,4),(13,1,3),(26,1,2),(40,1,5),(11,2,2),(34,2,3),
                  (4,3,3),(24,3,2),(39,3,3),(7,5,3),(26,5,4),(43,5,3),
                  (15,6,2),(29,6,2),(5,7,3),(28,7,3),(9,9,2),(24,9,3),(38,9,2),
                  (6,11,3),(34,11,2),(38,12,3),(11,13,3),(32,13,2),
                  (7,15,4),(25,15,3),(39,15,4),(17,16,2)]:
    forest.rect(x, y, x + n - 1, y, "T")
forest.rect(19, 10, 24, 12, "~")
forest.objects = [
    "npc 24 4 hermit",
    "sign 4 8 Тропа на запад ведёт в Ольховку, на восток и юг — дальше в глушь.",
    "sign 43 3 Расщелина в камне. Внутри пахнет сыростью.",
    "sign 23 15 Старая дорога на юг, к развалинам заставы.",
    "item 8 13 herb_potion 1",
    "item 33 16 bread 1",
    "item 15 4 torch 1",
    "chest 8 3 25 - herb_potion:1 whetstone:1",
    "spawn 12 6 wolf 3 5",
    "spawn 30 11 wolf 3 5",
    "spawn 20 14 wolf 2 4",
    "spawn 36 4 bandit 2 4",
    "spawn 41 13 wolf_alpha 1 3",
    "exit 1 8 village 45 8",
    "exit 45 2 cave 3 9",
    "exit 24 16 ruins 24 3",
]

# -------------------------------------------------------------- ПЕЩЕРА
cave = Map("cave", "Барсучья пещера", "#")
cave.rect(2, 8, 20, 10, ".")            # входной коридор
cave.rect(5, 4, 12, 7, ".")             # верхний зал
cave.rect(4, 11, 14, 14, ".")           # нижний зал
cave.rect(20, 5, 23, 13, ".")           # перемычка
cave.rect(24, 3, 33, 7, ".")            # северная галерея
cave.rect(24, 11, 34, 15, ".")          # южная галерея
cave.rect(30, 7, 33, 12, ".")           # соединение галерей
cave.rect(34, 6, 44, 12, ".")           # логово матки
cave.rect(18, 3, 20, 8, ".")
cave.rect(38, 13, 41, 14, ".")
cave.rect(14, 9, 20, 9, ".")
cave.rect(26, 8, 29, 9, ".")            # проход между галереями
cave.objects = [
    "sign 4 9 Свод низкий, под ногами хрустит хитин.",
    "item 10 5 glow_moss 1",
    "item 8 13 glow_moss 1",
    "item 27 4 glow_moss 1",
    "item 31 14 glow_moss 1",
    "item 12 12 torch 1",
    "chest 11 4 60 - antidote:2 frost_shard:1",
    "chest 40 14 120 - ember:1 elixir_guard:1",
    "spawn 8 6 spider 2 4",
    "spawn 27 5 bat 3 5",
    "spawn 28 13 spider 3 5",
    "spawn 20 12 bat 2 4",
    "spawn 40 9 spider_queen 1 3",
    "exit 2 9 forest 44 2",
]

# ------------------------------------------------------------ РАЗВАЛИНЫ
ruins = Map("ruins", "Развалины заставы", ",")
ruins.rect(6, 4, 20, 12, ".")           # двор
ruins.rect(6, 4, 20, 4, "#").rect(6, 12, 20, 12, "#")
ruins.rect(6, 5, 6, 11, "#").rect(20, 5, 20, 11, "#")
ruins.rect(12, 4, 14, 4, ".")           # пролом в северной стене
ruins.rect(6, 8, 6, 9, ".")             # пролом в западной стене
ruins.rect(20, 8, 20, 9, ".")           # пролом в восточной стене
ruins.rect(9, 6, 11, 8, "#")            # обломки внутри двора
ruins.rect(26, 5, 40, 13, ".")          # плац
ruins.rect(30, 8, 33, 10, "#")          # руины башни
ruins.rect(21, 8, 26, 9, ".")
ruins.rect(23, 1, 25, 4, ",")
ruins.rect(41, 8, 46, 10, ".")
for (x, y, n) in [(2,2,3),(43,2,3),(2,15,4),(30,16,3),(16,15,3)]:
    ruins.rect(x, y, x + n - 1, y, "T")
ruins.objects = [
    "sign 24 5 Застава сожжена давно. Сейчас тут живут не солдаты.",
    "item 8 10 bread 2",
    "item 38 6 whetstone 1",
    "chest 18 10 90 - elixir_might:1 ember:1",
    "chest 34 12 300 rusty_key plate_armor:1 rune_stone:1",
    "spawn 12 9 brigand 3 4",
    "spawn 30 6 brigand 3 5",
    "spawn 37 11 brigand 2 4",
    "spawn 44 9 bandit_chief 1 2",
    "exit 24 2 forest 24 15",
    "exit 46 9 sanctum 3 9",
]

# ------------------------------------------------------------ СВЯТИЛИЩЕ
sanctum = Map("sanctum", "Святилище Нулевой точки", "#")
sanctum.rect(2, 8, 12, 10, ".")         # вход
sanctum.rect(8, 4, 20, 14, ".")         # первый зал
sanctum.rect(21, 8, 26, 10, ".")        # переход
sanctum.rect(24, 3, 38, 15, ".")        # главный зал
sanctum.rect(29, 7, 33, 11, "#")        # алтарь в центре
sanctum.rect(31, 7, 31, 7, ".")
sanctum.rect(39, 8, 45, 10, ".")        # ниша
sanctum.rect(14, 2, 16, 4, ".")
sanctum.rect(14, 14, 16, 16, ".")
sanctum.objects = [
    "sign 5 9 Стены гладкие, будто их не строили, а вырастили.",
    "sign 27 4 Здесь сходятся линии. Нулевая точка близко.",
    "item 15 3 rune_stone 1",
    "item 15 15 frost_shard 1",
    "chest 43 9 250 - rune_stone:1 elixir_haste:1 portal_stone:1",
    "chest 10 13 140 - antidote:2 salve:1",
    "spawn 14 6 wraith 2 4",
    "spawn 17 12 wraith 2 4",
    "spawn 28 5 wraith 3 5",
    "spawn 35 13 wraith 2 4",
    "spawn 42 9 keeper 1 2",
    "exit 2 9 ruins 44 9",
]

MAPS = [village, forest, cave, ruins, sanctum]

# --------------------------------------------------------------- проверки
def reachable(m, start):
    seen = [[False] * W for _ in range(H)]
    sx, sy = start
    if m.g[sy][sx] not in WALKABLE:
        return None, seen
    q = deque([start]); seen[sy][sx] = True
    while q:
        x, y = q.popleft()
        for dx, dy in ((1,0),(-1,0),(0,1),(0,-1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < W and 0 <= ny < H and not seen[ny][nx] and m.g[ny][nx] in WALKABLE:
                seen[ny][nx] = True; q.append((nx, ny))
    return True, seen

def check():
    ok = True
    by_id = {m.id: m for m in MAPS}
    entries = {"village": (5, 8), "forest": (2, 8), "cave": (3, 9),
               "ruins": (24, 3), "sanctum": (3, 9)}

    for m in MAPS:
        start = entries[m.id]
        res, seen = reachable(m, start)
        if res is None:
            print("  %s: точка входа %s непроходима" % (m.id, start)); ok = False; continue

        total = sum(1 for r in m.g for c in r if c in WALKABLE)
        got = sum(r.count(True) for r in seen)
        if got != total:
            print("  %s: отрезано %d проходимых клеток из %d" % (m.id, total - got, total))
            ok = False

        for line in m.objects:
            p = line.split(); x, y = int(p[1]), int(p[2])
            if m.g[y][x] not in WALKABLE:
                print("  %s: объект в стене -> %s" % (m.id, line)); ok = False
            elif not seen[y][x]:
                print("  %s: объект недостижим -> %s" % (m.id, line)); ok = False

        print("%-9s объектов %2d, проходимых клеток %3d, все достижимы: %s"
              % (m.id, len(m.objects), total, "да" if got == total else "НЕТ"))

    # Переходы должны вести в существующую локацию на проходимую клетку.
    for m in MAPS:
        for line in m.objects:
            p = line.split()
            if p[0] != "exit":
                continue
            tgt, tx, ty = p[3], int(p[4]), int(p[5])
            if tgt not in by_id:
                print("  %s: переход в несуществующую локацию %s" % (m.id, tgt)); ok = False; continue
            dm = by_id[tgt]
            if dm.g[ty][tx] not in WALKABLE:
                print("  %s -> %s: точка прибытия (%d,%d) непроходима" % (m.id, tgt, tx, ty))
                ok = False
            _, dseen = reachable(dm, entries[tgt])
            if not dseen[ty][tx]:
                print("  %s -> %s: точка прибытия отрезана от локации" % (m.id, tgt)); ok = False
    return ok


good = check()
for m in MAPS:
    m.write("data/maps/%s.map" % m.id)
print("записано карт: %d" % len(MAPS))
sys.exit(0 if good else 1)
