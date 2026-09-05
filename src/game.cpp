#include "game.h"

#include <algorithm>

namespace {

int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

} // namespace

Game::Game(const std::string& data_root) : world_(data_root), log_epoch_(0) {}

void Game::msg(const std::string& m) {
    log_.push_back(m);
    if (log_.size() > LOG_MAX) {
        // Срезается сразу тысяча записей, а не одна на сообщение: иначе
        // каждая новая строка двигала бы весь журнал целиком.
        log_.erase(log_.begin(), log_.begin() + static_cast<std::ptrdiff_t>(LOG_TRIM));
        ++log_epoch_;
    }
}

// ------------------------------------------------------------- новая партия

void Game::new_game(const std::string& name, const std::string& race,
                    const std::string& spec) {
    plr_ = Player();
    pending_ending_.clear();
    if (!name.empty()) plr_.name = name;
    const Content& c = Content::get();
    plr_.race = c.race(race) ? race : "human";
    plr_.spec = c.spec(spec) ? spec : "swordsman";

    plr_.base = Stats();
    plr_.base.max_hp  = 30;
    plr_.base.max_ap  = 10;
    plr_.base.attack  = 62;
    plr_.base.dmg_min = 1;
    plr_.base.dmg_max = 3;
    plr_.base.block   = 4;
    plr_.base.armor   = 0;
    plr_.base.crit    = 4;
    plr_.base.ap_atk  = 4;

    plr_.loc = "village";
    plr_.pos = Vec2{5, 8};
    plr_.gold = 30;
    plr_.skill_points = 1;      // одно очко сразу, чтобы почувствовать систему

    add_item("bread", 2);
    if (const SpecDef* sp = c.spec(plr_.spec)) {
        if (!sp->start_item.empty()) {
            add_item(sp->start_item, sp->start_count > 0 ? sp->start_count : 1);
            equip(sp->start_item);
        }
    }
    if (plr_.equipped[static_cast<std::size_t>(Slot::Weapon)].empty()) {
        add_item("dagger", 1);
        equip("dagger");
    }

    Stats t = total();
    plr_.hp = t.max_hp;
    plr_.ap = t.max_ap;

    mobs_.clear();
    taken_.clear();
    visited_.clear();
    next_uid_ = 1;
    turn_ = 0;
    respawn_left_ = RESPAWN_TURNS;
    cb_ = Combat();
    log_.clear();
    ++log_epoch_;
    rng_.set_seed(0x5EEDC0DEULL);

    if (const Location* l = here()) spawn_initial(*l);
    chests_.clear();
    notes_.clear();
    {
        const RaceDef* r  = c.race(plr_.race);
        const SpecDef* sp = c.spec(plr_.spec);
        msg(std::string(r ? r->name : "Странник") + ", " +
            std::string(sp ? sp->name : "боец") + " — путь начинается.");
    }
    msg("Ольховка встречает тебя запахом дыма и мокрой соломы.");
    msg("Нажми ? — список команд.");
}

// -------------------------------------------------------- характеристики

void Game::apply_effect(std::vector<ActiveEffect>& list, const std::string& id,
                        int turns, int power) {
    if (id.empty() || turns <= 0) return;
    if (!Content::get().effect(id)) return;
    if (power < 1) power = 1;
    for (ActiveEffect& a : list) {
        if (a.id != id) continue;
        // Повторное наложение продлевает эффект и оставляет большую силу,
        // иначе список копил бы десятки одинаковых записей.
        if (turns > a.turns) a.turns = turns;
        if (power > a.power) a.power = power;
        return;
    }
    list.push_back(ActiveEffect(id, turns, power));
}

Stats Game::effect_stats(const std::vector<ActiveEffect>& list) {
    Stats t;
    const Content& c = Content::get();
    for (const ActiveEffect& a : list) {
        const EffectDef* d = c.effect(a.id);
        if (!d || d->kind != EffectKind::Stat) continue;
        for (int i = 0; i < a.power; ++i) t += d->per_power;
    }
    return t;
}

int Game::tick_effects(std::vector<ActiveEffect>& list) {
    const Content& c = Content::get();
    int delta = 0;
    for (std::size_t i = 0; i < list.size();) {
        ActiveEffect& a = list[i];
        const EffectDef* d = c.effect(a.id);
        if (d) {
            if (d->kind == EffectKind::Damage || d->kind == EffectKind::Heal)
                delta += d->hp_per_turn * a.power;
        }
        if (--a.turns <= 0) list.erase(list.begin() + static_cast<long>(i));
        else                ++i;
    }
    return delta;
}

int Game::cure_effects(std::vector<ActiveEffect>& list, const std::string& which) {
    const Content& c = Content::get();
    int removed = 0;
    for (std::size_t i = 0; i < list.size();) {
        const EffectDef* d = c.effect(list[i].id);
        const bool hit = (which == "*") ? (d && d->harmful) : (list[i].id == which);
        if (hit) { list.erase(list.begin() + static_cast<long>(i)); ++removed; }
        else     ++i;
    }
    return removed;
}

std::string Game::effects_line(const std::vector<ActiveEffect>& list) {
    const Content& c = Content::get();
    std::string out;
    for (const ActiveEffect& a : list) {
        const EffectDef* d = c.effect(a.id);
        if (!d) continue;
        if (!out.empty()) out += ", ";
        out += d->name;
        if (a.power > 1) out += " x" + to_str(a.power);
        out += "(" + to_str(a.turns) + ")";
    }
    return out;
}

// ---------------------------------------------------------- зачарования

Stats Game::enchant_bonus() const {
    Stats t;
    const Content& c = Content::get();
    for (const std::string& id : plr_.equipped) {
        if (id.empty()) continue;
        auto it = plr_.enchants.find(id);
        if (it == plr_.enchants.end()) continue;
        if (const EnchantDef* e = c.enchant(it->second)) t += e->bonus;
    }
    return t;
}

bool Game::can_enchant(const std::string& item_id) const {
    const ItemDef* d = Content::get().item(item_id);
    if (!d) return false;
    if (slot_for(d->kind) == Slot::Count) return false;      // только снаряжение
    return plr_.enchants.find(item_id) == plr_.enchants.end();
}

bool Game::enchant_item(const std::string& item_id, const std::string& ench_id) {
    const Content& c = Content::get();
    const ItemDef*    d = c.item(item_id);
    const EnchantDef* e = c.enchant(ench_id);
    if (!d || !e) return false;
    if (!can_enchant(item_id)) { msg("На этой вещи уже есть зачарование."); return false; }

    // Предмет должен быть у игрока: в сумке или надет.
    bool equipped = false;
    for (const std::string& id : plr_.equipped)
        if (id == item_id) equipped = true;
    if (!equipped && count_item(item_id) <= 0) return false;

    if (plr_.gold < e->price) {
        msg("Не хватает золота: нужно " + to_str(e->price) + ".");
        return false;
    }
    if (!e->reagent.empty() && count_item(e->reagent) < e->reagent_count) {
        const ItemDef* rd = c.item(e->reagent);
        msg("Нужен реагент: " + std::string(rd ? rd->name : e->reagent) +
            " x" + to_str(e->reagent_count) + ".");
        return false;
    }

    plr_.gold -= e->price;
    if (!e->reagent.empty()) remove_item(e->reagent, e->reagent_count);
    plr_.enchants[item_id] = ench_id;
    msg(d->name + " получает зачарование «" + e->name + "».");
    return true;
}

