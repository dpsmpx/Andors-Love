#include "content.h"

namespace {

ItemDef mk_item(const std::string& id, const std::string& name, ItemKind kind,
                int price, const std::string& desc) {
    ItemDef d;
    d.id = id; d.name = name; d.kind = kind; d.price = price; d.desc = desc;
    return d;
}

} // namespace

const Content& Content::get() {
    static const Content instance;
    return instance;
}

Content::Content() {
    build_effects();
    build_races();
    build_specs();
    build_items();
    build_enchants();
    build_enemies();
    build_skills();
    build_quests();
    build_shops();
    build_npcs();
    build_dialogues();
}

// ------------------------------------------------------------------ предметы

void Content::build_items() {
    auto add = [&](ItemDef d) { items_[d.id] = d; };

    // расходники
    ItemDef bread = mk_item("bread", "Ржаной хлеб", ItemKind::Consumable, 6,
                            "Восстанавливает 8 здоровья.");
    bread.heal_hp = 8;
    add(bread);

    ItemDef potion = mk_item("herb_potion", "Настой трав", ItemKind::Consumable, 25,
                             "Восстанавливает 22 здоровья.");
    potion.heal_hp = 22;
    add(potion);

    ItemDef tonic = mk_item("ap_tonic", "Тоник бодрости", ItemKind::Consumable, 30,
                            "Возвращает 5 очков действия прямо в бою.");
    tonic.heal_ap = 5;
    add(tonic);

    // оружие
    ItemDef dagger = mk_item("dagger", "Кинжал", ItemKind::Weapon, 40,
                             "Лёгкий и быстрый: атака дешевле на 1 AP.");
    dagger.bonus.dmg_min = 2; dagger.bonus.dmg_max = 4;
    dagger.bonus.attack = 5;  dagger.bonus.ap_atk = -1;
    add(dagger);

    ItemDef sword = mk_item("short_sword", "Короткий меч", ItemKind::Weapon, 95,
                            "Надёжный клинок без изысков.");
    sword.bonus.dmg_min = 3; sword.bonus.dmg_max = 6;
    add(sword);

    ItemDef axe = mk_item("axe", "Топор дровосека", ItemKind::Weapon, 170,
                          "Бьёт тяжело, но замахивается медленно: +1 AP на атаку.");
    axe.bonus.dmg_min = 5; axe.bonus.dmg_max = 10;
    axe.bonus.attack = -5; axe.bonus.ap_atk = 1;
    add(axe);

    ItemDef spear = mk_item("spear", "Копьё", ItemKind::Weapon, 210,
                            "Длинное древко даёт запас по меткости.");
    spear.bonus.dmg_min = 4; spear.bonus.dmg_max = 8; spear.bonus.attack = 8;
    add(spear);

    // броня
    ItemDef leather = mk_item("leather_armor", "Кожаный доспех", ItemKind::Armor, 85,
                              "Простая, но честная защита.");
    leather.bonus.armor = 2;
    add(leather);

    ItemDef chain = mk_item("chain_armor", "Кольчуга", ItemKind::Armor, 280,
                            "Работа кузнеца Брана. На заказ не продаётся.");
    chain.bonus.armor = 4; chain.bonus.block = 3;
    add(chain);

    ItemDef cap = mk_item("cap", "Войлочная шапка", ItemKind::Helmet, 30, "Лучше, чем ничего.");
    cap.bonus.armor = 1;
    add(cap);

    ItemDef helm = mk_item("helm", "Железный шлем", ItemKind::Helmet, 130, "Голова целее.");
    helm.bonus.armor = 2; helm.bonus.block = 2;
    add(helm);

    ItemDef buckler = mk_item("buckler", "Баклер", ItemKind::Shield, 75,
                              "Маленький щит, заметно поднимает шанс блока.");
    buckler.bonus.block = 8;
    add(buckler);

    // кольца
    ItemDef ring_hp = mk_item("ring_hp", "Кольцо жизни", ItemKind::Ring, 150,
                              "Носящий чувствует себя крепче.");
    ring_hp.bonus.max_hp = 10;
    add(ring_hp);

    ItemDef ring_crit = mk_item("ring_crit", "Кольцо охотника", ItemKind::Ring, 195,
                                "Рука сама находит слабое место.");
    ring_crit.bonus.crit = 6; ring_crit.bonus.attack = 3;
    add(ring_crit);

    // оружие специализаций
    ItemDef bow = mk_item("short_bow", "Короткий лук", ItemKind::Weapon, 120,
                          "Бьёт точно, хотя и не тяжело.");
    bow.bonus.dmg_min = 2; bow.bonus.dmg_max = 5; bow.bonus.attack = 10;
    add(bow);

    ItemDef staff = mk_item("staff", "Посох", ItemKind::Weapon, 130,
                            "Даёт лишнее очко действия в раунд.");
    staff.bonus.dmg_min = 2; staff.bonus.dmg_max = 4;
    staff.bonus.attack = 3;  staff.bonus.max_ap = 1;
    add(staff);

    // снаряжение поздних локаций
    ItemDef plate = mk_item("plate_armor", "Латный доспех", ItemKind::Armor, 520,
                            "Тяжёлый, зато почти непробиваемый.");
    plate.bonus.armor = 7; plate.bonus.block = 4; plate.bonus.ap_atk = 1;
    add(plate);

    ItemDef tower = mk_item("tower_shield", "Ростовой щит", ItemKind::Shield, 300,
                            "За таким можно переждать что угодно.");
    tower.bonus.block = 15; tower.bonus.armor = 1; tower.bonus.attack = -4;
    add(tower);

    ItemDef sabre = mk_item("sabre", "Сабля атамана", ItemKind::Weapon, 420,
                            "Трофей с заставы. Лёгкая и злая.");
    sabre.bonus.dmg_min = 6; sabre.bonus.dmg_max = 11;
    sabre.bonus.attack = 6; sabre.bonus.crit = 4;
    add(sabre);

    ItemDef ring_ward = mk_item("ring_ward", "Кольцо оберега", ItemKind::Ring, 340,
                                "Тёплое на ощупь даже в мороз.");
    ring_ward.bonus.armor = 2; ring_ward.bonus.block = 6;
    add(ring_ward);

    // расходники с эффектами
    ItemDef antidote = mk_item("antidote", "Противоядие", ItemKind::Consumable, 40,
                               "Снимает яд и прочую отраву.");
    antidote.cures = "*";
    add(antidote);

    ItemDef salve = mk_item("salve", "Целебная мазь", ItemKind::Consumable, 55,
                            "Затягивает раны понемногу, но долго.");
    salve.effect = "regen"; salve.effect_turns = 8; salve.effect_power = 1;
    add(salve);

    ItemDef el_might = mk_item("elixir_might", "Эликсир силы", ItemKind::Consumable, 70,
                               "Удар тяжелеет на десяток ходов.");
    el_might.effect = "might"; el_might.effect_turns = 10; el_might.effect_power = 2;
    add(el_might);

    ItemDef el_guard = mk_item("elixir_guard", "Эликсир стойкости", ItemKind::Consumable, 70,
                               "Держать удар становится заметно легче.");
    el_guard.effect = "guard"; el_guard.effect_turns = 10; el_guard.effect_power = 2;
    add(el_guard);

    ItemDef el_haste = mk_item("elixir_haste", "Настой скорости", ItemKind::Consumable, 90,
                               "Атака дешевеет на очко действия.");
    el_haste.effect = "haste"; el_haste.effect_turns = 8; el_haste.effect_power = 1;
    add(el_haste);

    // реагенты для зачарования
    add(mk_item("ember",       "Негасимый уголёк", ItemKind::Misc, 60,
                "Тёплый и не гаснет. Реагент для «Пламени»."));
    add(mk_item("frost_shard", "Осколок наледи",   ItemKind::Misc, 55,
                "Не тает в руке. Реагент для «Мороза»."));
    add(mk_item("venom_gland", "Ядовитая железа",  ItemKind::Misc, 50,
                "Из паучьего брюшка. Реагент для «Яда»."));
    add(mk_item("whetstone",   "Точильный камень", ItemKind::Misc, 65,
                "Правит любое лезвие. Реагент для «Остроты»."));
    add(mk_item("rune_stone",  "Рунный камень",    ItemKind::Misc, 80,
                "Тёплый на ощупь. Реагент для «Оберега»."));

    ItemDef pstone = mk_item("portal_stone", "Портальный камень", ItemKind::Misc, 250,
                             "Ставится на землю и связывается с другими такими же.");
    add(pstone);

    // ключи и квестовое
    add(mk_item("rusty_key",  "Ржавый ключ", ItemKind::Misc, 0,
                "От сундука на старой заставе."));
    add(mk_item("glow_moss",  "Светящийся мох", ItemKind::Misc, 0,
                "Растёт только в глубине пещеры."));
    add(mk_item("focus_node", "Узловой фокус", ItemKind::Misc, 0,
                "Гранёный камень. Таких нужно три."));

    // разное и квестовое
    add(mk_item("wolf_pelt", "Волчья шкура", ItemKind::Misc, 18, "Кузнец такие принимает."));
    add(mk_item("rat_tail",  "Крысиный хвост", ItemKind::Misc, 4,  "Ценности немного."));
    add(mk_item("torch",     "Факел", ItemKind::Misc, 10, "В лесу спокойнее."));
    add(mk_item("amulet",    "Амулет Лады", ItemKind::Misc, 0,
                "Медный амулет с зелёным камнем. Явно чужой."));
}

