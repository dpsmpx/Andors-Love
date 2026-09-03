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
WALKABLE = set(".,=:")   # ':' — мёртвая вода: стоит и держит вес


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
    "sign 4 6 Западная дорога кончается погостом. Дальше только топь.",
    "item 11 16 bread 2",
    "item 30 16 torch 1",
    "note 30 2 child",
    "note 2 16 double",
    "chest 44 3 40 - bread:2 herb_potion:1",
    "spawn 20 15 rat 2 4",
    "spawn 44 15 rat 1 3",
    "exit 46 8 forest 2 8",
    "exit 1 8 graveyard 44 9",
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
    "exit 45 13 swamp 24 14 quest=swamproot:1 "
    "deny=Низина за деревьями стоит под водой и пахнет тухлым. Лезть туда без нужды не стоит.",
]

# ------------------------------------------------------------ ПОГОСТ
# Сразу за околицей, по западной дороге. Крайние три надгробья стоят
# отдельно, и камень у них другой.
graveyard = Map("graveyard", "Ольховский погост", ",")
graveyard.rect(2, 3, 45, 15, ",")
graveyard.rect(2, 8, 45, 9, "=")                 # дорога насквозь
for y in (5, 12):                                # ряды могил
    for x in range(10, 40, 4):
        graveyard.rect(x, y, x, y, "#")
graveyard.rect(4, 12, 8, 14, "#")                # сарай могильщика
graveyard.rect(5, 13, 7, 13, ".")                # внутри: тетрадь и ящик
graveyard.rect(6, 14, 6, 14, ".")                # дверь
graveyard.rect(41, 4, 43, 6, "#")                # три отдельных надгробья
graveyard.rect(42, 5, 42, 7, ".")                # подход к ним с дороги
graveyard.rect(8, 3, 10, 3, "T").rect(30, 15, 33, 15, "T")
graveyard.objects = [
    "sign 24 10 Ольховский погост. Ограда низкая, калитка не запирается.",
    "sign 40 7 Три камня стоят отдельно, и тёсаны они не здешней рукой.",
    "note 42 5 graves",
    "note 5 13 sexton",
    "item 12 15 bread 1",
    "chest 7 13 60 - grave_list:1 salve:1 torch:2",
    "spawn 16 4 barrow_shade 2 4",
    "spawn 34 13 barrow_shade 2 4",
    "spawn 20 15 rat 2 4",
    "exit 45 9 village 2 8",
    "exit 2 9 swamp 44 9 quest=swamproot:1 "
    "deny=Дальше дороги нет, одна топь. Лада про неё говорила недоброе.",
]