Stats Game::total_no_stance() const {
    Stats t = plr_.base;
    const Content& c = Content::get();
    if (const RaceDef* r  = c.race(plr_.race)) t += r->bonus;
    if (const SpecDef* sp = c.spec(plr_.spec)) t += sp->bonus;
    for (const std::string& id : plr_.equipped) {
        if (id.empty()) continue;
        if (const ItemDef* d = c.item(id)) t += d->bonus;
    }
    t += enchant_bonus();
    for (const auto& kv : plr_.skills) {
        const SkillDef* s = c.skill(kv.first);
        if (!s) continue;
        for (int i = 0; i < kv.second; ++i) t += s->bonus;
    }
    t += effect_stats(plr_.effects);
    // Никакой набор штрафов не должен обнулить героя.
    if (t.max_hp < 1) t.max_hp = 1;
    if (t.max_ap < 1) t.max_ap = 1;
    if (t.attack < 5) t.attack = 5;
    return t;
}

Stats Game::total() const {
    Stats t = total_no_stance();
    t += stance_bonus(plr_.stance);
    return t;
}

int Game::attack_cost() const {
    int c = total().ap_atk;
    return c < 1 ? 1 : c;
}

int Game::exp_to_next() const {
    // Кривая квадратичная, а не прямая. При прямой каждый следующий уровень
    // стоил столько же, сколько предыдущий приносил силы, и всё содержимое
    // игры выводило героя к сороковому уровню — там уже некуда тратить очки
    // навыка, все семь развиты до предела, и мир перестаёт сопротивляться.
    // Теперь тот же опыт приводит примерно к двадцать шестому: выбирать,
    // куда вложить очко, приходится до самого конца.
    const int l = plr_.level;
    return 50 + 25 * l + 3 * l * l;
}

void Game::grant_exp(int amount) {
    if (amount <= 0) return;
    plr_.exp += amount;
    msg("Получено " + to_str(amount) + " опыта.");
    while (plr_.exp >= exp_to_next()) {
        plr_.exp -= exp_to_next();
        ++plr_.level;
        ++plr_.skill_points;
        plr_.base.max_hp += 4;
        plr_.base.attack += 1;
        if (plr_.level % 3 == 0) plr_.base.max_ap += 1;
        Stats t = total();
        plr_.hp = t.max_hp;
        plr_.ap = t.max_ap;
        msg("УРОВЕНЬ " + to_str(plr_.level) + "! Есть очко навыка (клавиша K).");
    }
}

bool Game::learn_skill(const std::string& id) {
    const SkillDef* s = Content::get().skill(id);
    if (!s) return false;
    if (plr_.skill_points <= 0) { msg("Нет свободных очков навыка."); return false; }
    int& rank = plr_.skills[id];
    if (rank >= s->max_rank) { msg("Навык уже развит до предела."); return false; }
    ++rank;
    --plr_.skill_points;
    msg("Навык «" + s->name + "» повышен до " + to_str(rank) + ".");
    Stats t = total();
    if (plr_.hp > t.max_hp) plr_.hp = t.max_hp;
    if (plr_.ap > t.max_ap) plr_.ap = t.max_ap;
    return true;
}

// ------------------------------------------------------------- инвентарь

int Game::count_item(const std::string& id) const {
    for (const ItemStack& s : plr_.inv)
        if (s.id == id) return s.count;
    return 0;
}

int Game::carries_item(const std::string& id) const {
    int n = count_item(id);
    for (const std::string& worn : plr_.equipped)
        if (worn == id) ++n;
    return n;
}

void Game::add_item(const std::string& id, int n) {
    if (n <= 0) return;
    bool merged = false;
    for (ItemStack& s : plr_.inv) {
        if (s.id != id) continue;
        s.count += n;
        merged = true;
        break;
    }
    if (!merged) plr_.inv.push_back(ItemStack(id, n));
    fire_event(TriggerKind::ItemGained, id);
}

bool Game::remove_item(const std::string& id, int n) {
    if (n <= 0) return true;
    for (std::size_t i = 0; i < plr_.inv.size(); ++i) {
        if (plr_.inv[i].id != id) continue;
        if (plr_.inv[i].count < n) return false;
        plr_.inv[i].count -= n;
        if (plr_.inv[i].count == 0) plr_.inv.erase(plr_.inv.begin() + static_cast<long>(i));
        return true;
    }
    return false;
}

bool Game::equip(const std::string& id) {
    const ItemDef* d = Content::get().item(id);
    if (!d) return false;
    Slot s = slot_for(d->kind);
    if (s == Slot::Count) { msg("Это нельзя надеть."); return false; }
    if (count_item(id) <= 0) return false;

    std::string& cur = plr_.equipped[static_cast<std::size_t>(s)];
    if (!cur.empty()) add_item(cur, 1);       // снятое возвращается в сумку
    remove_item(id, 1);
    cur = id;

    Stats t = total();
    if (plr_.hp > t.max_hp) plr_.hp = t.max_hp;
    if (plr_.ap > t.max_ap) plr_.ap = t.max_ap;
    msg("Надето: " + d->name + ".");
    return true;
}

bool Game::unequip(Slot s) {
    std::string& cur = plr_.equipped[static_cast<std::size_t>(s)];
    if (cur.empty()) return false;
    add_item(cur, 1);
    const ItemDef* d = Content::get().item(cur);
    msg("Снято: " + std::string(d ? d->name : cur) + ".");
    cur.clear();
    Stats t = total();
    if (plr_.hp > t.max_hp) plr_.hp = t.max_hp;
    if (plr_.ap > t.max_ap) plr_.ap = t.max_ap;
    return true;
}

bool Game::use_item(const std::string& id) {
    const ItemDef* d = Content::get().item(id);
    if (!d || d->kind != ItemKind::Consumable) { msg("Это не применить."); return false; }
    if (count_item(id) <= 0) return false;

    int before_hp = plr_.hp, before_ap = plr_.ap;
    std::string what;

    if (!d->cures.empty()) {
        int n = cure_effects(plr_.effects, d->cures);
        if (n > 0) what += plural(n, "снят эффект", "снято эффекта", "снято эффектов") +
                           std::string(": ") + to_str(n);
        else       what += "снимать нечего";
    }
    if (!d->effect.empty()) {
        apply_effect(plr_.effects, d->effect, d->effect_turns, d->effect_power);
        const EffectDef* ed = Content::get().effect(d->effect);
        if (ed) what += (what.empty() ? "" : ", ") + ed->name + " на " +
                        to_str(d->effect_turns) + " ходов";
    }

    // total() пересчитываем после эффектов: эликсир мог поднять предел.
    Stats t = total();
    plr_.hp = std::min(t.max_hp, plr_.hp + d->heal_hp);
    plr_.ap = std::min(t.max_ap, plr_.ap + d->heal_ap);
    remove_item(id, 1);

    if (plr_.hp > before_hp) what += (what.empty() ? "" : ", ") + std::string("+") +
                                     to_str(plr_.hp - before_hp) + " HP";
    if (plr_.ap > before_ap) what += (what.empty() ? "" : ", ") + std::string("+") +
                                     to_str(plr_.ap - before_ap) + " AP";
    if (what.empty()) what = "без эффекта";
    msg(d->name + ": " + what + ".");
    return true;
}

