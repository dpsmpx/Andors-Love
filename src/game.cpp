#include "game.h"

#include <algorithm>

namespace {

int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

} // namespace

Game::Game() : world_("data/maps") {}

void Game::msg(const std::string& m) {
    log_.push_back(m);
    if (log_.size() > 200) log_.erase(log_.begin());
}

// ------------------------------------------------------------- новая партия

void Game::new_game(const std::string& name) {
    plr_ = Player();
    if (!name.empty()) plr_.name = name;

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
    add_item("dagger", 1);
    equip("dagger");

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
    rng_.set_seed(0x5EEDC0DEULL);

    if (const Location* l = here()) spawn_initial(*l);
    msg("Ольховка встречает тебя запахом дыма и мокрой соломы.");
    msg("Нажми ? — список команд.");
}

// -------------------------------------------------------- характеристики

Stats Game::total_no_stance() const {
    Stats t = plr_.base;
    const Content& c = Content::get();
    for (const std::string& id : plr_.equipped) {
        if (id.empty()) continue;
        if (const ItemDef* d = c.item(id)) t += d->bonus;
    }
    for (const auto& kv : plr_.skills) {
        const SkillDef* s = c.skill(kv.first);
        if (!s) continue;
        for (int i = 0; i < kv.second; ++i) t += s->bonus;
    }
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

int Game::exp_to_next() const { return 40 + 35 * plr_.level; }

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

void Game::add_item(const std::string& id, int n) {
    if (n <= 0) return;
    for (ItemStack& s : plr_.inv) {
        if (s.id == id) { s.count += n; return; }
    }
    plr_.inv.push_back(ItemStack{id, n});
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

    Stats t = total();
    int before_hp = plr_.hp, before_ap = plr_.ap;
    plr_.hp = std::min(t.max_hp, plr_.hp + d->heal_hp);
    plr_.ap = std::min(t.max_ap, plr_.ap + d->heal_ap);
    remove_item(id, 1);

    std::string what;
    if (plr_.hp > before_hp) what += "+" + to_str(plr_.hp - before_hp) + " HP";
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

void Game::spawn_initial(const Location& loc) {
    if (visited_.count(loc.id)) return;
    visited_.insert(loc.id);
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
                if (const EnemyDef* e = Content::get().enemy(zone.enemy_id)) m.hp = e->stats.max_hp;
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
                if (const EnemyDef* e = Content::get().enemy(zone.enemy_id)) m.hp = e->stats.max_hp;
                mobs_.push_back(m);
                break;
            }
        }
    }
}

void Game::move_mobs() {
    const Location* loc = here();
    if (!loc) return;
    const Content& c = Content::get();

    for (Mob& m : mobs_) {
        if (m.loc != plr_.loc) continue;              // соседние локации спят
        const EnemyDef* e = c.enemy(m.enemy_id);
        if (!e) continue;

        int d = dist(m.pos, plr_.pos);
        Vec2 step{0, 0};

        if (e->aggressive && d <= e->detect) {
            // Преследование: шаг по оси с большей разницей.
            int dx = plr_.pos.x - m.pos.x, dy = plr_.pos.y - m.pos.y;
            if (std::abs(dx) >= std::abs(dy)) step.x = (dx > 0) - (dx < 0);
            else                              step.y = (dy > 0) - (dy < 0);
        } else if (rng_.chance(45)) {
            switch (rng_.range(0, 3)) {
                case 0: step.x =  1; break;
                case 1: step.x = -1; break;
                case 2: step.y =  1; break;
                default: step.y = -1; break;
            }
        }
        if (step.x == 0 && step.y == 0) continue;

        Vec2 np{ m.pos.x + step.x, m.pos.y + step.y };

        if (np == plr_.pos) {                          // дошёл до игрока — бой
            if (e->aggressive) {
                msg(e->name + " бросается на тебя!");
                start_combat(m.uid);
                return;                                // остальные ждут конца боя
            }
            continue;
        }
        if (!loc->walkable(np)) continue;
        if (loc->npc_at(np) || loc->exit_at(np)) continue;
        if (mob_at(np, m.loc)) continue;
        m.pos = np;
    }
}