# ------------------------------------------------------------- ГНИЛАЯ ТОПЬ
# Первый «неправильный» лоскут: вода солёная, а под водой — мощёная дорога,
# которой в этих краях быть не может.
swamp = Map("swamp", "Гнилая топь", "~")
swamp.rect(2, 3, 45, 15, "~")
swamp.rect(2, 8, 45, 10, ",")                    # гряда кочек поперёк
swamp.rect(10, 4, 13, 14, ",")                   # кочки-островки
swamp.rect(22, 4, 26, 14, ",")
swamp.rect(34, 4, 38, 14, ",")
swamp.rect(24, 5, 24, 13, "=")                   # мостовая, видная сквозь воду
swamp.rect(2, 14, 45, 15, "~")
swamp.rect(11, 12, 12, 14, ",")
swamp.rect(23, 14, 25, 15, ",")                  # мостовая выходит на юг, к лесу
swamp.objects = [
    "sign 24 11 Вода стоит и не цветёт. Под ней, где помельче, видна тёсаная мостовая.",
    "note 12 6 bog",
    "note 36 13 drovers",
    "item 24 5 road_stone 1",
    "item 36 5 bog_root 2",
    "item 11 13 bog_root 1",
    "chest 37 9 90 - bog_root:2 salve:2 drover_knife:1",
    "spawn 12 9 bog_leech 3 5",
    "spawn 36 9 bog_leech 2 4",
    "spawn 25 12 drowned_man 2 4",
    "spawn 24 4 bog_walker 1 2",
    "exit 44 9 graveyard 3 9",
    "exit 24 15 forest 44 13",
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
    "exit 15 16 homepath 44 9 quest=driftway:2 "
    "deny=Южная ниша глухая. По тропе, которой ты не прошёл, обратно не ходят.",
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
    "exit 23 16 halfcity 24 2 quest=cityroad:1 "
    "deny=Северная дорога уходит в туман. Улей говорил, откуда возят городской товар.",
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

# ===================== РЕГИОН III · ПОЛОВИНЫ =====================

# --- Половина Города: всё восточнее среза просто не существует ---
halfcity = Map("halfcity", "Половина Города", "#")
halfcity.rect(2, 2, 28, 16, ".")                 # уцелевшая западная половина
halfcity.rect(6, 4, 12, 7, "#")                  # кварталы
halfcity.rect(16, 4, 22, 7, "#")
halfcity.rect(6, 10, 12, 14, "#")
halfcity.rect(16, 10, 22, 14, "#")
halfcity.rect(8, 5, 10, 6, ".")                  # дворы внутри кварталов
halfcity.rect(18, 5, 20, 6, ".")
halfcity.rect(9, 7, 9, 7, ".")
halfcity.rect(19, 7, 19, 7, ".")
halfcity.rect(8, 11, 10, 13, ".")
halfcity.rect(9, 10, 9, 10, ".")
halfcity.objects = [
    "npc 14 9 survivor",
    "sign 27 9 Мостовая обрывается ровно, как ножом. Дальше не развалины — дальше ничего.",
    "note 9 6 cityhalf",
    "note 19 6 lastclerk",
    "item 26 5 city_brick 1",
    "item 26 13 scrap_iron 1",
    "chest 9 12 260 - strong_tea:1 scrap_iron:2 ledger_page:1",
    "spawn 14 3 city_rat 3 4",
    "spawn 14 15 city_rat 3 4",
    "spawn 26 7 cut_man 2 3",
    "spawn 27 15 half_warden 1 2",
    "exit 24 2 market 23 15",
    "exit 4 2 endless 3 9",
    "exit 4 16 foundry 24 3",
    "exit 24 16 canal 24 2",
]

# --- Улица без конца: восточный конец выводит в собственное начало ---
endless = Map("endless", "Улица без конца", "#")
endless.rect(2, 8, 45, 10, ".")                  # прямая улица
endless.rect(10, 5, 14, 8, ".")                  # ниши по сторонам
endless.rect(24, 10, 28, 13, ".")
endless.rect(36, 5, 40, 8, ".")
endless.objects = [
    "sign 6 9 Кольцевая улица. Она прямая. Восемьсот сорок шагов.",
    "note 26 12 endless",
    "item 12 6 ledger_page 1",
    "chest 38 6 220 - strong_tea:1 old_coin:3",
    "spawn 20 9 mad_clerk 3 5",
    "spawn 33 9 mad_clerk 2 4",
    "exit 2 9 halfcity 5 2",
    # Восточный конец не кончается: он и есть западное начало.
    "exit 45 9 endless 3 9",
]

# --- Литейный двор: печь не гасили двести лет ---
foundry = Map("foundry", "Литейный двор", "#")
foundry.rect(20, 2, 28, 5, ".")                  # ворота сверху
foundry.rect(6, 5, 42, 12, ".")                  # цех
foundry.rect(14, 7, 20, 10, "#")                 # печь
foundry.rect(17, 6, 17, 6, ".")                  # летка
foundry.rect(28, 7, 34, 10, "#")                 # формовочная
foundry.rect(31, 6, 31, 6, ".")
foundry.rect(8, 13, 20, 15, ".")                 # нижний цех
foundry.rect(36, 13, 42, 15, ".")
foundry.objects = [
    "npc 24 6 founder",
    "npc 8 7 scribe",
    "sign 24 4 Жар слышно за квартал. Летку не закрывают.",
    "sign 7 5 Дверь архива. На ступенях сидит писарь и смотрит на неё.",
    "note 40 14 foundry",
    "item 10 14 scrap_iron 2",
    "item 38 6 scrap_iron 1",
    "item 8 11 scrap_iron 1",
    "chest 41 11 300 - scrap_iron:3 ember:1 strong_tea:1",
    "spawn 10 8 slag_thing 3 5",
    "spawn 38 9 slag_thing 3 5",
    "spawn 14 14 slag_thing 2 3",
    "spawn 40 14 slag_master 1 2",
    "exit 24 3 halfcity 5 16",
    "exit 6 6 archive 24 15 key=archive_key "
    "deny=Дверь архива заперта. Феофан говорил, что ключ у смотрителя — и что тот его не отдаст.",
]

# --- Канал Мёртвой воды: вода стоит, по ней ходят ---
canal = Map("canal", "Канал Мёртвой воды", "#")
canal.rect(20, 2, 28, 6, ",")                    # спуск с города
canal.rect(2, 6, 45, 11, ":")                    # сам канал: стоячая вода
canal.rect(2, 11, 45, 13, ",")                   # южная набережная
canal.rect(6, 4, 10, 6, ",")                     # причал
canal.rect(38, 4, 44, 6, ",")
canal.objects = [
    "npc 8 5 ferryman",
    "sign 24 5 Вода не течёт и не сохнет. Ходить можно, плавать нельзя.",
    "note 42 5 deadwater",
    "item 16 8 ferry_token 1",
    "item 30 9 ferry_token 1",
    "item 12 12 ferry_token 1",
    "chest 43 12 280 - ferry_token:2 antidote:2 strong_tea:1",
    "spawn 20 8 canal_walker 3 5",
    "spawn 36 9 canal_walker 3 5",
    "spawn 28 12 city_rat 2 4",
    "exit 24 2 halfcity 24 15",
    "exit 44 12 counter 24 15",
]

# --- Башня Счетовода ---
counter_tower = Map("counter", "Башня Счетовода", "#")
counter_tower.rect(20, 12, 28, 16, ".")          # основание
counter_tower.rect(22, 4, 26, 12, ".")           # ствол башни
counter_tower.rect(16, 2, 32, 5, ".")            # верхняя площадка
counter_tower.rect(10, 13, 20, 15, ".")
counter_tower.rect(28, 13, 38, 15, ".")
counter_tower.objects = [
    "npc 24 3 counter",
    "sign 24 11 Лестница узкая, ветер сильный. Наверху кто-то считает вслух.",
    "note 30 3 halves",
    "note 18 3 counting",
    "item 12 14 ledger_page 1",
    "item 36 14 ledger_page 1",
    "chest 11 14 240 - ledger_page:2 strong_tea:2",
    "spawn 24 8 mad_clerk 2 3",
    "spawn 34 14 mad_clerk 2 4",
    "exit 24 15 canal 43 12",
    "exit 37 14 ordergate 24 15 key=order_seal "
    "deny=Ворота обители заперты наглухо. Аким говорил про печать — узел из серебра.",
]

# --- Городской архив ---
archive = Map("archive", "Городской архив", "#")
archive.rect(20, 12, 28, 16, ".")                # сени
archive.rect(6, 4, 42, 12, ".")                  # зал
for x in (10, 16, 22, 28, 34):                   # стеллажи
    archive.rect(x, 5, x + 2, 10, "#")
archive.rect(6, 11, 42, 11, ".")
archive.objects = [
    "sign 24 13 Полки до потолка. Половина полок пуста, и пыли на них нет.",
    "note 38 6 lists",
    "item 14 8 half_name 1",
    "item 26 8 ledger_page 2",
    "chest 40 9 380 - ledger_page:3 old_coin:4 strong_tea:1",
    "chest 8 11 320 - clerk_robe:1 ledger_page:2",
    "spawn 14 6 archive_moth 3 5",
    "spawn 32 8 archive_moth 3 5",
    "spawn 24 11 archive_moth 2 4",
    "exit 24 15 foundry 7 6",
]

# ===================== РЕГИОН IV · ОРДЕН =====================

def hall(mid, name, fill="#"):
    """Заготовка обительского зала: вход снизу по центру."""
    m = Map(mid, name, fill)
    m.rect(22, 13, 26, 16, ".")
    return m

# --- Ворота Ордена ---
ordergate = hall("ordergate", "Ворота Ордена")
ordergate.rect(8, 9, 40, 13, ".")                # площадь перед воротами
ordergate.rect(20, 4, 28, 9, ".")                # створ ворот
ordergate.rect(12, 5, 18, 8, ".")
ordergate.rect(30, 5, 36, 8, ".")
ordergate.objects = [
    "sign 24 12 На створке вычеканен узел. Замочной скважины нет — есть углубление по печати.",
    "note 15 6 gates",
    "item 34 6 order_draught 1",
    "chest 35 12 420 - order_draught:2 rune_stone:1",
    "spawn 14 11 gate_guard 2 4",
    "spawn 34 11 gate_guard 2 4",
    "exit 24 15 counter 36 14",
    "exit 24 4 gatehouse 24 15",
]

# --- Привратная ---
gatehouse = hall("gatehouse", "Привратная")
gatehouse.rect(6, 8, 42, 12, ".")                # длинные сени
gatehouse.rect(10, 4, 20, 8, ".")                # караулка
gatehouse.rect(28, 4, 38, 8, ".")                # оружейная
gatehouse.objects = [
    "npc 24 11 gatekeeper",
    "sign 24 9 Алебарды стоят в козлах. Ни одна не тронута.",
    "note 12 6 watchwrit",
    "item 36 6 scrap_iron 2",
    "chest 18 5 380 - order_draught:2 whetstone:1",
    "spawn 8 10 gate_guard 2 4",
    "spawn 40 10 acolyte 2 4",
    "exit 24 15 ordergate 24 5",
    "exit 8 8 library 24 15",
    "exit 40 8 cells 24 15",
]

# --- Библиотека Ордена ---
library = hall("library", "Библиотека Ордена")
library.rect(4, 3, 44, 12, ".")
for x in (8, 14, 20, 26, 32, 38):                # стеллажи
    library.rect(x, 4, x + 2, 10, "#")
library.rect(4, 11, 44, 12, ".")
library.objects = [
    "npc 6 6 librarian",
    "sign 24 12 Полки до свода. Одна лампа. Пыли нет.",
    "note 36 6 read",
    "note 12 8 unsealed",
    "item 24 6 torn_page 2",
    "item 42 5 torn_page 1",
    "chest 43 11 460 - order_draught:2 torn_page:2 strong_tea:1",
    "spawn 17 7 page_swarm 3 5",
    "spawn 35 8 page_swarm 3 5",
    "spawn 24 11 acolyte 2 4",
    "exit 24 15 gatehouse 9 8",
    "exit 4 3 drafting 24 15",
]

# --- Чертёжный зал ---
drafting = hall("drafting", "Чертёжный зал")
drafting.rect(6, 4, 42, 12, ".")
drafting.rect(16, 6, 32, 9, "#")                 # стол во всю комнату
drafting.rect(23, 6, 25, 6, ".")
drafting.objects = [
    "npc 24 5 draftsman",
    "sign 24 11 На столе лист во весь стол, и лист порван.",
    "note 8 5 charts",
    "item 10 11 chart_piece 1",
    "item 40 5 chart_piece 1",
    "chest 40 11 500 - chart_piece:1 order_draught:2",
    "spawn 12 8 draft_shade 3 5",
    "spawn 36 8 draft_shade 3 5",
    "exit 24 15 library 6 4",
    "exit 42 4 refusalhall 24 15",
]

# --- Кельи послушников ---
cells = hall("cells", "Кельи послушников")
cells.rect(4, 3, 44, 12, ".")
for x in (6, 12, 18, 30, 36):                    # ряды келий
    cells.rect(x, 4, x + 4, 7, "#")
    cells.rect(x + 1, 5, x + 3, 6, ".")
    cells.rect(x + 2, 7, x + 2, 7, ".")
cells.objects = [
    "sign 24 11 Двери не заперты. В каждой келье прибрано.",
    "note 7 5 novice",
    "note 13 5 keepsake",
    "item 19 5 keepsake 1",
    "item 31 5 order_draught 1",
    "chest 37 5 400 - acolyte_hood:1 order_draught:1",
    "spawn 24 9 cell_dweller 3 5",
    "spawn 40 10 cell_dweller 2 4",
    "exit 24 15 gatehouse 39 8",
    "exit 4 12 furnace 24 15",
]

# --- Печь Ордена ---
furnace = hall("furnace", "Печь Ордена")
furnace.rect(8, 5, 40, 12, ".")
furnace.rect(20, 6, 28, 10, "#")                 # сама печь
furnace.rect(24, 6, 24, 6, ".")                  # устье
furnace.objects = [
    "npc 18 8 stoker",
    "sign 24 11 Печь тёплая, хотя её не топят двести лет.",
    "note 38 6 ovens",
    "item 10 11 furnace_ash 2",
    "chest 38 11 480 - furnace_ash:3 ember:2 order_draught:1",
    "spawn 12 7 furnace_born 2 4",
    "spawn 34 9 furnace_born 2 4",
    "exit 24 15 cells 5 12",
]

# --- Зал Отказа ---
refusalhall = hall("refusalhall", "Зал Отказа")
refusalhall.rect(6, 4, 42, 12, ".")
refusalhall.rect(14, 7, 34, 9, "#")              # длинный стол совета
refusalhall.rect(24, 7, 24, 7, ".")              # место, где лежало кольцо
refusalhall.objects = [
    "npc 24 6 recorder",
    "sign 24 11 Одиннадцать мест по одну сторону, одно по другую. Оно пустое.",
    "note 8 5 refusal",
    "item 40 5 order_draught 1",
    "chest 40 11 520 - order_draught:2 rune_stone:1 strong_tea:1",
    "spawn 10 10 refusal_echo 2 4",
    "spawn 38 8 refusal_echo 2 4",
    "exit 24 15 drafting 41 4",
    "exit 6 4 node2 24 15",
]

# --- Узел Второй ---
node2 = hall("node2", "Узел Второй")
node2.rect(10, 4, 38, 12, ".")
node2.rect(20, 6, 28, 10, "#")                   # кольцо узла
node2.rect(24, 6, 24, 10, ".")
node2.objects = [
    "sign 24 11 Здесь что-то тянет в другую сторону. Ровно с той же силой.",
    "note 12 5 secondnode",
    "item 36 5 node_core 1",
    "chest 36 11 560 - node_shield:1 order_draught:2",
    "spawn 14 8 node_guard 2 3",
    "spawn 34 8 node_guard 2 3",
    "spawn 24 8 node_heart 1 2",
    "exit 24 15 refusalhall 7 4",
    "exit 38 4 node3 24 15",
]

# --- Узел Третий ---
node3 = hall("node3", "Узел Третий")
node3.rect(8, 4, 40, 12, ".")
node3.rect(18, 5, 30, 11, "#")                   # обломки кольца
node3.rect(22, 5, 26, 11, ".")
node3.rect(18, 8, 30, 8, ".")
node3.objects = [
    "sign 24 12 Кольцо узла разомкнуто. Через разрыв тянет сквозняком.",
    "note 10 5 thirdnode",
    "item 38 5 node_core 1",
    "chest 10 11 540 - portal_stone:1 order_draught:2",
    "spawn 12 8 node_guard 2 4",
    "spawn 36 9 node_guard 2 4",
    "exit 24 15 node2 37 4",
    "exit 8 12 grave 24 15",
    "exit 24 4 meadow 24 14 quest=node3q:100 "
    "deny=В разрыв тянет сквозняком. Шагнуть туда, не зная куда, — значит уйти совсем.",
]

# --- Могила Первого Мастера ---
grave = hall("grave", "Могила Первого Мастера")
grave.rect(10, 4, 38, 12, ".")
grave.rect(22, 6, 26, 9, "#")                    # камень
grave.rect(24, 9, 24, 9, ".")
grave.objects = [
    "sign 24 11 Камень стоит, могила пустая. Надпись дописана углём.",
    "note 24 10 emptygrave",
    "item 12 5 rune_stone 1",
    "chest 36 11 640 - order_draught:3 rune_stone:2 node_core:1",
    "spawn 24 5 master_shadow 1 2",
    "exit 24 15 node3 9 12",
]

# =============================================================== РЕГИОН V: ДРЕЙФ
# Лоскуты, которые не пристали. Земля обычная, небо — нет.

# --- Дрейфующий луг: первый лоскут без опоры ---
meadow = Map("meadow", "Дрейфующий луг", ",")
meadow.rect(4, 2, 44, 15, ",")
for x, y, n in ((8, 4, 3), (33, 5, 4), (14, 12, 5), (37, 12, 3)):
    meadow.rect(x, y, x + n - 1, y, "T")
meadow.rect(20, 7, 28, 9, "=")                   # остаток дороги, обрывается
meadow.objects = [
    "sign 24 10 Облака идут в одну сторону, тени от них — в другую.",
    "note 6 8 drift",
    "item 42 3 drift_grass 2",
    "chest 42 14 520 - drift_grass:2 still_water:1 bread:2",
    "spawn 12 6 drift_hare 3 5",
    "spawn 36 9 drift_hare 2 4",
    "spawn 24 13 cart_shade 2 4",
    "exit 24 15 node3 24 5",
    "exit 4 8 farhouse 44 9",
    "exit 44 6 upstair 4 14",
]

# --- Дом на отшибе: живут и не знают ---
farhouse = Map("farhouse", "Дом на отшибе", ",")
farhouse.rect(2, 3, 45, 15, ",")
farhouse.rect(16, 5, 30, 11, "#")                # сам дом
farhouse.rect(18, 6, 28, 10, ".")
farhouse.rect(23, 11, 24, 11, ".")               # порог
for x in (8, 12, 36, 40):                        # подводы во дворе
    farhouse.rect(x, 13, x + 2, 13, "#")
farhouse.objects = [
    "npc 23 8 driftwife",
    "sign 23 12 Двенадцать подвод гружены. Кони распряжены и стоят смирно.",
    "note 20 7 tomorrow",
    "item 34 4 drift_grass 1",
    "chest 27 7 480 - still_water:2 bread:3 salt_lump:2",
    "spawn 6 8 cart_shade 2 4",
    "spawn 42 8 cart_shade 2 4",
    "exit 45 9 meadow 5 8",
    "exit 2 14 well 44 5",
]

# --- Колодец Двух Вёдер ---
well = Map("well", "Колодец Двух Вёдер", ",")
well.rect(3, 3, 45, 15, ",")
well.rect(22, 7, 26, 10, "#")                    # сруб колодца
well.rect(24, 10, 24, 10, ".")                   # к вороту подходят снизу
for x, y, n in ((7, 5, 3), (38, 12, 4)):
    well.rect(x, y, x + n - 1, y, "T")
well.objects = [
    "sign 24 11 Ворот один, вёдер два. Второе холоднее.",
    "note 20 6 wellrule",
    "item 42 4 still_water 1",
    "chest 6 13 560 - still_water:2 drift_grass:2 antidote:2",
    "spawn 12 9 second_bucket 1 2",
    "spawn 36 7 drift_hare 2 4",
    "exit 45 5 farhouse 3 14",
    "exit 3 8 grove 44 9",
]

# --- Роща, где не темнеет ---
grove = Map("grove", "Роща, где не темнеет", ",")
grove.rect(2, 3, 45, 15, ",")
for y in range(4, 15, 3):
    for x in range(6, 43, 6):
        grove.rect(x, y, x + 1, y, "T")
grove.rect(20, 8, 28, 10, ",")                   # прогалина
grove.objects = [
    "npc 24 9 grovekeeper",
    "sign 24 11 Свет ровный и не меняется. Тени стоят на месте.",
    "note 22 5 marks",
    "item 40 5 grove_leaf 2",
    "chest 8 13 540 - grove_leaf:2 still_water:1 elixir_haste:1",
    "spawn 14 12 grove_sleeper 2 4",
    "spawn 34 12 grove_sleeper 2 4",
    "exit 45 9 well 4 8",
    "exit 2 6 battlefield 44 12",
]

# --- Поле после битвы: час идёт по кругу ---
battlefield = Map("battlefield", "Поле после битвы", ",")
battlefield.rect(2, 2, 45, 15, ",")
battlefield.rect(10, 8, 38, 9, "=")              # вытоптанная полоса строя
for x in (14, 22, 30):                           # брошенные щиты и телеги
    battlefield.rect(x, 5, x + 1, 5, "#")
    battlefield.rect(x, 12, x + 1, 12, "#")
battlefield.objects = [
    "npc 10 10 soldier",
    "sign 24 10 Строй истоптан до земли. Земля свежая.",
    "note 6 4 lastorder",
    "item 18 13 scrap_iron 2",
    "chest 6 13 600 - scrap_iron:3 still_water:2 elixir_might:1",
    "spawn 20 6 last_hour 3 5",
    "spawn 34 11 last_hour 2 4",
    "spawn 40 8 bannerman 1 2",
    "exit 45 12 grove 3 6",
    "exit 2 8 otherhalf 44 13",
]

# --- Вторая Половина Города: срез сходится с Половиной Города ---
otherhalf = Map("otherhalf", "Вторая Половина Города", "#")
otherhalf.rect(14, 2, 45, 15, ".")               # уцелевшая половина — восточная
otherhalf.rect(14, 8, 45, 9, "=")                # Мучная улица упирается в срез
for x in (20, 28, 36):
    otherhalf.rect(x, 4, x + 4, 6, "#")
    otherhalf.rect(x, 11, x + 4, 13, "#")
    otherhalf.rect(x + 2, 6, x + 2, 6, ".")
    otherhalf.rect(x + 2, 11, x + 2, 11, ".")
otherhalf.rect(14, 12, 14, 13, ".")
otherhalf.objects = [
    "npc 15 8 halfscribe",
    "sign 16 10 Улица кончается ровно здесь. Дальше четыре шага и всё небо.",
    "note 17 5 otherside",
    "item 43 12 still_water 1",
    "chest 43 4 620 - whetstone:1 still_water:2 clerk_robe:1",
    "spawn 26 3 other_rat 3 5",
    "spawn 34 14 other_rat 2 4",
    "exit 44 13 battlefield 3 8",
    "exit 14 13 emptyalder 24 15",
    "exit 44 3 homepath 4 9",
]

# --- Лестница вверх: поднимаешься и выходишь внизу ---
upstair = Map("upstair", "Лестница вверх", "#")
upstair.rect(4, 13, 12, 15, ".")                 # низ лестницы
upstair.rect(6, 3, 42, 12, ".")
for y in range(4, 12, 2):                        # марши со сменой направления
    upstair.rect(6, y, 42, y, "#")
    if (y // 2) % 2:
        upstair.rect(39, y, 42, y, ".")
    else:
        upstair.rect(6, y, 9, y, ".")
upstair.rect(40, 2, 44, 4, ".")                  # верхняя площадка
upstair.objects = [
    "sign 8 14 Ступеней двести двенадцать. Считаны трижды.",
    "note 10 13 stair",
    "item 42 3 drift_grass 1",
    "chest 42 2 580 - stair_hood:1 still_water:1 grove_leaf:1",
    "spawn 20 3 stair_walker 2 4",
    "spawn 30 11 stair_walker 2 4",
    "exit 4 14 meadow 43 6",
    "exit 44 4 upstair 6 12",
    "exit 6 3 edge 24 15",
]

# --- Край Лоскута: дальше земли нет ---
edge = Map("edge", "Край Лоскута", ",")
edge.rect(6, 6, 42, 15, ",")
edge.rect(6, 6, 42, 6, "=")                      # кромка, дальше пусто
for x, y, n in ((10, 10, 3), (34, 12, 4)):
    edge.rect(x, y, x + n - 1, y, "T")
edge.objects = [
    "sign 24 7 Это не обрыв: обрыв предполагает низ.",
    "note 24 8 edgeview",
    "item 40 9 drift_grass 2",
    "chest 8 14 700 - edge_ring:1 still_water:3 portal_stone:1",
    "spawn 16 9 edge_wind 1 2",
    "exit 24 15 upstair 7 3",
    "exit 24 6 firstseam 24 14 quest=inside:1 "
    "deny=Дальше кромки земли нет. Шагнуть туда просто так — не смелость, а глупость.",
]

# --- Пустая Ольховка: копия деревни, дом в дом ---
emptyalder = Map("emptyalder", "Пустая Ольховка", ",")
for y in (2, 3, 4):
    emptyalder.rect(3, y, 9, y, "#").rect(35, y, 41, y, "#")
emptyalder.rect(1, 7, W - 2, 8, "=")
emptyalder.rect(17, 10, 21, 12, "~")
for y in (11, 12, 13):
    emptyalder.rect(3, y, 9, y, "#").rect(35, y, 41, y, "#")
emptyalder.rect(7, 15, 9, 15, "T").rect(31, 15, 33, 15, "T")
emptyalder.rect(26, 12, 30, 14, "#")             # дом последний, лишний
emptyalder.rect(27, 13, 29, 13, ".")             # горница
emptyalder.rect(28, 14, 28, 14, ".")             # порог
emptyalder.objects = [
    "sign 12 6 Ольховка. Дом в дом, ставня в ставню. Ни одного человека.",
    "sign 27 15 Дом стоит там, где в настоящей Ольховке пустырь.",
    "note 24 6 houses",
    "item 28 13 own_key 1",
    "chest 11 9 660 - still_water:2 whetstone:1 elixir_guard:1",
    "spawn 14 3 own_copy 1 2",
    "spawn 34 13 own_copy 1 2",
    "exit 24 15 otherhalf 15 13",
]

# --- Тропа Возвращения: единственная надёжная дорога домой ---
homepath = Map("homepath", "Тропа Возвращения", ",")
homepath.rect(2, 8, 45, 10, "=")
homepath.rect(2, 6, 45, 12, ",")
homepath.rect(2, 8, 45, 9, "=")
for x, y, n in ((10, 6, 4), (26, 12, 5), (38, 6, 3)):
    homepath.rect(x, y, x + n - 1, y, "T")
homepath.objects = [
    "npc 24 9 pathkeeper",
    "sign 24 11 Тропа не петляет. Трава примята в обе стороны.",
    "note 8 11 homeward",
    "item 44 11 drift_grass 1",
    "chest 6 7 600 - still_water:2 grove_leaf:2 portal_stone:1",
    "spawn 14 10 drift_hare 2 4",
    "spawn 36 10 drift_hare 2 4",
    "exit 4 9 otherhalf 43 3",
    "exit 45 9 sanctum 15 16",
]

# ============================================================ РЕГИОН VI: ИЗНАНКА
# Внутренность сети. Пола тут нет — есть то, по чему можно идти.


def inside(mid, name):
    """Заготовка изнанки: вход снизу по центру, всё остальное строится."""
    m = Map(mid, name, "#")
    m.rect(22, 13, 26, 16, ".")
    return m


# --- Первый Шов: два лоскута, сшитые словом ---
firstseam = inside("firstseam", "Первый Шов")
firstseam.rect(4, 3, 44, 7, ".")                 # северный лоскут
firstseam.rect(4, 10, 44, 14, ".")               # южный лоскут
firstseam.rect(22, 7, 26, 10, ".")               # сам стык, узкий
firstseam.objects = [
    "npc 24 9 seamwatch",
    "sign 24 12 Два края сходятся и не срастаются. Между ними ничего, и по этому ничему ходят.",
    "note 8 5 oldseam",
    "item 40 4 cinch_draught 1",
    "chest 40 13 720 - line_dust:2 cinch_draught:2 rune_stone:1",
    "spawn 12 5 seam_moth 3 5",
    "spawn 36 12 seam_moth 2 4",
    "exit 24 15 edge 24 14",
    "exit 4 5 gallery 44 9",
]

# --- Галерея Линий: линии видны, если смотреть мимо ---
gallery = inside("gallery", "Галерея Линий")
gallery.rect(3, 3, 45, 14, ".")
for y in range(4, 14, 3):                        # сами линии — тонкие стены
    gallery.rect(6, y, 42, y, "#")
    gallery.rect(20, y, 22, y, ".")
    gallery.rect(32, y, 34, y, ".")
gallery.objects = [
    "npc 21 8 surveyor",
    "sign 24 14 Смотри мимо — увидишь. Смотри прямо — стена.",
    "note 5 6 readlines",
    "item 44 5 line_thread 1",
    "chest 44 12 740 - line_thread:2 line_dust:2 cinch_draught:1",
    "spawn 33 5 line_walker 3 5",
    "spawn 10 11 line_walker 2 4",
    "exit 44 9 firstseam 5 5",
    "exit 3 3 measures 24 15",
    "exit 3 14 meeting 24 15",
]

# --- Комната Измерений: расстояние лежит на полке ---
measures = inside("measures", "Комната Измерений")
measures.rect(8, 4, 40, 12, ".")
measures.rect(14, 6, 34, 6, "#")                 # полки
measures.rect(14, 10, 34, 10, "#")
measures.rect(24, 6, 24, 6, ".")
measures.rect(24, 10, 24, 10, ".")
measures.objects = [
    "sign 24 12 Полки пусты, и всё-таки на них что-то лежит.",
    "note 10 5 roomrule",
    "item 24 8 measure 1",
    "item 38 5 line_dust 2",
    "chest 38 11 760 - cinch_draught:2 rune_stone:2 elixir_might:1",
    "spawn 12 8 measure_thing 2 3",
    "spawn 36 8 measure_thing 1 3",
    "exit 24 15 gallery 4 4",
]

# --- Встреча: тот, кто помнит дорогу ---
meeting = inside("meeting", "Встреча")
meeting.rect(6, 5, 42, 12, ".")
meeting.rect(20, 7, 28, 10, "#")                 # опора, о которую он опирался
meeting.rect(24, 7, 24, 10, ".")
meeting.objects = [
    "npc 22 4 master",
    "sign 24 12 Пол вытерт до блеска ровно на два шага. Кто-то тут ходил взад-вперёд очень долго.",
    "note 8 6 walkers",
    "item 40 6 cinch_draught 1",
    "chest 40 11 780 - walk_staff:1 line_dust:3 cinch_draught:1",
    "exit 24 15 gallery 4 13",
    "exit 24 4 node1 24 15 quest=remembers:100 "
    "deny=Старик стоит на дороге и не двигается. Пока не выслушаешь — не пройдёшь.",
]
meeting.rect(22, 3, 26, 4, ".")

# --- Узел Первый: начало всего ---
node1 = inside("node1", "Узел Первый")
node1.rect(6, 4, 42, 12, ".")
node1.rect(16, 6, 32, 10, "#")                   # тело узла
node1.rect(20, 6, 20, 10, ".")
node1.rect(28, 6, 28, 10, ".")
node1.rect(16, 8, 32, 8, ".")
node1.objects = [
    "sign 24 12 С него всё началось. Он об этом не знает.",
    "note 8 5 firstnode",
    "item 40 5 line_thread 1",
    "chest 40 11 800 - first_ring:1 rune_stone:2 cinch_draught:2",
    "spawn 10 10 first_guard 2 3",
    "spawn 38 8 first_guard 2 3",
    "exit 24 15 meeting 24 5",
    "exit 6 8 cinchheart 44 9",
]

# --- Сердце Стяжения: ход, который тянет ---
cinchheart = inside("cinchheart", "Сердце Стяжения")
cinchheart.rect(4, 4, 44, 13, ".")
cinchheart.rect(18, 6, 30, 11, "#")              # сам ход
cinchheart.rect(24, 6, 24, 11, ".")
cinchheart.rect(18, 8, 30, 9, ".")
cinchheart.objects = [
    "sign 24 13 Ничего не крутится и не тикает. И всё-таки тянет.",
    "note 7 5 cinchwork",
    "item 41 5 heart_cog 1",
    "chest 7 12 850 - inside_plate:1 cinch_draught:3 rune_stone:2",
    "spawn 12 10 cinch_engine 2 4",
    "spawn 36 10 cinch_engine 2 4",
    "spawn 24 9 cinch_heart 1 2",
    "exit 44 9 node1 7 8",
    "exit 4 9 zeropoint 24 15",
]

# --- Точка Ноль: всё сразу и ничего ---
zeropoint = inside("zeropoint", "Точка Ноль")
zeropoint.rect(10, 3, 38, 13, ".")
zeropoint.rect(23, 7, 25, 9, "#")                # середина, к которой не подойти
zeropoint.objects = [
    "sign 24 12 До всего отсюда ноль шагов. Поэтому здесь ничего и нет.",
    "note 12 4 allatonce",
    "item 36 4 line_dust 2",
    "chest 36 12 900 - zero_shield:1 cinch_draught:3 portal_stone:1",
    "spawn 14 9 zero_echo 2 4",
    "spawn 34 9 zero_echo 2 4",
    "spawn 24 4 all_at_once 1 2",
    "exit 24 15 cinchheart 5 9",
    "exit 10 8 finale 24 15 key=measure "
    "deny=Ход в развязку есть, и до него ноль шагов, и пройти его нечем. Нужна мера.",
]

# --- Развязка: камень, лист и кольцо ---
finale = inside("finale", "Развязка")
finale.rect(14, 5, 34, 12, ".")
finale.rect(23, 8, 25, 9, "#")                   # камень с листом
finale.objects = [
    "npc 20 6 master_end",
    "sign 24 11 Камень, лист, кольцо сверху. Три строки, и все три разборчивы.",
    "note 24 7 threeways",
    "chest 32 11 1000 - cinch_draught:3 rune_stone:3 portal_stone:2",
    "exit 24 15 zeropoint 11 8",
]

MAPS = [village, forest, cave, ruins, sanctum, vault, graveyard, swamp,
        goatpath, glassfield, mill, market, bridge, saltmines,
        caravanserai, doubled,
        halfcity, endless, foundry, canal, counter_tower, archive,
        ordergate, gatehouse, library, drafting, cells, furnace,
        refusalhall, node2, node3, grave,
        meadow, farhouse, well, grove, battlefield, otherhalf,
        upstair, edge, emptyalder, homepath,
        firstseam, gallery, measures, meeting, node1, cinchheart,
        zeropoint, finale]

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
               "graveyard": (45, 9), "swamp": (44, 9),
               "goatpath": (24, 15), "glassfield": (3, 9), "mill": (24, 15),
               "market": (3, 9), "bridge": (24, 3), "saltmines": (24, 3),
               "caravanserai": (24, 15), "doubled": (3, 9),
               "halfcity": (24, 2), "endless": (3, 9), "foundry": (24, 3),
               "canal": (24, 2), "counter": (24, 15), "archive": (24, 15),
               "ordergate": (24, 15), "gatehouse": (24, 15), "library": (24, 15),
               "drafting": (24, 15), "cells": (24, 15), "furnace": (24, 15),
               "refusalhall": (24, 15), "node2": (24, 15), "node3": (24, 15),
               "grave": (24, 15),
               "meadow": (24, 15), "farhouse": (45, 9), "well": (45, 5),
               "grove": (45, 9), "battlefield": (45, 12), "otherhalf": (44, 13),
               "upstair": (4, 14), "edge": (24, 15), "emptyalder": (24, 15),
               "homepath": (45, 9),
               "firstseam": (24, 15), "gallery": (44, 9), "measures": (24, 15),
               "meeting": (24, 15), "node1": (24, 15), "cinchheart": (44, 9),
               "zeropoint": (24, 15), "finale": (24, 15)}

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