// ------------------------------------------------------------------ эффекты

void Content::build_effects() {
    auto add = [&](EffectDef d) { effects_[d.id] = d; };

    EffectDef e;

    e = EffectDef(); e.id = "poison"; e.name = "Яд"; e.kind = EffectKind::Damage;
    e.harmful = true; e.hp_per_turn = -2;
    e.desc = "Отнимает здоровье каждый ход.";
    add(e);

    e = EffectDef(); e.id = "bleed"; e.name = "Кровотечение"; e.kind = EffectKind::Damage;
    e.harmful = true; e.hp_per_turn = -3;
    e.desc = "Рана не закрывается сама.";
    add(e);

    e = EffectDef(); e.id = "burn"; e.name = "Горение"; e.kind = EffectKind::Damage;
    e.harmful = true; e.hp_per_turn = -4;
    e.desc = "Жжёт сильно, но недолго.";
    add(e);

    e = EffectDef(); e.id = "regen"; e.name = "Регенерация"; e.kind = EffectKind::Heal;
    e.hp_per_turn = 3;
    e.desc = "Восстанавливает здоровье каждый ход.";
    add(e);

    e = EffectDef(); e.id = "might"; e.name = "Сила"; e.kind = EffectKind::Stat;
    e.per_power.dmg_min = 1; e.per_power.dmg_max = 2;
    e.desc = "Удар тяжелее.";
    add(e);

    e = EffectDef(); e.id = "guard"; e.name = "Стойкость"; e.kind = EffectKind::Stat;
    e.per_power.block = 5; e.per_power.armor = 1;
    e.desc = "Держать удар легче.";
    add(e);

    e = EffectDef(); e.id = "haste"; e.name = "Скорость"; e.kind = EffectKind::Stat;
    e.per_power.ap_atk = -1;
    e.desc = "Атака дешевле на очко действия.";
    add(e);

    e = EffectDef(); e.id = "weaken"; e.name = "Слабость"; e.kind = EffectKind::Stat;
    e.harmful = true; e.per_power.attack = -8;
    e.desc = "Рука не слушается, попасть труднее.";
    add(e);

    e = EffectDef(); e.id = "slow"; e.name = "Оцепенение"; e.kind = EffectKind::Stat;
    e.harmful = true; e.per_power.ap_atk = 1;
    e.desc = "Каждый замах даётся дороже.";
    add(e);

    e = EffectDef(); e.id = "clarity"; e.name = "Прозрение"; e.kind = EffectKind::Stat;
    e.per_power.crit = 6; e.per_power.attack = 4;
    e.desc = "Слабое место само бросается в глаза.";
    add(e);
}

// -------------------------------------------------------------------- расы

void Content::build_races() {
    auto add = [&](const std::string& id, const std::string& name,
                   const std::string& desc, Stats b) {
        RaceDef r; r.id = id; r.name = name; r.desc = desc; r.bonus = b;
        races_.push_back(r);
    };

    Stats b;
    b = Stats(); b.max_hp = 2; b.attack = 3; b.crit = 2;
    add("human", "Человек", "Без крайностей: немного крепче и точнее прочих.", b);

    b = Stats(); b.max_hp = 7; b.armor = 2; b.attack = -4;
    add("dwarf", "Дворф", "Коренаст и вынослив, но неповоротлив в замахе.", b);

    b = Stats(); b.max_hp = 4; b.dmg_min = 1; b.dmg_max = 2; b.block = -5;
    add("orc", "Орк", "Бьёт тяжело, защищается плохо.", b);

    b = Stats(); b.max_hp = 3; b.block = 6; b.armor = 1; b.crit = -2;
    add("lizard", "Ящер", "Чешуя держит удар, зато точность страдает.", b);

    b = Stats(); b.max_hp = -3; b.attack = 7; b.crit = 5;
    add("elf", "Эльф", "Меток и быстр, но хрупок.", b);
}

// -------------------------------------------------------- специализации

void Content::build_specs() {
    auto add = [&](const std::string& id, const std::string& name,
                   const std::string& desc, Stats b,
                   const std::string& item, int count) {
        SpecDef sp; sp.id = id; sp.name = name; sp.desc = desc; sp.bonus = b;
        sp.start_item = item; sp.start_count = count;
        specs_.push_back(sp);
    };

    Stats b;
    b = Stats(); b.max_hp = 5; b.dmg_min = 1; b.dmg_max = 1; b.attack = 4;
    add("swordsman", "Мечник", "Ровный боец: урон и живучесть.", b, "short_sword", 1);

    b = Stats(); b.attack = 9; b.crit = 6; b.max_hp = -2;
    add("archer", "Лучник", "Бьёт точно и часто уходит в крит.", b, "short_bow", 1);

    b = Stats(); b.attack = 5; b.dmg_min = 1; b.dmg_max = 2; b.block = 3;
    add("spearman", "Копейщик", "Держит дистанцию: урон и защита разом.", b, "spear", 1);

    b = Stats(); b.max_ap = 3; b.crit = 4;
    add("mage", "Маг", "Больше очков действия — больше действий за раунд.", b, "staff", 1);

    b = Stats(); b.ap_atk = -1; b.crit = 7; b.block = 4;
    add("ninja", "Ниндзя", "Атака дешевле на очко действия.", b, "dagger", 1);
}

