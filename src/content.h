#pragma once
#include "types.h"

#include <map>
#include <string>
#include <vector>

// Этап квеста: 0 — не взят, 100 — завершён, между ними — прогресс.
constexpr int QUEST_NONE = 0;
constexpr int QUEST_DONE = 100;

// ---------- диалоги ----------
// Диалоги описаны данными, а не кодом: узел + варианты ответа, у каждого
// варианта условия показа и последствия. Этого хватает и на квесты, и на
// торговлю, и на обмен предметов.

struct DlgOption {
    std::string text;
    std::string next;                 // пустая строка — закрыть диалог

    // условия показа
    std::string req_quest;
    int         req_stage_min = -1;   // включительно; -1 — не проверять
    int         req_stage_max = -1;
    // Второе условие по квесту: нужно, чтобы открывать задание только тем,
    // кто уже закрыл предыдущее.
    std::string req_quest2;
    int         req_stage2_min = -1;
    int         req_stage2_max = -1;
    std::string req_counter;
    std::string req_note;                // требуется найденная записка
    int         req_counter_min = -1;
    int         req_counter_max = -1;
    std::string req_item;
    int         req_item_count = 0;

    // последствия выбора
    std::string set_quest;
    int         set_stage  = 0;
    std::string give_item;
    int         give_count = 0;
    std::string take_item;
    int         take_count = 0;
    int         give_gold  = 0;
    int         give_exp   = 0;
    bool        open_shop    = false;
    std::string shop_id;                 // если задан, открывается именно он
    bool        open_enchant = false;
    bool        portal_gift  = false;   // выдаёт умение ставить порталы
    bool        rest       = false;   // восстановить HP/AP
};

struct DlgNode {
    std::string            id;
    std::string            text;
    std::vector<DlgOption> options;
};

// ---------- эффекты ----------

struct EffectDef {
    std::string id;
    std::string name;
    std::string desc;
    EffectKind  kind    = EffectKind::Stat;
    bool        harmful = false;
    int         hp_per_turn = 0;   // для Damage/Heal, умножается на силу
    Stats       per_power;         // для Stat, умножается на силу
};

// ---------- расы и специализации ----------
// Выбираются один раз при создании героя и дальше только складываются
// в итоговые характеристики.

struct RaceDef {
    std::string id;
    std::string name;
    std::string desc;
    Stats       bonus;
};

struct SpecDef {
    std::string id;
    std::string name;
    std::string desc;
    Stats       bonus;
    std::string start_item;   // чем вооружают на старте
    int         start_count = 0;
};

// ---------- зачарования ----------
// Привязываются к коду предмета: все экземпляры одного предмета делят
// зачарование. Для снаряжения, которого по одному, это незаметно, зато
// инвентарь остаётся простым списком стопок.

struct EnchantDef {
    std::string id;
    std::string name;
    std::string desc;
    Stats       bonus;
    std::string on_hit_effect;    // что накладывается при попадании
    int         on_hit_chance = 0;
    int         on_hit_power  = 1;
    int         price = 0;
    std::string reagent;          // предмет-реагент, нужен вдобавок к золоту
    int         reagent_count = 0;
};

// ---------- записки ----------
// Небольшие тексты, разбросанные по миру: часть — просто находки, часть
// нужна по квесту. Подобранная записка попадает в библиотеку только для
// чтения и поднимает счётчик "note_<id>", по которому её можно требовать
// в диалоге.

struct NoteDef {
    std::string              id;
    std::string              title;
    std::vector<std::string> lines;
};

// ---------- существа и NPC ----------

struct NpcDef {
    std::string id;
    std::string name;
    std::string root;     // корневой узел диалога
    std::string shop;     // id магазина, если торгует
};

struct Drop {
    std::string item;
    int         percent = 0;
    Drop() = default;
    Drop(const std::string& i, int p) : item(i), percent(p) {}
};

struct EnemyDef {
    std::string       id;
    std::string       name;
    Stats             stats;
    int               exp = 0;
    int               gold_min = 0;
    int               gold_max = 0;
    std::vector<Drop> drops;
    bool              aggressive = true;
    bool              female = false;   // для согласования: «повержен» / «повержена»
    std::string       on_hit_effect;    // эффект, накладываемый ударом
    int               on_hit_chance = 0;
    int               on_hit_power  = 1;
    std::vector<ActiveEffect> innate;   // эффекты, висящие с рождения
    int               detect = 5;      // радиус обнаружения игрока
    std::string       kill_counter;    // какой счётчик растёт при убийстве
};

// ---------- магазины ----------

struct ShopDef {
    std::string              id;
    std::string              name;
    std::vector<std::string> goods;
    int                      buy_pct  = 100;  // цена покупки = price * buy_pct / 100
    int                      sell_pct = 40;   // цена продажи = price * sell_pct / 100
};

// ---------- квесты ----------

struct QuestStageDef {
    int         stage = 0;
    std::string text;
    QuestStageDef() = default;
    QuestStageDef(int s, const std::string& t) : stage(s), text(t) {}
};

struct QuestDef {
    std::string                id;
    std::string                name;
    std::vector<QuestStageDef> stages;
};

// ---------- навыки ----------

struct SkillDef {
    std::string id;
    std::string name;
    std::string desc;
    Stats       bonus;
    int         max_rank = 5;
};

// ---------- база ----------
// Единственный экземпляр, заполняется один раз при старте.

class Content {
public:
    static const Content& get();

    const ItemDef*   item(const std::string& id)   const;
    const EffectDef* effect(const std::string& id) const;
    const NoteDef*   note(const std::string& id)   const;
    const RaceDef*   race(const std::string& id)   const;
    const SpecDef*   spec(const std::string& id)   const;
    const EnchantDef* enchant(const std::string& id) const;
    const EnemyDef* enemy(const std::string& id) const;
    const NpcDef*   npc(const std::string& id)   const;
    const ShopDef*  shop(const std::string& id)  const;
    const QuestDef* quest(const std::string& id) const;
    const DlgNode*  node(const std::string& id)  const;
    const SkillDef* skill(const std::string& id) const;

    // Текст этапа квеста; пустая строка, если этап не описан.
    std::string quest_stage_text(const std::string& quest_id, int stage) const;

    const std::vector<SkillDef>&   skills()   const { return skills_; }
    const std::vector<QuestDef>&   quests()   const { return quests_; }
    const std::vector<RaceDef>&    races()    const { return races_; }
    const std::vector<SpecDef>&    specs()    const { return specs_; }
    const std::vector<EnchantDef>& enchants() const { return enchants_; }

private:
    Content();
    void build_items();
    void build_effects();
    void build_notes();
    void build_races();
    void build_specs();
    void build_enchants();
    void build_enemies();
    void build_skills();
    void build_quests();
    void build_shops();
    void build_npcs();
    void build_dialogues();

    std::map<std::string, ItemDef>  items_;
    std::map<std::string, EnemyDef> enemies_;
    std::map<std::string, NpcDef>   npcs_;
    std::map<std::string, ShopDef>  shops_;
    std::map<std::string, DlgNode>  nodes_;
    std::map<std::string, EffectDef>  effects_;
    std::map<std::string, NoteDef>    notes_;
    std::map<std::string, EnchantDef>  enchant_map_;
    std::vector<QuestDef>             quests_;
    std::vector<SkillDef>             skills_;
    std::vector<RaceDef>              races_;
    std::vector<SpecDef>              specs_;
    std::vector<EnchantDef>           enchants_;
};
