#pragma once
#include "content.h"
#include "rng.h"
#include "types.h"
#include "world.h"

#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

// ---------------------------------------------------------------- состояние

struct Player {
    std::string name = "Странник";
    int         level = 1;
    int         exp   = 0;
    int         gold  = 30;
    int         skill_points = 0;

    Stats       base;                 // растёт с уровнем
    int         hp = 0;
    int         ap = 0;

    std::string loc = "village";
    Vec2        pos;

    std::string race = "human";
    std::string spec = "swordsman";

    Stance      stance   = Stance::Balanced;
    int         momentum = 0;         // кураж, 0..MOMENTUM_MAX

    std::vector<ActiveEffect> effects;
    // Зачарования привязаны к коду предмета: код -> код зачарования.
    std::map<std::string, std::string> enchants;
    std::vector<Portal>       portals;
    bool                      portal_master = false;

    std::vector<Book>         books;          // библиотека героя
    int                       next_book = 1;  // счётчик для кодов книг

    std::array<std::string, static_cast<std::size_t>(Slot::Count)> equipped;
    std::vector<ItemStack>    inv;
    std::map<std::string,int> skills;    // id навыка -> ранг
    std::map<std::string,int> quests;    // id квеста -> этап
    std::map<std::string,int> counters;  // kill_wolf и прочие счётчики
};

// Что моб сейчас делает. Преследование держится на видимости: пока моб видит
// игрока — идёт за ним, потерял из виду — возвращается в свою зону спавна.
// Без возврата уползший моб продолжал занимать слот зоны и блокировал респавн.
enum class MobState { Idle = 0, Chase, Return };

struct Mob {
    int         uid = 0;
    std::string enemy_id;
    std::string loc;
    Vec2        pos;
    int         hp = 0;
    int         zone = -1;            // индекс зоны спавна в локации
    // Собственный инвентарь: заполняется при появлении и пополняется тем,
    // что моб подобрал с земли. Всё это достаётся победителю.
    std::vector<ItemStack>    inv;
    int                       gold = 0;
    std::vector<ActiveEffect> effects;
    MobState                  state = MobState::Idle;
};

// Бой идёт в отдельном режиме: у игрока пул AP на раунд, действия его тратят.
struct Combat {
    bool                     active = false;
    int                      mob_uid = -1;
    int                      enemy_hp = 0;
    bool                     stance_used = false;   // смена стойки — раз за раунд
    std::vector<std::string> log;
};

constexpr int MOMENTUM_MAX  = 5;
constexpr int MOMENTUM_COST = 3;      // цена мощного удара
constexpr int AP_ITEM_COST  = 3;
constexpr int RESPAWN_TURNS = 40;

enum class Bump {
    Moved, Blocked, Npc, Sign, Bed, Exit, Item, Combat, Chest, Portal, Note
};

constexpr int PORTAL_LIMIT = 4;

// ------------------------------------------------------------------- играть

class Game {
public:
    explicit Game(const std::string& data_root = "data/maps");

    // --- жизненный цикл ---
    void new_game(const std::string& name,
                  const std::string& race = "human",
                  const std::string& spec = "swordsman");
    bool load_from(const std::string& path);
    bool save_to(const std::string& path) const;

    // --- доступ ---
    Player&         player()       { return plr_; }
    const Player&   player() const { return plr_; }
    const World&    world() const  { return world_; }
    const Location* here() const   { return world_.location(plr_.loc); }
    Combat&         combat()       { return cb_; }
    const Combat&   combat() const { return cb_; }
    Rng&            rng()          { return rng_; }
    int             turn() const   { return turn_; }

    const std::vector<Mob>& mobs() const { return mobs_; }
    Mob*        mob_at(Vec2 p, const std::string& loc);
    const Mob*  mob_by_uid(int uid) const;
    Mob*        mob_by_uid(int uid);

    // --- эффекты ---
    // Наложение поверх такого же эффекта продлевает его и берёт большую силу,
    // а не копит дубликаты в списке.
    static void apply_effect(std::vector<ActiveEffect>& list, const std::string& id,
                             int turns, int power);
    static Stats effect_stats(const std::vector<ActiveEffect>& list);
    // Прокручивает один ход: возвращает изменение здоровья (может быть < 0).
    static int  tick_effects(std::vector<ActiveEffect>& list);
    // which == "*" — снять все вредные; иначе конкретный эффект.
    static int  cure_effects(std::vector<ActiveEffect>& list, const std::string& which);
    static std::string effects_line(const std::vector<ActiveEffect>& list);

    // --- книги и записки ---
    const std::vector<Book>& books() const { return plr_.books; }
    Book*       book(const std::string& id);
    const Book* book(const std::string& id) const;
    // Тратит чистую книгу и заводит новую запись в библиотеке.
    bool  start_book(const std::string& title);
    bool  delete_book(const std::string& id);
    // Правки идут через игру, а не напрямую по указателю: здесь же
    // проверяются пределы длины и запрет на правку найденных записок.
    bool  book_set_title(const std::string& id, const std::string& title);
    bool  book_set_line(const std::string& id, int index, const std::string& text);
    bool  book_insert_line(const std::string& id, int index, const std::string& text);
    bool  book_remove_line(const std::string& id, int index);

    bool  note_taken(const std::string& loc_id, int index) const;
    bool  take_note(int index);          // записка в текущей локации

