#pragma once
#include "types.h"

#include <map>
#include <string>
#include <vector>

// Объекты локации. Сетка тайлов отвечает только за проходимость, всё
// остальное лежит здесь списками с координатами — у объекта может быть
// сколько угодно своих полей, чего один символ в сетке не выдерживал бы.

struct MapNpc  { Vec2 pos; std::string npc_id; };
struct MapItem { Vec2 pos; std::string item_id; int count = 1; };
struct MapExit { Vec2 pos; std::string target; Vec2 dest; };
struct MapSign { Vec2 pos; std::string text; };

struct SpawnZone {
    Vec2        pos;
    int         radius    = 3;
    std::string enemy_id;
    int         max_count = 1;
};

struct Location {
    std::string            id;
    std::string            name;
    int                    w = 0;
    int                    h = 0;
    std::vector<Tile>      tiles;
    std::vector<MapNpc>    npcs;
    std::vector<MapItem>   items;
    std::vector<MapExit>   exits;
    std::vector<MapSign>   signs;
    std::vector<Vec2>      beds;
    std::vector<SpawnZone> zones;

    bool in_bounds(Vec2 p) const { return p.x >= 0 && p.y >= 0 && p.x < w && p.y < h; }
    Tile at(Vec2 p) const {
        if (!in_bounds(p)) return Tile::Wall;
        return tiles[static_cast<std::size_t>(p.y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(p.x)];
    }
    bool walkable(Vec2 p) const { return in_bounds(p) && tile_walkable(at(p)); }

    const MapExit* exit_at(Vec2 p) const;
    const MapSign* sign_at(Vec2 p) const;
    bool           bed_at(Vec2 p) const;
    const MapNpc*  npc_at(Vec2 p) const;
    // Индекс предмета в списке items или -1. Индекс служит стабильным
    // идентификатором в сохранении.
    int            item_index_at(Vec2 p) const;
};

class World {
public:
    explicit World(std::string root = "data/maps") : root_(std::move(root)) {}

    // Локация грузится один раз и кешируется. nullptr — ошибка, текст в last_error().
    const Location* location(const std::string& id) const;

    const std::string& last_error() const { return err_; }

private:
    bool load(const std::string& id) const;

    mutable std::map<std::string, Location> cache_;
    mutable std::string                     err_;
    std::string                             root_;
};
