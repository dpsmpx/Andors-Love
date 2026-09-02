#include "world.h"

#include <fstream>
#include <sstream>

const MapExit* Location::exit_at(Vec2 p) const {
    for (const MapExit& e : exits)
        if (e.pos == p) return &e;
    return nullptr;
}

const MapSign* Location::sign_at(Vec2 p) const {
    for (const MapSign& s : signs)
        if (s.pos == p) return &s;
    return nullptr;
}

bool Location::bed_at(Vec2 p) const {
    for (const Vec2& b : beds)
        if (b == p) return true;
    return false;
}

const MapNpc* Location::npc_at(Vec2 p) const {
    for (const MapNpc& n : npcs)
        if (n.pos == p) return &n;
    return nullptr;
}

int Location::item_index_at(Vec2 p) const {
    for (std::size_t i = 0; i < items.size(); ++i)
        if (items[i].pos == p) return static_cast<int>(i);
    return -1;
}

const Location* World::location(const std::string& id) const {
    auto it = cache_.find(id);
    if (it != cache_.end()) return &it->second;
    if (!load(id)) return nullptr;
    return &cache_[id];
}

// Формат файла локации (см. README, раздел «Формат карты»):
//   name  <название>
//   size  <ширина> <высота>
//   grid
//   <ровно height строк ровно по width символов>
//   objects
//   npc|item|exit|spawn|bed|sign ...
//   end
bool World::load(const std::string& id) const {
    const std::string path = root_ + "/" + id + ".map";
    std::ifstream in(path);
    if (!in) {
        err_ = "не удалось открыть файл карты: " + path;
        return false;
    }

    Location loc;
    loc.id = id;

    std::string line;
    int line_no = 0;
    auto fail = [&](const std::string& what) {
        err_ = path + ":" + to_str(line_no) + ": " + what;
        return false;
    };

    while (std::getline(in, line)) {
        ++line_no;
        // Убираем возможный CR от файлов с windows-переводами строк.
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == ';') continue;

        std::istringstream ls(line);
        std::string key;
        ls >> key;

        if (key == "name") {
            std::getline(ls, loc.name);
            if (!loc.name.empty() && loc.name[0] == ' ') loc.name.erase(0, 1);

        } else if (key == "size") {
            if (!(ls >> loc.w >> loc.h) || loc.w <= 0 || loc.h <= 0)
                return fail("size ожидает два положительных числа");

        } else if (key == "grid") {
            if (loc.w == 0 || loc.h == 0) return fail("grid до объявления size");
            loc.tiles.reserve(static_cast<std::size_t>(loc.w) * static_cast<std::size_t>(loc.h));
            for (int y = 0; y < loc.h; ++y) {
                if (!std::getline(in, line)) return fail("сетка обрывается, не хватает строк");
                ++line_no;
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (static_cast<int>(line.size()) != loc.w)
                    return fail("строка сетки шириной " + to_str(static_cast<int>(line.size())) +
                                ", ожидалось " + to_str(loc.w));
                for (int x = 0; x < loc.w; ++x) loc.tiles.push_back(tile_from_char(line[static_cast<std::size_t>(x)]));
            }

        } else if (key == "objects") {
            continue;   // разделитель, читается как обычные строки ниже

        } else if (key == "end") {
            break;

        } else if (key == "npc") {
            MapNpc n;
            if (!(ls >> n.pos.x >> n.pos.y >> n.npc_id)) return fail("npc <x> <y> <id>");
            loc.npcs.push_back(n);

        } else if (key == "item") {
            MapItem m;
            if (!(ls >> m.pos.x >> m.pos.y >> m.item_id)) return fail("item <x> <y> <id> [кол-во]");
            if (!(ls >> m.count)) m.count = 1;
            loc.items.push_back(m);

        } else if (key == "exit") {
            MapExit e;
            if (!(ls >> e.pos.x >> e.pos.y >> e.target >> e.dest.x >> e.dest.y))
                return fail("exit <x> <y> <локация> <цель_x> <цель_y>");
            loc.exits.push_back(e);

        } else if (key == "spawn") {
            SpawnZone z;
            if (!(ls >> z.pos.x >> z.pos.y >> z.enemy_id >> z.max_count >> z.radius))
                return fail("spawn <x> <y> <враг> <макс> <радиус>");
            loc.zones.push_back(z);

        } else if (key == "bed") {
            Vec2 b;
            if (!(ls >> b.x >> b.y)) return fail("bed <x> <y>");
            loc.beds.push_back(b);

        } else if (key == "sign") {
            MapSign s;
            if (!(ls >> s.pos.x >> s.pos.y)) return fail("sign <x> <y> <текст>");
            std::getline(ls, s.text);
            if (!s.text.empty() && s.text[0] == ' ') s.text.erase(0, 1);
            loc.signs.push_back(s);

        } else {
            return fail("неизвестная директива '" + key + "'");
        }
    }

    if (loc.tiles.size() != static_cast<std::size_t>(loc.w) * static_cast<std::size_t>(loc.h))
        return fail("сетка не заполнена целиком");

    cache_[id] = loc;
    return true;
}