bool Game::drop_item(const std::string& id) {
    const ItemDef* d = Content::get().item(id);
    if (!remove_item(id, 1)) return false;
    msg("Выброшено: " + std::string(d ? d->name : id) + ".");
    return true;
}

// --------------------------------------------------------------- свет

int Game::sight_radius() const {
    if (!Content::get().location_dark(plr_.loc)) return -1;

    // «В руке» — это надето, а не лежит в сумке: факел в мешке не светит.
    const std::size_t hand = static_cast<std::size_t>(Slot::Shield);
    if (hand < plr_.equipped.size() && !plr_.equipped[hand].empty()) {
        const ItemDef* d = Content::get().item(plr_.equipped[hand]);
        if (d && d->kind == ItemKind::Light) return SIGHT_TORCH;
    }
    return SIGHT_DARK;
}

bool Game::cell_lit(Vec2 p) const {
    const int r = sight_radius();
    if (r < 0) return true;

    const int dx = p.x - plr_.pos.x, dy = p.y - plr_.pos.y;
    // Круг, а не квадрат: свет от огня не бывает угловатым. Прибавка r к
    // квадрату радиуса включает в круг диагонали — иначе при радиусе в клетку
    // герой не видел бы даже углов вокруг себя.
    if (dx * dx + dy * dy > r * r + r) return false;

    // Стена свет не пропускает: за углом темно, как бы близко он ни был.
    const Location* loc = here();
    return !loc || loc->visible(plr_.pos, p);
}

// --------------------------------------------------------------- мобы

Mob* Game::mob_at(Vec2 p, const std::string& loc) {
    for (Mob& m : mobs_)
        if (m.loc == loc && m.pos == p) return &m;
    return nullptr;
}

const Mob* Game::mob_by_uid(int uid) const {
    for (const Mob& m : mobs_)
        if (m.uid == uid) return &m;
    return nullptr;
}

Mob* Game::mob_by_uid(int uid) {
    for (Mob& m : mobs_)
        if (m.uid == uid) return &m;
    return nullptr;
}

// Добыча разыгрывается при появлении моба, а не при смерти: моб реально
// носит с собой то, что с него потом упадёт, и может подобрать ещё.
void Game::fill_mob_inventory(Mob& m, const EnemyDef& e) {
    m.gold = rng_.range(e.gold_min, e.gold_max);
    for (const Drop& d : e.drops) {
        if (!rng_.chance(d.percent)) continue;
        bool merged = false;
        for (ItemStack& st : m.inv)
            if (st.id == d.item) { ++st.count; merged = true; break; }
        if (!merged) m.inv.push_back(ItemStack(d.item, 1));
    }
    for (const ActiveEffect& a : e.innate)
        apply_effect(m.effects, a.id, a.turns, a.power);
}

void Game::announce_dark() {
    // Сказать про темноту нужно каждый раз, а не только при первом входе:
    // игрок мог оставить факел на поверхности и вернуться уже без него.
    if (sight_radius() != SIGHT_DARK) return;
    msg("Здесь темно. Без огня в руке видно только на шаг вокруг себя.");
}

void Game::spawn_initial(const Location& loc) {
    if (visited_.count(loc.id)) return;
    visited_.insert(loc.id);
    fire_event(TriggerKind::LocationEntered, loc.id);
    for (std::size_t z = 0; z < loc.zones.size(); ++z) {
        const SpawnZone& zone = loc.zones[z];
        for (int i = 0; i < zone.max_count; ++i) {
            // Ищем свободную клетку в радиусе зоны.
            for (int tries = 0; tries < 40; ++tries) {
                Vec2 p{ zone.pos.x + rng_.range(-zone.radius, zone.radius),
                        zone.pos.y + rng_.range(-zone.radius, zone.radius) };
                if (!loc.walkable(p)) continue;
                if (loc.npc_at(p) || loc.exit_at(p)) continue;
                if (mob_at(p, loc.id)) continue;
                if (loc.id == plr_.loc && p == plr_.pos) continue;
                Mob m;
                m.uid = next_uid_++;
                m.enemy_id = zone.enemy_id;
                m.loc = loc.id;
                m.pos = p;
                m.zone = static_cast<int>(z);
                if (const EnemyDef* e = Content::get().enemy(zone.enemy_id)) {
                    m.hp = e->stats.max_hp;
                    fill_mob_inventory(m, *e);
                }
                mobs_.push_back(m);
                break;
            }
        }
    }
}

void Game::respawn_tick() {
    if (--respawn_left_ > 0) return;
    respawn_left_ = RESPAWN_TURNS;

    for (const std::string& loc_id : visited_) {
        const Location* loc = world_.location(loc_id);
        if (!loc) continue;
        for (std::size_t z = 0; z < loc->zones.size(); ++z) {
            const SpawnZone& zone = loc->zones[z];
            int alive = 0;
            for (const Mob& m : mobs_)
                if (m.loc == loc_id && m.zone == static_cast<int>(z)) ++alive;
            if (alive >= zone.max_count) continue;
            // Одна попытка на зону за тик — мир заселяется постепенно.
            for (int tries = 0; tries < 30; ++tries) {
                Vec2 p{ zone.pos.x + rng_.range(-zone.radius, zone.radius),
                        zone.pos.y + rng_.range(-zone.radius, zone.radius) };
                if (!loc->walkable(p)) continue;
                if (loc->npc_at(p) || loc->exit_at(p)) continue;
                if (mob_at(p, loc_id)) continue;
                if (loc_id == plr_.loc && dist(p, plr_.pos) < 4) continue;   // не в упор к игроку
                Mob m;
                m.uid = next_uid_++;
                m.enemy_id = zone.enemy_id;
                m.loc = loc_id;
                m.pos = p;
                m.zone = static_cast<int>(z);
                if (const EnemyDef* e = Content::get().enemy(zone.enemy_id)) {
                    m.hp = e->stats.max_hp;
                    fill_mob_inventory(m, *e);
                }
                mobs_.push_back(m);
                break;
            }
        }
    }
}

bool Game::mob_can_stand(const Location& loc, Vec2 p, int self_uid) const {
    if (!loc.walkable(p)) return false;
    if (loc.npc_at(p) || loc.exit_at(p)) return false;
    int ci = loc.chest_index_at(p);
    if (ci >= 0 && !chest_opened(loc.id, ci)) return false;
    for (const Mob& o : mobs_)
        if (o.uid != self_uid && o.loc == loc.id && o.pos == p) return false;
    return true;
}