bool Game::item_taken(const std::string& loc_id, int index) const {
    return taken_.count(loc_id + ":" + to_str(index)) != 0;
}

void Game::world_turn() {
    ++turn_;
    respawn_tick();
    move_mobs();
}

void Game::rest() {
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

    if (!loc->walkable(np)) return Bump::Blocked;

    if (const MapExit* ex = loc->exit_at(np)) {
        const Location* dst = world_.location(ex->target);
        if (!dst) { msg("Дорога обрывается: " + world_.last_error()); return Bump::Blocked; }
        plr_.loc = dst->id;
        plr_.pos = ex->dest;
        spawn_initial(*dst);
        msg("Ты приходишь в локацию «" + dst->name + "».");
        return Bump::Exit;
    }

    plr_.pos = np;

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

// ------------------------------------------------------------- диалоги

bool Game::option_available(const DlgOption& o) const {
    if (!o.req_quest.empty()) {
        auto it = plr_.quests.find(o.req_quest);
        int st = (it == plr_.quests.end()) ? QUEST_NONE : it->second;
        if (o.req_stage_min >= 0 && st < o.req_stage_min) return false;
        if (o.req_stage_max >= 0 && st > o.req_stage_max) return false;
    }
    if (!o.req_counter.empty()) {
        auto it = plr_.counters.find(o.req_counter);
        int c = (it == plr_.counters.end()) ? 0 : it->second;
        if (o.req_counter_min >= 0 && c < o.req_counter_min) return false;
        if (o.req_counter_max >= 0 && c > o.req_counter_max) return false;
    }
    if (!o.req_item.empty() && count_item(o.req_item) < o.req_item_count) return false;
    return true;
}

void Game::apply_option(const DlgOption& o, const std::string& npc_shop, std::string* shop_out) {
    const Content& c = Content::get();

    if (!o.set_quest.empty()) {
        plr_.quests[o.set_quest] = o.set_stage;
        const QuestDef* q = c.quest(o.set_quest);
        const std::string qname = q ? q->name : o.set_quest;
        if (o.set_stage == QUEST_DONE) msg("Квест завершён: «" + qname + "».");
        else if (o.set_stage == 1)     msg("Взят квест: «" + qname + "».");
        else                           msg("Квест «" + qname + "» продвинулся.");
    }
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
    if (o.give_exp > 0) grant_exp(o.give_exp);
    if (o.open_shop && shop_out) *shop_out = npc_shop;
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
    int dmg = raw - def.armor;
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

    Stats foe = e->stats;
    Stats me  = total();
    int eap = foe.max_ap;
    int atk_cost = foe.ap_atk < 1 ? 1 : foe.ap_atk;

    while (eap >= atk_cost && plr_.hp > 0) {
        eap -= atk_cost;
        std::string line;
        int dmg = resolve_hit(foe, me, 100, false, 0, &line);
        if (dmg > 0) {
            plr_.hp -= dmg;
            if (plr_.momentum > 0) --plr_.momentum;   // пропущенный удар сбивает кураж
            combat_log(e->name + ": " + line + ".");
        } else {
            combat_log(e->name + ": промах.");
        }
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

    int gold = rng_.range(e->gold_min, e->gold_max);
    if (gold > 0) {
        plr_.gold += gold;
        msg("Найдено " + to_str(gold) + " " +
            plural(gold, "монета", "монеты", "монет") + ".");
    }
    for (const Drop& d : e->drops) {
        if (!rng_.chance(d.percent)) continue;
        add_item(d.item, 1);
        const ItemDef* def = Content::get().item(d.item);
        msg("Добыча: " + std::string(def ? def->name : d.item) + ".");
    }
    if (!e->kill_counter.empty()) ++plr_.counters[e->kill_counter];

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
