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
    build_items();
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

    // разное и квестовое
    add(mk_item("wolf_pelt", "Волчья шкура", ItemKind::Misc, 18, "Кузнец такие принимает."));
    add(mk_item("rat_tail",  "Крысиный хвост", ItemKind::Misc, 4,  "Ценности немного."));
    add(mk_item("torch",     "Факел", ItemKind::Misc, 10, "В лесу спокойнее."));
    add(mk_item("amulet",    "Амулет Лады", ItemKind::Misc, 0,
                "Медный амулет с зелёным камнем. Явно чужой."));
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
    s.goods = {"dagger", "short_sword", "axe", "spear",
               "leather_armor", "cap", "helm", "buckler"};
    s.buy_pct = 100; s.sell_pct = 45;
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_general"; s.name = "Лавка Гурия";
    s.goods = {"bread", "torch", "herb_potion", "ap_tonic", "ring_hp", "ring_crit"};
    s.buy_pct = 115; s.sell_pct = 40;    // перекупщик берёт своё
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_herbs"; s.name = "Травы Лады";
    s.goods = {"herb_potion", "ap_tonic", "bread"};
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