    // --- сундуки ---
    bool chest_opened(const std::string& loc_id, int index) const;
    bool open_chest(int index);          // сундук в текущей локации

    // --- порталы ---
    const Portal* portal_at(Vec2 p, const std::string& loc_id) const;
    bool place_portal();
    bool remove_portal_here();

    // --- зачарование ---
    Stats enchant_bonus() const;         // от надетых вещей
    bool  can_enchant(const std::string& item_id) const;
    bool  enchant_item(const std::string& item_id, const std::string& ench_id);

    // --- характеристики ---
    Stats total() const;                    // база + снаряжение + навыки + стойка
    Stats total_no_stance() const;
    int   attack_cost() const;              // стоимость атаки в AP, не меньше 1
    int   exp_to_next() const;

    // --- перемещение и мир ---
    Bump  try_move(int dx, int dy);
    // Предмет с карты уже подобран? Нужно отрисовке, чтобы он исчез.
    bool  item_taken(const std::string& loc_id, int index) const;
    void  world_turn();                     // ход мира: мобы, респавн
    void  rest();                           // койка: восстановить HP/AP

    // --- инвентарь ---
    int   count_item(const std::string& id) const;
    // Сколько предмета при игроке всего: в сумке плюс надетое. Врата должны
    // считать именно так — ключ на пальце остаётся ключом, и надетое кольцо
    // не должно запирать дверь, которую само же и открывает.
    int   carries_item(const std::string& id) const;
    void  add_item(const std::string& id, int n);
    bool  remove_item(const std::string& id, int n);
    bool  equip(const std::string& id);
    bool  unequip(Slot s);
    bool  use_item(const std::string& id);  // расходники
    bool  drop_item(const std::string& id);

    // --- прокачка ---
    void  grant_exp(int amount);
    bool  learn_skill(const std::string& id);

    // --- события, открывающие квесты ---
    // Вызывается из мест, где что-то происходит: находка, убийство, добыча,
    // вход в локацию. Проверяет таблицу триггеров и открывает подходящие
    // квесты.
    void  fire_event(TriggerKind kind, const std::string& key);
    // Перепроверяет триггеры по состоянию (предмет на руках, число убитых).
    // «Ключ у меня» — это состояние, а не миг: игрок мог добыть его раньше,
    // чем узнал, зачем он нужен, и цепочка не должна на этом застревать.
    void  recheck_state_triggers();
    int   quest_stage(const std::string& id) const;

    // --- диалоги ---
    bool  option_available(const DlgOption& o) const;
    // Применяет последствия варианта. shop_out — id магазина, если открылась торговля.
    void  apply_option(const DlgOption& o, const std::string& npc_shop,
                       std::string* shop_out, bool* enchant_out = nullptr);

    // --- торговля ---
    int   buy_price(const ShopDef& s, const ItemDef& d) const;
    int   sell_price(const ShopDef& s, const ItemDef& d) const;
    bool  buy(const ShopDef& s, const std::string& item_id);
    bool  sell(const ShopDef& s, const std::string& item_id);

    // --- бой ---
    void  start_combat(int mob_uid);
    void  combat_attack(bool power);
    void  combat_set_stance(Stance s);
    bool  combat_use_item(const std::string& id);
    void  combat_end_turn();                // передать ход врагу
    bool  combat_flee();                    // true — удалось сбежать
    bool  player_dead() const { return plr_.hp <= 0; }

    // --- сообщения ---
    void  msg(const std::string& m);
    const std::vector<std::string>& log() const { return log_; }
    void  clear_log() { log_.clear(); }

    const std::string& error() const { return err_; }

private:
    void  fill_mob_inventory(Mob& m, const EnemyDef& e);
    void  spawn_initial(const Location& loc);
    void  respawn_tick();
    void  move_mobs();
    // Может ли моб занять клетку: проходима, не занята NPC, переходом,
    // закрытым сундуком или другим мобом. Клетка игрока сюда не входит —
    // шаг на неё означает бой и разбирается отдельно.
    bool  mob_can_stand(const Location& loc, Vec2 p, int self_uid) const;
    // Шаг к цели: сперва по оси с большей разницей, затем по другой. Этого
    // хватает, чтобы обойти угол, и не требует полноценного поиска пути.
    Vec2  step_toward(const Location& loc, const Mob& m, Vec2 target) const;
    void  kill_mob(Mob& m);
    int   roll_damage(const Stats& atk, int stance_pct, bool crit) const;
    // Возвращает нанесённый урон, -1 — промах/блок. Заполняет line описанием.
    int   resolve_hit(const Stats& atk, const Stats& def, int stance_pct,
                      bool guaranteed, int crit_bonus, std::string* line);
    void  combat_log(const std::string& s);
    void  enemy_turn();
    void  finish_combat();

    World                    world_;
    Player                   plr_;
    std::vector<Mob>         mobs_;
    Combat                   cb_;
    Rng                      rng_;
    int                      next_uid_ = 1;
    int                      turn_ = 0;
    int                      respawn_left_ = RESPAWN_TURNS;
    std::set<std::string>    taken_;      // "локация:индекс" подобранных предметов
    std::set<std::string>    chests_;     // "локация:индекс" вскрытых сундуков
    std::set<std::string>    notes_;      // "локация:индекс" подобранных записок
    std::set<std::string>    visited_;    // локации, где уже расставлены мобы
    std::vector<std::string> log_;
    mutable std::string      err_;

    friend class SaveIO;
};