// ------------------------------------------------------------ зачарования

void Content::build_enchants() {
    auto add = [&](EnchantDef d) { enchants_.push_back(d); enchant_map_[d.id] = d; };

    EnchantDef e;

    e = EnchantDef(); e.id = "flame"; e.name = "Пламя";
    e.desc = "Урон выше, удар иногда поджигает.";
    e.bonus.dmg_min = 1; e.bonus.dmg_max = 2;
    e.on_hit_effect = "burn"; e.on_hit_chance = 25; e.on_hit_power = 1;
    e.price = 180; e.reagent = "ember"; e.reagent_count = 1;
    add(e);

    e = EnchantDef(); e.id = "frost"; e.name = "Мороз";
    e.desc = "Сковывает противника, замедляя его замах.";
    e.bonus.block = 4;
    e.on_hit_effect = "slow"; e.on_hit_chance = 30; e.on_hit_power = 1;
    e.price = 160; e.reagent = "frost_shard"; e.reagent_count = 1;
    add(e);

    e = EnchantDef(); e.id = "venom"; e.name = "Яд";
    e.desc = "Клинок отравлен: рана продолжает работать после удара.";
    e.bonus.attack = 3;
    e.on_hit_effect = "poison"; e.on_hit_chance = 35; e.on_hit_power = 2;
    e.price = 150; e.reagent = "venom_gland"; e.reagent_count = 1;
    add(e);

    e = EnchantDef(); e.id = "keen"; e.name = "Острота";
    e.desc = "Точность и шанс критического удара.";
    e.bonus.attack = 6; e.bonus.crit = 5;
    e.price = 200; e.reagent = "whetstone"; e.reagent_count = 1;
    add(e);

    e = EnchantDef(); e.id = "ward"; e.name = "Оберег";
    e.desc = "Руна отводит удар.";
    e.bonus.armor = 2; e.bonus.block = 4;
    e.price = 220; e.reagent = "rune_stone"; e.reagent_count = 1;
    add(e);
}

// -------------------------------------------------------------------- враги

void Content::build_enemies() {
    auto add = [&](EnemyDef d) { enemies_[d.id] = d; };

    EnemyDef rat;
    rat.id = "rat"; rat.name = "Амбарная крыса";
    rat.stats.max_hp = 9;  rat.stats.max_ap = 6; rat.stats.attack = 55;
    rat.stats.dmg_min = 1; rat.stats.dmg_max = 3; rat.stats.block = 2;
    rat.stats.ap_atk = 3;
    rat.exp = 5; rat.gold_min = 0; rat.gold_max = 3;
    rat.drops = { {"rat_tail", 60} };
    rat.detect = 4; rat.kill_counter = "kill_rat"; rat.female = true;
    add(rat);

    EnemyDef wolf;
    wolf.id = "wolf"; wolf.name = "Лесной волк";
    wolf.stats.max_hp = 20; wolf.stats.max_ap = 8; wolf.stats.attack = 65;
    wolf.stats.dmg_min = 2; wolf.stats.dmg_max = 5; wolf.stats.block = 5;
    wolf.stats.armor = 1;   wolf.stats.ap_atk = 4;
    wolf.exp = 14; wolf.gold_min = 0; wolf.gold_max = 6;
    wolf.drops = { {"wolf_pelt", 70} };
    wolf.detect = 6; wolf.kill_counter = "kill_wolf";
    add(wolf);

    EnemyDef bandit;
    bandit.id = "bandit"; bandit.name = "Разбойник";
    bandit.stats.max_hp = 28; bandit.stats.max_ap = 8; bandit.stats.attack = 70;
    bandit.stats.dmg_min = 3; bandit.stats.dmg_max = 7; bandit.stats.block = 10;
    bandit.stats.armor = 2;   bandit.stats.ap_atk = 4;
    bandit.exp = 26; bandit.gold_min = 10; bandit.gold_max = 24;
    bandit.drops = { {"dagger", 25}, {"bread", 40} };
    bandit.detect = 6; bandit.kill_counter = "kill_bandit";
    add(bandit);

    EnemyDef spider;
    spider.id = "spider"; spider.name = "Пещерный паук";
    spider.stats.max_hp = 24; spider.stats.max_ap = 8; spider.stats.attack = 68;
    spider.stats.dmg_min = 2; spider.stats.dmg_max = 6; spider.stats.block = 6;
    spider.stats.armor = 1;   spider.stats.ap_atk = 4;
    spider.exp = 20; spider.gold_min = 0; spider.gold_max = 8;
    spider.drops = { Drop("venom_gland", 45) };
    spider.detect = 6; spider.kill_counter = "kill_spider";
    spider.on_hit_effect = "poison"; spider.on_hit_chance = 30; spider.on_hit_power = 2;
    add(spider);

    EnemyDef bat;
    bat.id = "bat"; bat.name = "Пещерная мышь"; bat.female = true;
    bat.stats.max_hp = 14; bat.stats.max_ap = 9; bat.stats.attack = 62;
    bat.stats.dmg_min = 1; bat.stats.dmg_max = 4; bat.stats.block = 12;
    bat.stats.ap_atk = 3;
    bat.exp = 11; bat.gold_min = 0; bat.gold_max = 4;
    bat.detect = 7; bat.kill_counter = "kill_bat";
    add(bat);

    EnemyDef queen;
    queen.id = "spider_queen"; queen.name = "Паучья матка"; queen.female = true;
    queen.stats.max_hp = 80; queen.stats.max_ap = 10; queen.stats.attack = 78;
    queen.stats.dmg_min = 6; queen.stats.dmg_max = 12; queen.stats.block = 14;
    queen.stats.armor = 4;    queen.stats.ap_atk = 4;
    queen.exp = 120; queen.gold_min = 40; queen.gold_max = 70;
    queen.drops = { Drop("venom_gland", 100), Drop("focus_node", 100),
                    Drop("ring_ward", 35) };
    queen.detect = 7; queen.kill_counter = "kill_queen";
    queen.on_hit_effect = "poison"; queen.on_hit_chance = 45; queen.on_hit_power = 3;
    add(queen);

    EnemyDef brigand;
    brigand.id = "brigand"; brigand.name = "Дозорный заставы";
    brigand.stats.max_hp = 38; brigand.stats.max_ap = 9; brigand.stats.attack = 74;
    brigand.stats.dmg_min = 4; brigand.stats.dmg_max = 9; brigand.stats.block = 14;
    brigand.stats.armor = 3;   brigand.stats.ap_atk = 4;
    brigand.exp = 38; brigand.gold_min = 14; brigand.gold_max = 32;
    brigand.drops = { Drop("whetstone", 35), Drop("bread", 40), Drop("herb_potion", 25) };
    brigand.detect = 6; brigand.kill_counter = "kill_brigand";
    brigand.on_hit_effect = "bleed"; brigand.on_hit_chance = 20; brigand.on_hit_power = 1;
    add(brigand);

    EnemyDef chief;
    chief.id = "bandit_chief"; chief.name = "Атаман Кривой";
    chief.stats.max_hp = 95; chief.stats.max_ap = 11; chief.stats.attack = 82;
    chief.stats.dmg_min = 7; chief.stats.dmg_max = 14; chief.stats.block = 18;
    chief.stats.armor = 5;    chief.stats.ap_atk = 4;
    chief.exp = 160; chief.gold_min = 70; chief.gold_max = 120;
    chief.drops = { Drop("sabre", 100), Drop("rusty_key", 100), Drop("focus_node", 100) };
    chief.detect = 7; chief.kill_counter = "kill_chief";
    chief.on_hit_effect = "bleed"; chief.on_hit_chance = 35; chief.on_hit_power = 2;
    add(chief);

    EnemyDef wraith;
    wraith.id = "wraith"; wraith.name = "Тень нулевой точки"; wraith.female = true;
    wraith.stats.max_hp = 55; wraith.stats.max_ap = 10; wraith.stats.attack = 80;
    wraith.stats.dmg_min = 5; wraith.stats.dmg_max = 10; wraith.stats.block = 20;
    wraith.stats.armor = 2;   wraith.stats.ap_atk = 4;
    wraith.exp = 90; wraith.gold_min = 20; wraith.gold_max = 45;
    wraith.drops = { Drop("rune_stone", 50), Drop("frost_shard", 40) };
    wraith.detect = 8; wraith.kill_counter = "kill_wraith";
    wraith.on_hit_effect = "weaken"; wraith.on_hit_chance = 40; wraith.on_hit_power = 1;
    add(wraith);

    EnemyDef keeper;
    keeper.id = "keeper"; keeper.name = "Страж нулевой точки";
    keeper.stats.max_hp = 120; keeper.stats.max_ap = 11; keeper.stats.attack = 85;
    keeper.stats.dmg_min = 8; keeper.stats.dmg_max = 16; keeper.stats.block = 20;
    keeper.stats.armor = 6;    keeper.stats.ap_atk = 4;
    keeper.exp = 220; keeper.gold_min = 90; keeper.gold_max = 160;
    keeper.drops = { Drop("focus_node", 100), Drop("rune_stone", 100),
                     Drop("plate_armor", 40) };
    keeper.detect = 8; keeper.kill_counter = "kill_keeper";
    keeper.on_hit_effect = "slow"; keeper.on_hit_chance = 35; keeper.on_hit_power = 1;
    add(keeper);

    // Вожак носит амулет Лады — цель квеста, а не случайная добыча.
    EnemyDef alpha;
    alpha.id = "wolf_alpha"; alpha.name = "Вожак стаи";
    alpha.stats.max_hp = 48; alpha.stats.max_ap = 10; alpha.stats.attack = 75;
    alpha.stats.dmg_min = 5; alpha.stats.dmg_max = 10; alpha.stats.block = 12;
    alpha.stats.armor = 3;   alpha.stats.ap_atk = 4;
    alpha.exp = 70; alpha.gold_min = 25; alpha.gold_max = 45;
    alpha.drops = { {"amulet", 100}, {"wolf_pelt", 100} };
    alpha.detect = 7; alpha.kill_counter = "kill_wolf";
    add(alpha);
}

