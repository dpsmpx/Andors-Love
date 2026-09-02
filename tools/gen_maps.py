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
    "note 30 2 child",
    "note 2 16 double",
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
    "item 10 7 oak_gall 1",
    "item 28 12 oak_gall 1",
    "item 35 15 oak_gall 1",
    "item 18 2 oak_gall 1",
    "note 12 3 ink",
    "note 26 4 hermit",
    "chest 8 3 25 - herb_potion:1 whetstone:1",
    "spawn 12 6 wolf 3 5",
    "spawn 30 11 wolf 3 5",
    "spawn 20 14 wolf 2 4",
    "spawn 36 4 bandit 2 4",
    "spawn 41 13 wolf_alpha 1 3",
    "exit 1 8 village 45 8",
    "exit 45 2 cave 3 9",
    "exit 24 16 ruins 24 3",
    "exit 12 1 goatpath 24 15 quest=goatpath:1 "
    "deny=Тропа наверх зарастает и никуда, кажется, не ведёт. Гурий что-то про неё говорил.",
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
    "note 13 12 miner",
    "note 38 7 proto",
    "note 30 13 order",
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
    "note 15 6 watch",
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
sanctum.rect(31, 7, 31, 9, ".")   # ход внутрь алтаря, к шву
sanctum.rect(39, 8, 45, 10, ".")        # ниша
sanctum.rect(14, 2, 16, 4, ".")
sanctum.rect(14, 14, 16, 16, ".")
sanctum.objects = [
    "sign 5 9 Стены гладкие, будто их не строили, а вырастили.",
    "sign 27 4 Здесь сходятся линии. Нулевая точка близко.",
    "item 15 3 rune_stone 1",
    "item 15 15 frost_shard 1",
    "note 15 4 zero",
    "note 35 4 seam",
    "chest 43 9 250 - rune_stone:1 elixir_haste:1 portal_stone:1",
    "chest 10 13 140 - antidote:2 salve:1",
    "spawn 14 6 wraith 2 4",
    "spawn 17 12 wraith 2 4",
    "spawn 28 5 wraith 3 5",
    "spawn 35 13 wraith 2 4",
    "spawn 42 9 keeper 1 2",
    "exit 2 9 ruins 44 9",
    "exit 31 9 vault 3 9 key=seam_key quest=seam:1 "
    "deny=Северная грань алтаря глухая. Здесь что-то есть, но не для тебя — пока.",
]

# ------------------------------------------------------------ СХРОН ОРДЕНА
# Попасть можно только через шов за алтарём святилища: нужен ключ стража
# и найденная заметка, открывающая тайну. Обычным путём сюда не выйти.
vault = Map("vault", "Схрон Ордена", "#")
vault.rect(2, 8, 10, 10, ".")        # входной коридор от шва
vault.rect(10, 5, 20, 13, ".")       # приёмный зал
vault.rect(20, 8, 26, 10, ".")       # перемычка
vault.rect(26, 4, 40, 14, ".")       # хранилище
vault.rect(31, 8, 35, 10, "#")       # стеллажи посередине
vault.rect(41, 8, 45, 10, ".")       # дальняя ниша
vault.objects = [
    "sign 4 9 Воздух стоячий, но не затхлый. Здесь двести лет никто не дышал.",
    "note 34 12 cinch",
    "item 12 6 rune_stone 1",
    "chest 14 12 200 - antidote:3 salve:2",
    "chest 38 5 400 - elixir_haste:1 rune_stone:1 portal_stone:1",
    "chest 44 9 600 - plate_armor:1 ember:1 frost_shard:1",
    "spawn 16 9 wraith 2 4",
    "spawn 33 6 wraith 2 4",
    "spawn 37 12 archivist 1 2",
    "exit 2 9 sanctum 31 6",
]

# ===================== РЕГИОН II · ШОВ =====================

# --- Козья тропа: узкая, вьётся вверх, разводит на два края Шва ---
goatpath = Map("goatpath", "Козья тропа", "#")
goatpath.rect(22, 13, 26, 16, ",")       # низ, выход обратно в лес
goatpath.rect(22, 10, 26, 13, ",")
goatpath.rect(8, 10, 26, 12, ",")        # длинный траверс налево
goatpath.rect(6, 4, 10, 12, ",")         # подъём к стеклянному полю
goatpath.rect(26, 9, 44, 12, ",")        # ветка направо, смыкается с траверсом
goatpath.rect(40, 4, 44, 10, ",")        # подъём к мельнице
goatpath.rect(14, 8, 16, 10, ",")        # карман с запиской
goatpath.rect(32, 6, 34, 9, ",")
goatpath.objects = [
    "sign 24 14 Тропа круче, чем помнится. Перевал всё не кончается.",
    "note 15 9 goat",
    "item 33 7 old_coin 1",
    "item 9 5 bread 1",
    "chest 15 8 80 - herb_potion:2 old_coin:1",
    "spawn 12 11 stray 2 4",
    "spawn 30 11 stray 2 4",
    "spawn 42 7 mill_rat 2 3",
    "exit 24 16 forest 10 2",
    "exit 7 4 glassfield 3 9",
    "exit 42 4 mill 24 15",
]

