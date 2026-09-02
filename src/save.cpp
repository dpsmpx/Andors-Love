// Сохранение и загрузка. Формат — построчный текст: его можно прочитать
// глазами и починить руками, а парсер на <fstream>/getline не повторяет
// ошибок ручной работы с FILE* (потеря последней строки, fclose(NULL)).
#include "game.h"

#include <fstream>
#include <sstream>

namespace {

constexpr const char* MAGIC   = "andors-love-save";
constexpr int         VERSION = 2;

void write_stats(std::ostream& os, const char* key, const Stats& s) {
    os << key << ' ' << s.max_hp << ' ' << s.max_ap << ' ' << s.attack << ' '
       << s.dmg_min << ' ' << s.dmg_max << ' ' << s.block << ' ' << s.armor << ' '
       << s.crit << ' ' << s.ap_atk << '\n';
}

bool read_stats(std::istream& is, Stats& s) {
    return static_cast<bool>(is >> s.max_hp >> s.max_ap >> s.attack >> s.dmg_min >>
                                   s.dmg_max >> s.block >> s.armor >> s.crit >> s.ap_atk);
}

} // namespace

bool Game::save_to(const std::string& path) const {
    std::ofstream out(path);
    if (!out) {
        err_ = "не удалось открыть файл для записи: " + path;
        return false;
    }

    out << MAGIC << ' ' << VERSION << '\n';
    out << "name " << plr_.name << '\n';
    out << "level " << plr_.level << '\n';
    out << "exp " << plr_.exp << '\n';
    out << "gold " << plr_.gold << '\n';
    out << "points " << plr_.skill_points << '\n';
    write_stats(out, "base", plr_.base);
    out << "hp " << plr_.hp << '\n';
    out << "ap " << plr_.ap << '\n';
    out << "loc " << plr_.loc << '\n';
    out << "pos " << plr_.pos.x << ' ' << plr_.pos.y << '\n';
    out << "race " << plr_.race << '\n';
    out << "spec " << plr_.spec << '\n';
    out << "stance " << static_cast<int>(plr_.stance) << '\n';
    out << "portalmaster " << (plr_.portal_master ? 1 : 0) << '\n';
    out << "momentum " << plr_.momentum << '\n';
    out << "turn " << turn_ << '\n';
    out << "respawn " << respawn_left_ << '\n';
    out << "nextuid " << next_uid_ << '\n';
    out << "seed " << rng_.state() << '\n';

    for (std::size_t i = 0; i < plr_.equipped.size(); ++i)
        if (!plr_.equipped[i].empty())
            out << "equip " << i << ' ' << plr_.equipped[i] << '\n';

    for (const ItemStack& s : plr_.inv)
        out << "inv " << s.id << ' ' << s.count << '\n';
    for (const auto& kv : plr_.skills)
        out << "skill " << kv.first << ' ' << kv.second << '\n';
    for (const auto& kv : plr_.quests)
        out << "quest " << kv.first << ' ' << kv.second << '\n';
    for (const auto& kv : plr_.counters)
        out << "counter " << kv.first << ' ' << kv.second << '\n';
    for (const ActiveEffect& a : plr_.effects)
        out << "effect " << a.id << ' ' << a.turns << ' ' << a.power << '\n';
    for (const auto& kv : plr_.enchants)
        out << "enchant " << kv.first << ' ' << kv.second << '\n';
    for (const Portal& pt : plr_.portals)
        out << "portal " << pt.loc << ' ' << pt.pos.x << ' ' << pt.pos.y << '\n';
    for (const std::string& t : taken_)
        out << "taken " << t << '\n';
    for (const std::string& ch : chests_)
        out << "chest " << ch << '\n';
    for (const std::string& v : visited_)
        out << "visited " << v << '\n';
    for (const Mob& m : mobs_) {
        out << "mob " << m.uid << ' ' << m.enemy_id << ' ' << m.loc << ' '
            << m.pos.x << ' ' << m.pos.y << ' ' << m.hp << ' ' << m.zone << ' '
            << m.gold << ' ' << static_cast<int>(m.state) << '\n';
        for (const ItemStack& st : m.inv)
            out << "mobinv " << m.uid << ' ' << st.id << ' ' << st.count << '\n';
        for (const ActiveEffect& a : m.effects)
            out << "mobfx " << m.uid << ' ' << a.id << ' ' << a.turns << ' ' << a.power << '\n';
    }

    out << "end\n";
    if (!out) {
        err_ = "ошибка записи в " + path;
        return false;
    }
    return true;
}

