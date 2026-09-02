#pragma once
#include <string>
#include <vector>

// ---------- геометрия ----------

struct Vec2 {
    int x = 0;
    int y = 0;
    Vec2() = default;
    // Конструктор нужен ради C++11: поля с инициализаторами лишают структуру
    // статуса агрегата, и запись Vec2{1, 2} без него не собирается.
    Vec2(int nx, int ny) : x(nx), y(ny) {}
};
inline bool operator==(const Vec2& a, const Vec2& b) { return a.x == b.x && a.y == b.y; }

// Манхэттенское расстояние: мир четырёхсвязный, диагоналей нет.
inline int dist(const Vec2& a, const Vec2& b) {
    int dx = a.x - b.x, dy = a.y - b.y;
    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
}

// ---------- тайлы ----------
// Сетка хранит ТОЛЬКО геометрию (проходимость). Всё остальное — объекты
// со своими координатами, см. world.h.

enum class Tile : unsigned char {
    Floor = 0,   // '.'  проходимо
    Wall,        // '#'  стена
    Water,       // '~'  непроходимо
    Tree,        // 'T'  непроходимо
    Grass,       // ','  проходимо
    Road,        // '='  проходимо
    Count
};

char tile_glyph(Tile t);
bool tile_walkable(Tile t);
Tile tile_from_char(char c);

// ---------- характеристики ----------
// Единый аддитивный контейнер: база игрока, бонусы снаряжения, бонусы навыков
// и модификатор стойки складываются одним и тем же оператором.

struct Stats {
    int max_hp   = 0;
    int max_ap   = 0;
    int attack   = 0;   // шанс попадания, %
    int dmg_min  = 0;
    int dmg_max  = 0;
    int block    = 0;   // шанс блока, %
    int armor    = 0;   // поглощение урона
    int crit     = 0;   // шанс критического удара, %
    int ap_atk   = 0;   // стоимость атаки в AP

    Stats& operator+=(const Stats& o) {
        max_hp  += o.max_hp;  max_ap  += o.max_ap;
        attack  += o.attack;  dmg_min += o.dmg_min; dmg_max += o.dmg_max;
        block   += o.block;   armor   += o.armor;   crit    += o.crit;
        ap_atk  += o.ap_atk;
        return *this;
    }
};
inline Stats operator+(Stats a, const Stats& b) { a += b; return a; }

// ---------- предметы ----------

enum class ItemKind { Misc, Weapon, Armor, Helmet, Shield, Ring, Consumable };

enum class Slot { Weapon = 0, Armor, Helmet, Shield, Ring, Count };

// Слот, в который надевается предмет; Slot::Count — предмет не надевается.
Slot slot_for(ItemKind k);
const char* slot_name(Slot s);
const char* kind_name(ItemKind k);

struct ItemDef {
    std::string id;
    std::string name;
    ItemKind    kind  = ItemKind::Misc;
    int         price = 0;      // цена покупки у торговца
    Stats       bonus;          // для снаряжения
    int         heal_hp = 0;    // для расходников
    int         heal_ap = 0;
    std::string desc;
};

// Стопка предметов в инвентаре.
struct ItemStack {
    std::string id;
    int         count = 0;
    ItemStack() = default;
    ItemStack(const std::string& i, int c) : id(i), count(c) {}
};

// ---------- стойки ----------
// Своя механика: стойку можно менять в бою бесплатно один раз за ход.

enum class Stance { Cautious = 0, Balanced, Fierce, Count };

const char* stance_name(Stance s);
const char* stance_hint(Stance s);
Stats       stance_bonus(Stance s);
// Множитель урона в процентах (100 — без изменений).
int         stance_damage_pct(Stance s);

// ---------- текстовые утилиты ----------

// Число ВИДИМЫХ символов в UTF-8 строке: std::string::size() считает байты,
// а кириллица занимает по два, из-за чего таблицы разъезжаются.
std::size_t utf8_len(const std::string& s);

// Дополняет строку пробелами до ширины width по видимым символам.
std::string pad(const std::string& s, std::size_t width);

// Обрезает строку до width видимых символов.
std::string trunc(const std::string& s, std::size_t width);

std::vector<std::string> split_ws(const std::string& s);

// Переносит текст по словам на ширину width (в видимых символах), сохраняя
// уже имеющиеся переводы строк. Без этого реплики NPC уезжали бы за край
// узкого экрана.
std::vector<std::string> wrap(const std::string& text, std::size_t width);
std::string to_str(int v);

// Русское склонение при числе: plural(1,"монета","монеты","монет") -> "монета".
std::string plural(int n, const std::string& one, const std::string& few,
                   const std::string& many);
int         to_int(const std::string& s, int fallback = 0);