Vec2 Game::step_toward(const Location& loc, const Mob& m, Vec2 target) const {
    const int dx = target.x - m.pos.x;
    const int dy = target.y - m.pos.y;
    const int sx = (dx > 0) - (dx < 0);
    const int sy = (dy > 0) - (dy < 0);
    if (sx == 0 && sy == 0) return Vec2(0, 0);

    Vec2 first(0, 0), second(0, 0);
    if (std::abs(dx) >= std::abs(dy)) { first = Vec2(sx, 0); second = Vec2(0, sy); }
    else                              { first = Vec2(0, sy); second = Vec2(sx, 0); }

    // Шаг на игрока разбирает вызывающий: для него это начало боя.
    Vec2 cand(m.pos.x + first.x, m.pos.y + first.y);
    if (first.x != 0 || first.y != 0) {
        if (cand == plr_.pos && m.loc == plr_.loc) return first;
        if (mob_can_stand(loc, cand, m.uid)) return first;
    }
    cand = Vec2(m.pos.x + second.x, m.pos.y + second.y);
    if (second.x != 0 || second.y != 0) {
        if (cand == plr_.pos && m.loc == plr_.loc) return second;
        if (mob_can_stand(loc, cand, m.uid)) return second;
    }
    return Vec2(0, 0);
}

void Game::move_mobs() {
    const Location* loc = here();
    if (!loc) return;
    const Content& c = Content::get();

    for (Mob& m : mobs_) {
        if (m.loc != plr_.loc) continue;              // соседние локации спят
        const EnemyDef* e = c.enemy(m.enemy_id);
        if (!e) continue;

        // Дом — центр зоны спавна; без зоны моб считает домом текущее место.
        Vec2 home = m.pos;
        int  radius = 0;
        if (m.zone >= 0 && m.zone < static_cast<int>(loc->zones.size())) {
            home   = loc->zones[static_cast<std::size_t>(m.zone)].pos;
            radius = loc->zones[static_cast<std::size_t>(m.zone)].radius;
        }

        // Видит — значит, есть и расстояние, и прямая линия. Через стену
        // и сквозь деревья моб игрока не замечает.
        const bool sees = e->aggressive &&
                          dist(m.pos, plr_.pos) <= e->detect &&
                          loc->visible(m.pos, plr_.pos);

        if (sees)                             m.state = MobState::Chase;
        else if (m.state == MobState::Chase)  m.state = MobState::Return;

        // Спокойный моб, оказавшийся вне своей зоны, тоже идёт домой: иначе
        // он остался бы снаружи навсегда, занимая слот зоны и блокируя
        // появление нового.
        if (m.state == MobState::Idle && radius > 0 && dist(m.pos, home) > radius)
            m.state = MobState::Return;

        if (m.state == MobState::Return && dist(m.pos, home) <= radius)
            m.state = MobState::Idle;

        Vec2 step(0, 0);
        if (m.state == MobState::Chase) {
            step = step_toward(*loc, m, plr_.pos);
        } else if (m.state == MobState::Return) {
            step = step_toward(*loc, m, home);
            if (step.x == 0 && step.y == 0) {
                // Упёрся в препятствие: делаем случайный шаг, чтобы сойти с
                // места, но домой идти не перестаём.
                switch (rng_.range(0, 3)) {
                    case 0: step = Vec2( 1,  0); break;
                    case 1: step = Vec2(-1,  0); break;
                    case 2: step = Vec2( 0,  1); break;
                    default: step = Vec2(0, -1); break;
                }
            }
        } else if (rng_.chance(45)) {
            switch (rng_.range(0, 3)) {
                case 0: step.x =  1; break;
                case 1: step.x = -1; break;
                case 2: step.y =  1; break;
                default: step.y = -1; break;
            }
            // Блуждание не уводит за пределы своей зоны: иначе мобы
            // расползаются по карте, а слот зоны остаётся занятым.
            Vec2 probe(m.pos.x + step.x, m.pos.y + step.y);
            if (radius > 0 && dist(probe, home) > radius) step = Vec2(0, 0);
        }

        if (step.x == 0 && step.y == 0) continue;

        Vec2 np(m.pos.x + step.x, m.pos.y + step.y);

        if (np == plr_.pos) {                          // дошёл до игрока — бой
            if (e->aggressive) {
                msg(e->name + " бросается на тебя!");
                start_combat(m.uid);
                return;                                // остальные ждут конца боя
            }
            continue;
        }
        if (!mob_can_stand(*loc, np, m.uid)) continue;
        m.pos = np;

        // Моб подбирает лежащее на земле — потом это достанется победителю.
        int idx = loc->item_index_at(np);
        if (idx >= 0) {
            const std::string key = loc->id + ":" + to_str(idx);
            if (!taken_.count(key)) {
                taken_.insert(key);
                const MapItem& mi = loc->items[static_cast<std::size_t>(idx)];
                bool merged = false;
                for (ItemStack& st : m.inv)
                    if (st.id == mi.item_id) { st.count += mi.count; merged = true; break; }
                if (!merged) m.inv.push_back(ItemStack(mi.item_id, mi.count));
            }
        }
    }
}

// ------------------------------------------------------ книги и записки

Book* Game::book(const std::string& id) {
    for (Book& b : plr_.books)
        if (b.id == id) return &b;
    return nullptr;
}

const Book* Game::book(const std::string& id) const {
    for (const Book& b : plr_.books)
        if (b.id == id) return &b;
    return nullptr;
}

bool Game::start_book(const std::string& title) {
    if (static_cast<int>(plr_.books.size()) >= BOOK_MAX_COUNT) {
        msg("Библиотека переполнена: больше " + to_str(BOOK_MAX_COUNT) + " не унести.");
        return false;
    }
    if (count_item("book_blank") <= 0) { msg("Нужна чистая книга."); return false; }
    remove_item("book_blank", 1);

    Book b;
    b.id    = "b" + to_str(plr_.next_book++);
    b.title = trunc(title.empty() ? "Без названия" : title, BOOK_TITLE_MAX);
    b.lines.push_back("");
    plr_.books.push_back(b);
    msg("Начата книга «" + b.title + "». Библиотека — клавиша B.");
    return true;
}

bool Game::delete_book(const std::string& id) {
    for (std::size_t i = 0; i < plr_.books.size(); ++i) {
        if (plr_.books[i].id != id) continue;
        const std::string title = plr_.books[i].title;
        plr_.books.erase(plr_.books.begin() + static_cast<long>(i));
        msg("Выброшено: «" + title + "».");
        return true;
    }
    return false;
}

bool Game::book_set_title(const std::string& id, const std::string& title) {
    Book* b = book(id);
    if (!b || b->readonly) return false;
    b->title = trunc(title.empty() ? "Без названия" : title, BOOK_TITLE_MAX);
    return true;
}

bool Game::book_set_line(const std::string& id, int index, const std::string& text) {
    Book* b = book(id);
    if (!b || b->readonly) return false;
    if (index < 0 || index >= static_cast<int>(b->lines.size())) return false;
    b->lines[static_cast<std::size_t>(index)] = trunc(text, BOOK_MAX_CHARS);
    return true;
}

bool Game::book_insert_line(const std::string& id, int index, const std::string& text) {
    Book* b = book(id);
    if (!b || b->readonly) return false;
    if (static_cast<int>(b->lines.size()) >= BOOK_MAX_LINES) {
        msg("В книге больше " + to_str(BOOK_MAX_LINES) + " строк не помещается.");
        return false;
    }
    if (index < 0) index = 0;
    if (index > static_cast<int>(b->lines.size())) index = static_cast<int>(b->lines.size());
    b->lines.insert(b->lines.begin() + index, trunc(text, BOOK_MAX_CHARS));
    return true;
}

