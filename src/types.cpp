#include "types.h"

#include <algorithm>
#include <sstream>

char tile_glyph(Tile t) {
    switch (t) {
        case Tile::Floor: return '.';
        case Tile::Wall:  return '#';
        case Tile::Water: return '~';
        case Tile::Tree:  return 'T';
        case Tile::Grass: return ',';
        case Tile::Road:  return '=';
        case Tile::DeadWater: return ':';
        default:          return '?';
    }
}

bool tile_walkable(Tile t) {
    // Мёртвая вода стоит и держит вес — по ней ходят, как по мостовой.
    return t == Tile::Floor || t == Tile::Grass || t == Tile::Road ||
           t == Tile::DeadWater;
}

bool tile_transparent(Tile t) {
    return t != Tile::Wall && t != Tile::Tree;
}

Tile tile_from_char(char c) {
    switch (c) {
        case '.': return Tile::Floor;
        case '#': return Tile::Wall;
        case '~': return Tile::Water;
        case 'T': return Tile::Tree;
        case ',': return Tile::Grass;
        case '=': return Tile::Road;
        case ':': return Tile::DeadWater;
        default:  return Tile::Wall;   // неизвестный символ безопаснее считать стеной
    }
}

Slot slot_for(ItemKind k) {
    switch (k) {
        case ItemKind::Weapon: return Slot::Weapon;
        case ItemKind::Armor:  return Slot::Armor;
        case ItemKind::Helmet: return Slot::Helmet;
        case ItemKind::Shield: return Slot::Shield;
        case ItemKind::Light:  return Slot::Shield;
        case ItemKind::Ring:   return Slot::Ring;
        default:               return Slot::Count;
    }
}

const char* slot_name(Slot s) {
    switch (s) {
        case Slot::Weapon: return "Оружие";
        case Slot::Armor:  return "Броня";
        case Slot::Helmet: return "Шлем";
        case Slot::Shield: return "Левая рука";
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
        case ItemKind::Book:       return "книга";
        case ItemKind::Light:      return "светильник";
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

std::vector<std::string> wrap(const std::string& text, std::size_t width) {
    std::vector<std::string> out;
    if (width < 4) width = 4;

    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t nl = text.find('\n', start);
        std::string para = text.substr(start, nl == std::string::npos ? std::string::npos
                                                                     : nl - start);
        // Отступ в начале строки сохраняем у всех кусков абзаца.
        std::string indent;
        for (char c : para) {
            if (c == ' ') indent += ' ';
            else break;
        }
        if (indent.size() >= width) indent.clear();

        std::vector<std::string> words = split_ws(para);
        if (words.empty()) {
            out.push_back("");
        } else {
            std::string line = indent;
            bool empty_line = true;
            for (const std::string& w : words) {
                std::size_t add = utf8_len(w) + (empty_line ? 0 : 1);
                if (!empty_line && utf8_len(line) + add > width) {
                    out.push_back(line);
                    line = indent + w;
                } else {
                    if (!empty_line) line += ' ';
                    line += w;
                }
                empty_line = false;
                // Слово длиннее строки режем принудительно.
                while (utf8_len(line) > width) {
                    out.push_back(trunc(line, width));
                    std::string rest = line.substr(trunc(line, width).size());
                    line = indent + rest;
                    if (rest.empty()) break;
                }
            }
            if (!line.empty() || out.empty()) out.push_back(line);
        }

        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    return out;
}

std::string reflow(const std::vector<std::string>& lines) {
    std::vector<std::string> out;
    std::string para;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string& l = lines[i];
        const bool blank = l.find_first_not_of(" \t") == std::string::npos;
        const bool literal = !blank && (l[0] == ' ' || l[0] == '\t');
        // Тире в начале строки — это новая реплика. Приклеив её к предыдущей,
        // мы свели бы двух говорящих в одну строку.
        const bool speech = l.compare(0, 3, "\xE2\x80\x94") == 0;
        if (blank || literal) {
            if (!para.empty()) { out.push_back(para); para.clear(); }
            out.push_back(blank ? std::string() : l);
            continue;
        }
        if (speech && !para.empty()) { out.push_back(para); para.clear(); }
        if (!para.empty()) para += ' ';
        para += l;
    }
    if (!para.empty()) out.push_back(para);

    std::string s;
    for (std::size_t i = 0; i < out.size(); ++i) {
        if (i) s += '\n';
        s += out[i];
    }
    return s;
}

std::string reflow(const std::string& text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (true) {
        const std::size_t nl = text.find('\n', start);
        lines.push_back(text.substr(start,
                        nl == std::string::npos ? std::string::npos : nl - start));
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    return reflow(lines);
}

std::vector<std::string> log_tail(const std::vector<std::string>& lines,
                                  std::size_t width, int rows) {
    std::vector<std::string> out;
    if (rows <= 0) return out;

    // Идём с конца: разворачивать весь журнал ради нескольких последних
    // строк незачем, а обрывать надо именно сверху.
    for (std::size_t k = lines.size(); k-- > 0; ) {
        const std::vector<std::string> part = wrap(lines[k], width);
        for (std::size_t i = part.size(); i-- > 0; ) {
            out.push_back(part[i]);
            if (static_cast<int>(out.size()) >= rows) break;
        }
        if (static_cast<int>(out.size()) >= rows) break;
    }
    std::reverse(out.begin(), out.end());
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