// ------------------------------------------------------------------- навыки

void Content::build_skills() {
    auto add = [&](const std::string& id, const std::string& name,
                   const std::string& desc, Stats b) {
        SkillDef s; s.id = id; s.name = name; s.desc = desc; s.bonus = b;
        skills_.push_back(s);
    };

    Stats s;
    s = Stats(); s.dmg_min = 1; s.dmg_max = 2; add("might",  "Мощь",     "+1/+2 к урону",       s);
    s = Stats(); s.max_hp  = 7;                add("vigor",  "Крепость", "+7 к здоровью",       s);
    s = Stats(); s.attack  = 6;                add("focus",  "Меткость", "+6% к попаданию",     s);
    s = Stats(); s.block   = 5;                add("guard",  "Защита",   "+5% к блоку",         s);
    s = Stats(); s.armor   = 1;                add("temper", "Закалка",  "+1 к броне",          s);
    s = Stats(); s.max_ap  = 2;                add("breath", "Дыхание",  "+2 очка действия",    s);
    s = Stats(); s.crit    = 5;                add("edge",   "Острота",  "+5% к криту",         s);
}

// ------------------------------------------------------------------- квесты

void Content::build_quests() {
    QuestDef q;

    q = QuestDef();
    q.id = "wolves"; q.name = "Волчья напасть";
    q.stages = {
        {1,          "Старейшина Мирон просит перебить 5 лесных волков."},
        {2,          "Волки перебиты. Вернуться к Мирону за наградой."},
        {QUEST_DONE, "Мирон заплатил за работу. Деревня спит спокойнее."}
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "amulet"; q.name = "Амулет Лады";
    q.stages = {
        {1,          "Травница Лада потеряла амулет в лесу. Волки утащили его к вожаку."},
        {QUEST_DONE, "Амулет вернулся к хозяйке."}
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "moss"; q.name = "Тьма под корнями";
    q.stages = {
        QuestStageDef(1, "Ладе нужны 3 пучка светящегося мха из Барсучьей пещеры."),
        QuestStageDef(QUEST_DONE, "Мох собран, Лада варит из него мази.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "queen"; q.name = "Паучья матка";
    q.stages = {
        QuestStageDef(1, "Убить паучью матку в глубине Барсучьей пещеры."),
        QuestStageDef(QUEST_DONE, "Матка мертва, пещера затихла.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "outpost"; q.name = "Застава";
    q.stages = {
        QuestStageDef(1, "Разобраться с атаманом Кривым на развалинах заставы."),
        QuestStageDef(QUEST_DONE, "Атаман повержен, дорога на юг свободна.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "zero_point"; q.name = "Мастер нулевой точки";
    q.stages = {
        QuestStageDef(1, "Собрать 3 узловых фокуса. Их носят те, кто сторожит линии."),
        QuestStageDef(QUEST_DONE, "Отшельник передал умение ставить порталы.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "enchanter"; q.name = "Ремесло Вельда";
    q.stages = {
        QuestStageDef(1, "Вельду нужен рунный камень из святилища."),
        QuestStageDef(QUEST_DONE, "Вельд взялся зачаровывать снаряжение.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "pelts"; q.name = "Заказ кузнеца";
    q.stages = {
        {1,          "Кузнец Бран ждёт 3 волчьи шкуры для подбоя кольчуги."},
        {QUEST_DONE, "Кольчуга получена."}
    };
    quests_.push_back(q);
}

// ----------------------------------------------------------------- магазины

void Content::build_shops() {
    ShopDef s;

    s = ShopDef();
    s.id = "shop_smith"; s.name = "Кузница Брана";
    s.goods = {"dagger", "short_sword", "axe", "spear", "short_bow", "staff",
               "leather_armor", "cap", "helm", "buckler", "tower_shield"};
    s.buy_pct = 100; s.sell_pct = 45;
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_general"; s.name = "Лавка Гурия";
    s.goods = {"bread", "torch", "herb_potion", "ap_tonic", "antidote",
               "salve", "ring_hp", "ring_crit", "portal_stone"};
    s.buy_pct = 115; s.sell_pct = 40;    // перекупщик берёт своё
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_herbs"; s.name = "Травы Лады";
    s.goods = {"herb_potion", "ap_tonic", "bread", "antidote", "salve",
               "elixir_might", "elixir_guard", "elixir_haste"};
    s.buy_pct = 90; s.sell_pct = 35;     // своим дешевле
    shops_[s.id] = s;
}

// --------------------------------------------------------------------- NPC

void Content::build_npcs() {
    auto add = [&](const std::string& id, const std::string& name,
                   const std::string& root, const std::string& shop) {
        NpcDef n; n.id = id; n.name = name; n.root = root; n.shop = shop;
        npcs_[id] = n;
    };

    add("elder",     "Старейшина Мирон", "elder_root",     "");
    add("herbalist", "Травница Лада",    "herbalist_root", "shop_herbs");
    add("smith",     "Кузнец Бран",      "smith_root",     "shop_smith");
    add("trader",    "Торговец Гурий",   "trader_root",    "shop_general");
    add("hermit",    "Отшельник Свет",   "hermit_root",    "");
    add("enchanter", "Зачарователь Вельд", "ench_root",    "");
}

// ----------------------------------------------------------------- диалоги

void Content::build_dialogues() {
    auto add = [&](DlgNode n) { nodes_[n.id] = n; };

    auto bye = [](const std::string& text) {
        DlgOption o; o.text = text; return o;
    };

    // --- Старейшина Мирон: квест на волков ---
    {
        DlgNode n; n.id = "elder_root";
        n.text = "Мирон опирается на палку и щурится.\n"
                 "— Чужак, значит. Ольховка тебе рада, пока ты не волк.";

        DlgOption ask;                       // квест ещё не взят
        ask.text = "Чем в деревне помочь?";
        ask.next = "elder_offer";
        ask.req_quest = "wolves"; ask.req_stage_min = QUEST_NONE; ask.req_stage_max = QUEST_NONE;
        n.options.push_back(ask);

        DlgOption progress;                  // взят, волков ещё мало
        progress.text = "Я всё ещё считаю волков.";
        progress.next = "elder_progress";
        progress.req_quest = "wolves"; progress.req_stage_min = 1; progress.req_stage_max = 1;
        progress.req_counter = "kill_wolf"; progress.req_counter_max = 4;
        n.options.push_back(progress);

        DlgOption done;                      // взят, волков хватает
        done.text = "Пять волков больше не выйдут из леса.";
        done.next = "elder_reward";
        done.req_quest = "wolves"; done.req_stage_min = 1; done.req_stage_max = 2;
        done.req_counter = "kill_wolf"; done.req_counter_min = 5;
        n.options.push_back(done);

        DlgOption after;
        after.text = "Как деревня?";
        after.next = "elder_after";
        after.req_quest = "wolves"; after.req_stage_min = QUEST_DONE; after.req_stage_max = QUEST_DONE;
        n.options.push_back(after);

        DlgOption q_offer;                   // открывается тем, кто закрыл волков
        q_offer.text = "Есть работа посерьёзнее волков?";
        q_offer.next = "elder_queen_offer";
        q_offer.req_quest  = "queen";  q_offer.req_stage_min  = QUEST_NONE; q_offer.req_stage_max  = QUEST_NONE;
        q_offer.req_quest2 = "wolves"; q_offer.req_stage2_min = QUEST_DONE; q_offer.req_stage2_max = QUEST_DONE;
        n.options.push_back(q_offer);

        DlgOption q_done;
        q_done.text = "Матка мертва.";
        q_done.next = "elder_queen_reward";
        q_done.req_quest = "queen"; q_done.req_stage_min = 1; q_done.req_stage_max = 1;
        q_done.req_counter = "kill_queen"; q_done.req_counter_min = 1;
        n.options.push_back(q_done);

        DlgOption q_wait;
        q_wait.text = "Ещё не добрался до матки.";
        q_wait.next = "elder_queen_wait";
        q_wait.req_quest = "queen"; q_wait.req_stage_min = 1; q_wait.req_stage_max = 1;
        q_wait.req_counter = "kill_queen"; q_wait.req_counter_max = 0;
        n.options.push_back(q_wait);

        n.options.push_back(bye("Пойду."));
        add(n);
    }
    {
        DlgNode n; n.id = "elder_offer";
        n.text = "— Волки обнаглели. Задрали двух коз, теперь ходят к самой околице.\n"
                 "Убей пятерых в лесу за восточными воротами — деревня заплатит.";
        DlgOption take;
        take.text = "Берусь.";
        take.next = "elder_taken";
        take.set_quest = "wolves"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Мне это не по силам."));
        add(n);
    }
    {
        DlgNode n; n.id = "elder_taken";
        n.text = "— Восточные ворота, тропа за ними. И не ходи туда с голыми руками:\n"
                 "у Брана в кузнице есть железо, у Гурия — хлеб.";
        n.options.push_back(bye("Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "elder_progress";
        n.text = "— Считай быстрее, чужак. Пятеро. Не трое, не четверо.";
        n.options.push_back(bye("Иду."));
        add(n);
    }
    {
        DlgNode n; n.id = "elder_reward";
        n.text = "Мирон долго смотрит на тебя, потом лезет за пазуху.\n"
                 "— Значит, не хвастал. Держи, честно заработал.";
        DlgOption take;
        take.text = "Принять награду. [80 золотых, 60 опыта]";
        take.next = "";
        take.set_quest = "wolves"; take.set_stage = QUEST_DONE;
        take.give_gold = 80; take.give_exp = 60;
        take.give_item = "herb_potion"; take.give_count = 2;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "elder_after";
        n.text = "— Спокойнее стало. Козы целы, и то хлеб.\n"
                 "Если совсем прижмёт — переночуй в доме старосты, там койка.";
        n.options.push_back(bye("Спасибо."));
        add(n);
    }

    {
        DlgNode n; n.id = "elder_queen_offer";
        n.text = "— Раз с волками сладил, слушай дальше. В Барсучьей пещере,\n"
                 "что за расщелиной на востоке леса, завелась паучья матка.\n"
                 "Пастухи туда больше не ходят. Убей её — деревня в долгу не останется.";
        DlgOption take;
        take.text = "Возьмусь.";
        take.next = "elder_queen_wait";
        take.set_quest = "queen"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Пауки — это не ко мне."));
        add(n);
    }
    {
        DlgNode n; n.id = "elder_queen_wait";
        n.text = "— Матка сидит в самой глубине. И возьми противоядие, не дури:\n"
                 "паучий яд валит быстрее, чем ты успеешь передумать.";
        n.options.push_back(bye("Учту."));
        add(n);
    }
    {
        DlgNode n; n.id = "elder_queen_reward";
        n.text = "Мирон медленно садится на лавку.\n"
                 "— Значит, правда. Держи, это от всей Ольховки.";
        DlgOption take;
        take.text = "Принять награду. [220 золотых, 150 опыта]";
        take.set_quest = "queen"; take.set_stage = QUEST_DONE;
        take.give_gold = 220; take.give_exp = 150;
        take.give_item = "tower_shield"; take.give_count = 1;
        n.options.push_back(take);
        add(n);
    }

    // --- Травница Лада: квест на амулет + травы ---
    {
        DlgNode n; n.id = "herbalist_root";
        n.text = "Пахнет сушёной мятой. Лада перебирает пучки трав.\n"
                 "— Заходи, только не наступи на корзину.";

        DlgOption ask;
        ask.text = "Ты чем-то расстроена?";
        ask.next = "herb_offer";
        ask.req_quest = "amulet"; ask.req_stage_min = QUEST_NONE; ask.req_stage_max = QUEST_NONE;
        n.options.push_back(ask);

        DlgOption ret;                      // амулет уже в сумке
        ret.text = "Твой амулет был у вожака стаи.";
        ret.next = "herb_reward";
        ret.req_quest = "amulet"; ret.req_stage_min = 1; ret.req_stage_max = 1;
        ret.req_item = "amulet";  ret.req_item_count = 1;
        n.options.push_back(ret);

        DlgOption search;                   // квест взят, амулета нет
        search.text = "Ещё ищу.";
        search.next = "herb_progress";
        search.req_quest = "amulet"; search.req_stage_min = 1; search.req_stage_max = 1;
        n.options.push_back(search);

        DlgOption moss_offer;
        moss_offer.text = "Нужно что-нибудь из пещеры?";
        moss_offer.next = "moss_offer";
        moss_offer.req_quest  = "moss";   moss_offer.req_stage_min  = QUEST_NONE; moss_offer.req_stage_max  = QUEST_NONE;
        moss_offer.req_quest2 = "amulet"; moss_offer.req_stage2_min = QUEST_DONE; moss_offer.req_stage2_max = QUEST_DONE;
        n.options.push_back(moss_offer);

        DlgOption moss_done;
        moss_done.text = "Мох собран, три пучка.";
        moss_done.next = "moss_reward";
        moss_done.req_quest = "moss"; moss_done.req_stage_min = 1; moss_done.req_stage_max = 1;
        moss_done.req_item = "glow_moss"; moss_done.req_item_count = 3;
        n.options.push_back(moss_done);

        DlgOption moss_wait;
        moss_wait.text = "Мха пока мало.";
        moss_wait.next = "moss_wait";
        moss_wait.req_quest = "moss"; moss_wait.req_stage_min = 1; moss_wait.req_stage_max = 1;
        n.options.push_back(moss_wait);

        DlgOption trade;
        trade.text = "Покажи, что есть из настоев.";
        trade.open_shop = true;
        n.options.push_back(trade);

        n.options.push_back(bye("До встречи."));
        add(n);
    }
    {
        DlgNode n; n.id = "herb_offer";
        n.text = "— Амулет матери потеряла. Собирала корни у ручья, волки налетели,\n"
                 "я — бежать. Шнурок и порвался. Они всё блестящее тащат к вожаку.";
        DlgOption take;
        take.text = "Принесу.";
        take.next = "herb_taken";
        take.set_quest = "amulet"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Вряд ли я его найду."));
        add(n);
    }
    {
        DlgNode n; n.id = "herb_taken";
        n.text = "— Вожак крупный, светлее прочих. Держится в глубине леса.\n"
                 "Возьми настой, за счёт заведения. Пригодится.";
        DlgOption take;
        take.text = "Спасибо.";
        take.give_item = "herb_potion"; take.give_count = 1;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "herb_progress";
        n.text = "— Ищи вожака, не рядовых. У рядовых только блохи.";
        n.options.push_back(bye("Найду."));
        add(n);
    }
    {
        DlgNode n; n.id = "herb_reward";
        n.text = "Лада берёт амулет обеими руками и долго молчит.\n"
                 "— Мать носила его сорок лет. Возьми вот это, оно теперь твоё.";
        DlgOption take;
        take.text = "Принять кольцо жизни. [70 опыта]";
        take.set_quest = "amulet"; take.set_stage = QUEST_DONE;
        take.take_item = "amulet"; take.take_count = 1;
        take.give_item = "ring_hp"; take.give_count = 1;
        take.give_exp = 70;
        n.options.push_back(take);
        add(n);
    }

    {
        DlgNode n; n.id = "moss_offer";
        n.text = "— Есть. В Барсучьей пещере растёт светящийся мох, на камнях\n"
                 "у воды. Мне нужно три пучка: из него выходит мазь, которая\n"
                 "затягивает раны сама, без присмотра.";
        DlgOption take;
        take.text = "Принесу.";
        take.next = "moss_wait";
        take.set_quest = "moss"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("В пещеру я не полезу."));
        add(n);
    }
    {
        DlgNode n; n.id = "moss_wait";
        n.text = "— Три пучка, не меньше. И смотри под ноги: там не только мох живёт.";
        n.options.push_back(bye("Хорошо."));
        add(n);
    }
    {
        DlgNode n; n.id = "moss_reward";
        n.text = "Лада перебирает мох, довольно щурится.\n"
                 "— Отличный. Держи первую партию мази, она твоя по праву.";
        DlgOption take;
        take.text = "Принять мази. [90 опыта]";
        take.set_quest = "moss"; take.set_stage = QUEST_DONE;
        take.take_item = "glow_moss"; take.take_count = 3;
        take.give_item = "salve"; take.give_count = 3;
        take.give_gold = 60; take.give_exp = 90;
        n.options.push_back(take);
        add(n);
    }

    // --- Кузнец Бран: заказ на шкуры + кузница ---
    {
        DlgNode n; n.id = "smith_root";
        n.text = "Бран бьёт по заготовке, не поднимая головы.\n"
                 "— Говори, пока горн не остыл.";

        DlgOption ask;
        ask.text = "Нужна работа?";
        ask.next = "smith_offer";
        ask.req_quest = "pelts"; ask.req_stage_min = QUEST_NONE; ask.req_stage_max = QUEST_NONE;
        n.options.push_back(ask);

        DlgOption deliver;
        deliver.text = "Шкуры у меня. Три штуки.";
        deliver.next = "smith_reward";
        deliver.req_quest = "pelts"; deliver.req_stage_min = 1; deliver.req_stage_max = 1;
        deliver.req_item = "wolf_pelt"; deliver.req_item_count = 3;
        n.options.push_back(deliver);

        DlgOption progress;
        progress.text = "Шкуры ещё в лесу.";
        progress.next = "smith_progress";
        progress.req_quest = "pelts"; progress.req_stage_min = 1; progress.req_stage_max = 1;
        n.options.push_back(progress);

        DlgOption out_offer;
        out_offer.text = "Слышал про развалины заставы?";
        out_offer.next = "outpost_offer";
        out_offer.req_quest  = "outpost"; out_offer.req_stage_min  = QUEST_NONE; out_offer.req_stage_max  = QUEST_NONE;
        out_offer.req_quest2 = "pelts";   out_offer.req_stage2_min = QUEST_DONE; out_offer.req_stage2_max = QUEST_DONE;
        n.options.push_back(out_offer);

        DlgOption out_done;
        out_done.text = "Атаман Кривой больше никого не грабит.";
        out_done.next = "outpost_reward";
        out_done.req_quest = "outpost"; out_done.req_stage_min = 1; out_done.req_stage_max = 1;
        out_done.req_counter = "kill_chief"; out_done.req_counter_min = 1;
        n.options.push_back(out_done);

        DlgOption out_wait;
        out_wait.text = "До атамана ещё не дошёл.";
        out_wait.next = "outpost_wait";
        out_wait.req_quest = "outpost"; out_wait.req_stage_min = 1; out_wait.req_stage_max = 1;
        out_wait.req_counter = "kill_chief"; out_wait.req_counter_max = 0;
        n.options.push_back(out_wait);

        DlgOption trade;
        trade.text = "Показывай товар.";
        trade.open_shop = true;
        n.options.push_back(trade);

        n.options.push_back(bye("Работай."));
        add(n);
    }
    {
        DlgNode n; n.id = "smith_offer";
        n.text = "— Кольчугу свёл, а подбить нечем. Нужны три волчьи шкуры, целых.\n"
                 "Принесёшь — кольчуга твоя. Мне вторая ни к чему.";
        DlgOption take;
        take.text = "Договорились.";
        take.next = "smith_taken";
        take.set_quest = "pelts"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Подумаю."));
        add(n);
    }
    {
        DlgNode n; n.id = "smith_taken";
        n.text = "— Целых, я сказал. Не лоскуты.";
        n.options.push_back(bye("Услышал."));
        add(n);
    }
    {
        DlgNode n; n.id = "smith_progress";
        n.text = "— Значит, и кольчуга ещё в горне. Иди.";
        n.options.push_back(bye("Иду."));
        add(n);
    }
    {
        DlgNode n; n.id = "smith_reward";
        n.text = "Бран мнёт шкуры пальцами, кивает и снимает кольчугу с крюка.\n"
                 "— Держи. Носи, не снимая, пока ходишь в лес.";
        DlgOption take;
        take.text = "Принять кольчугу. [40 опыта]";
        take.set_quest = "pelts"; take.set_stage = QUEST_DONE;
        take.take_item = "wolf_pelt"; take.take_count = 3;
        take.give_item = "chain_armor"; take.give_count = 1;
        take.give_exp = 40;
        n.options.push_back(take);
        add(n);
    }

    {
        DlgNode n; n.id = "outpost_offer";
        n.text = "Бран откладывает молот.\n"
                 "— Слышал. Южная дорога от леса ведёт к старой заставе. Там сидит\n"
                 "Кривой со своей сворой и берёт мзду со всех, кто идёт мимо.\n"
                 "Убери атамана — обозы снова пойдут, и я наконец получу железо.";
        DlgOption take;
        take.text = "Схожу.";
        take.next = "outpost_wait";
        take.set_quest = "outpost"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Их там слишком много."));
        add(n);
    }
    {
        DlgNode n; n.id = "outpost_wait";
        n.text = "— Кривой держится в восточной части плаца. Дозорные бьют больно\n"
                 "и норовят открыть рану — возьми, чем перевязаться.";
        n.options.push_back(bye("Иду."));
        add(n);
    }
    {
        DlgNode n; n.id = "outpost_reward";
        n.text = "— Ну наконец-то. Держи, честно заслужил.\n"
                 "И ключ его прибери — от сундука на плацу, там добро не его.";
        DlgOption take;
        take.text = "Принять награду. [300 золотых, 200 опыта]";
        take.set_quest = "outpost"; take.set_stage = QUEST_DONE;
        take.give_gold = 300; take.give_exp = 200;
        take.give_item = "helm"; take.give_count = 1;
        n.options.push_back(take);
        add(n);
    }

    // --- Торговец Гурий ---
    {
        DlgNode n; n.id = "trader_root";
        n.text = "Гурий раскладывает товар и улыбается чуть шире, чем нужно.\n"
                 "— Хлеб, факелы, настои. Всё своё, всё честное.";
        DlgOption trade;
        trade.text = "Показывай.";
        trade.open_shop = true;
        n.options.push_back(trade);
        DlgOption talk;
        talk.text = "Почему у тебя дороже, чем у Лады?";
        talk.next = "trader_talk";
        n.options.push_back(talk);
        n.options.push_back(bye("В другой раз."));
        add(n);
    }
    {
        DlgNode n; n.id = "trader_talk";
        n.text = "— У Лады травы свои, а я их везу. Дорога денег стоит.\n"
                 "Зато у меня и кольца бывают, а у неё — мята да мята.";
        n.options.push_back(bye("Логично."));
        add(n);
    }

    // --- Зачарователь Вельд: ремесло по рунному камню ---
    {
        DlgNode n; n.id = "ench_root";
        n.text = "Вельд разложил на тряпице десяток камешков и щурится на них,\n"
                 "будто читает. Из-под навеса тянет палёным.\n"
                 "— Руны — дело нехитрое. Материал — вот беда.";

        DlgOption offer;
        offer.text = "Чем помочь?";
        offer.next = "ench_offer";
        offer.req_quest = "enchanter"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption deliver;
        deliver.text = "Рунный камень у меня.";
        deliver.next = "ench_reward";
        deliver.req_quest = "enchanter"; deliver.req_stage_min = 1; deliver.req_stage_max = 1;
        deliver.req_item = "rune_stone"; deliver.req_item_count = 1;
        n.options.push_back(deliver);

        DlgOption wait;
        wait.text = "Камня пока нет.";
        wait.next = "ench_wait";
        wait.req_quest = "enchanter"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption work;
        work.text = "Зачаруй мне вещь.";
        work.open_enchant = true;
        work.req_quest = "enchanter"; work.req_stage_min = QUEST_DONE; work.req_stage_max = QUEST_DONE;
        n.options.push_back(work);

        DlgOption about;
        about.text = "Что ты вообще умеешь?";
        about.next = "ench_about";
        n.options.push_back(about);

        n.options.push_back(bye("После зайду."));
        add(n);
    }
    {
        DlgNode n; n.id = "ench_about";
        n.text = "— Кладу на вещь руну, и вещь начинает делать чуть больше,\n"
                 "чем должна. Пламя жжёт, мороз сковывает, яд травит.\n"
                 "Плата — золотом и реагентом: без реагента руна не держится.\n"
                 "Одна вещь — одна руна, и снять её уже нельзя. Выбирай с умом.";
        n.options.push_back(bye("Ясно."));
        add(n);
    }
    {
        DlgNode n; n.id = "ench_offer";
        n.text = "— Нужен рунный камень. Настоящий, а не речная галька.\n"
                 "Такие лежат в святилище за развалинами заставы — и там же\n"
                 "ходит то, что их стережёт. Принесёшь один — открою лавку.";
        DlgOption take;
        take.text = "Достану.";
        take.next = "ench_wait";
        take.set_quest = "enchanter"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Далековато."));
        add(n);
    }
    {
        DlgNode n; n.id = "ench_wait";
        n.text = "— Святилище. За заставой, по восточному проходу. Один камень,\n"
                 "больше не прошу.";
        n.options.push_back(bye("Помню."));
        add(n);
    }
    {
        DlgNode n; n.id = "ench_reward";
        n.text = "Вельд берёт камень двумя пальцами, подносит к уху и улыбается.\n"
                 "— Живой. Ну всё, теперь работаем. Неси, что зачаровывать.";
        DlgOption take;
        take.text = "Договорились. [100 опыта]";
        take.set_quest = "enchanter"; take.set_stage = QUEST_DONE;
        take.take_item = "rune_stone"; take.take_count = 1;
        take.give_exp = 100;
        n.options.push_back(take);
        add(n);
    }

    // --- Отшельник в лесу: подсказки и отдых ---
    {
        DlgNode n; n.id = "hermit_root";
        n.text = "У костра сидит худой человек в вылинявшем плаще.\n"
                 "— Садись. Огня хватит на двоих.";
        DlgOption rest;
        rest.text = "Отдохнуть у костра. [восстановить здоровье и силы]";
        rest.next = "hermit_rest";
        rest.rest = true;
        n.options.push_back(rest);
        DlgOption hint;
        hint.text = "Что нужно знать о здешнем лесе?";
        hint.next = "hermit_hint";
        n.options.push_back(hint);

        DlgOption zp_offer;
        zp_offer.text = "Ты ведь не просто так сидишь у этого костра?";
        zp_offer.next = "zp_offer";
        zp_offer.req_quest  = "zero_point"; zp_offer.req_stage_min  = QUEST_NONE; zp_offer.req_stage_max  = QUEST_NONE;
        zp_offer.req_quest2 = "outpost";    zp_offer.req_stage2_min = QUEST_DONE; zp_offer.req_stage2_max = QUEST_DONE;
        n.options.push_back(zp_offer);

        DlgOption zp_done;
        zp_done.text = "Три фокуса у меня.";
        zp_done.next = "zp_reward";
        zp_done.req_quest = "zero_point"; zp_done.req_stage_min = 1; zp_done.req_stage_max = 1;
        zp_done.req_item = "focus_node";  zp_done.req_item_count = 3;
        n.options.push_back(zp_done);

        DlgOption zp_wait;
        zp_wait.text = "Фокусов пока не хватает.";
        zp_wait.next = "zp_wait";
        zp_wait.req_quest = "zero_point"; zp_wait.req_stage_min = 1; zp_wait.req_stage_max = 1;
        n.options.push_back(zp_wait);

        DlgOption zp_after;
        zp_after.text = "Расскажи ещё про нулевую точку.";
        zp_after.next = "zp_after";
        zp_after.req_quest = "zero_point"; zp_after.req_stage_min = QUEST_DONE; zp_after.req_stage_max = QUEST_DONE;
        n.options.push_back(zp_after);

        n.options.push_back(bye("Пойду дальше."));
        add(n);
    }
    {
        DlgNode n; n.id = "hermit_rest";
        n.text = "Ты греешься у огня. Усталость отступает.";
        n.options.push_back(bye("Спасибо."));
        add(n);
    }
    {
        DlgNode n; n.id = "zp_offer";
        n.text = "Отшельник долго молчит, потом откидывает капюшон.\n"
                 "— Не просто так. Мир прошит линиями, и там, где они сходятся,\n"
                 "расстояние ничего не значит. Это и есть нулевая точка.\n"
                 "Я умею её открывать, но одному мне уже не дойти.\n"
                 "Принеси три узловых фокуса. Их носят те, кто сторожит линии:\n"
                 "паучья матка, атаман на заставе и страж в святилище за ней.\n"
                 "Принесёшь — научу ставить порталы.";
        DlgOption take;
        take.text = "Найду все три.";
        take.next = "zp_wait";
        take.set_quest = "zero_point"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Звучит как сказка."));
        add(n);
    }
    {
        DlgNode n; n.id = "zp_wait";
        n.text = "— Три. Матка, атаман, страж. Страж хуже прочих: он не устаёт\n"
                 "и бьёт так, что замах становится дороже. Не лезь к нему рано.";
        n.options.push_back(bye("Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "zp_reward";
        n.text = "Отшельник складывает фокусы треугольником и держит над ними ладонь.\n"
                 "— Смотри и запоминай. Больше я этого не покажу.\n"
                 "Теперь ты — мастер нулевой точки. Ставь камень, ставь второй,\n"
                 "и между ними не будет расстояния.";
        DlgOption take;
        take.text = "Принять умение. [250 опыта]";
        take.set_quest = "zero_point"; take.set_stage = QUEST_DONE;
        take.take_item = "focus_node";   take.take_count = 3;
        take.give_item = "portal_stone"; take.give_count = 2;
        take.give_exp = 250;
        take.portal_gift = true;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "zp_after";
        n.text = "— Камень ставится под ноги, клавиша P. Поставишь два — пойдёшь\n"
                 "между ними в обе стороны. Поставишь больше — они выстроятся\n"
                 "в кольцо, и каждый поведёт к следующему.\n"
                 "Камни продаёт Гурий, если сам не найдёшь.";
        n.options.push_back(bye("Спасибо."));
        add(n);
    }
    {
        DlgNode n; n.id = "hermit_hint";
        n.text = "— Волк бьёт часто, но слабо: против него держи ЯРОСТНУЮ стойку.\n"
                 "Вожак и разбойник бьют тяжело — тут лучше ОСТОРОЖНАЯ, переждать.\n"
                 "И запомни: каждый твой удар копит кураж. Накопишь три — бей наверняка.";
        n.options.push_back(bye("Запомню."));
        add(n);
    }
}

// ------------------------------------------------------------------ доступ

const ItemDef* Content::item(const std::string& id) const {
    auto it = items_.find(id);
    return it == items_.end() ? nullptr : &it->second;
}
const EffectDef* Content::effect(const std::string& id) const {
    auto it = effects_.find(id);
    return it == effects_.end() ? nullptr : &it->second;
}
const EnchantDef* Content::enchant(const std::string& id) const {
    auto it = enchant_map_.find(id);
    return it == enchant_map_.end() ? nullptr : &it->second;
}
const RaceDef* Content::race(const std::string& id) const {
    for (const RaceDef& r : races_)
        if (r.id == id) return &r;
    return nullptr;
}
const SpecDef* Content::spec(const std::string& id) const {
    for (const SpecDef& sp : specs_)
        if (sp.id == id) return &sp;
    return nullptr;
}
const EnemyDef* Content::enemy(const std::string& id) const {
    auto it = enemies_.find(id);
    return it == enemies_.end() ? nullptr : &it->second;
}
const NpcDef* Content::npc(const std::string& id) const {
    auto it = npcs_.find(id);
    return it == npcs_.end() ? nullptr : &it->second;
}
const ShopDef* Content::shop(const std::string& id) const {
    auto it = shops_.find(id);
    return it == shops_.end() ? nullptr : &it->second;
}
const DlgNode* Content::node(const std::string& id) const {
    auto it = nodes_.find(id);
    return it == nodes_.end() ? nullptr : &it->second;
}
const QuestDef* Content::quest(const std::string& id) const {
    for (const QuestDef& q : quests_)
        if (q.id == id) return &q;
    return nullptr;
}
const SkillDef* Content::skill(const std::string& id) const {
    for (const SkillDef& s : skills_)
        if (s.id == id) return &s;
    return nullptr;
}

std::string Content::quest_stage_text(const std::string& quest_id, int stage) const {
    const QuestDef* q = quest(quest_id);
    if (!q) return "";
    for (const QuestStageDef& s : q->stages)
        if (s.stage == stage) return s.text;
    return "";
}