bool Game::book_remove_line(const std::string& id, int index) {
    Book* b = book(id);
    if (!b || b->readonly) return false;
    if (index < 0 || index >= static_cast<int>(b->lines.size())) return false;
    if (b->lines.size() <= 1) { b->lines[0].clear(); return true; }  // последнюю чистим
    b->lines.erase(b->lines.begin() + index);
    return true;
}

bool Game::note_taken(const std::string& loc_id, int index) const {
    return notes_.count(loc_id + ":" + to_str(index)) != 0;
}

bool Game::take_note(int index) {
    const Location* loc = here();
    if (!loc || index < 0 || index >= static_cast<int>(loc->notes.size())) return false;
    if (note_taken(loc->id, index)) return false;

    const MapNote& mn = loc->notes[static_cast<std::size_t>(index)];
    const NoteDef* nd = Content::get().note(mn.note_id);
    if (!nd) { msg("Листок рассыпался в руках."); notes_.insert(loc->id + ":" + to_str(index)); return false; }

    notes_.insert(loc->id + ":" + to_str(index));

    Book b;
    b.id       = "n_" + nd->id;
    b.title    = nd->title;
    b.lines    = nd->lines;
    b.readonly = true;
    if (!book(b.id)) plr_.books.push_back(b);

    // Счётчик позволяет требовать находку в диалоге.
    plr_.counters["note_" + nd->id] = 1;
    msg("Найдена записка: «" + nd->title + "». Читать — клавиша B.");
    fire_event(TriggerKind::NoteTaken, nd->id);
    return true;
}

bool Game::chest_opened(const std::string& loc_id, int index) const {
    return chests_.count(loc_id + ":" + to_str(index)) != 0;
}

bool Game::open_chest(int index) {
    const Location* loc = here();
    if (!loc || index < 0 || index >= static_cast<int>(loc->chests.size())) return false;
    const MapChest& ch = loc->chests[static_cast<std::size_t>(index)];

    if (chest_opened(loc->id, index)) { msg("Сундук уже пуст."); return false; }

    // Ключ считается так же, как во вратах: надетое кольцо остаётся ключом.
    if (!ch.key.empty() && carries_item(ch.key) <= 0) {
        const ItemDef* kd = Content::get().item(ch.key);
        msg("Заперто. Нужен ключ: " + std::string(kd ? kd->name : ch.key) + ".");
        return false;
    }

    chests_.insert(loc->id + ":" + to_str(index));
    msg("Сундук открыт.");
    if (ch.gold > 0) {
        plr_.gold += ch.gold;
        msg("В сундуке " + to_str(ch.gold) + " " +
            plural(ch.gold, "монета", "монеты", "монет") + ".");
    }
    for (const ItemStack& st : ch.items) {
        add_item(st.id, st.count);
        const ItemDef* d = Content::get().item(st.id);
        msg("Взято: " + std::string(d ? d->name : st.id) +
            (st.count > 1 ? " x" + to_str(st.count) : "") + ".");
    }
    return true;
}

const Portal* Game::portal_at(Vec2 p, const std::string& loc_id) const {
    for (const Portal& pt : plr_.portals)
        if (pt.loc == loc_id && pt.pos == p) return &pt;
    return nullptr;
}

bool Game::place_portal() {
    if (!plr_.portal_master) {
        msg("Ты не умеешь ставить порталы.");
        return false;
    }
    const Location* loc = here();
    if (!loc) return false;
    if (portal_at(plr_.pos, plr_.loc)) { msg("Здесь уже стоит портал."); return false; }
    if (loc->exit_at(plr_.pos) || loc->bed_at(plr_.pos)) {
        msg("Слишком близко к переходу — портал не встанет.");
        return false;
    }
    if (static_cast<int>(plr_.portals.size()) >= PORTAL_LIMIT) {
        msg("Больше " + to_str(PORTAL_LIMIT) + " порталов не удержать. Сними лишний.");
        return false;
    }
    if (count_item("portal_stone") <= 0) { msg("Нужен портальный камень."); return false; }

    remove_item("portal_stone", 1);
    plr_.portals.push_back(Portal(plr_.loc, plr_.pos));
    msg("Портал поставлен (" + to_str(static_cast<int>(plr_.portals.size())) +
        " из " + to_str(PORTAL_LIMIT) + ").");
    return true;
}

bool Game::remove_portal_here() {
    for (std::size_t i = 0; i < plr_.portals.size(); ++i) {
        if (!(plr_.portals[i].loc == plr_.loc && plr_.portals[i].pos == plr_.pos)) continue;
        plr_.portals.erase(plr_.portals.begin() + static_cast<long>(i));
        add_item("portal_stone", 1);
        msg("Портал снят, камень вернулся в сумку.");
        return true;
    }
    msg("Здесь нет портала.");
    return false;
}

bool Game::item_taken(const std::string& loc_id, int index) const {
    return taken_.count(loc_id + ":" + to_str(index)) != 0;
}

void Game::world_turn() {
    ++turn_;

    // Эффекты тикают вне боя тоже: яд не ждёт, пока ты снова подерёшься.
    int delta = tick_effects(plr_.effects);
    if (delta != 0) {
        Stats t = total();
        int before = plr_.hp;
        plr_.hp += delta;
        if (plr_.hp > t.max_hp) plr_.hp = t.max_hp;
        if (plr_.hp < 0) plr_.hp = 0;
        if (plr_.hp != before && delta < 0)
            msg("Эффекты отнимают " + to_str(before - plr_.hp) + " здоровья.");
    }
    for (Mob& m : mobs_) {
        if (m.loc != plr_.loc) continue;
        int d = tick_effects(m.effects);
        if (d != 0) m.hp += d;
    }
    // Мобов, добитых эффектом вне боя, убираем без наград.
    for (std::size_t i = 0; i < mobs_.size();) {
        if (mobs_[i].hp <= 0) mobs_.erase(mobs_.begin() + static_cast<long>(i));
        else ++i;
    }

    respawn_tick();
    move_mobs();
}

void Game::rest() {
    cure_effects(plr_.effects, "*");   // отдых снимает отраву
    Stats t = total();
    plr_.hp = t.max_hp;
    plr_.ap = t.max_ap;
    plr_.momentum = 0;
    msg("Ты отдыхаешь. Здоровье и силы восстановлены.");
}

// ---------------------------------------------------------- перемещение

