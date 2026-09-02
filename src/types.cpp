#include "types.h"

#include <sstream>

char tile_glyph(Tile t) {
    switch (t) {
        case Tile::Floor: return '.';
        case Tile::Wall:  return '#';
        case Tile::Water: return '~';
        case Tile::Tree:  return 'T';
        case Tile::Grass: return ',';
        case Tile::Road:  return '=';
        default:          return '?';
    }
}

bool tile_walkable(Tile t) {
    return t == Tile::Floor || t == Tile::Grass || t == Tile::Road;
}

Tile tile_from_char(char c) {
    switch (c) {
        case '.': return Tile::Floor;
        case '#': return Tile::Wall;
        case '~': return Tile::Water;
        case 'T': return Tile::Tree;
        case ',': return Tile::Grass;
        case '=': return Tile::Road;
        default:  return Tile::Wall;   // неизвестный символ безопаснее считать стеной
    }
}

Slot slot_for(ItemKind k) {
    switch (k) {
        case ItemKind::Weapon: return Slot::Weapon;
        case ItemKind::Armor:  return Slot::Armor;
        case ItemKind::Helmet: return Slot::Helmet;
        case ItemKind::Shield: return Slot::Shield;
        case ItemKind::Ring:   return Slot::Ring;
        default:               return Slot::Count;
    }
}

const char* slot_name(Slot s) {
    switch (s) {
        case Slot::Weapon: return "Оружие";
        case Slot::Armor:  return "Броня";
        case Slot::Helmet: return "Шлем";
        case Slot::Shield: return "Щит";
        case Slot::Ring:   return "Кольцо";
        default:           return "—";
    }
}

const char* kind_name(ItemKind k) {
    switch (k) {
        case ItemKind::Weapon:     return "оружие";
        case ItemKind::Armor:      return "броня";
        case ItemKind::Helmet:     return "шлем";
        case ItemKind::Shield:     return "щит";
        case ItemKind::Ring:       return "кольцо";
        case ItemKind::Consumable: return "расходник";
        default:                   return "разное";
    }
}

const char* stance_name(Stance s) {
    switch (s) {
        case Stance::Cautious: return "Осторожная";
        case Stance::Fierce:   return "Яростная";
        default:               return "Ровная";
    }
}

const char* stance_hint(Stance s) {
    switch (s) {
        case Stance::Cautious: return "+15 блок, +2 броня, -20 меткость, урон 70%";
        case Stance::Fierce:   return "+15 меткость, урон 140%, -15 блок, -2 броня";
        default:               return "без модификаторов";
    }
}

Stats stance_bonus(Stance s) {
    Stats st;
    if (s == Stance::Cautious) { st.block = 15; st.armor = 2;  st.attack = -20; }
    if (s == Stance::Fierce)   { st.block = -15; st.armor = -2; st.attack = 15; }
    return st;
}

int stance_damage_pct(Stance s) {
    switch (s) {
        case Stance::Cautious: return 70;
        case Stance::Fierce:   return 140;
        default:               return 100;
    }
}

std::size_t utf8_len(const std::string& s) {
    std::size_t n = 0;
    for (unsigned char c : s)
        if ((c & 0xC0) != 0x80) ++n;   // не продолжение многобайтовой последовательности
    return n;
}

std::string trunc(const std::string& s, std::size_t width) {
    std::size_t seen = 0, i = 0;
    for (; i < s.size(); ++i) {
        if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) {
            if (seen == width) break;
            ++seen;
        }
    }
    return s.substr(0, i);
}

std::string pad(const std::string& s, std::size_t width) {
    std::string r = trunc(s, width);
    std::size_t n = utf8_len(r);
    if (n < width) r.append(width - n, ' ');
    return r;
}

std::vector<std::string> split_ws(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

std::string to_str(int v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

std::string plural(int n, const std::string& one, const std::string& few,
                   const std::string& many) {
    int a = n % 100;
    if (a >= 11 && a <= 14) return many;
    switch (n % 10) {
        case 1:  return one;
        case 2: case 3: case 4: return few;
        default: return many;
    }
}

int to_int(const std::string& s, int fallback) {
    if (s.empty()) return fallback;
    std::istringstream iss(s);
    int v = 0;
    if (!(iss >> v)) return fallback;
    return v;
}