# --- Стеклянное поле: открытое, режущее ---
glassfield = Map("glassfield", "Стеклянное поле", ",")
for (x, y, n) in [(6,3,4),(20,2,5),(34,4,4),(10,14,5),(28,13,4),(40,11,3)]:
    glassfield.rect(x, y, x + n - 1, y, "#")     # гряды спёкшегося стекла
glassfield.rect(18, 7, 24, 9, "#")
glassfield.rect(2, 8, 12, 10, ".")               # тропа от перевала
glassfield.rect(30, 7, 44, 10, ".")
glassfield.objects = [
    "npc 14 9 glazier",
    "sign 5 9 Земля здесь спеклась за одну ночь. И не от огня.",
    "note 25 5 glass",
    "item 8 12 glass_shard 1",
    "item 31 3 glass_shard 1",
    "item 43 14 glass_shard 1",
    "item 16 15 old_coin 1",
    "chest 42 3 140 - glass_dust:1 antidote:2",
    "spawn 10 5 glass_hound 3 5",
    "spawn 36 13 glass_hound 3 5",
    "spawn 24 12 glass_hound 2 4",
    "exit 2 9 goatpath 8 5",
    "exit 45 9 market 3 9",
    "exit 24 16 bridge 24 3",
]

# --- Мельница на Тихой: река втекает и не вытекает ---
mill = Map("mill", "Мельница на Тихой", ",")
mill.rect(1, 6, 46, 8, "~")                      # русло Тихой
mill.rect(18, 6, 22, 8, "#")                     # плотина и колесо
mill.rect(20, 6, 20, 8, ".")                     # проход по гребню плотины
mill.rect(14, 9, 26, 13, ".")                    # двор мельницы
mill.rect(16, 10, 18, 12, "#")                   # сама мельница
mill.rect(2, 9, 14, 11, ".")
mill.rect(26, 9, 44, 12, ".")
mill.rect(30, 13, 34, 16, ".")                   # спуск к затвору
for (x, y, n) in [(4,2,4),(28,2,5),(40,3,4),(6,14,3),(42,14,3)]:
    mill.rect(x, y, x + n - 1, y, "T")
mill.objects = [
    "npc 22 11 miller",
    "npc 32 15 digger",
    "sign 24 10 Колесо стоит третий год. Воды хватает, а тяги нет.",
    "note 12 10 mill",
    "item 40 11 bread 2",
    "chest 44 10 110 - salve:1 old_coin:2",
    "spawn 8 10 mill_rat 3 4",
    "spawn 36 11 mill_rat 3 4",
    "spawn 24 15 stray 1 3",
    "exit 24 15 goatpath 42 5",
    "exit 32 16 saltmines 24 3 key=mill_key "
    "deny=Нижний затвор заперт наглухо. Мельник говорил, что ключ у него.",
]

# --- Рынок Шва: узел региона, ничей ---
market = Map("market", "Рынок Шва", ",")
for row in (5, 8, 11):                           # торговые ряды
    market.rect(6, row, 20, row, "#")
    market.rect(26, row, 40, row, "#")
market.rect(2, 8, 6, 10, ".")
market.rect(20, 2, 26, 16, "=")                  # главный проход
market.rect(40, 8, 46, 10, ".")
market.objects = [
    "npc 23 9 warden",
    "sign 23 12 Уложение Рынка: у товара не спрашивают года, у человека — лоскута.",
    "note 23 4 market",
    "item 8 6 old_coin 1",
    "item 38 12 spice_bag 1",
    "chest 8 12 160 - caravan_stew:2 old_coin:2",
    "chest 38 6 190 - trader_hood:1 herb_potion:2",
    "spawn 12 14 stray 2 4",
    "spawn 34 14 stray 2 4",
    "exit 2 9 glassfield 44 9",
    "exit 23 2 caravanserai 24 15",
    "exit 45 9 doubled 3 9",
]

# --- Подвесной мост в никуда ---
bridge = Map("bridge", "Подвесной мост в никуда", "#")
bridge.rect(20, 2, 28, 6, ",")                   # площадка у входа
bridge.rect(22, 6, 26, 13, "=")                  # сам мост
bridge.rect(20, 13, 28, 15, "=")                 # обрыв: дальше ничего
bridge.rect(16, 3, 20, 5, ",")
bridge.rect(28, 3, 34, 5, ",")
bridge.objects = [
    "sign 24 5 Мост висел через ущелье. Ущелья нет, а мост есть.",
    "note 24 14 bridge",
    "item 18 4 rope_end 1",
    "chest 32 4 130 - frost_shard:1 elixir_guard:1",
    "spawn 24 10 bridge_walker 1 2",
    "spawn 31 4 caravan_shade 1 2",
    "exit 24 2 glassfield 24 15",
]