Bump Game::try_move(int dx, int dy) {
    const Location* loc = here();
    if (!loc) return Bump::Blocked;

    Vec2 np{ plr_.pos.x + dx, plr_.pos.y + dy };

    if (Mob* m = mob_at(np, plr_.loc)) {
        start_combat(m->uid);
        return Bump::Combat;
    }
    if (loc->npc_at(np))  return Bump::Npc;
    if (loc->sign_at(np)) return Bump::Sign;

    int ci = loc->chest_index_at(np);
    if (ci >= 0 && !chest_opened(loc->id, ci)) { open_chest(ci); return Bump::Chest; }

    if (!loc->walkable(np)) return Bump::Blocked;

    if (const MapExit* ex = loc->exit_at(np)) {
        if (!ex->gate.empty()) {
            const Gate& gt = ex->gate;
            bool pass = true;
            if (!gt.key.empty() && carries_item(gt.key) <= 0) pass = false;
            if (!gt.req_quest.empty() && quest_stage(gt.req_quest) < gt.req_stage) pass = false;
            if (!gt.req_counter.empty()) {
                auto it = plr_.counters.find(gt.req_counter);
                int have = (it == plr_.counters.end()) ? 0 : it->second;
                if (have < gt.req_counter_min) pass = false;
            }
            if (!pass) {
                if (!gt.denied.empty()) msg(gt.denied);
                else if (!gt.key.empty()) {
                    const ItemDef* kd = Content::get().item(gt.key);
                    msg("Заперто. Нужен ключ: " + std::string(kd ? kd->name : gt.key) + ".");
                } else {
                    msg("Проход закрыт.");
                }
                return Bump::Blocked;
            }
        }
        const Location* dst = world_.location(ex->target);
        if (!dst) { msg("Дорога обрывается: " + world_.last_error()); return Bump::Blocked; }
        plr_.loc = dst->id;
        plr_.pos = ex->dest;
        spawn_initial(*dst);
        msg("Ты приходишь в локацию «" + dst->name + "».");
        announce_dark();
        return Bump::Exit;
    }

    plr_.pos = np;

    // Портал переносит к следующему по списку: с двумя это связка туда-обратно.
    if (portal_at(np, plr_.loc)) {
        if (plr_.portals.size() < 2) {
            msg("Портал один — связывать не с чем. Поставь второй.");
        } else {
            std::size_t cur = 0;
            for (std::size_t i = 0; i < plr_.portals.size(); ++i)
                if (plr_.portals[i].loc == plr_.loc && plr_.portals[i].pos == np) cur = i;
            const Portal& dst = plr_.portals[(cur + 1) % plr_.portals.size()];
            const Location* dl = world_.location(dst.loc);
            if (!dl) { msg("Портал ведёт в никуда: " + world_.last_error()); return Bump::Moved; }
            plr_.loc = dst.loc;
            plr_.pos = dst.pos;
            spawn_initial(*dl);
            msg("Портал переносит тебя в локацию «" + dl->name + "».");
            announce_dark();
            return Bump::Portal;
        }
    }

    int ni = loc->note_index_at(np);
    if (ni >= 0 && !note_taken(loc->id, ni)) { take_note(ni); return Bump::Note; }

    int idx = loc->item_index_at(np);
    if (idx >= 0) {
        const std::string key = loc->id + ":" + to_str(idx);
        if (!taken_.count(key)) {
            taken_.insert(key);
            const MapItem& mi = loc->items[static_cast<std::size_t>(idx)];
            add_item(mi.item_id, mi.count);
            const ItemDef* d = Content::get().item(mi.item_id);
            msg("Поднято: " + std::string(d ? d->name : mi.item_id) +
                (mi.count > 1 ? " x" + to_str(mi.count) : "") + ".");
            return Bump::Item;
        }
    }
    if (loc->bed_at(np)) { rest(); return Bump::Bed; }

    return Bump::Moved;
}

// -------------------------------------------------- события и триггеры

int Game::quest_stage(const std::string& id) const {
    auto it = plr_.quests.find(id);
    return it == plr_.quests.end() ? QUEST_NONE : it->second;
}

void Game::recheck_state_triggers() {
    const Content& c = Content::get();
    // Несколько проходов: одно сработавшее условие может открыть следующее.
    for (int pass = 0; pass < 8; ++pass) {
        bool changed = false;
        for (const QuestTrigger& t : c.triggers()) {
            if (t.kind != TriggerKind::ItemGained && t.kind != TriggerKind::MobKilled) continue;
            const int cur = quest_stage(t.quest);
            if (cur < t.min_stage || cur >= t.stage) continue;

            int have = 0;
            if (t.kind == TriggerKind::ItemGained) {
                have = count_item(t.key);
            } else {
                auto it = plr_.counters.find(t.key);
                have = (it == plr_.counters.end()) ? 0 : it->second;
            }
            if (have < t.count) continue;

            plr_.quests[t.quest] = t.stage;
            if (!t.message.empty()) msg(t.message);
            changed = true;
        }
        if (!changed) return;
    }
}

void Game::fire_event(TriggerKind kind, const std::string& key) {
    const Content& c = Content::get();
    bool advanced = false;
    for (const QuestTrigger& t : c.triggers()) {
        if (t.kind != kind || t.key != key) continue;

        // Порог события: сколько убито, сколько предметов на руках, какой этап.
        int have = 1;
        if (kind == TriggerKind::MobKilled) {
            auto it = plr_.counters.find(key);
            have = (it == plr_.counters.end()) ? 0 : it->second;
        } else if (kind == TriggerKind::ItemGained) {
            have = count_item(key);
        } else if (kind == TriggerKind::QuestStage) {
            have = quest_stage(key);
        }
        if (have < t.count) continue;

        const int cur = quest_stage(t.quest);
        if (cur < t.min_stage) continue;    // цепочку нельзя пройти с конца
        if (cur >= t.stage) continue;       // назад квест не откатываем

        plr_.quests[t.quest] = t.stage;
        const QuestDef* q = c.quest(t.quest);
        if (!t.message.empty()) msg(t.message);
        else if (q) msg((q->secret ? "Открыта тайна: «" : "Новый след: «") + q->name + "».");
        advanced = true;
    }
    // Продвинувшийся квест мог сделать выполнимым условие, которое игрок
    // закрыл давно — например, ключ добыт задолго до того, как стало ясно,
    // от чего он.
    if (advanced) recheck_state_triggers();
}

// ------------------------------------------------------------- диалоги

bool Game::option_available(const DlgOption& o) const {
    if (!o.req_quest.empty()) {
        auto it = plr_.quests.find(o.req_quest);
        int st = (it == plr_.quests.end()) ? QUEST_NONE : it->second;
        if (o.req_stage_min >= 0 && st < o.req_stage_min) return false;
        if (o.req_stage_max >= 0 && st > o.req_stage_max) return false;
    }
    if (!o.req_quest2.empty()) {
        auto it = plr_.quests.find(o.req_quest2);
        int st = (it == plr_.quests.end()) ? QUEST_NONE : it->second;
        if (o.req_stage2_min >= 0 && st < o.req_stage2_min) return false;
        if (o.req_stage2_max >= 0 && st > o.req_stage2_max) return false;
    }
    if (!o.req_counter.empty()) {
        auto it = plr_.counters.find(o.req_counter);
        int c = (it == plr_.counters.end()) ? 0 : it->second;
        if (o.req_counter_min >= 0 && c < o.req_counter_min) return false;
        if (o.req_counter_max >= 0 && c > o.req_counter_max) return false;
    }
    if (!o.req_note.empty()) {
        auto it = plr_.counters.find("note_" + o.req_note);
        if (it == plr_.counters.end() || it->second <= 0) return false;
    }
    if (!o.req_item.empty() && count_item(o.req_item) < o.req_item_count) return false;
    return true;
}