bool Game::load_from(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        err_ = "сохранение не найдено: " + path;
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) { err_ = "пустой файл сохранения"; return false; }
    {
        std::istringstream hs(line);
        std::string magic;
        int ver = 0;
        hs >> magic >> ver;
        if (magic != MAGIC) { err_ = "это не файл сохранения игры"; return false; }
        if (ver != VERSION) {
            err_ = "версия сохранения " + to_str(ver) + " не поддерживается (нужна " +
                   to_str(VERSION) + ")";
            return false;
        }
    }

    Player p;
    p.inv.clear();
    p.skills.clear();
    p.quests.clear();
    p.counters.clear();
    p.effects.clear();
    p.enchants.clear();
    p.portals.clear();
    for (std::string& e : p.equipped) e.clear();

    std::vector<Mob>      mobs;
    std::set<std::string> taken, visited, chests;
    int      turn = 0, respawn = RESPAWN_TURNS, next_uid = 1;
    unsigned long long seed = 0;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::istringstream ls(line);
        std::string key;
        ls >> key;

        if (key == "end") break;
        else if (key == "name")     { std::getline(ls, p.name);
                                      if (!p.name.empty() && p.name[0] == ' ') p.name.erase(0, 1); }
        else if (key == "level")    ls >> p.level;
        else if (key == "exp")      ls >> p.exp;
        else if (key == "gold")     ls >> p.gold;
        else if (key == "points")   ls >> p.skill_points;
        else if (key == "base")     { if (!read_stats(ls, p.base)) { err_ = "повреждена строка base"; return false; } }
        else if (key == "hp")       ls >> p.hp;
        else if (key == "ap")       ls >> p.ap;
        else if (key == "loc")      ls >> p.loc;
        else if (key == "race")     ls >> p.race;
        else if (key == "spec")     ls >> p.spec;
        else if (key == "portalmaster") { int v = 0; ls >> v; p.portal_master = (v != 0); }
        else if (key == "pos")      ls >> p.pos.x >> p.pos.y;
        else if (key == "momentum") ls >> p.momentum;
        else if (key == "turn")     ls >> turn;
        else if (key == "respawn")  ls >> respawn;
        else if (key == "nextuid")  ls >> next_uid;
        else if (key == "seed")     ls >> seed;
        else if (key == "stance") {
            int s = 1;
            ls >> s;
            if (s < 0 || s >= static_cast<int>(Stance::Count)) s = 1;
            p.stance = static_cast<Stance>(s);
        }
        else if (key == "equip") {
            std::size_t slot = 0;
            std::string id;
            if (ls >> slot >> id && slot < p.equipped.size()) p.equipped[slot] = id;
        }
        else if (key == "inv") {
            ItemStack s;
            if ((ls >> s.id >> s.count) && s.count > 0) p.inv.push_back(s);
        }
        else if (key == "skill")   { std::string id; int r = 0; if (ls >> id >> r) p.skills[id] = r; }
        else if (key == "quest")   { std::string id; int v = 0; if (ls >> id >> v) p.quests[id] = v; }
        else if (key == "counter") { std::string id; int v = 0; if (ls >> id >> v) p.counters[id] = v; }
        else if (key == "taken")   { std::string t; if (ls >> t) taken.insert(t); }
        else if (key == "chest")   { std::string t; if (ls >> t) chests.insert(t); }
        else if (key == "effect") {
            std::string id; int t = 0, pw = 1;
            if ((ls >> id >> t >> pw) && t > 0) p.effects.push_back(ActiveEffect(id, t, pw));
        }
        else if (key == "enchant") {
            std::string item, ench;
            if (ls >> item >> ench) p.enchants[item] = ench;
        }
        else if (key == "portal") {
            Portal pt;
            if (ls >> pt.loc >> pt.pos.x >> pt.pos.y) p.portals.push_back(pt);
        }
        else if (key == "mobinv") {
            int uid = 0; ItemStack st;
            if ((ls >> uid >> st.id >> st.count) && st.count > 0)
                for (Mob& mm : mobs)
                    if (mm.uid == uid) { mm.inv.push_back(st); break; }
        }
        else if (key == "mobfx") {
            int uid = 0; std::string id; int t = 0, pw = 1;
            if ((ls >> uid >> id >> t >> pw) && t > 0)
                for (Mob& mm : mobs)
                    if (mm.uid == uid) { mm.effects.push_back(ActiveEffect(id, t, pw)); break; }
        }
        else if (key == "visited") { std::string v; if (ls >> v) visited.insert(v); }
        else if (key == "mob") {
            Mob m;
            if (ls >> m.uid >> m.enemy_id >> m.loc >> m.pos.x >> m.pos.y >> m.hp >> m.zone) {
                ls >> m.gold;          // необязательные поля: пусто — значения по умолчанию
                int st = 0;
                if (ls >> st && st >= 0 && st <= static_cast<int>(MobState::Return))
                    m.state = static_cast<MobState>(st);
                mobs.push_back(m);
            }
        }
        // Неизвестные ключи пропускаем: старые сохранения не должны падать
        // на полях, добавленных позже.
    }

    // Проверяем, что мир вообще может принять этого героя.
    const Location* loc = world_.location(p.loc);
    if (!loc) {
        err_ = "локация из сохранения не читается: " + world_.last_error();
        return false;
    }
    if (!loc->in_bounds(p.pos)) {
        err_ = "координаты из сохранения вне карты";
        return false;
    }

    plr_          = p;
    mobs_         = mobs;
    taken_        = taken;
    chests_       = chests;
    visited_      = visited;
    turn_         = turn;
    respawn_left_ = respawn > 0 ? respawn : RESPAWN_TURNS;
    next_uid_     = next_uid > 0 ? next_uid : 1;
    cb_           = Combat();
    log_.clear();
    if (seed) rng_.set_seed(seed);

    // Клампим текущие HP/AP: снаряжение могло измениться между версиями.
    Stats t = total();
    if (plr_.hp > t.max_hp) plr_.hp = t.max_hp;
    if (plr_.ap > t.max_ap) plr_.ap = t.max_ap;
    if (plr_.hp < 0) plr_.hp = 0;

    visited_.insert(plr_.loc);
    msg("Игра загружена: " + plr_.name + ", уровень " + to_str(plr_.level) + ".");
    return true;
}