# --- Соляные шахты: соль держит то, что не должна ---
saltmines = Map("saltmines", "Соляные шахты", "#")
saltmines.rect(22, 2, 26, 6, ".")                # ствол сверху
saltmines.rect(10, 6, 38, 8, ".")                # верхний горизонт
saltmines.rect(8, 8, 12, 14, ".")                # западная штольня
saltmines.rect(6, 12, 20, 14, ".")
saltmines.rect(34, 8, 40, 15, ".")               # восточная штольня
saltmines.rect(20, 10, 36, 12, ".")              # нижний горизонт
saltmines.rect(24, 12, 28, 16, ".")              # заложенная нижняя
saltmines.objects = [
    "sign 24 4 Стены белые и мокрые. Соль скрипит под ногой.",
    "note 10 13 salt",
    "item 37 9 salt_lump 1",
    "item 12 7 old_coin 1",
    "chest 18 13 210 - salt_lump:2 antidote:2 old_coin:2",
    "chest 39 14 260 - brine_ring:1 salt_lump:2",
    "spawn 14 7 salt_ghoul 3 5",
    "spawn 32 11 salt_ghoul 3 5",
    "spawn 37 12 salt_ghoul 2 4",
    "spawn 26 15 salt_mother 1 2",
    "exit 24 2 mill 32 15",
]

# --- Пустой караван-сарай: накрыто на сорок человек ---
caravanserai = Map("caravanserai", "Пустой караван-сарай", "#")
caravanserai.rect(8, 4, 40, 14, ".")             # двор под навесом
caravanserai.rect(12, 6, 18, 8, "#")             # столы
caravanserai.rect(22, 6, 28, 8, "#")
caravanserai.rect(32, 6, 36, 8, "#")
caravanserai.rect(12, 11, 18, 12, "#")
caravanserai.rect(30, 11, 36, 12, "#")
caravanserai.rect(20, 14, 28, 16, ".")           # ворота
caravanserai.objects = [
    "sign 24 13 Двенадцать подвод распряжены. Лошадей нет.",
    "note 20 7 caravan",
    "item 30 9 caravan_stew 1",
    "item 11 13 old_coin 2",
    "item 38 5 spice_bag 1",
    "chest 38 13 240 - caravan_stew:2 spice_bag:1 old_coin:3",
    "chest 10 5 180 - old_coin:3 herb_potion:1",
    "spawn 16 10 caravan_shade 3 5",
    "spawn 33 10 caravan_shade 3 5",
    "spawn 24 5 caravan_shade 2 4",
    "exit 24 15 market 23 3",
]

# --- Хутор Двоеданный: два двора, два Прохора ---
doubled = Map("doubled", "Хутор Двоеданный", ",")
doubled.rect(2, 8, 10, 10, ".")                  # дорога с рынка
doubled.rect(10, 4, 20, 7, "#")                  # левый двор
doubled.rect(12, 5, 18, 6, ".")
doubled.rect(15, 7, 15, 7, ".")                  # калитка на межу
doubled.rect(10, 11, 20, 14, "#")                # правый двор — зеркально
doubled.rect(12, 12, 18, 13, ".")
doubled.rect(15, 11, 15, 11, ".")
doubled.rect(10, 8, 40, 10, ".")                 # межа между дворами
doubled.rect(28, 4, 38, 7, "#")
doubled.rect(30, 5, 36, 6, ".")
doubled.rect(33, 7, 33, 7, ".")
doubled.rect(28, 11, 38, 14, "#")
doubled.rect(30, 12, 36, 13, ".")
doubled.rect(33, 11, 33, 11, ".")
doubled.objects = [
    "npc 15 6 prohor_l",
    "npc 15 12 prohor_r",
    "sign 24 9 Межа посреди хутора. По обе стороны всё одинаковое.",
    "note 33 5 prohor",
    "item 33 12 bread 2",
    "chest 40 9 170 - salve:2 old_coin:2",
    "spawn 44 12 stray 1 3",
    "exit 2 9 market 44 9",
]

MAPS = [village, forest, cave, ruins, sanctum, vault,
        goatpath, glassfield, mill, market, bridge, saltmines,
        caravanserai, doubled]

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
               "ruins": (24, 3), "sanctum": (3, 9), "vault": (3, 9),
               "goatpath": (24, 15), "glassfield": (3, 9), "mill": (24, 15),
               "market": (3, 9), "bridge": (24, 3), "saltmines": (24, 3),
               "caravanserai": (24, 15), "doubled": (3, 9)}

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