void Game::apply_option(const DlgOption& o, const std::string& npc_shop,
                        std::string* shop_out, bool* enchant_out) {
    const Content& c = Content::get();

    if (!o.set_quest.empty()) {
        plr_.quests[o.set_quest] = o.set_stage;
        fire_event(TriggerKind::QuestStage, o.set_quest);
        recheck_state_triggers();
        const QuestDef* q = c.quest(o.set_quest);
        const std::string qname = q ? q->name : o.set_quest;
        if (o.set_stage == QUEST_DONE) msg("Квест завершён: «" + qname + "».");
        else if (o.set_stage == 1)     msg("Взят квест: «" + qname + "».");
        else                           msg("Квест «" + qname + "» продвинулся.");
    }
    if (!o.set_counter.empty()) plr_.counters[o.set_counter] = o.set_counter_value;
    // Забираем до выдачи, чтобы обмен «предмет за предмет» не упирался в порядок.
    if (!o.take_item.empty() && o.take_count > 0) {
        if (remove_item(o.take_item, o.take_count)) {
            const ItemDef* d = c.item(o.take_item);
            msg("Отдано: " + std::string(d ? d->name : o.take_item) +
                (o.take_count > 1 ? " x" + to_str(o.take_count) : "") + ".");
        }
    }
    if (!o.give_item.empty() && o.give_count > 0) {
        add_item(o.give_item, o.give_count);
        const ItemDef* d = c.item(o.give_item);
        msg("Получено: " + std::string(d ? d->name : o.give_item) +
            (o.give_count > 1 ? " x" + to_str(o.give_count) : "") + ".");
    }
    if (o.give_gold > 0) {
        plr_.gold += o.give_gold;
        msg("Получено " + to_str(o.give_gold) + " золотых.");
    }
    if (o.rest) rest();
    if (o.portal_gift && !plr_.portal_master) {
        plr_.portal_master = true;
        msg("Открыт навык «Мастер нулевой точки»: теперь ты умеешь ставить порталы (P).");
    }
    if (o.give_exp > 0) grant_exp(o.give_exp);
    if (o.open_shop && shop_out) *shop_out = o.shop_id.empty() ? npc_shop : o.shop_id;
    if (o.open_enchant && enchant_out) *enchant_out = true;
    // Развязка выбирается последней: сперва пусть отработают квест, счётчик
    // и награды, и только потом интерфейс покажет эпилог.
    if (!o.ending.empty() && c.ending(o.ending)) pending_ending_ = o.ending;
}

std::string Game::take_pending_ending() {
    std::string id;
    id.swap(pending_ending_);
    return id;
}

// ------------------------------------------------------------- торговля

int Game::buy_price(const ShopDef& s, const ItemDef& d) const {
    int p = d.price * s.buy_pct / 100;
    return p < 1 ? 1 : p;
}

int Game::sell_price(const ShopDef& s, const ItemDef& d) const {
    return d.price * s.sell_pct / 100;
}

bool Game::buy(const ShopDef& s, const std::string& item_id) {
    const ItemDef* d = Content::get().item(item_id);
    if (!d) return false;
    int p = buy_price(s, *d);
    if (plr_.gold < p) { msg("Не хватает золота: нужно " + to_str(p) + "."); return false; }
    plr_.gold -= p;
    add_item(item_id, 1);
    msg("Куплено: " + d->name + " за " + to_str(p) + ".");
    return true;
}

bool Game::sell(const ShopDef& s, const std::string& item_id) {
    const ItemDef* d = Content::get().item(item_id);
    if (!d) return false;
    if (!remove_item(item_id, 1)) return false;
    int p = sell_price(s, *d);
    plr_.gold += p;
    msg("Продано: " + d->name + " за " + to_str(p) + ".");
    return true;
}

// ------------------------------------------------------------------ бой

void Game::combat_log(const std::string& s) {
    cb_.log.push_back(s);
    if (cb_.log.size() > 40) cb_.log.erase(cb_.log.begin());
}

void Game::start_combat(int mob_uid) {
    const Mob* m = mob_by_uid(mob_uid);
    if (!m) return;
    const EnemyDef* e = Content::get().enemy(m->enemy_id);
    if (!e) return;

    cb_ = Combat();
    cb_.active = true;
    cb_.mob_uid = mob_uid;
    cb_.enemy_hp = m->hp;
    plr_.ap = total().max_ap;
    plr_.momentum = 0;
    if (!plr_.effects.empty())
        combat_log("На тебе: " + effects_line(plr_.effects) + ".");
    combat_log("— " + e->name + " преграждает путь. —");
}

int Game::resolve_hit(const Stats& atk, const Stats& def, int stance_pct,
                      bool guaranteed, int crit_bonus, std::string* line) {
    int chance = clampi(atk.attack - def.block, 5, 95);
    if (!guaranteed && !rng_.chance(chance)) {
        if (line) *line = "промах";
        return -1;
    }
    bool crit = rng_.chance(atk.crit + crit_bonus);
    int raw = rng_.range(atk.dmg_min, atk.dmg_max) * stance_pct / 100;
    if (crit) raw *= 2;

    // Броня снимает часть удара, но не весь. Раньше она вычиталась начисто,
    // и к середине игры любой удар доходил ровно единицей: броня героя
    // обгоняла урон почти всех врагов, и его переставали пробивать вовсе.
    // Теперь сквозь броню всегда проходит хотя бы ARMOR_MIN_PCT удара —
    // на слабых ударах это ничего не меняет (там и так был пол в единицу),
    // а тяжёлый удар остаётся тяжёлым, сколько брони ни надень.
    int dmg = raw - def.armor;
    const int floor_dmg = raw * ARMOR_MIN_PCT / 100;
    if (dmg < floor_dmg) dmg = floor_dmg;
    if (dmg < 1) dmg = 1;
    if (line) *line = (crit ? "КРИТ " : "") + to_str(dmg) + " урона";
    return dmg;
}

void Game::combat_attack(bool power) {
    if (!cb_.active) return;
    Mob* m = mob_by_uid(cb_.mob_uid);
    if (!m) { finish_combat(); return; }
    const EnemyDef* e = Content::get().enemy(m->enemy_id);
    if (!e) { finish_combat(); return; }

    int cost = attack_cost();
    if (power && plr_.momentum < MOMENTUM_COST) {
        combat_log("Куража не хватает: нужно " + to_str(MOMENTUM_COST) + ".");
        return;
    }
    if (plr_.ap < cost) {
        combat_log("Не хватает очков действия — заканчивай ход (E).");
        return;
    }
    plr_.ap -= cost;

    Stats me  = total();
    Stats foe = e->stats;
    foe += effect_stats(m->effects);       // враг тоже под своими эффектами
    int pct = stance_damage_pct(plr_.stance);
    std::string line;
    int dmg;

    if (power) {
        plr_.momentum -= MOMENTUM_COST;
        // Мощный удар: бьёт наверняка, двойной урон и половина брони игнорируется.
        Stats soft = foe;
        soft.armor = foe.armor / 2;
        dmg = resolve_hit(me, soft, pct * 2, true, 100, &line);
        combat_log("МОЩНЫЙ УДАР: " + line + ".");
    } else {
        dmg = resolve_hit(me, foe, pct, false, 0, &line);
        combat_log("Ты бьёшь: " + line + ".");
    }

    if (dmg > 0) {
        cb_.enemy_hp -= dmg;
        if (!power && plr_.momentum < MOMENTUM_MAX) {
            ++plr_.momentum;
            combat_log("Кураж: " + to_str(plr_.momentum) + "/" + to_str(MOMENTUM_MAX) + ".");
        }
        // Зачарование оружия срабатывает только на попадании.
        const Content& c = Content::get();
        const std::string& wid = plr_.equipped[static_cast<std::size_t>(Slot::Weapon)];
        auto ench = plr_.enchants.find(wid);
        if (ench != plr_.enchants.end()) {
            const EnchantDef* ed = c.enchant(ench->second);
            if (ed && !ed->on_hit_effect.empty() && rng_.chance(ed->on_hit_chance)) {
                apply_effect(m->effects, ed->on_hit_effect, 4, ed->on_hit_power);
                const EffectDef* fx = c.effect(ed->on_hit_effect);
                if (fx) combat_log("«" + ed->name + "»: на противника наложено «" +
                                   fx->name + "».");
            }
        }
    }

    if (cb_.enemy_hp <= 0) { kill_mob(*m); return; }
    m->hp = cb_.enemy_hp;

    if (plr_.ap < attack_cost()) {
        combat_log("Силы на исходе — ход переходит противнику.");
        enemy_turn();
    }
}

void Game::combat_set_stance(Stance s) {
    if (!cb_.active) { plr_.stance = s; return; }
    if (cb_.stance_used) { combat_log("Стойку можно менять раз за раунд."); return; }
    if (s == plr_.stance) return;
    plr_.stance = s;
    cb_.stance_used = true;
    combat_log("Стойка: " + std::string(stance_name(s)) + ".");
}

bool Game::combat_use_item(const std::string& id) {
    if (plr_.ap < AP_ITEM_COST) {
        combat_log("На это нужно " + to_str(AP_ITEM_COST) + " AP.");
        return false;
    }
    int hp_before = plr_.hp, ap_before = plr_.ap;
    if (!use_item(id)) return false;
    // Стоимость снимается уже после применения, иначе тоник вернул бы AP и тут же их потерял.
    plr_.ap = std::max(0, plr_.ap - AP_ITEM_COST);
    combat_log("Использовано в бою: +" + to_str(plr_.hp - hp_before) + " HP, AP " +
               to_str(ap_before) + "→" + to_str(plr_.ap) + ".");
    return true;
}

void Game::combat_end_turn() {
    if (!cb_.active) return;
    enemy_turn();
}

bool Game::combat_flee() {
    if (!cb_.active) return false;
    int chance = (plr_.stance == Stance::Cautious) ? 70 : 55;
    if (rng_.chance(chance)) {
        combat_log("Ты разрываешь дистанцию.");
        msg("Ты сбежал из боя.");
        cb_.active = false;
        cb_.mob_uid = -1;
        plr_.momentum = 0;
        return true;
    }
    combat_log("Сбежать не вышло!");
    enemy_turn();
    return false;
}

void Game::enemy_turn() {
    Mob* m = mob_by_uid(cb_.mob_uid);
    if (!m) { finish_combat(); return; }
    const EnemyDef* e = Content::get().enemy(m->enemy_id);
    if (!e) { finish_combat(); return; }

    const Content& c = Content::get();
    Stats foe = e->stats;
    foe += effect_stats(m->effects);
    Stats me  = total();
    int eap = foe.max_ap;
    if (eap < 1) eap = 1;
    int atk_cost = foe.ap_atk < 1 ? 1 : foe.ap_atk;

    while (eap >= atk_cost && plr_.hp > 0) {
        eap -= atk_cost;
        std::string line;
        int dmg = resolve_hit(foe, me, 100, false, 0, &line);
        if (dmg > 0) {
            plr_.hp -= dmg;
            if (plr_.momentum > 0) --plr_.momentum;   // пропущенный удар сбивает кураж
            combat_log(e->name + ": " + line + ".");
            if (!e->on_hit_effect.empty() && rng_.chance(e->on_hit_chance)) {
                apply_effect(plr_.effects, e->on_hit_effect, 5, e->on_hit_power);
                const EffectDef* fx = c.effect(e->on_hit_effect);
                if (fx) combat_log("На тебя наложено: «" + fx->name + "».");
            }
        } else {
            combat_log(e->name + ": промах.");
        }
    }

    // Конец раунда: эффекты тикают у обеих сторон.
    int mine = tick_effects(plr_.effects);
    if (mine != 0) {
        plr_.hp += mine;
        Stats t2 = total();
        if (plr_.hp > t2.max_hp) plr_.hp = t2.max_hp;
        combat_log(mine < 0 ? "Эффекты отнимают " + to_str(-mine) + " здоровья."
                            : "Эффекты возвращают " + to_str(mine) + " здоровья.");
    }
    int theirs = tick_effects(m->effects);
    if (theirs != 0) {
        cb_.enemy_hp += theirs;
        combat_log(theirs < 0 ? e->name + " теряет " + to_str(-theirs) + " от эффектов."
                              : e->name + " восстанавливает " + to_str(theirs) + ".");
        if (cb_.enemy_hp <= 0) { kill_mob(*m); return; }
        m->hp = cb_.enemy_hp;
    }

    if (plr_.hp <= 0) {
        plr_.hp = 0;
        combat_log("Ты падаешь без сил...");
        cb_.active = false;
        return;
    }

    // Новый раунд.
    plr_.ap = total().max_ap;
    cb_.stance_used = false;
    combat_log("— Новый раунд. Очки действия восстановлены. —");
}

void Game::kill_mob(Mob& m) {
    const EnemyDef* e = Content::get().enemy(m.enemy_id);
    if (!e) { finish_combat(); return; }

    const std::string beaten = e->female ? " повержена" : " повержен";
    combat_log(e->name + beaten + "!");
    msg(e->name + beaten + ".");

    if (m.gold > 0) {
        plr_.gold += m.gold;
        msg("Найдено " + to_str(m.gold) + " " +
            plural(m.gold, "монета", "монеты", "монет") + ".");
    }
    for (const ItemStack& st : m.inv) {
        add_item(st.id, st.count);
        const ItemDef* def = Content::get().item(st.id);
        msg("Добыча: " + std::string(def ? def->name : st.id) +
            (st.count > 1 ? " x" + to_str(st.count) : "") + ".");
    }
    if (!e->kill_counter.empty()) {
        ++plr_.counters[e->kill_counter];
        fire_event(TriggerKind::MobKilled, e->kill_counter);
    }

    int uid = m.uid;
    for (std::size_t i = 0; i < mobs_.size(); ++i) {
        if (mobs_[i].uid == uid) { mobs_.erase(mobs_.begin() + static_cast<long>(i)); break; }
    }

    grant_exp(e->exp);
    finish_combat();
}

void Game::finish_combat() {
    cb_.active = false;
    cb_.mob_uid = -1;
    plr_.momentum = 0;
}
