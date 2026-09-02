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
    build_notes();
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
    build_endings();
    build_triggers();
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

    ItemDef gall = mk_item("oak_gall", "Чернильный орешек", ItemKind::Misc, 12,
                           "Из таких варят чернила. Растут на дубовых листьях.");
    add(gall);

    ItemDef blank = mk_item("book_blank", "Чистая книга", ItemKind::Book, 140,
                            "Переплёт и полсотни страниц. Применить, чтобы начать её.");
    add(blank);

    // --- Регион II: Шов ---
    add(mk_item("glass_shard", "Осколок поля", ItemKind::Misc, 22,
                "Стекло не заводское: земля спеклась сама. Режет сквозь рукавицу."));
    add(mk_item("salt_lump", "Ком соли", ItemKind::Misc, 16,
                "Соль из нижних штолен. Держит запах того, что в ней лежало."));
    add(mk_item("old_coin", "Монета не той чеканки", ItemKind::Misc, 30,
                "Профиль незнакомый, год — за сорок лет до Стяжения."));
    add(mk_item("spice_bag", "Мешочек пряностей", ItemKind::Misc, 45,
                "Пахнет так, будто его завязали вчера. Караван шёл двести лет."));
    add(mk_item("rope_end", "Обрывок каната", ItemKind::Misc, 20,
                "Срез ровный. Канат не перетёрся — его отрезало."));
    add(mk_item("salt_heart", "Соляное сердце", ItemKind::Misc, 0,
                "Тяжёлое, тёплое. Внутри что-то есть, и лучше не смотреть."));
    add(mk_item("mill_key", "Ключ от нижнего затвора", ItemKind::Misc, 0,
                "Мельник берёг его сорок лет и ни разу не открывал."));

    ItemDef glass_blade = mk_item("glass_blade", "Стеклянный резак", ItemKind::Weapon, 330,
                                  "Осколок поля в рукояти. Бьёт больно и рвёт рану.");
    glass_blade.bonus.dmg_min = 5; glass_blade.bonus.dmg_max = 12;
    glass_blade.bonus.crit = 7; glass_blade.bonus.attack = -3;
    add(glass_blade);

    ItemDef salt_mail = mk_item("salt_mail", "Просоленный доспех", ItemKind::Armor, 360,
                                "Кожа, вымоченная в рассоле. Не гниёт и не поддаётся.");
    salt_mail.bonus.armor = 5; salt_mail.bonus.block = 5;
    add(salt_mail);

    ItemDef brine_ring = mk_item("brine_ring", "Кольцо рассола", ItemKind::Ring, 380,
                                 "Холодит палец. Тело перестаёт торопиться.");
    brine_ring.bonus.max_hp = 8; brine_ring.bonus.armor = 2; brine_ring.bonus.max_ap = 1;
    add(brine_ring);

    ItemDef trader_hood = mk_item("trader_hood", "Куколь торговца", ItemKind::Helmet, 240,
                                  "На Рынке Шва в таких ходят все. Так проще.");
    trader_hood.bonus.armor = 2; trader_hood.bonus.block = 4; trader_hood.bonus.attack = 3;
    add(trader_hood);

    ItemDef caravan_stew = mk_item("caravan_stew", "Похлёбка каравана", ItemKind::Consumable, 60,
                                   "Ещё горячая. Не думай об этом.");
    caravan_stew.heal_hp = 40; caravan_stew.heal_ap = 3;
    add(caravan_stew);

    ItemDef glass_dust = mk_item("glass_dust", "Стеклянная пыль", ItemKind::Consumable, 80,
                                 "Вдохнуть — и глаз находит слабое место сам.");
    glass_dust.effect = "clarity"; glass_dust.effect_turns = 10; glass_dust.effect_power = 2;
    add(glass_dust);

    // --- Регион III: Половины ---
    add(mk_item("scrap_iron", "Ломаное железо", ItemKind::Misc, 28,
                "Литейный двор принимает такое на переплавку."));
    add(mk_item("city_brick", "Городской кирпич", ItemKind::Misc, 14,
                "Клеймо с обратной стороны — того завода больше нет нигде."));
    add(mk_item("ledger_page", "Лист из гроссбуха", ItemKind::Misc, 26,
                "Столбцы цифр, и в каждом третьем ошибка на одну и ту же величину."));
    add(mk_item("ferry_token", "Перевозный жетон", ItemKind::Misc, 40,
                "Медь, дырка посередине. Перевозчик берёт только такие."));
    add(mk_item("archive_key", "Ключ городского архива", ItemKind::Misc, 0,
                "Смотритель Половины носил его на шее и не отдавал."));
    add(mk_item("half_name", "Половина имени", ItemKind::Misc, 0,
                "Клочок списка. Имя обрывается ровно посередине."));

    ItemDef hammer = mk_item("foundry_hammer", "Молот литейной", ItemKind::Weapon, 620,
                             "Тяжёлый до неприличия. Бьёт так, что броня не помогает.");
    hammer.bonus.dmg_min = 9; hammer.bonus.dmg_max = 18;
    hammer.bonus.attack = -6; hammer.bonus.ap_atk = 1; hammer.bonus.crit = 5;
    add(hammer);

    ItemDef robe = mk_item("clerk_robe", "Мантия счетовода", ItemKind::Armor, 480,
                           "Карманы на все случаи. Кто-то в них ещё и считал.");
    robe.bonus.armor = 5; robe.bonus.max_ap = 2; robe.bonus.block = 4;
    add(robe);

    ItemDef lens = mk_item("counter_lens", "Линза Счетовода", ItemKind::Helmet, 520,
                           "Сквозь неё видно, где вещь тоньше всего.");
    lens.bonus.crit = 10; lens.bonus.attack = 7; lens.bonus.armor = 1;
    add(lens);

    ItemDef sshield = mk_item("slag_shield", "Щит из шлака", ItemKind::Shield, 470,
                              "Некрасивый и почти неподъёмный. Держит всё.");
    sshield.bonus.block = 14; sshield.bonus.armor = 4; sshield.bonus.attack = -5;
    add(sshield);

    ItemDef fring = mk_item("ferry_ring", "Кольцо перевозчика", ItemKind::Ring, 500,
                            "Пока оно на пальце, стоячая вода держит крепче.");
    fring.bonus.max_hp = 10; fring.bonus.block = 7; fring.bonus.max_ap = 1;
    add(fring);

    ItemDef tea = mk_item("strong_tea", "Крепкий чай счетоводов", ItemKind::Consumable, 95,
                          "Три ложки на кружку. Считать становится легче, жить — нет.");
    tea.heal_ap = 6; tea.effect = "clarity"; tea.effect_turns = 8; tea.effect_power = 2;
    add(tea);

    // --- Регион IV: Орден ---
    add(mk_item("chart_piece", "Обрывок чертежа", ItemKind::Misc, 55,
                "Линии сходятся к точке за краем листа."));
    add(mk_item("torn_page", "Вырванный лист", ItemKind::Misc, 34,
                "Из устава. Вырвано ровно там, где начиналось важное."));
    add(mk_item("order_writ", "Приказ о смене караула", ItemKind::Misc, 0,
                "Подписан, скреплён и не доставлен. Двести лет пролежал в ящике."));
    add(mk_item("keepsake", "Оловянный солдатик", ItemKind::Misc, 0,
                "Краска стёрлась там, где его держали пальцами."));
    add(mk_item("node_core", "Ядро узла", ItemKind::Misc, 0,
                "Не тяжёлое и не лёгкое. Вес зависит от того, как держать."));
    add(mk_item("furnace_ash", "Зола Ордена", ItemKind::Misc, 48,
                "Не пачкает рук. Печь берёт вещь и отдаёт вот это."));

    ItemDef halberd = mk_item("gate_halberd", "Привратная алебарда", ItemKind::Weapon, 780,
                              "Древко в рост человека. Ею не бьют — ею не пускают.");
    halberd.bonus.dmg_min = 11; halberd.bonus.dmg_max = 20;
    halberd.bonus.attack = 8; halberd.bonus.block = 6; halberd.bonus.ap_atk = 1;
    add(halberd);

    ItemDef oplate = mk_item("order_plate", "Доспех Ордена", ItemKind::Armor, 900,
                             "Узел вычеканен на груди. Внутри подбой из чужой шерсти.");
    oplate.bonus.armor = 9; oplate.bonus.block = 7; oplate.bonus.max_hp = 10;
    add(oplate);

    ItemDef ahood = mk_item("acolyte_hood", "Куколь послушника", ItemKind::Helmet, 610,
                            "В таких ходили парами. Второй куколь всегда оставался снаружи.");
    ahood.bonus.armor = 3; ahood.bonus.max_ap = 2; ahood.bonus.attack = 5;
    add(ahood);

    ItemDef nshield = mk_item("node_shield", "Щит узлового стража", ItemKind::Shield, 820,
                              "Держит удар так, будто удара и не было.");
    nshield.bonus.block = 18; nshield.bonus.armor = 5; nshield.bonus.attack = -4;
    add(nshield);

    ItemDef mring = mk_item("master_ring", "Кольцо Первого Мастера", ItemKind::Ring, 1200,
                            "Он снял его в Зале Отказа и больше не надевал.");
    mring.bonus.max_hp = 16; mring.bonus.attack = 8; mring.bonus.crit = 8;
    mring.bonus.armor = 3;
    add(mring);

    ItemDef odraught = mk_item("order_draught", "Настой Ордена", ItemKind::Consumable, 130,
                               "Готовили перед входом в узел. Держит и тело, и голову.");
    odraught.heal_hp = 55; odraught.heal_ap = 4;
    odraught.effect = "guard"; odraught.effect_turns = 10; odraught.effect_power = 2;
    add(odraught);

    ItemDef seal = mk_item("order_seal", "Печать Ордена", ItemKind::Ring, 700,
                           "Узел из серебра. Тёплая, будто её только что держали.");
    seal.bonus.max_hp = 12; seal.bonus.attack = 5; seal.bonus.armor = 2; seal.bonus.crit = 4;
    add(seal);

    add(mk_item("seam_key", "Ключ шва", ItemKind::Misc, 0,
                "Не ключ даже — узел из чёрного металла. Страж носил его в себе."));

    // --- Регион V: Дрейф ---
    add(mk_item("caravan_tally", "Путевая бирка", ItemKind::Misc, 0,
                "Двенадцать подвод, сорок душ. Обоз числится в пути."));
    add(mk_item("drift_grass", "Дрейфующая трава", ItemKind::Misc, 42,
                "Корни висят в воздухе и ничего не держат. Растёт всё равно."));
    add(mk_item("two_bucket", "Второе ведро", ItemKind::Misc, 0,
                "Спустили одно, подняли два. Второе тяжелее и холоднее."));
    add(mk_item("torn_banner", "Обрывок знамени", ItemKind::Misc, 90,
                "Цвета не разобрать. Держали крепко — ткань порвана поперёк хватки."));
    add(mk_item("own_key", "Ключ с пустой биркой", ItemKind::Misc, 0,
                "От дома, в котором никто не жил. Бирку не заполнили."));

    ItemDef wname = mk_item("whole_name", "Имя целиком", ItemKind::Ring, 1350,
                            "Две половины, сложенные и сшитые ниткой. Чьё — уже не важно.");
    wname.bonus.max_hp = 20; wname.bonus.attack = 7; wname.bonus.block = 6;
    wname.bonus.armor = 3;
    add(wname);

    ItemDef hblade = mk_item("hour_blade", "Клинок последнего часа", ItemKind::Weapon, 1050,
                             "Заточен утром того дня. С тех пор его не точили и не надо.");
    hblade.bonus.dmg_min = 13; hblade.bonus.dmg_max = 23;
    hblade.bonus.attack = 10; hblade.bonus.crit = 6;
    add(hblade);

    ItemDef dcloak = mk_item("drift_cloak", "Плащ дрейфующего", ItemKind::Armor, 1100,
                             "Не греет и не мокнет. Ветра на лоскуте нет, а плащ шевелится.");
    dcloak.bonus.armor = 11; dcloak.bonus.block = 8; dcloak.bonus.max_ap = 1;
    add(dcloak);

    ItemDef shood = mk_item("stair_hood", "Клобук обходчика", ItemKind::Helmet, 780,
                            "Обходчик считал ступени вслух. Ткань у висков вытерта ладонями.");
    shood.bonus.max_ap = 3; shood.bonus.attack = 6; shood.bonus.armor = 2;
    add(shood);

    ItemDef bshield = mk_item("banner_shield", "Щит знаменосца", ItemKind::Shield, 960,
                              "Умбон вмят внутрь. Знамя он всё-таки не выронил.");
    bshield.bonus.block = 20; bshield.bonus.armor = 6; bshield.bonus.max_hp = 8;
    bshield.bonus.attack = -3;
    add(bshield);

    ItemDef ering = mk_item("edge_ring", "Кольцо Края", ItemKind::Ring, 1250,
                            "Смотришь сквозь — видно другие лоскуты. Смотреть подолгу нельзя.");
    ering.bonus.crit = 12; ering.bonus.attack = 9; ering.bonus.max_ap = 2;
    add(ering);

    ItemDef swater = mk_item("still_water", "Стоячая вода", ItemKind::Consumable, 145,
                             "Из колодца Двух Вёдер. Не портится, потому что не идёт время.");
    swater.heal_hp = 70; swater.cures = "*";
    add(swater);

    ItemDef gleaf = mk_item("grove_leaf", "Лист из рощи", ItemKind::Consumable, 120,
                            "Сорван и не вянет. Пока держишь — не устаёшь.");
    gleaf.heal_ap = 8; gleaf.effect = "haste"; gleaf.effect_turns = 10; gleaf.effect_power = 2;
    add(gleaf);

    // --- Регион VI: Изнанка ---
    add(mk_item("line_thread", "Нить линии", ItemKind::Misc, 70,
                "Тонкая до невидимости. На ощупь — как натянутая струна."));
    add(mk_item("seam_word", "Честное слово", ItemKind::Misc, 0,
                "Расписка Первого Мастера. Ею и держится самый старый стык."));
    add(mk_item("heart_cog", "Зуб стяжного хода", ItemKind::Misc, 130,
                "Ход не крутится и не тикает. Зуб всё равно стёрт."));
    add(mk_item("line_dust", "Пыль линий", ItemKind::Misc, 85,
                "Осыпается там, где линия трётся о линию. Годится в зачарование."));

    // Мера — единственная вещь, которой не должно быть: расстояние,
    // взятое в руку. Ею и открывается Развязка.
    add(mk_item("measure", "Мера расстояния", ItemKind::Misc, 0,
                "Держишь — и до всего ровно столько, сколько ты решил."));

    ItemDef lblade = mk_item("line_blade", "Клинок по линии", ItemKind::Weapon, 1400,
                             "Заточен вдоль линии, а не поперёк. Режет то, что держит.");
    lblade.bonus.dmg_min = 16; lblade.bonus.dmg_max = 28;
    lblade.bonus.attack = 12; lblade.bonus.crit = 8;
    add(lblade);

    ItemDef iplate = mk_item("inside_plate", "Доспех изнанки", ItemKind::Armor, 1450,
                             "Швы наружу. Изнутри гладко — там ему и место.");
    iplate.bonus.armor = 14; iplate.bonus.block = 10; iplate.bonus.max_hp = 14;
    add(iplate);

    ItemDef shelm = mk_item("seam_helm", "Шлем сторожа шва", ItemKind::Helmet, 1050,
                            "Елисей носил его двести лет и ни разу не снял на ночь.");
    shelm.bonus.armor = 6; shelm.bonus.max_hp = 12; shelm.bonus.max_ap = 2;
    add(shelm);

    ItemDef zshield = mk_item("zero_shield", "Щит нулевой точки", ItemKind::Shield, 1350,
                              "Удар до него не доходит: расстояние до щита равно нулю.");
    zshield.bonus.block = 24; zshield.bonus.armor = 8; zshield.bonus.max_hp = 10;
    zshield.bonus.attack = -4;
    add(zshield);

    ItemDef firstr = mk_item("first_ring", "Кольцо Первого узла", ItemKind::Ring, 1600,
                             "С него всё началось. Оно об этом не знает.");
    firstr.bonus.max_hp = 24; firstr.bonus.attack = 10; firstr.bonus.crit = 10;
    firstr.bonus.armor = 4; firstr.bonus.max_ap = 1;
    add(firstr);

    ItemDef cdraught = mk_item("cinch_draught", "Настой изнанки", ItemKind::Consumable, 190,
                               "Пахнет пылью линий. Держит, пока держишься сам.");
    cdraught.heal_hp = 90; cdraught.heal_ap = 6;
    cdraught.effect = "might"; cdraught.effect_turns = 10; cdraught.effect_power = 2;
    add(cdraught);

    ItemDef wstaff = mk_item("walk_staff", "Посох ходока", ItemKind::Weapon, 1250,
                             "Стёрт снизу на две ладони. Двести лет — это много шагов.");
    wstaff.bonus.dmg_min = 12; wstaff.bonus.dmg_max = 22;
    wstaff.bonus.attack = 14; wstaff.bonus.block = 10; wstaff.bonus.max_ap = 2;
    add(wstaff);

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

// ----------------------------------------------------------------- записки

void Content::build_notes() {
    auto add = [&](const std::string& id, const std::string& title,
                   const std::vector<std::string>& lines) {
        NoteDef n; n.id = id; n.title = title; n.lines = lines;
        notes_[id] = n;
    };

    add("ink", "Рецепт чернил", {
        "Писано рукой торговца, неровно:",
        "",
        "Орешек чернильный истолочь, три штуки",
        "на кружку. Залить дождевой водой, держать",
        "в тепле три дня. Прибавить сажи на кончике",
        "ножа и капнуть смолы, чтобы не выцветало.",
        "",
        "Без орешка не выйдет ничего. Проверено."
    });

    add("miner", "Записка рудокопа", {
        "Уголь тут пустой, зря лезли. Но глубже",
        "нашли ход, и там паутина в руку толщиной.",
        "",
        "Семён сунулся первым. Больше не выходил.",
        "Уходим налегке, бросаем инструмент.",
        "",
        "Кто прочтёт — не ходи один."
    });

    add("watch", "Последний рапорт", {
        "Пост три, ночь. Обоза не будет, дорога",
        "перерезана. Нас осталось семеро.",
        "",
        "Казну заперли, ключ у десятника. Если",
        "падём — искать при нём, больше негде.",
        "",
        "Держимся до утра."
    });

    add("zero", "Обрывок из святилища", {
        "…и тогда стало ясно, что расстояние —",
        "не свойство мира, а свойство идущего.",
        "",
        "Там, где линии сходятся, шаг из одной",
        "точки кончается в другой. Нужен только",
        "камень и тот, кто помнит дорогу.",
        "",
        "Стражи не злы. Они просто не пускают."
    });

    add("child", "Детский листок", {
        "Каракули, углём:",
        "",
        "«У старосты борода как метла",
        "у кузнеца руки как лопаты",
        "а у Лады пахнет мятой и она добрая»",
        "",
        "Внизу нарисован волк. Или собака."
    });

    add("proto", "Странный обрывок", {
        "Бумага не здешняя, слишком ровная.",
        "Список, зачёркнутый наполовину:",
        "",
        "  TODO:",
        "  1. отрисовка карты",
        "  не забыть, что игровые файлы в другой папке",
        "",
        "  toolate:",
        "  СУНДУКИ! toolate…",
        "",
        "Последняя строка обведена дважды."
    });

    add("seam", "Заметка на полях", {
        "Приписка чужой рукой поверх старого чертежа:",
        "",
        "«Зал сходится к алтарю, но алтарь — не конец.",
        "За северной гранью шов, заложенный изнутри.",
        "Открывается ключом самого стража: он и есть",
        "замок, пока стоит.»",
        "",
        "Ниже, торопливо: «Никому. Особенно Ордену.»"
    });

    add("cinch", "Отчёт о Стяжении", {
        "Лист плотный, с водяным знаком в виде узла.",
        "",
        "«День сорок первый. Сеть не порвалась.",
        "Она затянулась. Мы полагали, что связываем",
        "точки; на деле мы стягивали расстояние,",
        "и оно поддалось.",
        "",
        "Заставу с южной дороги принесло к лесу,",
        "которого от неё было двенадцать дней пути.",
        "Половину Верхнего Города не нашли вовсе.",
        "",
        "Хуже другое: узел продолжает тянуть. Медленно,",
        "но мы измерили — за поколение на локоть.",
        "Значит, Стяжение не кончилось. Оно идёт.",
        "",
        "Остановить может тот, кто помнит дорогу.",
        "Мы таких больше не делаем.»"
    });

    add("order", "Устав, лист девятый", {
        "«…и потому послушник, вошедший в узел,",
        "не выходит прежним. Отсюда правило:",
        "в узел — только вдвоём, и второй остаётся",
        "снаружи, чтобы было кому вспомнить.",
        "",
        "Стражи ставятся не против людей. Стражи",
        "ставятся против того, что идёт по линиям",
        "с той стороны.»",
        "",
        "Дальше вырвано."
    });

    add("double", "Жалоба на межевание", {
        "«…а хутор Двоеданный оттого так и зовут,",
        "что дворов там два, а хутор один. Стоят",
        "друг против друга, и в каждом свой Прохор,",
        "и оба божатся, что настоящий.",
        "",
        "Староста наш ездил разбирать и вернулся",
        "мрачный. Сказал только: межу не проведёшь",
        "там, где земля сама себя догнала.»"
    });

    // --- Регион II: Шов ---

    add("goat", "Путевой лист обоза", {
        "«Вышли шестого, четыре подводы, соль и лён.",
        "Козьей тропой, как всегда.",
        "",
        "Седьмого: тропа длиннее, чем помню. Шли весь",
        "день, а перевал всё тот же.",
        "",
        "Восьмого: вышли не туда. Внизу поле, всё",
        "стеклянное. Такого нет ни на одной карте.",
        "Возвращаться некуда — тропа за спиной другая.",
        "",
        "Гурию скажите, что не по нашей вине.»"
    });

    add("glass", "Свидетельство стекольщика", {
        "«Отец говорил: поле спеклось за одну ночь,",
        "и не от огня. Земля не горела — она сошлась",
        "сама с собой, и в месте схождения потекла.",
        "",
        "Стекло с того поля помнит, чем было. Если",
        "смотреть в осколок долго, видно поле до",
        "Стяжения: рожь и межа.",
        "",
        "Я смотрел. Больше не смотрю.»"
    });

    add("mill", "Расчёт мельника", {
        "«Считал три года. В Тихую втекает вода,",
        "а вытекает вполовину меньше. Разницу",
        "не найти: ни в земле, ни в озере.",
        "",
        "Пошёл по руслу вниз, дошёл до старого",
        "затвора. За ним слышно воду. Много.",
        "Открывать не стал: за затвором солёное,",
        "а до моря отсюда четыреста вёрст.",
        "",
        "Ключ держу при себе. И держать буду.»"
    });

    add("market", "Уложение Рынка Шва", {
        "«Правило первое: у товара не спрашивают года.",
        "Правило второе: у человека не спрашивают",
        "лоскута.",
        "Правило третье: кто спросил — тот и платит.",
        "",
        "Рынок стоит на шве и никому не принадлежит.",
        "Здесь сходятся те, кого Стяжение развело",
        "на четыреста лет, и торгуют, как ни в чём",
        "не бывало. Это единственное, что у нас есть",
        "общего: цена.»"
    });

    add("prohor", "Записка Прохора о Прохоре", {
        "«Пишу, чтоб не сойти с ума.",
        "",
        "Он знает всё, что знаю я. Помнит, как мать",
        "звала со двора. Помнит, где у меня шрам,",
        "и шрам у него на том же месте.",
        "",
        "Я думал: пусть уходит. А потом понял —",
        "он думает то же самое про меня, и с тем же",
        "правом. Один из нас лишний, и никто из нас",
        "не знает, который.",
        "",
        "Если придёт чужой и рассудит — приму любое."
    });

    add("caravan", "Опись каравана", {
        "«Двенадцать подвод, сорок душ, пряности",
        "и соль. Ночуем в сарае у Шва.",
        "",
        "Хозяин накрыл столы, но сам не сел. Сказал:",
        "«Ешьте, а я подожду, пока пройдёт.»",
        "Мы спросили — что пройдёт?",
        "Он сказал: «Оно уже идёт.»",
        "",
        "Похлёбка ещё горячая. Пишу и не понимаю,",
        "почему пишу.»"
    });

    add("salt", "Наказ копача", {
        "«В нижних штольнях соль другая. Она держит.",
        "Мышь, попавшая туда осенью, весной как живая,",
        "только не дышит.",
        "",
        "Мы вынули из пласта человека. Одежда",
        "орденская, узел на вороте. Пролежал двести",
        "лет и не истлел.",
        "",
        "Он открыл глаза. Мы заложили штольню.",
        "Кто прочтёт — не разбирай кладку.»"
    });

    add("bridge", "Донесение о мосте", {
        "«Мост висел через ущелье и вёл к трём дворам",
        "на той стороне. После Стяжения ущелья нет,",
        "а мост есть. Обрывается в воздухе на середине.",
        "",
        "Канат осмотрели. Не перетёрся: срез ровный,",
        "как ножом. Только резать было нечем и некому —",
        "обрыв висит в двадцати саженях над ничем.",
        "",
        "На той стороне иногда виден дым. Значит,",
        "дворы уцелели. Значит, они дрейфуют.»"
    });

    // --- Регион III: Половины ---

    add("cityhalf", "Прошение о восстановлении", {
        "«В городскую управу, от жителей Верхней стороны.",
        "",
        "Просим определить, кому подчиняется наша половина.",
        "Ратуша осталась на той стороне среза. Казначейство",
        "тоже. Управа, куда мы это пишем, — на нашей, но",
        "управляет она половиной города, а числится за целым.",
        "",
        "Срез прошёл по Мучной улице. Дома с чётной стороны",
        "стоят, с нечётной — нет. Не разрушены: их просто",
        "нет, и мостовая обрывается ровно, как ножом.",
        "",
        "Ответа не последовало. Ждём двести лет.»"
    });

    add("endless", "Заметка обходчика", {
        "«Обход по Кольцевой, третий год.",
        "",
        "Улица не кольцевая. Я проверял: она прямая.",
        "Идёшь до конца — выходишь в начало, но поворота",
        "нет. Считал шаги: восемьсот сорок, всегда.",
        "",
        "Выход есть, но не в конце. Он сбоку, и открыт",
        "только тому, кто понял, что конца нет.",
        "Понять — это не догадаться. Это перестать идти.»"
    });

    add("deadwater", "Наставление перевозчику", {
        "«Вода в канале не течёт и не сохнет. Стоит",
        "с самого Стяжения на одном уровне.",
        "",
        "Ходить по ней можно — держит. Плавать нельзя:",
        "не тонешь и не выплываешь, так и стоишь.",
        "",
        "Жетон бери с каждого. Не ради денег: жетон —",
        "это счёт. Сколько роздал, столько должно",
        "вернуться. Если вернётся меньше — кто-то",
        "остался на воде, и его надо идти искать.»"
    });

    add("counting", "Черновик Счетовода", {
        "«Замер сорок девятый.",
        "",
        "Беру две точки, между которыми до Стяжения",
        "было триста саженей. Меряю. Двести девяносто",
        "одна.",
        "",
        "Замер сорок восьмой, прошлый год: двести",
        "девяносто три.",
        "",
        "Две сажени в год. Не локоть в поколение, как",
        "писали орденские. Две сажени. В год.",
        "",
        "Я пересчитывал четырежды. Я хочу ошибиться.»"
    });

    add("lists", "Опись городского архива", {
        "«Списки жителей за год Стяжения.",
        "",
        "Всего душ по переписи: четыре тысячи двести.",
        "Числится на нашей половине: две тысячи сто.",
        "Числится на той: две тысячи сто.",
        "",
        "Сходится. Но фамилии в списках не разделены",
        "по улицам — они разрезаны по буквам. Половина",
        "имени здесь, половина там.",
        "",
        "Мы не знаем, кто выжил. Мы знаем только,",
        "что каждого — половина.»"
    });

    add("foundry", "Запись в цеховой книге", {
        "«Смену отработали полным составом.",
        "Печь не гасили с шестого числа.",
        "",
        "Мастер сказал: пока льём — стоим. Перестанем",
        "лить — поймём, что кончилось, и тогда всё.",
        "Никто не спорил.",
        "",
        "Лили двести лет. Заказов нет, возить некому,",
        "металл берём из своих же отливок.",
        "Мастер прав: пока льём — стоим.»"
    });

    add("halves", "Донесение о второй половине", {
        "«С Башни Счетовода в ясный день видно вторую",
        "половину. Не на горизонте — выше него.",
        "",
        "Она не стоит на земле. Она дрейфует, и её",
        "сносит. За двенадцать лет прошла треть неба.",
        "",
        "Там дым из труб. Там живут и, надо думать,",
        "тоже смотрят сюда и тоже пишут донесения.",
        "",
        "Если Стяжение доведут до конца — половины",
        "сойдутся. Если сеть разрежут — разойдутся",
        "навсегда. Третьего для нас нет.»"
    });

    add("lastclerk", "Последняя запись писаря", {
        "«Сего числа переписал начисто списки",
        "и запер архив.",
        "",
        "Ключ отдал смотрителю, как положено. Смотритель",
        "с тех пор не сменялся и ключа не отдаёт.",
        "Я спрашивал. Он смотрит и молчит.",
        "",
        "Он не злой. Он просто на посту, а поста",
        "уже двести лет как нет.»"
    });

    // --- Регион IV: Орден ---

    add("gates", "Табличка у ворот", {
        "«Входящий предъявляет печать.",
        "Печать не выдаётся, а передаётся: от того,",
        "кто выходит, тому, кто входит.",
        "",
        "Если ты держишь печать и не помнишь, кто",
        "тебе её передал, — значит, тот не вышел.",
        "Входи и помни об этом.»"
    });

    add("watchwrit", "Приказ, который не дошёл", {
        "«Караулу привратной башни.",
        "",
        "Пост снять. Смену не высылать. Обитель",
        "закрывается на неопределённый срок.",
        "",
        "Подписано: настоятель. Скреплено печатью.",
        "",
        "Приписка гонца: не донёс. Дороги нет —",
        "ни туда, ни обратно. Оставляю в ящике,",
        "может, кто прочтёт.",
        "",
        "Прочли. Через двести лет.»"
    });

    add("read", "Каталог, полка первая", {
        "«О расстоянии как о свойстве.",
        "О расстоянии как о веществе.",
        "О расстоянии как о привычке.",
        "",
        "Ниже, другой рукой:",
        "«Все три ошибочны. Расстояние — это",
        "согласие. Мир согласен быть большим,",
        "пока его об этом не переспрашивают.",
        "",
        "Мы переспросили.»"
    });

    add("unsealed", "То, что вырвали", {
        "Лист совпадает с обрывом в уставе, листе",
        "девятом. Его вырвали, но не сожгли.",
        "",
        "«…и потому связывать последний узел",
        "запрещается, доколе не будет найден способ",
        "развязать.",
        "",
        "Мы такого способа не имеем. Мы имеем лишь",
        "уверенность, что он существует, а это не",
        "одно и то же.",
        "",
        "Первый Мастер настаивал на внесении сего",
        "в устав. Совет внёс. Совет же и вырвал,",
        "когда Мастер ушёл.»"
    });

    add("charts", "Пояснение к чертежу", {
        "«Сеть на листе выглядит колесом: узлы",
        "по ободу, спицы к середине.",
        "",
        "Это неверно. Спиц нет. Узлы связаны",
        "не с серединой, а друг с другом, и середина",
        "образуется сама, когда связей становится",
        "достаточно.",
        "",
        "Мы не строили Точку Ноль. Мы её получили.»"
    });

    add("novice", "Письмо домой, не отправленное", {
        "«Матушка, у нас тут кормят хорошо и учат",
        "многому, только я почти ничего не понимаю.",
        "",
        "Завтра первый раз в узел. Пойдём вдвоём",
        "с Игнатом: правило такое — в узел вдвоём,",
        "и один остаётся снаружи, чтобы было кому",
        "вспомнить.",
        "",
        "Игнат остаётся. Он говорит, что в другой",
        "раз поменяемся.",
        "",
        "Другого раза не было.»"
    });

    add("keepsake", "Опись имущества кельи", {
        "«Келья одиннадцатая. Послушник Игнат.",
        "",
        "Плащ шерстяной — один.",
        "Обувь — одна пара.",
        "Солдатик оловянный — один.",
        "",
        "Помета настоятеля: солдатика не выбрасывать.",
        "Игнат просил сохранить и передать, если",
        "кто-нибудь когда-нибудь придёт.",
        "",
        "Он ждал сорок лет и ушёл сам. Куда — не",
        "сказал. Сказал только: буду сидеть у огня.»"
    });

    add("ovens", "Правило печи", {
        "«Печь не плавит. Печь берёт.",
        "",
        "Кладёшь вещь — получаешь золу и то, чего",
        "в вещи не было. Обратно вещь не достаётся:",
        "печь не хранилище, печь — обмен.",
        "",
        "Клали книги, кольца, однажды человека.",
        "После человека печь молчала полгода,",
        "и настоятель запретил.",
        "",
        "Запрет висит до сих пор. Печь до сих пор",
        "тёплая.»"
    });

    add("refusal", "Стенограмма Зала", {
        "«Совет: последний узел готов к связыванию.",
        "Первый Мастер: не связывайте.",
        "Совет: основание?",
        "Первый Мастер: у нас нет способа развязать.",
        "Совет: он не потребуется.",
        "Первый Мастер: это и есть основание.",
        "",
        "Голосование: одиннадцать за, один против.",
        "",
        "Первый Мастер снял кольцо, положил на стол",
        "и вышел. В протокол внесено, что он был",
        "не в себе.",
        "",
        "Через пять лет случилось Стяжение.»"
    });

    add("secondnode", "Отметка на Втором", {
        "«Узел Второй держит. Проверено сего числа.",
        "",
        "Держать — это не стоять. Это тянуть в другую",
        "сторону ровно с той силой, с какой тянет",
        "сеть. Устанешь — стянется.",
        "",
        "Второй тянет двести лет. Мы не знаем чем.»"
    });

    add("thirdnode", "Отметка на Третьем", {
        "«Узел Третий сломан.",
        "",
        "Не разрушен — именно сломан, как ломается",
        "счёт, если сбиться. Он не держит и не тянет.",
        "Он просто открыт.",
        "",
        "Через него уходит то, что не пристаёт ни",
        "к чему: обрывки земли, дома, люди. Уходит",
        "и дрейфует.",
        "",
        "Если искать пропавших — искать там.»"
    });

    add("emptygrave", "Надпись на пустой могиле", {
        "«Здесь не лежит Первый Мастер.",
        "",
        "Он не умер в обители и не вернулся в неё.",
        "Могилу вырыли на случай, если вернётся",
        "и умрёт, — так у нас положено.",
        "",
        "Место осталось свободным. Мы оставили",
        "камень, чтобы кто-нибудь однажды дописал.»",
        "",
        "Ниже, недавно, углём и другой рукой:",
        "«Не дописывайте. Я ещё хожу.»"
    });

    // --- Регион V: Дрейф ---

    add("drift", "Как понять, что дрейфуешь", {
        "«Земля под ногами твёрдая, и это сбивает.",
        "",
        "Проверять надо по небу. Если облака идут",
        "в одну сторону, а тени от них — в другую,",
        "значит, идёшь не ты и не облака: идёт",
        "лоскут, на котором стоишь.",
        "",
        "Второй способ: брось камень и слушай. Здесь",
        "он падает чуть позже, чем должен.",
        "",
        "Третий способ — спросить у местных. Не",
        "советую. Они не знают и расстраиваются.»"
    });

    add("tomorrow", "Запись хозяйки", {
        "«Завтра выходим затемно. Подводы гружены,",
        "кони кормлены, похлёбка поставлена на утро.",
        "",
        "Хозяин сарая сказал ждать, пока пройдёт.",
        "Прошло, наверное: тихо стало.",
        "",
        "Пишу вечером. Завтра допишу, что дошли.»",
        "",
        "Ниже — та же рука, тот же вечер, ещё раз.",
        "И ещё раз. Страница исписана до низу",
        "одной и той же записью."
    });

    add("otherside", "С той стороны среза", {
        "«Прошение о восстановлении улицы подано",
        "и здесь. Ответа нет и здесь.",
        "",
        "Мучная улица кончается ровно там же, где",
        "у них. Мы стоим на своём краю, они на своём,",
        "и между нами четыре шага и всё небо.",
        "",
        "Кричать пробовали. Слышно. Отвечать",
        "перестали: за двести лет надоедает",
        "перекрикиваться с теми, до кого не дойти.»"
    });

    add("stair", "Замер лестницы", {
        "«Ступеней двести двенадцать. Считал трижды.",
        "",
        "Поднимаешься двести двенадцать — выходишь",
        "внизу. Не разворачиваешься: именно выходишь",
        "внизу, лицом туда же, куда шёл.",
        "",
        "Спускаться пробовал. Спуска нет: ступени",
        "под ногой всегда поднимаются.",
        "",
        "Я не устал. В этом и беда — я не устал.»"
    });

    add("lastorder", "Приказ, отданный вчера", {
        "«Держать до темноты. Подмога к вечеру.»",
        "",
        "Бумага свежая. Чернила не выцвели.",
        "",
        "На обороте, карандашом, много раз:",
        "«темноты нет».",
        "«темноты нет».",
        "«темноты нет»."
    });

    add("marks", "Зарубки на стволе", {
        "Зарубок на стволе сто с лишним, и все",
        "одинаково старые. Ни одна не заросла.",
        "",
        "Ниже вырезано ножом:",
        "«Считал дни. Дни не идут — идёт счёт.",
        "Бросил на сто четвёртой.",
        "",
        "Хорошо тут. Вот это и плохо.»"
    });

    add("wellrule", "Правило колодца", {
        "«Спускаешь одно ведро — поднимаешь два.",
        "",
        "Первое — твоё. Второе не твоё, но такое же.",
        "Вода в нём холоднее, и она не портится.",
        "",
        "Пить можно. Оставлять на виду нельзя:",
        "второе ведро к утру наливается само.",
        "",
        "Мы не знаем, откуда берётся вторая вода.",
        "Мы знаем, что где-то её на одно ведро",
        "меньше.»"
    });

    add("homeward", "Наказ проводника", {
        "«Дорог отсюда много, надёжная одна.",
        "",
        "Приметы: тропа не петляет, трава примята",
        "в обе стороны, и на середине пути слышно",
        "воду, которой нигде не видно.",
        "",
        "Идти молча. Оборачиваться можно —",
        "возвращаться нельзя. Кто повернул назад,",
        "выходит не туда, откуда шёл.»"
    });

    add("houses", "Роспись домов", {
        "«Дом первый — Мирон, староста.",
        "Дом второй — Лада, травница.",
        "Дом третий — Бран, кузнец.",
        "Дом четвёртый — Гурий, приезжий.",
        "…",
        "Всё сходится. Всё до одного.",
        "",
        "Дом последний — бирка пустая.",
        "Рукой писаря приписано: «ждёт».",
        "",
        "Ключ на гвозде рядом. Тоже с пустой биркой.»"
    });

    add("edgeview", "Наблюдение с края", {
        "«Дальше земли нет, и это не обрыв: обрыв",
        "предполагает низ.",
        "",
        "Видно другие лоскуты. Считал — семнадцать.",
        "На четырёх дым. На одном, кажется, машут.",
        "",
        "Все идут в одну сторону, и я наконец понял,",
        "в какую: к середине. Нас не разбросало —",
        "нас сносит вместе.",
        "",
        "Стяжение не кончилось. Оно просто идёт",
        "медленнее, чем нам казалось, и мы внутри.»"
    });

    // --- Регион VI: Изнанка ---

    add("oldseam", "Опись Первого Шва", {
        "«Стык первый. Соединены лоскут северный",
        "и лоскут южный. Крепление: слово.",
        "",
        "Так и записано в описи: слово. Не замок,",
        "не узел, не скоба — слово.",
        "",
        "Первый Мастер сказал: «Держать буду я»,",
        "и это внесли в опись как крепление,",
        "потому что другого не было.",
        "",
        "Стык держит до сих пор. Значит, он ещё",
        "не отказался от своих слов.»"
    });

    add("readlines", "Как читать линии", {
        "«Линия видна, если не смотреть на неё прямо.",
        "Смотри чуть мимо — и увидишь.",
        "",
        "Толстая линия — короткая дорога. Тонкая —",
        "длинная. Оборванная — та, по которой кто-то",
        "прошёл в последний раз.",
        "",
        "Оборванных теперь больше половины. Мы это",
        "называли «износ» и чинили. Потом перестали",
        "чинить, потому что перестали успевать.",
        "",
        "Ниже, другой рукой: «Не износ. Их подтянули",
        "нарочно, чтобы стало ближе. Ближе стало.»"
    });

    add("roomrule", "Правило комнаты", {
        "«В этой комнате расстояние — вещь.",
        "Её можно взять в руку, положить в суму,",
        "потерять.",
        "",
        "Мерой мерили, прежде чем тянуть: сколько",
        "убрать и сколько оставить. Оставить хотели",
        "половину.",
        "",
        "Мера уцелела одна. Держащий её решает,",
        "далеко ли до всего остального.",
        "",
        "Осторожно. Тут был человек, который решил,",
        "что до всего далеко, и мы его больше",
        "не нашли — хотя он не выходил.»"
    });

    add("walkers", "Список ушедших внутрь", {
        "«В сеть уходили не только по глупости.",
        "",
        "Первый Мастер — сам, после Зала Отказа.",
        "Разметчик Пров — за ним, догонять.",
        "Сторож Елисей — держать стык, пока не вернутся.",
        "Ещё двадцать девять — по разным причинам.",
        "",
        "Вернулся один и ничего не рассказал.",
        "Сказал только: там некуда возвращаться,",
        "потому что оттуда никуда не уходил.",
        "",
        "Мы его не поняли. Теперь, кажется, понимаем.»"
    });

    add("firstnode", "Узел Первый", {
        "«Первый вязали втроём и без чертежа —",
        "чертить было ещё нечего.",
        "",
        "Он вышел кривой. Все остальные вязали",
        "по нему, и потому кривые все.",
        "",
        "Развязать его нельзя: он не завязан,",
        "он сросся. Разрезать можно, но тогда",
        "разойдётся всё, что вязали после.",
        "",
        "Мы записали это как «особенность»",
        "и больше к ней не возвращались.»"
    });

    add("cinchwork", "Устройство хода", {
        "«Ход не крутится и не тикает. Он тянет.",
        "",
        "Тянет тем, что расстояние между двумя",
        "точками для него всегда чуть меньше, чем",
        "было мгновение назад. Он не делает ничего",
        "лишнего. Он просто не умеет иначе.",
        "",
        "Остановить его можно. Он не сопротивляется —",
        "он даже не знает, что кто-то пришёл.",
        "",
        "Только помни: пока он тянет, всё держится",
        "вместе. Мы не проверяли, что будет, если",
        "перестанет.»"
    });

    add("allatonce", "Что здесь есть", {
        "«Точка Ноль. Расстояние до всего — ноль.",
        "",
        "Отсюда видно Ольховку, Рынок Шва, обе",
        "половины Города, дно океана и то место,",
        "где ты стоял вчера. Всё сразу.",
        "",
        "И потому здесь нет ничего: чтобы что-то",
        "было, оно должно быть в стороне от",
        "остального, а сторон тут нет.",
        "",
        "Мы приходили сюда мерить. Померили один раз",
        "и больше не приходили: мерить нечего,",
        "и меряющего тоже нечем отделить.»"
    });

    add("threeways", "Три исхода", {
        "Лист лежит на камне, придавленный кольцом.",
        "Рука Первого Мастера, писано без спешки.",
        "",
        "«Дотянуть. Все рядом, никто никуда не идёт.",
        "Разрезать. Все далеко, и каждый сам.",
        "Удержать. Всё как есть, и держать вечно.",
        "",
        "Я стою здесь двести лет и не выбрал.",
        "Не потому, что не решаюсь: потому, что",
        "выбирать должен тот, кому потом жить.",
        "",
        "Мне-то уже всё равно, а это плохая",
        "рекомендация для выбирающего.",
        "",
        "Кто прочтёт — выбирай сам. И знай, что",
        "правильного нет. Есть только твой.»"
    });

    add("hermit", "Страница из дневника", {
        "Двадцать лет назад я тоже думал, что",
        "сила решает. Ходил в ярости, бил первым.",
        "",
        "Потом понял: удар копит удар. Тот, кто",
        "не пропускает, бьёт втрое. Тот, кто лезет",
        "напролом, устаёт первым.",
        "",
        "Стойка — это не про плечи. Это про то,",
        "готов ли ты ждать."
    });
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
                     Drop("seam_key", 100), Drop("plate_armor", 40) };
    keeper.detect = 8; keeper.kill_counter = "kill_keeper";
    keeper.on_hit_effect = "slow"; keeper.on_hit_chance = 35; keeper.on_hit_power = 1;
    add(keeper);

    // --- Регион II: Шов ---

    EnemyDef stray;
    stray.id = "stray"; stray.name = "Приблудный";
    stray.stats.max_hp = 42; stray.stats.max_ap = 9; stray.stats.attack = 72;
    stray.stats.dmg_min = 4; stray.stats.dmg_max = 9; stray.stats.block = 12;
    stray.stats.armor = 2;   stray.stats.ap_atk = 4;
    stray.exp = 34; stray.gold_min = 8; stray.gold_max = 26;
    stray.drops = { Drop("bread", 45), Drop("old_coin", 40), Drop("dagger", 15) };
    stray.detect = 6; stray.kill_counter = "kill_stray";
    add(stray);

    EnemyDef ghound;
    ghound.id = "glass_hound"; ghound.name = "Стеклянный пёс";
    ghound.stats.max_hp = 48; ghound.stats.max_ap = 10; ghound.stats.attack = 76;
    ghound.stats.dmg_min = 5; ghound.stats.dmg_max = 10; ghound.stats.block = 8;
    ghound.stats.armor = 3;   ghound.stats.ap_atk = 4;
    ghound.exp = 44; ghound.gold_min = 0; ghound.gold_max = 6;
    ghound.drops = { Drop("glass_shard", 70) };
    ghound.detect = 7; ghound.kill_counter = "kill_hound";
    ghound.on_hit_effect = "bleed"; ghound.on_hit_chance = 40; ghound.on_hit_power = 2;
    add(ghound);

    EnemyDef mrat;
    mrat.id = "mill_rat"; mrat.name = "Мучной крыс";
    mrat.stats.max_hp = 24; mrat.stats.max_ap = 8; mrat.stats.attack = 64;
    mrat.stats.dmg_min = 2; mrat.stats.dmg_max = 6; mrat.stats.block = 6;
    mrat.stats.armor = 1;   mrat.stats.ap_atk = 3;
    mrat.exp = 17; mrat.gold_min = 0; mrat.gold_max = 5;
    mrat.drops = { Drop("rat_tail", 60), Drop("bread", 30) };
    mrat.detect = 5; mrat.kill_counter = "kill_millrat";
    add(mrat);

    EnemyDef ghoul;
    ghoul.id = "salt_ghoul"; ghoul.name = "Солевик";
    ghoul.stats.max_hp = 58; ghoul.stats.max_ap = 9; ghoul.stats.attack = 74;
    ghoul.stats.dmg_min = 5; ghoul.stats.dmg_max = 11; ghoul.stats.block = 10;
    ghoul.stats.armor = 4;   ghoul.stats.ap_atk = 4;
    ghoul.exp = 52; ghoul.gold_min = 4; ghoul.gold_max = 18;
    ghoul.drops = { Drop("salt_lump", 65), Drop("old_coin", 30) };
    ghoul.detect = 6; ghoul.kill_counter = "kill_ghoul";
    ghoul.on_hit_effect = "slow"; ghoul.on_hit_chance = 35; ghoul.on_hit_power = 1;
    add(ghoul);

    EnemyDef shade;
    shade.id = "caravan_shade"; shade.name = "Тень каравана"; shade.female = true;
    shade.stats.max_hp = 50; shade.stats.max_ap = 10; shade.stats.attack = 78;
    shade.stats.dmg_min = 4; shade.stats.dmg_max = 10; shade.stats.block = 18;
    shade.stats.armor = 2;   shade.stats.ap_atk = 4;
    shade.exp = 48; shade.gold_min = 10; shade.gold_max = 30;
    shade.drops = { Drop("old_coin", 70), Drop("spice_bag", 35) };
    shade.detect = 7; shade.kill_counter = "kill_shade";
    shade.on_hit_effect = "weaken"; shade.on_hit_chance = 35; shade.on_hit_power = 1;
    add(shade);

    EnemyDef smother;
    smother.id = "salt_mother"; smother.name = "Соляная матерь"; smother.female = true;
    smother.stats.max_hp = 135; smother.stats.max_ap = 11; smother.stats.attack = 82;
    smother.stats.dmg_min = 7; smother.stats.dmg_max = 15; smother.stats.block = 16;
    smother.stats.armor = 6;    smother.stats.ap_atk = 4;
    smother.exp = 200; smother.gold_min = 50; smother.gold_max = 95;
    smother.drops = { Drop("salt_lump", 100), Drop("salt_heart", 100),
                      Drop("brine_ring", 40) };
    smother.detect = 7; smother.kill_counter = "kill_saltmother";
    smother.on_hit_effect = "slow"; smother.on_hit_chance = 50; smother.on_hit_power = 2;
    add(smother);

    EnemyDef walker;
    walker.id = "bridge_walker"; walker.name = "Мостовой";
    walker.stats.max_hp = 118; walker.stats.max_ap = 11; walker.stats.attack = 84;
    walker.stats.dmg_min = 8; walker.stats.dmg_max = 14; walker.stats.block = 20;
    walker.stats.armor = 5;    walker.stats.ap_atk = 4;
    walker.exp = 185; walker.gold_min = 40; walker.gold_max = 80;
    walker.drops = { Drop("rope_end", 100), Drop("glass_shard", 100),
                     Drop("frost_shard", 50) };
    walker.detect = 8; walker.kill_counter = "kill_walker";
    walker.on_hit_effect = "bleed"; walker.on_hit_chance = 40; walker.on_hit_power = 2;
    add(walker);

    // --- Регион III: Половины ---

    EnemyDef crat;
    crat.id = "city_rat"; crat.name = "Городская крыса"; crat.female = true;
    crat.stats.max_hp = 52; crat.stats.max_ap = 9; crat.stats.attack = 76;
    crat.stats.dmg_min = 5; crat.stats.dmg_max = 11; crat.stats.block = 10;
    crat.stats.armor = 2;   crat.stats.ap_atk = 3;
    crat.exp = 46; crat.gold_min = 2; crat.gold_max = 14;
    crat.drops = { Drop("rat_tail", 60), Drop("scrap_iron", 35) };
    crat.detect = 6; crat.kill_counter = "kill_cityrat";
    add(crat);

    EnemyDef cutman;
    cutman.id = "cut_man"; cutman.name = "Срезанный";
    cutman.stats.max_hp = 74; cutman.stats.max_ap = 10; cutman.stats.attack = 80;
    cutman.stats.dmg_min = 6; cutman.stats.dmg_max = 12; cutman.stats.block = 14;
    cutman.stats.armor = 3;   cutman.stats.ap_atk = 4;
    cutman.exp = 72; cutman.gold_min = 8; cutman.gold_max = 30;
    cutman.drops = { Drop("old_coin", 45), Drop("city_brick", 40) };
    cutman.detect = 7; cutman.kill_counter = "kill_cutman";
    cutman.on_hit_effect = "weaken"; cutman.on_hit_chance = 40; cutman.on_hit_power = 2;
    add(cutman);

    EnemyDef clerk;
    clerk.id = "mad_clerk"; clerk.name = "Обезумевший счетовод";
    clerk.stats.max_hp = 64; clerk.stats.max_ap = 11; clerk.stats.attack = 78;
    clerk.stats.dmg_min = 5; clerk.stats.dmg_max = 10; clerk.stats.block = 16;
    clerk.stats.armor = 2;   clerk.stats.ap_atk = 3;
    clerk.exp = 64; clerk.gold_min = 12; clerk.gold_max = 36;
    clerk.drops = { Drop("ledger_page", 60), Drop("old_coin", 40) };
    clerk.detect = 7; clerk.kill_counter = "kill_clerk";
    clerk.on_hit_effect = "slow"; clerk.on_hit_chance = 35; clerk.on_hit_power = 1;
    add(clerk);

    EnemyDef slag;
    slag.id = "slag_thing"; slag.name = "Шлаковик";
    slag.stats.max_hp = 90; slag.stats.max_ap = 9; slag.stats.attack = 78;
    slag.stats.dmg_min = 7; slag.stats.dmg_max = 14; slag.stats.block = 8;
    slag.stats.armor = 6;   slag.stats.ap_atk = 4;
    slag.exp = 88; slag.gold_min = 6; slag.gold_max = 24;
    slag.drops = { Drop("scrap_iron", 70), Drop("ember", 30) };
    slag.detect = 6; slag.kill_counter = "kill_slag";
    slag.on_hit_effect = "burn"; slag.on_hit_chance = 40; slag.on_hit_power = 2;
    add(slag);

    EnemyDef walker3;
    walker3.id = "canal_walker"; walker3.name = "Ходящий по воде";
    walker3.stats.max_hp = 78; walker3.stats.max_ap = 10; walker3.stats.attack = 82;
    walker3.stats.dmg_min = 6; walker3.stats.dmg_max = 13; walker3.stats.block = 20;
    walker3.stats.armor = 3;   walker3.stats.ap_atk = 4;
    walker3.exp = 80; walker3.gold_min = 10; walker3.gold_max = 34;
    walker3.drops = { Drop("ferry_token", 55), Drop("frost_shard", 30) };
    walker3.detect = 8; walker3.kill_counter = "kill_canalwalker";
    walker3.on_hit_effect = "slow"; walker3.on_hit_chance = 45; walker3.on_hit_power = 2;
    add(walker3);

    EnemyDef moth;
    moth.id = "archive_moth"; moth.name = "Архивная моль"; moth.female = true;
    moth.stats.max_hp = 46; moth.stats.max_ap = 12; moth.stats.attack = 74;
    moth.stats.dmg_min = 4; moth.stats.dmg_max = 9; moth.stats.block = 22;
    moth.stats.armor = 1;   moth.stats.ap_atk = 3;
    moth.exp = 42; moth.gold_min = 0; moth.gold_max = 8;
    moth.drops = { Drop("ledger_page", 50) };
    moth.detect = 7; moth.kill_counter = "kill_moth";
    add(moth);

    EnemyDef hwarden;
    hwarden.id = "half_warden"; hwarden.name = "Смотритель Половины";
    hwarden.stats.max_hp = 205; hwarden.stats.max_ap = 12; hwarden.stats.attack = 88;
    hwarden.stats.dmg_min = 10; hwarden.stats.dmg_max = 19; hwarden.stats.block = 22;
    hwarden.stats.armor = 7;    hwarden.stats.ap_atk = 4;
    hwarden.exp = 400; hwarden.gold_min = 140; hwarden.gold_max = 250;
    hwarden.drops = { Drop("archive_key", 100), Drop("half_name", 100),
                      Drop("counter_lens", 45) };
    hwarden.detect = 8; hwarden.kill_counter = "kill_halfwarden";
    hwarden.on_hit_effect = "weaken"; hwarden.on_hit_chance = 45; hwarden.on_hit_power = 2;
    add(hwarden);

    EnemyDef smaster;
    smaster.id = "slag_master"; smaster.name = "Мастер Литейной";
    smaster.stats.max_hp = 185; smaster.stats.max_ap = 11; smaster.stats.attack = 86;
    smaster.stats.dmg_min = 10; smaster.stats.dmg_max = 18; smaster.stats.block = 16;
    smaster.stats.armor = 9;    smaster.stats.ap_atk = 4;
    smaster.exp = 350; smaster.gold_min = 110; smaster.gold_max = 210;
    smaster.drops = { Drop("foundry_hammer", 100), Drop("scrap_iron", 100),
                      Drop("slag_shield", 50) };
    smaster.detect = 7; smaster.kill_counter = "kill_slagmaster";
    smaster.on_hit_effect = "burn"; smaster.on_hit_chance = 50; smaster.on_hit_power = 3;
    add(smaster);

    EnemyDef archivist;
    archivist.id = "archivist"; archivist.name = "Архивариус Ордена";
    archivist.stats.max_hp = 150; archivist.stats.max_ap = 12; archivist.stats.attack = 88;
    archivist.stats.dmg_min = 9; archivist.stats.dmg_max = 18; archivist.stats.block = 22;
    archivist.stats.armor = 7;    archivist.stats.ap_atk = 4;
    archivist.exp = 300; archivist.gold_min = 120; archivist.gold_max = 220;
    archivist.drops = { Drop("order_seal", 100), Drop("rune_stone", 100),
                        Drop("portal_stone", 100) };
    archivist.detect = 8; archivist.kill_counter = "kill_archivist";
    archivist.on_hit_effect = "weaken"; archivist.on_hit_chance = 45; archivist.on_hit_power = 2;
    add(archivist);

    // --- Регион IV: Орден ---

    EnemyDef acolyte;
    acolyte.id = "acolyte"; acolyte.name = "Послушник, не вышедший";
    acolyte.stats.max_hp = 95; acolyte.stats.max_ap = 11; acolyte.stats.attack = 84;
    acolyte.stats.dmg_min = 7; acolyte.stats.dmg_max = 14; acolyte.stats.block = 18;
    acolyte.stats.armor = 4;   acolyte.stats.ap_atk = 4;
    acolyte.exp = 115; acolyte.gold_min = 14; acolyte.gold_max = 44;
    acolyte.drops = { Drop("torn_page", 40), Drop("order_draught", 35) };
    acolyte.detect = 7; acolyte.kill_counter = "kill_acolyte";
    acolyte.on_hit_effect = "weaken"; acolyte.on_hit_chance = 35; acolyte.on_hit_power = 2;
    add(acolyte);

    EnemyDef gguard;
    gguard.id = "gate_guard"; gguard.name = "Привратный страж";
    gguard.stats.max_hp = 115; gguard.stats.max_ap = 11; gguard.stats.attack = 86;
    gguard.stats.dmg_min = 9; gguard.stats.dmg_max = 16; gguard.stats.block = 24;
    gguard.stats.armor = 8;   gguard.stats.ap_atk = 4;
    gguard.exp = 145; gguard.gold_min = 20; gguard.gold_max = 60;
    gguard.drops = { Drop("gate_halberd", 25), Drop("scrap_iron", 45) };
    gguard.detect = 7; gguard.kill_counter = "kill_gguard";
    add(gguard);

    EnemyDef pages;
    pages.id = "page_swarm"; pages.name = "Ворох листов";
    pages.stats.max_hp = 72; pages.stats.max_ap = 13; pages.stats.attack = 82;
    pages.stats.dmg_min = 5; pages.stats.dmg_max = 11; pages.stats.block = 28;
    pages.stats.armor = 1;   pages.stats.ap_atk = 3;
    pages.exp = 92; pages.gold_min = 4; pages.gold_max = 22;
    pages.drops = { Drop("torn_page", 55), Drop("ledger_page", 40) };
    pages.detect = 8; pages.kill_counter = "kill_pages";
    add(pages);

    EnemyDef dshade;
    dshade.id = "draft_shade"; dshade.name = "Тень чертёжника"; dshade.female = true;
    dshade.stats.max_hp = 105; dshade.stats.max_ap = 11; dshade.stats.attack = 86;
    dshade.stats.dmg_min = 8; dshade.stats.dmg_max = 15; dshade.stats.block = 22;
    dshade.stats.armor = 3;   dshade.stats.ap_atk = 4;
    dshade.exp = 135; dshade.gold_min = 16; dshade.gold_max = 52;
    dshade.drops = { Drop("chart_piece", 55), Drop("torn_page", 30) };
    dshade.detect = 8; dshade.kill_counter = "kill_dshade";
    dshade.on_hit_effect = "slow"; dshade.on_hit_chance = 40; dshade.on_hit_power = 2;
    add(dshade);

    EnemyDef celld;
    celld.id = "cell_dweller"; celld.name = "Жилец кельи";
    celld.stats.max_hp = 88; celld.stats.max_ap = 10; celld.stats.attack = 82;
    celld.stats.dmg_min = 7; celld.stats.dmg_max = 13; celld.stats.block = 20;
    celld.stats.armor = 3;   celld.stats.ap_atk = 4;
    celld.exp = 108; celld.gold_min = 10; celld.gold_max = 38;
    celld.drops = { Drop("acolyte_hood", 25), Drop("order_draught", 40) };
    celld.detect = 6; celld.kill_counter = "kill_celld";
    add(celld);

    EnemyDef fborn;
    fborn.id = "furnace_born"; fborn.name = "Рождённый печью";
    fborn.stats.max_hp = 135; fborn.stats.max_ap = 10; fborn.stats.attack = 84;
    fborn.stats.dmg_min = 10; fborn.stats.dmg_max = 18; fborn.stats.block = 12;
    fborn.stats.armor = 9;   fborn.stats.ap_atk = 4;
    fborn.exp = 185; fborn.gold_min = 18; fborn.gold_max = 56;
    fborn.drops = { Drop("furnace_ash", 70), Drop("ember", 45) };
    fborn.detect = 7; fborn.kill_counter = "kill_fborn";
    fborn.on_hit_effect = "burn"; fborn.on_hit_chance = 45; fborn.on_hit_power = 3;
    add(fborn);

    EnemyDef recho;
    recho.id = "refusal_echo"; recho.name = "Эхо Отказа"; recho.female = true;
    recho.stats.max_hp = 125; recho.stats.max_ap = 12; recho.stats.attack = 88;
    recho.stats.dmg_min = 8; recho.stats.dmg_max = 16; recho.stats.block = 26;
    recho.stats.armor = 4;   recho.stats.ap_atk = 4;
    recho.exp = 175; recho.gold_min = 20; recho.gold_max = 64;
    recho.drops = { Drop("order_draught", 50), Drop("torn_page", 40) };
    recho.detect = 8; recho.kill_counter = "kill_recho";
    recho.on_hit_effect = "weaken"; recho.on_hit_chance = 45; recho.on_hit_power = 3;
    add(recho);

    EnemyDef nguard;
    nguard.id = "node_guard"; nguard.name = "Узловой страж";
    nguard.stats.max_hp = 165; nguard.stats.max_ap = 12; nguard.stats.attack = 90;
    nguard.stats.dmg_min = 11; nguard.stats.dmg_max = 19; nguard.stats.block = 24;
    nguard.stats.armor = 8;    nguard.stats.ap_atk = 4;
    nguard.exp = 245; nguard.gold_min = 40; nguard.gold_max = 100;
    nguard.drops = { Drop("node_core", 40), Drop("rune_stone", 55) };
    nguard.detect = 8; nguard.kill_counter = "kill_nguard";
    nguard.on_hit_effect = "slow"; nguard.on_hit_chance = 40; nguard.on_hit_power = 2;
    add(nguard);

    EnemyDef nheart;
    nheart.id = "node_heart"; nheart.name = "Сердце Узла"; nheart.female = true;
    nheart.stats.max_hp = 265; nheart.stats.max_ap = 13; nheart.stats.attack = 92;
    nheart.stats.dmg_min = 13; nheart.stats.dmg_max = 23; nheart.stats.block = 26;
    nheart.stats.armor = 9;    nheart.stats.ap_atk = 4;
    nheart.exp = 560; nheart.gold_min = 180; nheart.gold_max = 320;
    nheart.drops = { Drop("node_core", 100), Drop("node_shield", 60),
                     Drop("portal_stone", 100) };
    nheart.detect = 9; nheart.kill_counter = "kill_nheart";
    nheart.on_hit_effect = "slow"; nheart.on_hit_chance = 50; nheart.on_hit_power = 3;
    add(nheart);

    EnemyDef mshadow;
    mshadow.id = "master_shadow"; mshadow.name = "Тень Первого Мастера";
    mshadow.stats.max_hp = 290; mshadow.stats.max_ap = 13; mshadow.stats.attack = 94;
    mshadow.stats.dmg_min = 14; mshadow.stats.dmg_max = 25; mshadow.stats.block = 28;
    mshadow.stats.armor = 10;   mshadow.stats.ap_atk = 4;
    mshadow.exp = 620; mshadow.gold_min = 220; mshadow.gold_max = 400;
    // Кольца у Тени нет: Мастер оставил его на столе в Зале Отказа,
    // и получить его можно только там — признав его правоту.
    mshadow.drops = { Drop("order_plate", 100), Drop("node_core", 100),
                      Drop("acolyte_hood", 60), Drop("rune_stone", 100) };
    mshadow.detect = 9; mshadow.kill_counter = "kill_mshadow";
    mshadow.on_hit_effect = "weaken"; mshadow.on_hit_chance = 50; mshadow.on_hit_power = 3;
    add(mshadow);

    // --- Регион V: Дрейф ---

    EnemyDef dhare;
    dhare.id = "drift_hare"; dhare.name = "Заяц, сбившийся";
    dhare.stats.max_hp = 140; dhare.stats.max_ap = 14; dhare.stats.attack = 96;
    dhare.stats.dmg_min = 10; dhare.stats.dmg_max = 17; dhare.stats.block = 20;
    dhare.stats.armor = 4;    dhare.stats.ap_atk = 3;
    dhare.exp = 230; dhare.gold_min = 18; dhare.gold_max = 55;
    dhare.drops = { Drop("drift_grass", 55), Drop("bread", 30) };
    dhare.detect = 9; dhare.kill_counter = "kill_dhare";
    add(dhare);

    EnemyDef cshade;
    cshade.id = "cart_shade"; cshade.name = "Тень подводы";
    cshade.stats.max_hp = 185; cshade.stats.max_ap = 12; cshade.stats.attack = 94;
    cshade.stats.dmg_min = 12; cshade.stats.dmg_max = 21; cshade.stats.block = 24;
    cshade.stats.armor = 8;    cshade.stats.ap_atk = 4;
    cshade.exp = 285; cshade.gold_min = 40; cshade.gold_max = 110;
    // Путевой бирки у теней нет: её отдаёт Улита, и разговор с ней —
    // и есть ответ, за которым Гурий послал.
    cshade.drops = { Drop("drift_grass", 40), Drop("salt_lump", 45),
                     Drop("still_water", 30) };
    cshade.detect = 8; cshade.kill_counter = "kill_cshade";
    cshade.on_hit_effect = "slow"; cshade.on_hit_chance = 40; cshade.on_hit_power = 2;
    add(cshade);

    EnemyDef swalker;
    swalker.id = "stair_walker"; swalker.name = "Идущий по лестнице";
    swalker.stats.max_hp = 195; swalker.stats.max_ap = 14; swalker.stats.attack = 98;
    swalker.stats.dmg_min = 13; swalker.stats.dmg_max = 22; swalker.stats.block = 26;
    swalker.stats.armor = 7;    swalker.stats.ap_atk = 3;
    swalker.exp = 310; swalker.gold_min = 50; swalker.gold_max = 130;
    swalker.drops = { Drop("stair_hood", 25), Drop("drift_grass", 35) };
    swalker.detect = 9; swalker.kill_counter = "kill_swalker";
    swalker.on_hit_effect = "weaken"; swalker.on_hit_chance = 40; swalker.on_hit_power = 2;
    add(swalker);

    EnemyDef lhour;
    lhour.id = "last_hour"; lhour.name = "Боец последнего часа";
    lhour.stats.max_hp = 210; lhour.stats.max_ap = 13; lhour.stats.attack = 99;
    lhour.stats.dmg_min = 14; lhour.stats.dmg_max = 24; lhour.stats.block = 28;
    lhour.stats.armor = 9;    lhour.stats.ap_atk = 4;
    lhour.exp = 335; lhour.gold_min = 55; lhour.gold_max = 140;
    lhour.drops = { Drop("scrap_iron", 50), Drop("still_water", 35) };
    lhour.detect = 8; lhour.kill_counter = "kill_lasthour";
    lhour.on_hit_effect = "bleed"; lhour.on_hit_chance = 45; lhour.on_hit_power = 3;
    add(lhour);

    EnemyDef banner;
    banner.id = "bannerman"; banner.name = "Знаменосец";
    banner.stats.max_hp = 300; banner.stats.max_ap = 13; banner.stats.attack = 101;
    banner.stats.dmg_min = 16; banner.stats.dmg_max = 27; banner.stats.block = 30;
    banner.stats.armor = 11;   banner.stats.ap_atk = 4;
    banner.exp = 640; banner.gold_min = 200; banner.gold_max = 360;
    banner.drops = { Drop("torn_banner", 100), Drop("banner_shield", 70),
                     Drop("hour_blade", 45) };
    banner.detect = 9; banner.kill_counter = "kill_banner";
    banner.on_hit_effect = "bleed"; banner.on_hit_chance = 50; banner.on_hit_power = 3;
    add(banner);

    EnemyDef sleeper;
    sleeper.id = "grove_sleeper"; sleeper.name = "Спящий в роще"; sleeper.female = true;
    sleeper.stats.max_hp = 205; sleeper.stats.max_ap = 11; sleeper.stats.attack = 95;
    sleeper.stats.dmg_min = 12; sleeper.stats.dmg_max = 20; sleeper.stats.block = 22;
    sleeper.stats.armor = 6;    sleeper.stats.ap_atk = 4;
    sleeper.exp = 300; sleeper.gold_min = 45; sleeper.gold_max = 120;
    sleeper.drops = { Drop("grove_leaf", 55), Drop("drift_grass", 40) };
    sleeper.detect = 6; sleeper.kill_counter = "kill_sleeper";
    sleeper.on_hit_effect = "slow"; sleeper.on_hit_chance = 55; sleeper.on_hit_power = 3;
    add(sleeper);

    EnemyDef bucket;
    bucket.id = "second_bucket"; bucket.name = "Второе ведро"; bucket.female = true;
    bucket.stats.max_hp = 230; bucket.stats.max_ap = 12; bucket.stats.attack = 97;
    bucket.stats.dmg_min = 13; bucket.stats.dmg_max = 23; bucket.stats.block = 25;
    bucket.stats.armor = 8;    bucket.stats.ap_atk = 4;
    bucket.exp = 420; bucket.gold_min = 90; bucket.gold_max = 200;
    bucket.drops = { Drop("still_water", 100), Drop("two_bucket", 100) };
    bucket.detect = 7; bucket.kill_counter = "kill_bucket";
    add(bucket);

    EnemyDef orat;
    orat.id = "other_rat"; orat.name = "Крыса второй половины";
    orat.stats.max_hp = 150; orat.stats.max_ap = 13; orat.stats.attack = 93;
    orat.stats.dmg_min = 10; orat.stats.dmg_max = 18; orat.stats.block = 19;
    orat.stats.armor = 5;    orat.stats.ap_atk = 3;
    orat.exp = 240; orat.gold_min = 20; orat.gold_max = 70;
    orat.drops = { Drop("rat_tail", 60), Drop("still_water", 30) };
    orat.detect = 8; orat.kill_counter = "kill_orat";
    orat.on_hit_effect = "poison"; orat.on_hit_chance = 35; orat.on_hit_power = 2;
    add(orat);

    // Повтор дерётся тем, чем дерёшься ты, и потому неудобен.
    EnemyDef copy;
    copy.id = "own_copy"; copy.name = "Повтор";
    copy.stats.max_hp = 245; copy.stats.max_ap = 14; copy.stats.attack = 100;
    copy.stats.dmg_min = 14; copy.stats.dmg_max = 24; copy.stats.block = 30;
    copy.stats.armor = 9;    copy.stats.ap_atk = 3;
    copy.exp = 400; copy.gold_min = 70; copy.gold_max = 180;
    copy.drops = { Drop("drift_cloak", 35), Drop("still_water", 45) };
    copy.detect = 10; copy.kill_counter = "kill_copy";
    copy.on_hit_effect = "weaken"; copy.on_hit_chance = 45; copy.on_hit_power = 3;
    add(copy);

    EnemyDef ewind;
    ewind.id = "edge_wind"; ewind.name = "Ветер Края"; ewind.female = true;
    ewind.stats.max_hp = 330; ewind.stats.max_ap = 14; ewind.stats.attack = 103;
    ewind.stats.dmg_min = 17; ewind.stats.dmg_max = 29; ewind.stats.block = 30;
    ewind.stats.armor = 11;   ewind.stats.ap_atk = 4;
    ewind.exp = 720; ewind.gold_min = 250; ewind.gold_max = 430;
    ewind.drops = { Drop("edge_ring", 100), Drop("drift_cloak", 70),
                    Drop("portal_stone", 100) };
    ewind.detect = 10; ewind.kill_counter = "kill_edgewind";
    ewind.on_hit_effect = "slow"; ewind.on_hit_chance = 50; ewind.on_hit_power = 3;
    add(ewind);

    // --- Регион VI: Изнанка ---

    EnemyDef smoth;
    smoth.id = "seam_moth"; smoth.name = "Мотылёк шва"; smoth.female = false;
    smoth.stats.max_hp = 250; smoth.stats.max_ap = 15; smoth.stats.attack = 104;
    smoth.stats.dmg_min = 16; smoth.stats.dmg_max = 27; smoth.stats.block = 26;
    smoth.stats.armor = 8;    smoth.stats.ap_atk = 3;
    smoth.exp = 460; smoth.gold_min = 60; smoth.gold_max = 170;
    smoth.drops = { Drop("line_dust", 55), Drop("cinch_draught", 35) };
    smoth.detect = 9; smoth.kill_counter = "kill_smoth";
    smoth.on_hit_effect = "weaken"; smoth.on_hit_chance = 40; smoth.on_hit_power = 3;
    add(smoth);

    EnemyDef lwalker;
    lwalker.id = "line_walker"; lwalker.name = "Ходящий по линии";
    lwalker.stats.max_hp = 285; lwalker.stats.max_ap = 15; lwalker.stats.attack = 106;
    lwalker.stats.dmg_min = 18; lwalker.stats.dmg_max = 30; lwalker.stats.block = 30;
    lwalker.stats.armor = 11;   lwalker.stats.ap_atk = 4;
    lwalker.exp = 520; lwalker.gold_min = 80; lwalker.gold_max = 210;
    lwalker.drops = { Drop("line_thread", 60), Drop("line_dust", 45) };
    lwalker.detect = 10; lwalker.kill_counter = "kill_lwalker";
    lwalker.on_hit_effect = "bleed"; lwalker.on_hit_chance = 45; lwalker.on_hit_power = 4;
    add(lwalker);

    EnemyDef mthing;
    mthing.id = "measure_thing"; mthing.name = "Мера, ставшая вещью"; mthing.female = true;
    mthing.stats.max_hp = 305; mthing.stats.max_ap = 14; mthing.stats.attack = 105;
    mthing.stats.dmg_min = 17; mthing.stats.dmg_max = 29; mthing.stats.block = 32;
    mthing.stats.armor = 12;   mthing.stats.ap_atk = 4;
    mthing.exp = 545; mthing.gold_min = 90; mthing.gold_max = 230;
    mthing.drops = { Drop("line_dust", 50), Drop("cinch_draught", 40) };
    mthing.detect = 9; mthing.kill_counter = "kill_mthing";
    mthing.on_hit_effect = "slow"; mthing.on_hit_chance = 50; mthing.on_hit_power = 3;
    add(mthing);

    EnemyDef fguard;
    fguard.id = "first_guard"; fguard.name = "Страж Первого узла";
    fguard.stats.max_hp = 340; fguard.stats.max_ap = 15; fguard.stats.attack = 108;
    fguard.stats.dmg_min = 19; fguard.stats.dmg_max = 32; fguard.stats.block = 34;
    fguard.stats.armor = 13;   fguard.stats.ap_atk = 4;
    fguard.exp = 590; fguard.gold_min = 110; fguard.gold_max = 260;
    fguard.drops = { Drop("line_dust", 45), Drop("rune_stone", 60),
                     Drop("first_ring", 20) };
    fguard.detect = 10; fguard.kill_counter = "kill_fguard";
    fguard.on_hit_effect = "weaken"; fguard.on_hit_chance = 50; fguard.on_hit_power = 3;
    add(fguard);

    EnemyDef cengine;
    cengine.id = "cinch_engine"; cengine.name = "Стяжной ход";
    cengine.stats.max_hp = 360; cengine.stats.max_ap = 14; cengine.stats.attack = 109;
    cengine.stats.dmg_min = 20; cengine.stats.dmg_max = 34; cengine.stats.block = 30;
    cengine.stats.armor = 15;   cengine.stats.ap_atk = 4;
    cengine.exp = 630; cengine.gold_min = 130; cengine.gold_max = 290;
    cengine.drops = { Drop("heart_cog", 65), Drop("scrap_iron", 60) };
    cengine.detect = 9; cengine.kill_counter = "kill_cengine";
    cengine.on_hit_effect = "slow"; cengine.on_hit_chance = 50; cengine.on_hit_power = 4;
    add(cengine);

    // Сердце тянет всё, до чего дотянется, и потому бьёт чаще, чем должно.
    EnemyDef cheart;
    cheart.id = "cinch_heart"; cheart.name = "Сердце Стяжения"; cheart.female = true;
    cheart.stats.max_hp = 430; cheart.stats.max_ap = 16; cheart.stats.attack = 112;
    cheart.stats.dmg_min = 22; cheart.stats.dmg_max = 38; cheart.stats.block = 34;
    cheart.stats.armor = 15;   cheart.stats.ap_atk = 3;
    cheart.exp = 1000; cheart.gold_min = 400; cheart.gold_max = 700;
    cheart.drops = { Drop("heart_cog", 100), Drop("inside_plate", 70),
                     Drop("portal_stone", 100), Drop("rune_stone", 100) };
    cheart.detect = 11; cheart.kill_counter = "kill_cheart";
    cheart.on_hit_effect = "bleed"; cheart.on_hit_chance = 55; cheart.on_hit_power = 4;
    add(cheart);

    EnemyDef zecho;
    zecho.id = "zero_echo"; zecho.name = "Эхо Нулевой точки"; zecho.female = false;
    zecho.stats.max_hp = 320; zecho.stats.max_ap = 16; zecho.stats.attack = 110;
    zecho.stats.dmg_min = 19; zecho.stats.dmg_max = 33; zecho.stats.block = 33;
    zecho.stats.armor = 12;   zecho.stats.ap_atk = 3;
    zecho.exp = 610; zecho.gold_min = 120; zecho.gold_max = 280;
    zecho.drops = { Drop("line_dust", 55), Drop("cinch_draught", 45) };
    zecho.detect = 11; zecho.kill_counter = "kill_zecho";
    zecho.on_hit_effect = "weaken"; zecho.on_hit_chance = 50; zecho.on_hit_power = 4;
    add(zecho);

    // «Всё сразу» — то, чем Точка Ноль отвечает на любого, кто до неё дошёл.
    EnemyDef allat;
    allat.id = "all_at_once"; allat.name = "Всё сразу"; allat.female = true;
    allat.stats.max_hp = 470; allat.stats.max_ap = 16; allat.stats.attack = 114;
    allat.stats.dmg_min = 24; allat.stats.dmg_max = 40; allat.stats.block = 36;
    allat.stats.armor = 16;   allat.stats.ap_atk = 4;
    allat.exp = 1200; allat.gold_min = 500; allat.gold_max = 850;
    allat.drops = { Drop("zero_shield", 100), Drop("line_blade", 70),
                    Drop("first_ring", 60), Drop("portal_stone", 100) };
    allat.detect = 12; allat.kill_counter = "kill_allat";
    allat.on_hit_effect = "burn"; allat.on_hit_chance = 55; allat.on_hit_power = 4;
    add(allat);

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
    q.id = "books"; q.name = "Слово и бумага";
    q.stages = {
        QuestStageDef(1, "Гурию нужен рецепт чернил и 3 чернильных орешка из леса."),
        QuestStageDef(QUEST_DONE, "Гурий начал возить чистые книги.")
    };
    quests_.push_back(q);

    // --- Регион II: Шов ---

    q = QuestDef();
    q.id = "goatpath"; q.name = "Обоз не вернулся";
    q.stages = {
        QuestStageDef(1, "Гурий потерял обоз на Козьей тропе. Пройти её и узнать, куда он делся."),
        QuestStageDef(2, "Тропа вывела в Шов — край, которого нет на картах. Вернуться к Гурию."),
        QuestStageDef(QUEST_DONE, "Гурий узнал, где кончился его обоз.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "glass"; q.name = "Стекло помнит";
    q.stages = {
        QuestStageDef(1, "Ферапонту нужны 6 осколков со Стеклянного поля."),
        QuestStageDef(QUEST_DONE, "Ферапонт получил осколки и рассказал, что в них видно.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "mill"; q.name = "Куда уходит вода";
    q.stages = {
        QuestStageDef(1, "Мельник дал ключ от нижнего затвора. За ним — соляные штольни."),
        QuestStageDef(2, "Соляная матерь мертва. Вода пойдёт как прежде. Сказать мельнику."),
        QuestStageDef(QUEST_DONE, "Тихая снова вытекает столько же, сколько втекает.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "market"; q.name = "Не той чеканки";
    q.stages = {
        QuestStageDef(1, "Смотрителю Улью нужны 8 монет чужой чеканки — для расчёта, насколько сошлись лоскуты."),
        QuestStageDef(QUEST_DONE, "Улей посчитал. Ответ ему не понравился.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "doubled"; q.name = "Два Прохора";
    q.stages = {
        QuestStageDef(1, "На хуторе Двоеданном два Прохора, и оба настоящие. Оба просят рассудить."),
        QuestStageDef(QUEST_DONE, "Ты рассудил. Правильного ответа не было.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "caravan"; q.name = "Столы накрыты"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "В караван-сарае накрыто на сорок человек. Похлёбка горячая. Двести лет."),
        QuestStageDef(QUEST_DONE, "Опись каравана объясняет, чего они ждали. Лучше бы не объясняла.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "salt"; q.name = "Соль помнит"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Копачи заложили нижнюю штольню и написали: не разбирай кладку."),
        QuestStageDef(QUEST_DONE, "Разобрал. То, что соль держала двести лет, больше не держится.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "bridge"; q.name = "Обрезанный канат"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Срез каната ровный. Его отрезало, а резать было нечем и некому."),
        QuestStageDef(QUEST_DONE, "Донесение о мосте: на той стороне дым. Значит, дворы уцелели и дрейфуют.")
    };
    quests_.push_back(q);

    // --- Регион III: Половины ---

    q = QuestDef();
    q.id = "cityroad"; q.name = "Дорога в Город";
    q.stages = {
        QuestStageDef(1, "Улей рассказал, откуда на Рынок приходит городской товар. "
                         "Дорога на север от рынка ведёт к Половине Города."),
        QuestStageDef(QUEST_DONE, "Половина Города найдена. Она и правда половина.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "foundry"; q.name = "Пока льём — стоим";
    q.stages = {
        QuestStageDef(1, "Литейщику Кузьме нужно 8 кусков ломаного железа: печь нельзя гасить."),
        QuestStageDef(QUEST_DONE, "Печь не погасла. Кузьма отдал молот.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "counting"; q.name = "Две сажени в год";
    q.stages = {
        QuestStageDef(1, "Счетоводу Акиму нужно 6 листов гроссбуха для проверки замеров."),
        QuestStageDef(QUEST_DONE, "Аким пересчитал. Мир сходится вчетверо быстрее, чем писал Орден.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "lists"; q.name = "Половина имени";
    q.stages = {
        QuestStageDef(1, "Писарь Феофан просит попасть в архив. Ключ у Смотрителя Половины, "
                         "и тот его не отдаёт."),
        QuestStageDef(2, "Опись архива прочитана. Каждого жителя — половина."),
        QuestStageDef(QUEST_DONE, "Феофан дочитал опись. Он ждал этого двести лет.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "endless"; q.name = "Улица без конца"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Улица прямая, но приводит в собственное начало. Восемьсот сорок шагов."),
        QuestStageDef(QUEST_DONE, "Выход не в конце, а сбоку. Понять — это перестать идти.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "deadwater"; q.name = "Мёртвая вода"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Жетон — это счёт. Сколько роздал, столько должно вернуться."),
        QuestStageDef(QUEST_DONE, "Пять жетонов собраны. Значит, пятеро всё-таки вернулись.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "halves"; q.name = "Вторая половина"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Клочок списка. Имя обрывается ровно посередине — вторая половина где-то есть."),
        QuestStageDef(QUEST_DONE, "Вторая половина Города не на горизонте, а выше него. Она дрейфует.")
    };
    quests_.push_back(q);

    // --- Регион IV: Орден ---

    q = QuestDef();
    q.id = "orderway"; q.name = "Путь Ордена";
    q.stages = {
        QuestStageDef(1, "Аким показал с башни ворота обители. Печать Ордена открывает их."),
        QuestStageDef(QUEST_DONE, "Ворота открылись. Обитель стоит нетронутой двести лет.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "watch4"; q.name = "Смена караула";
    q.stages = {
        QuestStageDef(1, "Привратник Севир двести лет ждёт смены. Приказ о ней лежит "
                         "в библиотеке — гонец его не донёс."),
        QuestStageDef(QUEST_DONE, "Приказ доставлен. Караул снят.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "charts"; q.name = "Карта сети";
    q.stages = {
        QuestStageDef(1, "Чертёжнику Гордею нужны 4 обрывка чертежа, чтобы собрать карту сети."),
        QuestStageDef(QUEST_DONE, "Карта собрана. Спиц нет: узлы связаны друг с другом, "
                                  "а середина образуется сама.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "keepsake"; q.name = "Оловянный солдатик";
    q.stages = {
        QuestStageDef(1, "В келье одиннадцатой лежал солдатик. Настоятель велел передать "
                         "его, если кто-нибудь когда-нибудь придёт."),
        QuestStageDef(QUEST_DONE, "Отшельник взял солдатика. Его звали Игнат.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "ovens"; q.name = "Печь берёт";
    q.stages = {
        QuestStageDef(1, "Истопник Фома растопит печь за 6 вырванных листов. "
                         "Обратно вещь не достаётся."),
        QuestStageDef(QUEST_DONE, "Печь взяла и отдала. Обмен состоялся.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "refusal"; q.name = "Зал Отказа";
    q.stages = {
        QuestStageDef(1, "Стенограмма Зала: одиннадцать за, один против. Через пять лет "
                         "случилось Стяжение. Осталось сказать, кто был прав."),
        QuestStageDef(QUEST_DONE, "Ты сказал. Зал услышал.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "unsealed"; q.name = "То, что вырвали"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Вырванный лист совпадает с обрывом в уставе: связывать последний "
                         "узел запрещалось, пока нет способа развязать."),
        QuestStageDef(QUEST_DONE, "Аврелий прочёл вырванное. Совет знал, на что шёл.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "node2q"; q.name = "Второй держит"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Узел Второй тянет в другую сторону ровно с той силой, с какой "
                         "тянет сеть. Двести лет. Неизвестно чем."),
        QuestStageDef(QUEST_DONE, "Сердце Узла остановлено. Второй больше не тянет — "
                                  "и это тоже выбор.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "node3q"; q.name = "Третий открыт"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Узел Третий не держит и не тянет. Он просто открыт, и через него "
                         "уходит всё, что ни к чему не пристаёт."),
        QuestStageDef(QUEST_DONE, "Через Третий уходят в Дрейф. Там и надо искать пропавших.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "firstmaster"; q.name = "Он ещё ходит"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Могилу вырыли на случай, если вернётся. Место осталось свободным, "
                         "и кто-то недавно приписал углём: «Я ещё хожу»."),
        QuestStageDef(QUEST_DONE, "Тень Первого Мастера повержена. Кольцо он всё-таки снял.")
    };
    quests_.push_back(q);

    // --- Регион V: Дрейф ---

    q = QuestDef();
    q.id = "driftway"; q.name = "Сорок душ";
    q.stages = {
        QuestStageDef(1, "Гурий так и не узнал, куда делись сорок человек из обоза. "
                         "Третий узел открыт — уходят через него."),
        QuestStageDef(2, "Обоз стоит на дрейфующем лоскуте. Хозяйка ждёт утра, "
                         "которое не наступает. Забрать путевую бирку."),
        QuestStageDef(QUEST_DONE, "Гурий получил бирку. Обоз числится в пути — "
                                  "и это, пожалуй, правда.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "water"; q.name = "Ведро воды";
    q.stages = {
        QuestStageDef(1, "Улите нужна вода из колодца. Ведро одно, и она просит "
                         "принести именно одно."),
        QuestStageDef(QUEST_DONE, "Вёдер вышло два. Улита посмотрела на второе "
                                  "и первый раз за двести лет замолчала.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "wholename"; q.name = "Имя целиком";
    q.stages = {
        QuestStageDef(1, "У Пелагеи вторая половина списков. Половину имени "
                         "надо принести с той стороны среза."),
        QuestStageDef(QUEST_DONE, "Половины сошлись. Одно имя из четырёх тысяч "
                                  "двухсот снова целое.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "lasthour"; q.name = "Последний час";
    q.stages = {
        QuestStageDef(1, "Ратмир держит до темноты, а темноты нет. Знамя у "
                         "знаменосца: пока оно поднято, час начинается заново."),
        QuestStageDef(QUEST_DONE, "Знамя опущено. Час кончился — впервые.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "twobuckets"; q.name = "Колодец Двух Вёдер"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Правило колодца: спускаешь одно ведро — поднимаешь два. "
                         "Где-то воды на ведро меньше."),
        QuestStageDef(QUEST_DONE, "Второе ведро поднялось само и не захотело "
                                  "обратно. Теперь понятно, чем кормится Дрейф.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "nodark"; q.name = "Роща, где не темнеет"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "В роще светло, и свет не меняется. Отдых даром, "
                         "и это настораживает."),
        QuestStageDef(QUEST_DONE, "Сто четыре зарубки одинаковой старости. "
                                  "Здесь не отдыхают — здесь остаются.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "emptyalder"; q.name = "Пустая Ольховка"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Ольховка стоит целая, дом в дом. Людей нет, "
                         "и не похоже, что они уходили."),
        QuestStageDef(QUEST_DONE, "Роспись домов сходится вся, кроме последнего. "
                                  "Бирка пустая, ключ на гвозде, приписано: «ждёт».")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "edgeq"; q.name = "Край Лоскута"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "С края видно семнадцать лоскутов, и все идут в одну "
                         "сторону — к середине."),
        QuestStageDef(QUEST_DONE, "Ветер Края улёгся. Стяжение не кончилось: "
                                  "оно идёт медленно, и мы внутри.")
    };
    quests_.push_back(q);

    // --- Регион VI: Изнанка ---

    q = QuestDef();
    q.id = "inside"; q.name = "Тот, кто шагнул";
    q.stages = {
        QuestStageDef(1, "Тихон рассказал про того, кто шагнул с Края. Он не разбился: "
                         "падать там некуда. Шагнуть следом — единственный способ проверить."),
        QuestStageDef(QUEST_DONE, "Изнанка сети. Внутри она держится на честном слове, "
                                  "и это не оборот речи.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "firstjoint"; q.name = "На честном слове";
    q.stages = {
        QuestStageDef(1, "Елисей держит Первый Шов двести лет, потому что крепления нет. "
                         "В описи крепление названо словом — значит, слово где-то записано."),
        QuestStageDef(QUEST_DONE, "Расписка Первого Мастера легла в стык. Елисей отпустил.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "lines"; q.name = "По линиям";
    q.stages = {
        QuestStageDef(1, "Разметчику Прову нужны 5 нитей линии: он хочет знать, "
                         "рвутся они сами или их подтянули."),
        QuestStageDef(QUEST_DONE, "Не рвутся. Подтянуты, все до одной, ровно и нарочно.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "remembers"; q.name = "Тот, кто помнит дорогу";
    q.stages = {
        QuestStageDef(1, "У Первого узла стоит человек, который ушёл из обители "
                         "двести лет назад и с тех пор идёт."),
        QuestStageDef(QUEST_DONE, "Он рассказал дорогу до конца и пошёл следом. "
                                  "Выбирать будет не он.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "finale"; q.name = "Развязка";
    q.stages = {
        QuestStageDef(1, "Три исхода записаны на листе, придавленном кольцом. "
                         "Правильного нет. Есть только твой."),
        QuestStageDef(QUEST_DONE, "Ты выбрал. Обратно не переигрывается.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "measure"; q.name = "Мера"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "В Комнате Измерений расстояние — вещь. Её можно взять в руку "
                         "и потерять, и был человек, который потерялся сам."),
        QuestStageDef(QUEST_DONE, "Мера у тебя. Теперь до всего ровно столько, "
                                  "сколько ты решишь.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "heart"; q.name = "Сердце Стяжения"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Ход не крутится и не тикает: он просто не умеет иначе, "
                         "чем тянуть. Никто не проверял, что будет, если перестанет."),
        QuestStageDef(QUEST_DONE, "Проверил. Сердце остановлено, и мир не рассыпался — "
                                  "он просто перестал стягиваться сам собой.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "seam"; q.name = "Шов за алтарём"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Заметка говорит о шве за северной гранью алтаря. "
                         "Открывает его ключ самого стража."),
        QuestStageDef(2, "Ключ шва у тебя. Осталось найти сам шов — он в святилище."),
        QuestStageDef(QUEST_DONE, "Схрон Ордена открыт.")
    };
    quests_.push_back(q);

    q = QuestDef();
    q.id = "cinch"; q.name = "Стяжение не кончилось"; q.secret = true;
    q.stages = {
        QuestStageDef(1, "Отчёт Ордена: сеть не порвалась, а затянулась — и тянет до сих пор. "
                         "Отшельник должен знать больше."),
        QuestStageDef(QUEST_DONE, "Отшельник рассказал, чем всё это кончится. "
                                  "И чем может кончиться иначе.")
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
    s.id = "shop_books"; s.name = "Книжный угол Гурия";
    s.goods = {"book_blank", "oak_gall"};
    s.buy_pct = 100; s.sell_pct = 45;
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_glass"; s.name = "Стекло Ферапонта";
    s.goods = {"glass_blade", "glass_dust", "glass_shard", "antidote"};
    s.buy_pct = 105; s.sell_pct = 45;
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_market"; s.name = "Рынок Шва";
    s.goods = {"trader_hood", "caravan_stew", "salt_mail", "brine_ring",
               "herb_potion", "antidote", "elixir_haste", "portal_stone",
               "spice_bag", "whetstone"};
    s.buy_pct = 95; s.sell_pct = 55;      // на Шве и берут, и дают честнее
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_foundry"; s.name = "Литейный двор";
    s.goods = {"foundry_hammer", "slag_shield", "clerk_robe", "scrap_iron", "ember"};
    s.buy_pct = 100; s.sell_pct = 60;      // железо здесь берут охотно
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_ferry"; s.name = "Лодка Хмурого";
    s.goods = {"ferry_ring", "strong_tea", "ferry_token", "herb_potion", "antidote"};
    s.buy_pct = 110; s.sell_pct = 40;
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_order"; s.name = "Кладовая Аврелия";
    s.goods = {"order_draught", "acolyte_hood", "node_shield", "gate_halberd",
               "rune_stone", "portal_stone", "antidote", "strong_tea"};
    s.buy_pct = 100; s.sell_pct = 50;
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_drift"; s.name = "Мешок Тихона";
    s.goods = {"still_water", "grove_leaf", "drift_grass", "drift_cloak",
               "stair_hood", "portal_stone", "order_draught", "antidote"};
    s.buy_pct = 115; s.sell_pct = 55;    // проводник знает, что деваться некуда
    shops_[s.id] = s;

    s = ShopDef();
    s.id = "shop_inside"; s.name = "Короб Елисея";
    s.goods = {"cinch_draught", "line_dust", "rune_stone", "seam_helm",
               "zero_shield", "portal_stone", "still_water", "antidote"};
    s.buy_pct = 120; s.sell_pct = 60;    // сюда мало кто доходит, и он это знает
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

    // --- Регион II: Шов ---
    add("glazier",  "Стекольщик Ферапонт", "glazier_root", "shop_glass");
    add("miller",   "Мельник Онисим",      "miller_root",  "");
    add("warden",   "Смотритель Улей",     "warden_root",  "shop_market");
    add("digger",   "Копач Тишка",         "digger_root",  "");
    add("prohor_l", "Прохор",              "prohor_l_root", "");
    add("prohor_r", "Прохор",              "prohor_r_root", "");

    // --- Регион III: Половины ---
    add("survivor", "Горожанка Верея",  "survivor_root", "");
    add("founder",  "Литейщик Кузьма",  "founder_root",  "shop_foundry");
    add("counter",  "Счетовод Аким",    "counter_root",  "");
    add("scribe",   "Писарь Феофан",    "scribe_root",   "");
    add("ferryman", "Перевозчик Хмурый","ferry_root",    "shop_ferry");

    // --- Регион IV: Орден ---
    add("gatekeeper", "Привратник Севир", "gatekeeper_root", "");
    add("librarian",  "Книжник Аврелий",  "librarian_root",  "shop_order");
    add("draftsman",  "Чертёжник Гордей", "draftsman_root",  "");
    add("stoker",     "Истопник Фома",    "stoker_root",     "");
    add("recorder",   "Протоколист Никон","recorder_root",   "");

    // --- Регион V: Дрейф ---
    add("driftwife",  "Хозяйка Улита",    "driftwife_root",  "");
    add("halfscribe", "Секретарь Пелагея","halfscribe_root", "");
    add("soldier",    "Ратмир, десятник", "soldier_root",    "");
    add("grovekeeper","Ерофей, сидящий",  "grovekeeper_root","");
    add("pathkeeper", "Проводник Тихон",  "pathkeeper_root", "shop_drift");

    // --- Регион VI: Изнанка ---
    // Первый Мастер стоит дважды: у Первого узла, где рассказывает дорогу,
    // и в Развязке, где молчит и ждёт ответа. Это один человек, но два
    // разговора, и второй не должен быть доступен из первого.
    add("seamwatch",  "Сторож Елисей",    "seamwatch_root",  "shop_inside");
    add("surveyor",   "Разметчик Пров",   "surveyor_root",   "");
    add("master",     "Первый Мастер",    "master_root",     "");
    add("master_end", "Первый Мастер",    "finale_root",     "");
}

// ---------------------------------------------------------------- развязки

void Content::build_endings() {
    auto add = [&](const std::string& id, const std::string& name,
                   const std::vector<std::string>& lines) {
        EndingDef e; e.id = id; e.name = name; e.lines = lines;
        endings_[id] = e;
    };

    add("pull", "Дотянуть", {
        "Ты доводишь замысел Ордена до конца.",
        "",
        "Расстояние отменяется не сразу. Сначала пропадает дорога между",
        "Ольховкой и лесом — просто оказывается, что идти там некуда,",
        "потому что уже пришёл. Потом то же самое делается с Рынком Шва,",
        "с Половиной Города, с обеими половинами разом.",
        "",
        "Лада поднимает голову от трав и видит Пелагею, которая правит",
        "списки. Пелагея видит Улиту у котла. Улита видит Гурия, и Гурий",
        "наконец пересчитывает свои сорок душ по головам, все сорок,",
        "и садится прямо на землю.",
        "",
        "Никто больше не пропадёт. Некуда пропадать.",
        "",
        "Ратмир говорит: «Вот и подмога». Ерофей выходит из рощи первым",
        "шагом за сто четыре зарубки. Севир садится и не встаёт.",
        "",
        "А потом становится тихо, и тишина эта не кончается, потому что",
        "кончаться ей больше некуда. Ты хочешь отойти в сторону и понимаешь,",
        "что стороны нет. Есть двор, и на нём весь мир, и все на нём рядом,",
        "и это навсегда.",
        "",
        "Игнат подходит и говорит: «Спасибо. Он вышел».",
        "И правда: рядом стоит послушник из кельи одиннадцатой, живой,",
        "и держит оловянного солдатика.",
        "",
        "Ты сделал так, что все нашлись. Ценой того, что никто больше",
        "никуда не пойдёт."
    });

    add("cut", "Разрезать", {
        "Ты режешь сеть.",
        "",
        "Расстояние возвращается сразу и целиком, как вода в прорванную",
        "запруду. Мир делается огромным и честным.",
        "",
        "До Ольховки отсюда полгода пути, и Лада не узнает, что ты выбрал:",
        "письмо будет идти дольше, чем она проживёт. Гурий довезёт товар",
        "до Рынка Шва за четыре месяца вместо двух дней и скажет, что так",
        "и надо, что раньше было неправильно.",
        "",
        "Лоскуты расходятся по своим настоящим местам. Дом на отшибе",
        "опускается на землю где-то далеко на севере, и Улита выходит утром",
        "во двор — впервые утром, — и видит незнакомые горы.",
        "",
        "Половина Города соединяется со своей половиной. За тысячу вёрст",
        "отсюда, на пустом месте, встаёт целый город, и Мучная улица идёт",
        "насквозь, и Пелагея кричит через неё имя, и ей отвечают.",
        "",
        "А один лоскут опускается в океан, потому что настоящее место у него",
        "там. Ты видел его с Края и не знал, чей он.",
        "",
        "Ратмир доходит до своих через шесть недель. Опоздал, но дошёл.",
        "",
        "Ты вернул миру размер. Это оказалось дорого, и заплатили не все",
        "поровну."
    });

    add("hold", "Удержать", {
        "Ты не делаешь ничего. Это самое трудное из трёх.",
        "",
        "Сердце Стяжения тянет, как тянуло, и ты держишь, как держал Второй",
        "узел двести лет — ровно с той силой, с какой тянет сеть. Устанешь —",
        "стянется. Значит, не уставать.",
        "",
        "Мир остаётся такой, какой есть: криво сшитый, с половинами имён,",
        "с обозом, числящимся в пути, с рощей, где не темнеет. Все, кого",
        "ты встретил, остаются там, где ты их оставил. Ничего не",
        "исправлено, и ничего больше не сломано.",
        "",
        "Через год ты замечаешь, что перестал считать дни, и вспоминаешь",
        "Ерофея и его сто четыре зарубки. Через пять — что не хочется есть.",
        "Через сорок к твоему костру выходит Игнат, садится, ничего",
        "не спрашивает и уходит утром.",
        "",
        "Потом приходит кто-то новый. Молодой, с чужой печатью на пальце,",
        "и печать ему передал не человек.",
        "",
        "Ты подвигаешься и говоришь:",
        "",
        "— Садись. Огня хватит на двоих."
    });
}

// ---------------------------------------------------------------- события

void Content::build_triggers() {
    auto add = [&](TriggerKind k, const std::string& key, int count,
                   const std::string& quest, int stage, int min_stage,
                   const std::string& message) {
        QuestTrigger t;
        t.kind = k; t.key = key; t.count = count;
        t.quest = quest; t.stage = stage; t.min_stage = min_stage; t.message = message;
        triggers_.push_back(t);
    };

    // Тайна открывается находкой, а не разговором.
    add(TriggerKind::NoteTaken, "seam", 1, "seam", 1, 0,
        "Заметка на полях говорит о шве за алтарём. Открыта тайна: «Шов за алтарём».");

    // Ключ добывается со стража — сам факт добычи двигает квест.
    // Ключ двигает тайну только тому, кто уже прочёл заметку: иначе цепочку
    // можно пройти с конца, добыв ключ и не разгадав ничего.
    add(TriggerKind::ItemGained, "seam_key", 1, "seam", 2, 1,
        "Ключ шва у тебя. Теперь понятно, о каком замке писали на полях.");

    // Вход в схрон закрывает квест: дошёл — значит, разгадал.
    add(TriggerKind::LocationEntered, "vault", 1, "seam", QUEST_DONE, 1,
        "Схрон Ордена открыт. Тайна «Шов за алтарём» разгадана.");

    // Отчёт в схроне открывает следующую тайну.
    add(TriggerKind::NoteTaken, "cinch", 1, "cinch", 1, 0,
        "Отчёт о Стяжении меняет всё. Открыта тайна: «Стяжение не кончилось».");

    // --- Регион II ---
    // Тропа выводит в Шов — квест Гурия двигается самим приходом на Рынок.
    add(TriggerKind::LocationEntered, "market", 1, "goatpath", 2, 1,
        "Так вот куда выводит тропа. Гурию будет что рассказать.");

    // Накрытые столы сами по себе — уже загадка.
    add(TriggerKind::LocationEntered, "caravanserai", 1, "caravan", 1, 0,
        "Столы накрыты на сорок человек, и похлёбка ещё горячая. Открыта тайна: «Столы накрыты».");
    add(TriggerKind::NoteTaken, "caravan", 1, "caravan", QUEST_DONE, 1,
        "Опись каравана дочитана. Тайна «Столы накрыты» разгадана.");

    // Наказ копача открывает тайну, а Соляная матерь её закрывает.
    add(TriggerKind::NoteTaken, "salt", 1, "salt", 1, 0,
        "«Не разбирай кладку». Открыта тайна: «Соль помнит».");
    add(TriggerKind::MobKilled, "kill_saltmother", 1, "salt", QUEST_DONE, 1,
        "То, что соль держала двести лет, больше не держится. Тайна «Соль помнит» разгадана.");

    // Обрывок каната — вещь, с которой начинается вопрос.
    add(TriggerKind::ItemGained, "rope_end", 1, "bridge", 1, 0,
        "Срез каната ровный, будто ножом. Открыта тайна: «Обрезанный канат».");
    add(TriggerKind::NoteTaken, "bridge", 1, "bridge", QUEST_DONE, 1,
        "Донесение о мосте объясняет и срез, и дым на той стороне.");

    // --- Регион III ---
    // Половина Города находится приходом, а не рассказом.
    add(TriggerKind::LocationEntered, "halfcity", 1, "cityroad", QUEST_DONE, 1,
        "Мостовая обрывается ровно, как ножом. Улей не преувеличивал.");

    // Улица объясняет себя сама — тем, что не кончается.
    add(TriggerKind::LocationEntered, "endless", 1, "endless", 1, 0,
        "Улица прямая, но ты уже проходил этот угол. Открыта тайна: «Улица без конца».");
    add(TriggerKind::NoteTaken, "endless", 1, "endless", QUEST_DONE, 1,
        "Заметка обходчика: выход не в конце, а сбоку. Тайна «Улица без конца» разгадана.");

    // Счёт перевозчика: жетоны должны вернуться.
    add(TriggerKind::NoteTaken, "deadwater", 1, "deadwater", 1, 0,
        "«Сколько роздал, столько должно вернуться». Открыта тайна: «Мёртвая вода».");
    add(TriggerKind::ItemGained, "ferry_token", 5, "deadwater", QUEST_DONE, 1,
        "Пять жетонов. Счёт сошёлся — значит, пятеро вернулись. Тайна «Мёртвая вода» разгадана.");

    // Опись архива читается на месте — Феофану остаётся только выслушать.
    add(TriggerKind::NoteTaken, "lists", 1, "lists", 2, 1,
        "Опись прочитана. Считали души, а надо было считать имена.");

    // --- Регион IV ---
    add(TriggerKind::LocationEntered, "ordergate", 1, "orderway", QUEST_DONE, 1,
        "Ворота открылись печатью. Обитель стоит нетронутой.");

    // Солдатик находится сам и сам же становится квестом.
    add(TriggerKind::ItemGained, "keepsake", 1, "keepsake", 1, 0,
        "Оловянный солдатик. Краска стёрлась там, где его держали пальцами. "
        "Новый след: «Оловянный солдатик».");

    add(TriggerKind::NoteTaken, "unsealed", 1, "unsealed", 1, 0,
        "Вырванный лист совпадает с обрывом в уставе. Открыта тайна: «То, что вырвали».");
    add(TriggerKind::LocationEntered, "node2", 1, "node2q", 1, 0,
        "Здесь что-то тянет в другую сторону. Открыта тайна: «Второй держит».");
    add(TriggerKind::MobKilled, "kill_nheart", 1, "node2q", QUEST_DONE, 1,
        "Сердце Узла остановлено. Второй больше не тянет.");
    add(TriggerKind::LocationEntered, "node3", 1, "node3q", 1, 0,
        "Узел не держит и не тянет — он открыт. Открыта тайна: «Третий открыт».");
    add(TriggerKind::NoteTaken, "thirdnode", 1, "node3q", QUEST_DONE, 1,
        "Отметка на Третьем: через него уходят в Дрейф.");
    add(TriggerKind::NoteTaken, "emptygrave", 1, "firstmaster", 1, 0,
        "«Не дописывайте. Я ещё хожу». Открыта тайна: «Он ещё ходит».");
    add(TriggerKind::MobKilled, "kill_mshadow", 1, "firstmaster", QUEST_DONE, 1,
        "Тень Первого Мастера повержена. Кольцо он всё-таки снял.");

    // --- Регион V ---
    // Приход на лоскут двигает поиск обоза: дальше ищут уже глазами.
    add(TriggerKind::LocationEntered, "meadow", 1, "driftway", 2, 1,
        "Луг дрейфует. Где-то здесь кончился обоз Гурия.");

    add(TriggerKind::NoteTaken, "wellrule", 1, "twobuckets", 1, 0,
        "Правило колодца записано чужой рукой. Открыта тайна: «Колодец Двух Вёдер».");
    add(TriggerKind::MobKilled, "kill_bucket", 1, "twobuckets", QUEST_DONE, 1,
        "Второе ведро больше не поднимается. Где-то воды снова столько, сколько было.");

    add(TriggerKind::LocationEntered, "grove", 1, "nodark", 1, 0,
        "Свет в роще не меняется. Открыта тайна: «Роща, где не темнеет».");
    add(TriggerKind::NoteTaken, "marks", 1, "nodark", QUEST_DONE, 1,
        "Сто четыре зарубки, и все одинаково старые.");

    add(TriggerKind::LocationEntered, "emptyalder", 1, "emptyalder", 1, 0,
        "Ольховка стоит целая, и в ней никого. Открыта тайна: «Пустая Ольховка».");
    add(TriggerKind::ItemGained, "own_key", 1, "emptyalder", QUEST_DONE, 1,
        "Ключ с пустой биркой. От дома, в котором никто не жил.");

    add(TriggerKind::NoteTaken, "edgeview", 1, "edgeq", 1, 0,
        "Семнадцать лоскутов идут в одну сторону. Открыта тайна: «Край Лоскута».");
    add(TriggerKind::MobKilled, "kill_edgewind", 1, "edgeq", QUEST_DONE, 1,
        "Ветер Края улёгся. Стало видно, куда всё это сносит.");

    // --- Регион VI ---
    // Шаг с Края — сам по себе ответ на вопрос Тихона.
    add(TriggerKind::LocationEntered, "firstseam", 1, "inside", QUEST_DONE, 1,
        "Ты не разбился: падать здесь некуда. Это изнанка сети.");

    add(TriggerKind::LocationEntered, "measures", 1, "measure", 1, 0,
        "Расстояние тут лежит на полке. Открыта тайна: «Мера».");
    add(TriggerKind::ItemGained, "measure", 1, "measure", QUEST_DONE, 1,
        "Мера у тебя в руке, и рука не стала тяжелее.");

    add(TriggerKind::NoteTaken, "cinchwork", 1, "heart", 1, 0,
        "Устройство хода записано в трёх абзацах. Открыта тайна: «Сердце Стяжения».");
    add(TriggerKind::MobKilled, "kill_cheart", 1, "heart", QUEST_DONE, 1,
        "Сердце остановлено. Мир не рассыпался — он просто перестал стягиваться.");

    // Лист под кольцом — то, ради чего сюда шли.
    add(TriggerKind::NoteTaken, "threeways", 1, "finale", 1, 0,
        "Три исхода, писано рукой Первого Мастера. Правильного нет.");

    // Половина имени — вещь, с которой начинается вопрос о второй половине.
    add(TriggerKind::ItemGained, "half_name", 1, "halves", 1, 0,
        "Имя обрывается ровно посередине. Открыта тайна: «Вторая половина».");
    add(TriggerKind::NoteTaken, "halves", 1, "halves", QUEST_DONE, 1,
        "Донесение с Башни: вторая половина выше горизонта и дрейфует.");
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

        DlgOption b_offer;
        b_offer.text = "Бумагой не торгуешь?";
        b_offer.next = "books_offer";
        b_offer.req_quest  = "books";  b_offer.req_stage_min  = QUEST_NONE; b_offer.req_stage_max  = QUEST_NONE;
        b_offer.req_quest2 = "amulet"; b_offer.req_stage2_min = QUEST_DONE; b_offer.req_stage2_max = QUEST_DONE;
        n.options.push_back(b_offer);

        DlgOption b_done;
        b_done.text = "Рецепт и орешки у меня.";
        b_done.next = "books_reward";
        b_done.req_quest = "books"; b_done.req_stage_min = 1; b_done.req_stage_max = 1;
        b_done.req_item = "oak_gall"; b_done.req_item_count = 3;
        b_done.req_note = "ink";
        n.options.push_back(b_done);

        DlgOption b_wait;
        b_wait.text = "Ещё собираю.";
        b_wait.next = "books_wait";
        b_wait.req_quest = "books"; b_wait.req_stage_min = 1; b_wait.req_stage_max = 1;
        n.options.push_back(b_wait);

        DlgOption gp_offer;
        gp_offer.text = "Обозы все дошли?";
        gp_offer.next = "goat_offer";
        gp_offer.req_quest  = "goatpath"; gp_offer.req_stage_min  = QUEST_NONE; gp_offer.req_stage_max = QUEST_NONE;
        gp_offer.req_quest2 = "outpost";  gp_offer.req_stage2_min = QUEST_DONE; gp_offer.req_stage2_max = QUEST_DONE;
        n.options.push_back(gp_offer);

        DlgOption gp_done;
        gp_done.text = "Твоя тропа выводит в край, которого нет на картах.";
        gp_done.next = "goat_reward";
        gp_done.req_quest = "goatpath"; gp_done.req_stage_min = 2; gp_done.req_stage_max = 2;
        n.options.push_back(gp_done);

        DlgOption gp_wait;
        gp_wait.text = "Ещё не дошёл до перевала.";
        gp_wait.next = "goat_wait";
        gp_wait.req_quest = "goatpath"; gp_wait.req_stage_min = 1; gp_wait.req_stage_max = 1;
        n.options.push_back(gp_wait);

        DlgOption b_shop;
        b_shop.text = "Показывай книги.";
        b_shop.open_shop = true;
        b_shop.shop_id = "shop_books";
        b_shop.req_quest = "books"; b_shop.req_stage_min = QUEST_DONE; b_shop.req_stage_max = QUEST_DONE;
        n.options.push_back(b_shop);

        // Дрейф: сорок душ из обоза так и не нашлись, а Третий узел открыт.
        DlgOption dw_offer;
        dw_offer.text = "А люди из обоза? Сорок человек.";
        dw_offer.next = "driftway_offer";
        dw_offer.req_quest  = "driftway"; dw_offer.req_stage_min  = QUEST_NONE; dw_offer.req_stage_max  = QUEST_NONE;
        dw_offer.req_quest2 = "caravan";  dw_offer.req_stage2_min = QUEST_DONE; dw_offer.req_stage2_max = QUEST_DONE;
        n.options.push_back(dw_offer);

        DlgOption dw_done;
        dw_done.text = "Вот путевая бирка. Обоз числится в пути.";
        dw_done.next = "driftway_reward";
        dw_done.req_quest = "driftway"; dw_done.req_stage_min = 2; dw_done.req_stage_max = 2;
        dw_done.req_item = "caravan_tally"; dw_done.req_item_count = 1;
        n.options.push_back(dw_done);

        DlgOption dw_wait;
        dw_wait.text = "Ещё ищу обоз.";
        dw_wait.next = "driftway_wait";
        dw_wait.req_quest = "driftway"; dw_wait.req_stage_min = 1; dw_wait.req_stage_max = 1;
        n.options.push_back(dw_wait);

        DlgOption dw_after;
        dw_after.text = "Так что теперь с обозом?";
        dw_after.next = "driftway_after";
        dw_after.req_quest = "driftway"; dw_after.req_stage_min = QUEST_DONE; dw_after.req_stage_max = QUEST_DONE;
        n.options.push_back(dw_after);

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

    {
        DlgNode n; n.id = "books_offer";
        n.text = "Гурий оживляется.\n"
                 "— Бумагой? Да я бы рад. Переплёты привезти нетрудно, а вот\n"
                 "чернила надо варить на месте, и рецепт я посеял где-то в лесу.\n"
                 "Найди листок и добудь три чернильных орешка — и будут тебе книги.";
        DlgOption take;
        take.text = "Поищу.";
        take.next = "books_wait";
        take.set_quest = "books"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Мне и без книг неплохо."));
        add(n);
    }
    {
        DlgNode n; n.id = "books_wait";
        n.text = "— Листок с рецептом, три орешка. Орешки на дубовых листьях,\n"
                 "в лесу их полно, если под ноги смотреть.\n"
                 "Найденное читается в библиотеке — у тебя же есть куда складывать?";
        n.options.push_back(bye("Разберусь."));
        add(n);
    }
    {
        DlgNode n; n.id = "books_reward";
        n.text = "Гурий читает рецепт, шевеля губами, и хлопает по прилавку.\n"
                 "— Он самый! Ну всё, с завтрашнего дня вожу переплёты.\n"
                 "Вот тебе первая, за находку. Пиши что хочешь — бумага стерпит.";
        DlgOption take;
        take.text = "Принять книгу. [120 опыта]";
        take.set_quest = "books"; take.set_stage = QUEST_DONE;
        take.take_item = "oak_gall"; take.take_count = 3;
        take.give_item = "book_blank"; take.give_count = 1;
        take.give_exp = 120;
        n.options.push_back(take);
        add(n);
    }

    {
        DlgNode n; n.id = "goat_offer";
        n.text = "Улыбка сходит.\n"
                 "— Нет. Один не дошёл. Четыре подводы, соль и лён, Козьей тропой\n"
                 "на север — и как в воду. Тропа-то в двух шагах от леса, я по ней\n"
                 "мальчишкой бегал. А теперь смотрю на неё — и не по себе.\n"
                 "Сходи, а? Мне надо знать, где кончились мои люди.";
        DlgOption take;
        take.text = "Схожу.";
        take.next = "goat_wait";
        take.set_quest = "goatpath"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Своих ищи сам."));
        add(n);
    }
    {
        DlgNode n; n.id = "goat_wait";
        n.text = "— Тропа от северной опушки. Возьми чего покрепче: там, говорят,\n"
                 "ходят приблудные — не наши, не разбойники. Просто чужие.";
        n.options.push_back(bye("Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "goat_reward";
        n.text = "Гурий слушает и садится прямо на прилавок.\n"
                 "— Стеклянное поле. Рынок какой-то на шве. Значит, они дошли —\n"
                 "только не туда, куда шли.\n"
                 "Держи. И вот что: если там торгуют — я туда повезу. Дорога\n"
                 "дорогой, а торг торгом.";
        DlgOption take;
        take.text = "Принять плату. [250 золотых, 260 опыта]";
        take.set_quest = "goatpath"; take.set_stage = QUEST_DONE;
        take.give_gold = 250; take.give_exp = 260;
        take.give_item = "portal_stone"; take.give_count = 1;
        n.options.push_back(take);
        add(n);
    }

    // --- Стекольщик Ферапонт: осколки поля ---
    {
        DlgNode n; n.id = "glazier_root";
        n.text = "Человек в толстых рукавицах сидит на корточках и перебирает\n"
                 "осколки, поднося каждый к глазу.\n"
                 "— Осторожнее ступай. Поле режет тех, кто спешит.";

        DlgOption offer;
        offer.text = "Что ты в них высматриваешь?";
        offer.next = "glass_offer";
        offer.req_quest = "glass"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Шесть осколков, как просил.";
        done.next = "glass_reward";
        done.req_quest = "glass"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "glass_shard"; done.req_item_count = 6;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ещё собираю.";
        wait.next = "glass_wait";
        wait.req_quest = "glass"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption trade;
        trade.text = "Показывай товар.";
        trade.open_shop = true;
        n.options.push_back(trade);

        n.options.push_back(bye("Пойду дальше."));
        add(n);
    }
    {
        DlgNode n; n.id = "glass_offer";
        n.text = "— Прошлое. Стекло с этого поля помнит, чем было до того, как\n"
                 "потекло. Смотришь в осколок — видишь рожь и межу.\n"
                 "Мне нужно шесть чистых, крупных. Псы их не любят и грызут,\n"
                 "так что придётся отбирать.";
        DlgOption take;
        take.text = "Отберу.";
        take.next = "glass_wait";
        take.set_quest = "glass"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Мне и настоящего хватает."));
        add(n);
    }
    {
        DlgNode n; n.id = "glass_wait";
        n.text = "— Шесть. И не смотри в них подолгу, пока не принесёшь.\n"
                 "Я смотрел. Больше не смотрю.";
        n.options.push_back(bye("Учту."));
        add(n);
    }
    {
        DlgNode n; n.id = "glass_reward";
        n.text = "Ферапонт складывает осколки в ряд и долго молчит.\n"
                 "— Всё та же рожь. Каждый год всё та же рожь.\n"
                 "Держи резак. Из седьмого осколка, который я никому не показывал.";
        DlgOption take;
        take.text = "Принять резак. [280 опыта]";
        take.set_quest = "glass"; take.set_stage = QUEST_DONE;
        take.take_item = "glass_shard"; take.take_count = 6;
        take.give_item = "glass_blade"; take.give_count = 1;
        take.give_gold = 120; take.give_exp = 280;
        n.options.push_back(take);
        add(n);
    }

    // --- Мельник Онисим: куда уходит вода ---
    {
        DlgNode n; n.id = "miller_root";
        n.text = "Мельница стоит, колесо не крутится. Мельник сидит на пороге\n"
                 "и смотрит на реку.\n"
                 "— Втекает больше, чем вытекает. Третий год.";

        DlgOption offer;
        offer.text = "Куда девается разница?";
        offer.next = "mill_offer";
        offer.req_quest = "mill"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "За затвором больше некому пить твою воду.";
        done.next = "mill_reward";
        done.req_quest = "mill"; done.req_stage_min = 1; done.req_stage_max = 2;
        done.req_counter = "kill_saltmother"; done.req_counter_min = 1;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ещё не дошёл до затвора.";
        wait.next = "mill_wait";
        wait.req_quest = "mill"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        n.options.push_back(bye("Пойду."));
        add(n);
    }
    {
        DlgNode n; n.id = "mill_offer";
        n.text = "— Считал три года. Внизу по руслу старый затвор, за ним слышно\n"
                 "воду. Много воды. И солёную — а до моря отсюда четыреста вёрст.\n"
                 "Ключ у меня. Сорок лет держал и ни разу не открывал.\n"
                 "Возьми. Мне уже всё равно, а мельница стоит.";
        DlgOption take;
        take.text = "Возьму ключ.";
        take.next = "mill_wait";
        take.set_quest = "mill"; take.set_stage = 1;
        take.give_item = "mill_key"; take.give_count = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Держи при себе."));
        add(n);
    }
    {
        DlgNode n; n.id = "mill_wait";
        n.text = "— Затвор ниже по руслу. Открывать — на твою голову.\n"
                 "И вот что: там солоно. Соль держит. Что она удержала за двести\n"
                 "лет, то и выйдет тебе навстречу.";
        n.options.push_back(bye("Пойду смотреть."));
        add(n);
    }
    {
        DlgNode n; n.id = "mill_reward";
        n.text = "Мельник встаёт впервые за разговор.\n"
                 "— Слышишь? Колесо. Третий год не слышал.\n"
                 "Бери, что есть. Мне теперь мука пойдёт.";
        DlgOption take;
        take.text = "Принять награду. [320 опыта]";
        take.set_quest = "mill"; take.set_stage = QUEST_DONE;
        take.give_gold = 260; take.give_exp = 320;
        take.give_item = "salt_mail"; take.give_count = 1;
        n.options.push_back(take);
        add(n);
    }

    // --- Смотритель Улей: счёт по монетам ---
    {
        DlgNode n; n.id = "warden_root";
        n.text = "Смотритель в куколе стоит посреди рядов и ничего не продаёт.\n"
                 "— На Шве не спрашивают года у товара и лоскута у человека.\n"
                 "Спросишь — платишь.";

        DlgOption offer;
        offer.text = "А ты чем занят, если не торгуешь?";
        offer.next = "market_offer";
        offer.req_quest = "market"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Восемь монет чужой чеканки.";
        done.next = "market_reward";
        done.req_quest = "market"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "old_coin"; done.req_item_count = 8;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Собираю.";
        wait.next = "market_wait";
        wait.req_quest = "market"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption trade;
        trade.text = "Показывай ряды.";
        trade.open_shop = true;
        n.options.push_back(trade);

        DlgOption cr_offer;
        cr_offer.text = "Откуда у тебя городской товар?";
        cr_offer.next = "cityroad_offer";
        cr_offer.req_quest  = "cityroad"; cr_offer.req_stage_min  = QUEST_NONE; cr_offer.req_stage_max = QUEST_NONE;
        cr_offer.req_quest2 = "market";   cr_offer.req_stage2_min = QUEST_DONE; cr_offer.req_stage2_max = QUEST_DONE;
        n.options.push_back(cr_offer);

        DlgOption rules;
        rules.text = "Почему рынок стоит именно здесь?";
        rules.next = "market_rules";
        n.options.push_back(rules);

        n.options.push_back(bye("Хорошего торга."));
        add(n);
    }
    {
        DlgNode n; n.id = "market_rules";
        n.text = "— Потому что шов не принадлежит никому. По одну сторону ряда\n"
                 "человек из года, которого ты не застал; по другую — из года,\n"
                 "которого не застанет он. Общего у нас ровно одно: цена.\n"
                 "Пока торгуем — не режем друг друга. Вот и всё уложение.";
        n.options.push_back(bye("Разумно."));
        add(n);
    }
    {
        DlgNode n; n.id = "market_offer";
        n.text = "— Считаю. Каждая монета — это год и место, где её били.\n"
                 "Чем больше чужих монет ходит по Шву, тем ближе сошлись лоскуты.\n"
                 "Мне нужно восемь. Приблудные их носят, солевики держат при себе,\n"
                 "и в караван-сарае их полно — только туда никто не ходит.";
        DlgOption take;
        take.text = "Соберу восемь.";
        take.next = "market_wait";
        take.set_quest = "market"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Считай сам."));
        add(n);
    }
    {
        DlgNode n; n.id = "market_wait";
        n.text = "— Восемь, и разных. Одинаковые ничего не скажут.";
        n.options.push_back(bye("Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "market_reward";
        n.text = "Улей раскладывает монеты по годам и считает вслух. Потом ещё раз.\n"
                 "Потом снимает куколь — под ним лицо старика, которому страшно.\n"
                 "— Двенадцать лет назад разброс был вдвое шире. Значит, сходится\n"
                 "быстрее, чем мы думали. Значит, я это увижу.\n"
                 "Бери, что хочешь, и не говори никому. Здесь торгуют, а не паникуют.";
        DlgOption take;
        take.text = "Принять расчёт. [300 опыта]";
        take.set_quest = "market"; take.set_stage = QUEST_DONE;
        take.take_item = "old_coin"; take.take_count = 8;
        take.give_item = "trader_hood"; take.give_count = 1;
        take.give_gold = 300; take.give_exp = 300;
        n.options.push_back(take);
        add(n);
    }

    {
        DlgNode n; n.id = "cityroad_offer";
        n.text = "— Из Города. Северной дорогой, два дня.\n"
                 "Только это не город, а половина города. Срез прошёл по улице,\n"
                 "и дома с одной стороны есть, а с другой нет. Не развалины —\n"
                 "просто нет, и мостовая обрывается ровно.\n"
                 "Они там до сих пор ждут ответа из ратуши, которая осталась\n"
                 "на той половине. Сходи, если крепкий. Оттуда всё видно яснее.";
        DlgOption take;
        take.text = "Схожу посмотреть.";
        take.next = "";
        take.set_quest = "cityroad"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Мне и Шва хватает."));
        add(n);
    }

    // --- Горожанка Верея: что тут вообще произошло ---
    {
        DlgNode n; n.id = "survivor_root";
        n.text = "Женщина метёт крыльцо дома, у которого нет соседнего дома.\n"
                 "Метёт до самого среза и там останавливается.\n"
                 "— Не подходи к краю. Смотреть можно, стоять нельзя.";
        DlgOption ask;
        ask.text = "Что случилось с той стороной?";
        ask.next = "survivor_cut";
        n.options.push_back(ask);
        DlgOption ask2;
        ask2.text = "Кто у вас тут остался?";
        ask2.next = "survivor_who";
        n.options.push_back(ask2);
        n.options.push_back(bye("Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "survivor_cut";
        n.text = "— Ничего не случилось. В том и дело.\n"
                 "Не рухнуло, не сгорело. Просто перестало быть — ровно по\n"
                 "Мучной улице. Чётная сторона стоит, нечётной нет.\n"
                 "\n"
                 "У меня там свекровь жила. Через дорогу.";
        n.options.push_back(bye("Сочувствую."));
        add(n);
    }
    {
        DlgNode n; n.id = "survivor_who";
        n.text = "— Кузьма в литейной, он печь не гасит. Аким на башне, считает.\n"
                 "Феофан при архиве, только архив заперт, а ключ у смотрителя.\n"
                 "Смотритель на посту. Двести лет.\n"
                 "\n"
                 "И Кольцевая. На Кольцевую не ходи, если не понял её.";
        n.options.push_back(bye("Учту."));
        add(n);
    }

    // --- Литейщик Кузьма ---
    {
        DlgNode n; n.id = "founder_root";
        n.text = "Жар от печи слышно за квартал. Кузьма стоит у летки, весь в саже.\n"
                 "— Печь не гашу. Спрашивать не надо, скажу сам, если поможешь.";

        DlgOption offer;
        offer.text = "Чем помочь?";
        offer.next = "foundry_offer";
        offer.req_quest = "foundry"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Восемь кусков железа.";
        done.next = "foundry_reward";
        done.req_quest = "foundry"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "scrap_iron"; done.req_item_count = 8;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ещё собираю.";
        wait.next = "foundry_wait";
        wait.req_quest = "foundry"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption trade;
        trade.text = "Показывай, что отлил.";
        trade.open_shop = true;
        n.options.push_back(trade);

        n.options.push_back(bye("Работай."));
        add(n);
    }
    {
        DlgNode n; n.id = "foundry_offer";
        n.text = "— Металл кончается. Двести лет льём из своих же отливок,\n"
                 "а всякий раз чуть меньше выходит: угар.\n"
                 "Принеси восемь кусков ломаного, хоть откуда. Печь не должна\n"
                 "остыть.\n"
                 "\n"
                 "Мастер говорил: пока льём — стоим. Перестанем — поймём, что\n"
                 "кончилось, и тогда всё. Он в нижнем цеху остался. Не ходи туда.";
        DlgOption take;
        take.text = "Принесу железо.";
        take.next = "foundry_wait";
        take.set_quest = "foundry"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Гаси и живи как все."));
        add(n);
    }
    {
        DlgNode n; n.id = "foundry_wait";
        n.text = "— Восемь кусков. Шлаковики его таскают, крысы растаскивают,\n"
                 "по всему Городу валяется.";
        n.options.push_back(bye("Иду."));
        add(n);
    }
    {
        DlgNode n; n.id = "foundry_reward";
        n.text = "Кузьма кидает железо в летку не глядя и наконец садится.\n"
                 "— Ещё год простоим.\n"
                 "Держи молот. Он мастеров, из нижнего цеха. Мне его носить\n"
                 "нельзя, а тебе — можно: ты не отсюда.";
        DlgOption take;
        take.text = "Принять молот. [420 опыта]";
        take.set_quest = "foundry"; take.set_stage = QUEST_DONE;
        take.take_item = "scrap_iron"; take.take_count = 8;
        take.give_item = "slag_shield"; take.give_count = 1;
        take.give_gold = 340; take.give_exp = 420;
        n.options.push_back(take);
        add(n);
    }

    // --- Счетовод Аким ---
    {
        DlgNode n; n.id = "counter_root";
        n.text = "На верхней площадке ветер и человек с мерной цепью.\n"
                 "— Тише. Сбиваюсь.";

        DlgOption offer;
        offer.text = "Что ты меряешь?";
        offer.next = "counting_offer";
        offer.req_quest = "counting"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Шесть листов гроссбуха.";
        done.next = "counting_reward";
        done.req_quest = "counting"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "ledger_page"; done.req_item_count = 6;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ещё ищу листы.";
        wait.next = "counting_wait";
        wait.req_quest = "counting"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption ow_offer;
        ow_offer.text = "Что это за стены на севере?";
        ow_offer.next = "orderway_offer";
        ow_offer.req_quest  = "orderway"; ow_offer.req_stage_min  = QUEST_NONE; ow_offer.req_stage_max = QUEST_NONE;
        ow_offer.req_quest2 = "counting"; ow_offer.req_stage2_min = QUEST_DONE; ow_offer.req_stage2_max = QUEST_DONE;
        n.options.push_back(ow_offer);

        DlgOption view;
        view.text = "Что видно с башни?";
        view.next = "counter_view";
        n.options.push_back(view);

        n.options.push_back(bye("Не буду мешать."));
        add(n);
    }
    {
        DlgNode n; n.id = "counter_view";
        n.text = "— В ясный день — вторая половина. Не на горизонте: выше него.\n"
                 "Она не стоит на земле, её сносит. За двенадцать лет прошла\n"
                 "треть неба.\n"
                 "\n"
                 "Там дым из труб. Значит, живут. Значит, тоже смотрят сюда.";
        n.options.push_back(bye("Жутко."));
        add(n);
    }
    {
        DlgNode n; n.id = "counting_offer";
        n.text = "— Расстояние. Беру две точки, между которыми до Стяжения было\n"
                 "триста саженей, и меряю.\n"
                 "\n"
                 "Орденские писали: локоть в поколение. У меня выходит две сажени\n"
                 "в год. Вчетверо быстрее. Я хочу ошибиться, но проверять не на чем:\n"
                 "мои гроссбухи растащили счетоводы, которые тут ходят и считают\n"
                 "вслух. Принеси шесть листов — сверю почерк и цифры.";
        DlgOption take;
        take.text = "Найду листы.";
        take.next = "counting_wait";
        take.set_quest = "counting"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Считай сам."));
        add(n);
    }
    {
        DlgNode n; n.id = "counting_wait";
        n.text = "— Шесть. И не читай их подолгу: в каждом третьем столбце ошибка\n"
                 "на одну и ту же величину, и от этого делается нехорошо.";
        n.options.push_back(bye("Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "counting_reward";
        n.text = "Аким сверяет листы, водя пальцем, и вдруг откладывает цепь.\n"
                 "— Сходится. Две сажени.\n"
                 "Значит, я не ошибся, и значит, я это увижу.\n"
                 "\n"
                 "Возьми линзу. Мне она больше не нужна: я уже посмотрел.";
        DlgOption take;
        take.text = "Принять линзу. [450 опыта]";
        take.set_quest = "counting"; take.set_stage = QUEST_DONE;
        take.take_item = "ledger_page"; take.take_count = 6;
        take.give_item = "counter_lens"; take.give_count = 1;
        take.give_gold = 300; take.give_exp = 450;
        n.options.push_back(take);
        add(n);
    }

    {
        DlgNode n; n.id = "orderway_offer";
        n.text = "Аким наводит цепь на север и долго молчит.\n"
                 "— Обитель. Та самая. Стоит нетронутая, ворота заперты.\n"
                 "\n"
                 "Отпирает печать: узел из серебра. Их не выдавали — их передавали.\n"
                 "Выходящий отдавал входящему. Если печать у тебя, а отдал её\n"
                 "не человек, а мертвец, — значит, тот не вышел.\n"
                 "\n"
                 "Иди. Оттуда всё видно ещё яснее, и легче тебе от этого не станет.";
        DlgOption take;
        take.text = "Пойду к воротам.";
        take.next = "";
        take.set_quest = "orderway"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Мне и здесь ясно."));
        add(n);
    }

    // --- Привратник Севир: смена, которая не пришла ---
    {
        DlgNode n; n.id = "gatekeeper_root";
        n.text = "Страж стоит у внутренних ворот. Доспех цел, глаза живые.\n"
                 "— Пост три. Смены не было. Проходи, если по делу.";

        DlgOption offer;
        offer.text = "Сколько ты уже стоишь?";
        offer.next = "watch4_offer";
        offer.req_quest = "watch4"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Вот приказ. Пост снять.";
        done.next = "watch4_reward";
        done.req_quest = "watch4"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "order_writ"; done.req_item_count = 1;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ищу приказ.";
        wait.next = "watch4_wait";
        wait.req_quest = "watch4"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption after;
        after.text = "Как оно — не стоять?";
        after.next = "watch4_after";
        after.req_quest = "watch4"; after.req_stage_min = QUEST_DONE; after.req_stage_max = QUEST_DONE;
        n.options.push_back(after);

        n.options.push_back(bye("Стой дальше."));
        add(n);
    }
    {
        DlgNode n; n.id = "watch4_offer";
        n.text = "— Не считал. Считать — это ждать, а ждать нельзя: собьёшься.\n"
                 "\n"
                 "Приказ о смене должен был прийти. Гонец не дошёл — дороги не стало\n"
                 "ни туда, ни обратно. Без приказа я пост не оставлю: не потому,\n"
                 "что верю, будто кто-то придёт. Потому что иначе выйдет, что я\n"
                 "стоял зря.\n"
                 "\n"
                 "В библиотеке есть ящик для недоставленного. Посмотри там.";
        DlgOption take;
        take.text = "Поищу приказ.";
        take.next = "watch4_wait";
        take.set_quest = "watch4"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Стой, раз надо."));
        add(n);
    }
    {
        DlgNode n; n.id = "watch4_wait";
        n.text = "— Ящик для недоставленного. Аврелий знает, где он.\n"
                 "Только не говори ему, зачем. Он начнёт объяснять, а мне слушать\n"
                 "нельзя: собьюсь.";
        n.options.push_back(bye("Молчу."));
        add(n);
    }
    {
        DlgNode n; n.id = "watch4_reward";
        n.text = "Севир читает, шевеля губами, потом читает ещё раз.\n"
                 "Потом ставит алебарду к стене — впервые за двести лет,\n"
                 "и по тому, как он это делает, видно, что руки помнят движение.\n"
                 "\n"
                 "— Значит, не зря. Значит, приказ был.\n"
                 "Держи алебарду. Мне она теперь не по чину: я не караул.";
        DlgOption take;
        take.text = "Принять алебарду. [700 опыта]";
        take.set_quest = "watch4"; take.set_stage = QUEST_DONE;
        take.take_item = "order_writ"; take.take_count = 1;
        take.give_item = "gate_halberd"; take.give_count = 1;
        take.give_gold = 400; take.give_exp = 700;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "watch4_after";
        n.text = "— Непривычно. Сажусь — и встаю, будто кто окликнул.\n"
                 "Пройдёт, наверное. За двести лет привык, за двести и отвыкну.";
        n.options.push_back(bye("Бывай, Севир."));
        add(n);
    }

    // --- Книжник Аврелий ---
    {
        DlgNode n; n.id = "librarian_root";
        n.text = "Между стеллажами горит одна лампа. Книжник сидит под ней\n"
                 "и переписывает что-то в тетрадь.\n"
                 "— Тише, пожалуйста. Не потому что тайна — потому что привычка.";

        DlgOption box;
        box.text = "Где у вас ящик для недоставленного?";
        box.next = "librarian_box";
        box.req_quest = "watch4"; box.req_stage_min = 1; box.req_stage_max = 1;
        n.options.push_back(box);

        DlgOption torn;
        torn.text = "Я нашёл лист, вырванный из устава.";
        torn.next = "unsealed_talk";
        torn.req_quest = "unsealed"; torn.req_stage_min = 1; torn.req_stage_max = 1;
        n.options.push_back(torn);

        DlgOption what;
        what.text = "Что вы вообще читали?";
        what.next = "librarian_read";
        n.options.push_back(what);

        DlgOption trade;
        trade.text = "Что осталось в кладовой?";
        trade.open_shop = true;
        n.options.push_back(trade);

        n.options.push_back(bye("Не буду мешать."));
        add(n);
    }
    {
        DlgNode n; n.id = "librarian_box";
        n.text = "— Второй стеллаж, нижний ящик. Там всё, что не дошло.\n"
                 "Писем восемьдесят, приказов четыре, одно завещание.\n"
                 "\n"
                 "Я их разобрал по годам. Больше с ними делать нечего:\n"
                 "адресаты либо здесь, либо нигде.";
        DlgOption take;
        take.text = "Взять приказ о смене караула.";
        take.give_item = "order_writ"; take.give_count = 1;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "librarian_read";
        n.text = "— О расстоянии. Всё остальное — приложения.\n"
                 "\n"
                 "Три школы было. Одна считала расстояние свойством, вторая —\n"
                 "веществом, третья — привычкой. Все три ошиблись.\n"
                 "\n"
                 "Расстояние — это согласие. Мир согласен быть большим, пока\n"
                 "его об этом не переспрашивают.\n"
                 "\n"
                 "Мы переспросили.";
        n.options.push_back(bye("И он передумал."));
        add(n);
    }
    {
        DlgNode n; n.id = "unsealed_talk";
        n.text = "Аврелий берёт лист, прикладывает к уставу — совпадает по обрыву.\n"
                 "Долго молчит.\n"
                 "\n"
                 "— Значит, было записано. «Связывать последний узел запрещается,\n"
                 "доколе не будет найден способ развязать».\n"
                 "\n"
                 "Совет внёс это в устав по настоянию Мастера. И Совет же вырвал,\n"
                 "когда Мастер ушёл. Не сжёг — вырвал и оставил лежать.\n"
                 "\n"
                 "Знаешь, что это значит? Что они не забыли. Они помнили и сделали.";
        DlgOption take;
        take.text = "Отдать лист в архив. [650 опыта]";
        take.set_quest = "unsealed"; take.set_stage = QUEST_DONE;
        take.take_item = "torn_page"; take.take_count = 1;
        take.give_item = "acolyte_hood"; take.give_count = 1;
        take.give_exp = 650;
        n.options.push_back(take);
        add(n);
    }

    // --- Чертёжник Гордей ---
    {
        DlgNode n; n.id = "draftsman_root";
        n.text = "Стол во всю комнату, на нём лист во весь стол, и лист порван.\n"
                 "Человек стоит над ним и не поднимает головы.\n"
                 "— Не наступи на обрывки. Я их два века собираю.";

        DlgOption offer;
        offer.text = "Что это за чертёж?";
        offer.next = "charts_offer";
        offer.req_quest = "charts"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Четыре обрывка.";
        done.next = "charts_reward";
        done.req_quest = "charts"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "chart_piece"; done.req_item_count = 4;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ещё ищу.";
        wait.next = "charts_wait";
        wait.req_quest = "charts"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        n.options.push_back(bye("Собирай."));
        add(n);
    }
    {
        DlgNode n; n.id = "charts_offer";
        n.text = "— Карта сети. Все узлы и все связи, как было до Стяжения.\n"
                 "\n"
                 "Не хватает четырёх кусков. Их растащили тени — те, что здесь\n"
                 "ходят. Они не злые, они просто тоже чертят.\n"
                 "\n"
                 "Принеси четыре. Я хочу увидеть целиком хотя бы раз.";
        DlgOption take;
        take.text = "Принесу.";
        take.next = "charts_wait";
        take.set_quest = "charts"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Черти по памяти."));
        add(n);
    }
    {
        DlgNode n; n.id = "charts_wait";
        n.text = "— Четыре. Тени носят их с собой и не отдают по-хорошему.";
        n.options.push_back(bye("Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "charts_reward";
        n.text = "Гордей раскладывает обрывки, и лист впервые за двести лет сходится.\n"
                 "Он смотрит долго, потом садится прямо на пол.\n"
                 "\n"
                 "— Спиц нет.\n"
                 "Мы всю жизнь думали, что узлы связаны с серединой. А они связаны\n"
                 "друг с другом, и середина получается сама, когда связей довольно.\n"
                 "\n"
                 "Значит, Точку Ноль никто не строил. Значит, она просто вышла.\n"
                 "Значит, и развязать её никто не умеет — потому что никто не вязал.";
        DlgOption take;
        take.text = "Забрать копию карты. [750 опыта]";
        take.set_quest = "charts"; take.set_stage = QUEST_DONE;
        take.take_item = "chart_piece"; take.take_count = 4;
        take.give_item = "node_shield"; take.give_count = 1;
        take.give_gold = 500; take.give_exp = 750;
        n.options.push_back(take);
        add(n);
    }

    // --- Истопник Фома: печь берёт ---
    {
        DlgNode n; n.id = "stoker_root";
        n.text = "Печь тёплая, хотя её не топят. Истопник сидит рядом на чурбаке.\n"
                 "— Не подходи близко. Она берёт то, что ближе.";

        DlgOption offer;
        offer.text = "Что значит «берёт»?";
        offer.next = "ovens_offer";
        offer.req_quest = "ovens"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Шесть вырванных листов. Топи.";
        done.next = "ovens_reward";
        done.req_quest = "ovens"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "torn_page"; done.req_item_count = 6;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ещё собираю.";
        wait.next = "ovens_wait";
        wait.req_quest = "ovens"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        n.options.push_back(bye("Грейся."));
        add(n);
    }
    {
        DlgNode n; n.id = "ovens_offer";
        n.text = "— Печь не плавит. Печь берёт вещь и отдаёт то, чего в вещи не было.\n"
                 "Обратно вещь не достаётся. Совсем.\n"
                 "\n"
                 "Клали книги, кольца. Один раз положили человека — после этого\n"
                 "она полгода молчала, и настоятель запретил.\n"
                 "\n"
                 "Хочешь обмен — неси шесть вырванных листов. Зола у меня своя.\n"
                 "Листов тут довольно: их вырывали охотно.";
        DlgOption take;
        take.text = "Соберу.";
        take.next = "ovens_wait";
        take.set_quest = "ovens"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Обойдусь."));
        add(n);
    }
    {
        DlgNode n; n.id = "ovens_wait";
        n.text = "— Шесть листов. И подумай ещё раз: обратно не достанешь.";
        n.options.push_back(bye("Подумаю."));
        add(n);
    }
    {
        DlgNode n; n.id = "ovens_reward";
        n.text = "Фома кладёт листы в устье, сыплет золу сверху и закрывает заслонку.\n"
                 "Ничего не гудит и не вспыхивает. Просто через минуту внутри\n"
                 "лежит доспех, которого туда не клали.\n"
                 "\n"
                 "— Вот и весь фокус. Она отдала то, чего в них не было.\n"
                 "А листов больше нет. Совсем нет — понимаешь?";
        DlgOption take;
        take.text = "Забрать доспех. [800 опыта]";
        take.set_quest = "ovens"; take.set_stage = QUEST_DONE;
        take.take_item = "torn_page"; take.take_count = 6;
        take.give_item = "order_plate"; take.give_count = 1;
        take.give_exp = 800;
        n.options.push_back(take);
        add(n);
    }

    // --- Протоколист Никон: Зал Отказа ---
    {
        DlgNode n; n.id = "recorder_root";
        n.text = "Зал пуст, стол длинный, на нём протокол и перо. За столом сидит\n"
                 "один человек и смотрит на пустую строку.\n"
                 "— Заседание не закрыто. Не хватает последней записи.";

        DlgOption offer;
        offer.text = "Какой записи?";
        offer.next = "refusal_offer";
        offer.req_quest = "refusal"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption say_master;
        say_master.text = "Запиши: Мастер был прав.";
        say_master.next = "refusal_master";
        say_master.req_quest = "refusal"; say_master.req_stage_min = 1; say_master.req_stage_max = 1;
        say_master.set_quest = "refusal"; say_master.set_stage = QUEST_DONE;
        say_master.set_counter = "refusal_choice"; say_master.set_counter_value = 1;
        say_master.give_item = "master_ring"; say_master.give_count = 1;
        say_master.give_exp = 700;
        n.options.push_back(say_master);

        DlgOption say_council;
        say_council.text = "Запиши: Совет был прав.";
        say_council.next = "refusal_council";
        say_council.req_quest = "refusal"; say_council.req_stage_min = 1; say_council.req_stage_max = 1;
        say_council.set_quest = "refusal"; say_council.set_stage = QUEST_DONE;
        say_council.set_counter = "refusal_choice"; say_council.set_counter_value = 2;
        say_council.give_item = "order_draught"; say_council.give_count = 3;
        say_council.give_gold = 800;
        say_council.give_exp = 700;
        n.options.push_back(say_council);

        DlgOption after_m;
        after_m.text = "Протокол закрыт?";
        after_m.next = "refusal_after_m";
        after_m.req_quest = "refusal"; after_m.req_stage_min = QUEST_DONE; after_m.req_stage_max = QUEST_DONE;
        after_m.req_counter = "refusal_choice"; after_m.req_counter_min = 1; after_m.req_counter_max = 1;
        n.options.push_back(after_m);

        DlgOption after_c;
        after_c.text = "Протокол закрыт?";
        after_c.next = "refusal_after_c";
        after_c.req_quest = "refusal"; after_c.req_stage_min = QUEST_DONE; after_c.req_stage_max = QUEST_DONE;
        after_c.req_counter = "refusal_choice"; after_c.req_counter_min = 2; after_c.req_counter_max = 2;
        n.options.push_back(after_c);

        n.options.push_back(bye("Мне нечего сказать."));
        add(n);
    }
    {
        DlgNode n; n.id = "refusal_offer";
        n.text = "— Одиннадцать за, один против. Мастер снял кольцо, положил на стол\n"
                 "и вышел. В протокол внесли, что он был не в себе.\n"
                 "\n"
                 "Через пять лет случилось Стяжение, и заседание так и не закрыли:\n"
                 "закрыть — значит записать, кто был прав, а записывать было некому.\n"
                 "\n"
                 "Ты не из Ордена. Тебе можно. Скажи — и я допишу.";
        DlgOption take;
        take.text = "Дай подумать.";
        take.next = "";
        take.set_quest = "refusal"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Это не мне решать."));
        add(n);
    }
    {
        DlgNode n; n.id = "refusal_master";
        n.text = "Никон записывает медленно, выводя каждую букву.\n"
                 "«Особое мнение Первого Мастера признано верным. Заседание закрыто.»\n"
                 "\n"
                 "— Двести лет. Спасибо.\n"
                 "Кольцо возьми. Оно тут с того дня лежит, и я не смел его тронуть,\n"
                 "пока протокол был открыт.";
        n.options.push_back(bye("Прощай, Никон."));
        add(n);
    }
    {
        DlgNode n; n.id = "refusal_council";
        n.text = "Никон записывает быстро, не глядя.\n"
                 "«Решение Совета признано верным. Заседание закрыто.»\n"
                 "\n"
                 "— Так тоже можно. Он ведь и правда был не в себе — кто в себе\n"
                 "уйдёт из обители в никуда?\n"
                 "\n"
                 "Возьми из кладовой, что причитается. Совет платил хорошо.";
        n.options.push_back(bye("Прощай, Никон."));
        add(n);
    }
    {
        DlgNode n; n.id = "refusal_after_m";
        n.text = "— Закрыт. Впервые за двести лет мне нечего делать.\n"
                 "Странное чувство. Хорошее, кажется.";
        n.options.push_back(bye("Отдыхай."));
        add(n);
    }
    {
        DlgNode n; n.id = "refusal_after_c";
        n.text = "— Закрыт.\n"
                 "Только знаешь... я всё равно смотрю на пустое место, где лежало\n"
                 "кольцо. Его никто не забирал двести лет.\n"
                 "Теперь и не заберёт.";
        n.options.push_back(bye("Бывает."));
        add(n);
    }

    // ================= Регион V: Дрейф =================

    {
        DlgNode n; n.id = "driftway_offer";
        n.text = "Гурий перестаёт улыбаться, и без улыбки лицо у него старое.\n"
                 "— Сорок. Я их всех по именам знаю, я же нанимал.\n"
                 "\n"
                 "Столы в сарае накрыты до сих пор, а людей нет. Значит, встали\n"
                 "из-за стола и куда-то пошли, и это «куда-то» на картах не значится.\n"
                 "\n"
                 "Ты был у орденских. Говорят, у них узел сломан и через него\n"
                 "уходит всё, что ни к чему не пристало.\n"
                 "\n"
                 "Пойди туда. Не за ними — за ответом. Я хочу знать, живы ли,\n"
                 "и если нет, то с какого дня считать.";
        DlgOption take;
        take.text = "Схожу в Дрейф.";
        take.next = "driftway_wait";
        take.set_quest = "driftway"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Мёртвых не ищут."));
        add(n);
    }
    {
        DlgNode n; n.id = "driftway_wait";
        n.text = "— Через сломанный узел. Другого пути мне не назвали.\n"
                 "И вот что: если найдёшь их живыми — не радуйся сразу.\n"
                 "Сорок человек, которые двести лет не постарели, — это не\n"
                 "«живы». Это что-то другое, и я не знаю, как оно называется.";
        n.options.push_back(bye("Разберусь на месте."));
        add(n);
    }
    {
        DlgNode n; n.id = "driftway_reward";
        n.text = "Гурий берёт бирку, поворачивает к свету, читает свою же запись\n"
                 "двухсотлетней давности и кладёт на прилавок.\n"
                 "\n"
                 "— «В пути». Ну да. Формально всё верно.\n"
                 "\n"
                 "Он долго молчит.\n"
                 "\n"
                 "— Знаешь, что хуже всего? Что Улита ждёт утра. Она хорошая была,\n"
                 "стряпуха каких мало, и она ждёт утра, а утро не придёт.\n"
                 "Я бы лучше похоронил. Хоронить я умею.\n"
                 "\n"
                 "Бирку оставь себе. Мне на неё смотреть нечем.";
        DlgOption take;
        take.text = "Взять плату. [900 опыта]";
        take.set_quest = "driftway"; take.set_stage = QUEST_DONE;
        take.give_gold = 700; take.give_exp = 900;
        take.give_item = "portal_stone"; take.give_count = 2;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "driftway_after";
        n.text = "— Числится в пути. Так и буду писать в книге, пока книга есть.\n"
                 "\n"
                 "А вот подводы я из описи вычеркнул. Подводы-то точно не вернутся.";
        n.options.push_back(bye("Бывай, Гурий."));
        add(n);
    }

    // --- Хозяйка Улита: завтра, которое не наступит ---
    {
        DlgNode n; n.id = "driftwife_root";
        n.text = "Женщина мешает похлёбку в котле над холодными углями.\n"
                 "— Садись, коли с дороги. Утром выходим, так что ешь сейчас.";

        DlgOption tally;
        tally.text = "Покажи путевую бирку обоза.";
        tally.next = "driftwife_tally";
        tally.req_quest = "driftway"; tally.req_stage_min = 2; tally.req_stage_max = 2;
        n.options.push_back(tally);

        DlgOption offer;
        offer.text = "Помочь чем-нибудь?";
        offer.next = "water_offer";
        offer.req_quest = "water"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Вода. Только вёдер вышло два.";
        done.next = "water_reward";
        done.req_quest = "water"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "two_bucket"; done.req_item_count = 1;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Иду за водой.";
        wait.next = "water_wait";
        wait.req_quest = "water"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption after;
        after.text = "Как ты?";
        after.next = "water_after";
        after.req_quest = "water"; after.req_stage_min = QUEST_DONE; after.req_stage_max = QUEST_DONE;
        n.options.push_back(after);

        n.options.push_back(bye("Спасибо, я сыт."));
        add(n);
    }
    {
        DlgNode n; n.id = "driftwife_tally";
        n.text = "— Бирка? Вон на гвозде висит, где ей и висеть.\n"
                 "Двенадцать подвод, сорок душ. Всё в пути, всё как надо.\n"
                 "\n"
                 "Она снимает бирку и подаёт, не глядя.\n"
                 "\n"
                 "— Завтра допишу, что дошли. Я каждый вечер собираюсь дописать.";
        DlgOption take;
        take.text = "Взять бирку.";
        take.give_item = "caravan_tally"; take.give_count = 1;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "water_offer";
        n.text = "— Помочь? Помоги, отчего нет. Воды принеси из колодца.\n"
                 "Одно ведро, больше не надо: нам к утру только похлёбку долить.\n"
                 "\n"
                 "Колодец за рощей. Ходить недалеко, только я туда не хожу.\n"
                 "Не почему-то, а просто не хожу. Некогда всё.";
        DlgOption take;
        take.text = "Принесу ведро.";
        take.next = "water_wait";
        take.set_quest = "water"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Сама сходишь."));
        add(n);
    }
    {
        DlgNode n; n.id = "water_wait";
        n.text = "— Одно ведро. Слышишь — одно.\n"
                 "Больше в котёл не влезет, а лишнюю выливать жалко.";
        n.options.push_back(bye("Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "water_reward";
        n.text = "Улита берёт первое ведро и ставит у котла. Тянется за вторым —\n"
                 "и не берёт.\n"
                 "\n"
                 "Долго стоит с протянутой рукой.\n"
                 "\n"
                 "— Два.\n"
                 "\n"
                 "Потом садится на лавку, вытирает руки о передник и говорит уже\n"
                 "тише, без хозяйского напева:\n"
                 "\n"
                 "— Я эту похлёбку сколько раз ставила? Сегодня, вчера, позавчера.\n"
                 "И каждый раз на утро. И каждый раз вечер.\n"
                 "\n"
                 "Она смотрит на второе ведро.\n"
                 "\n"
                 "— Не говори мне ничего. Я сама, по-своему. Только не сегодня.";
        DlgOption take;
        take.text = "Оставить оба ведра. [850 опыта]";
        take.set_quest = "water"; take.set_stage = QUEST_DONE;
        take.take_item = "two_bucket"; take.take_count = 1;
        take.give_item = "drift_cloak"; take.give_count = 1;
        take.give_gold = 300; take.give_exp = 850;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "water_after";
        n.text = "— Похлёбку я больше не ставлю. Стоит одна, старая, и пусть стоит.\n"
                 "\n"
                 "Странно: думала, станет хуже. А стало просто тихо.";
        n.options.push_back(bye("Держись, Улита."));
        add(n);
    }

    // --- Секретарь Пелагея: вторая половина списков ---
    {
        DlgNode n; n.id = "halfscribe_root";
        n.text = "Стол вынесен прямо на срез улицы, к самому обрыву. За ним\n"
                 "сидит женщина и правит списки.\n"
                 "— Не заслоняй свет. Мне отсюда видно их сторону.";

        DlgOption offer;
        offer.text = "Что это за списки?";
        offer.next = "wholename_offer";
        offer.req_quest = "wholename"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "У меня половина имени с той стороны.";
        done.next = "wholename_reward";
        done.req_quest = "wholename"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "half_name"; done.req_item_count = 1;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Половину ещё не принёс.";
        wait.next = "wholename_wait";
        wait.req_quest = "wholename"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption shout;
        shout.text = "Вы что, перекрикиваетесь через обрыв?";
        shout.next = "halfscribe_shout";
        n.options.push_back(shout);

        n.options.push_back(bye("Не буду мешать."));
        add(n);
    }
    {
        DlgNode n; n.id = "wholename_offer";
        n.text = "— Списки жителей. Наша половина.\n"
                 "\n"
                 "Город разрезало не по улицам, а по людям. Каждое имя пополам:\n"
                 "начало осталось у них, конец у нас. Четыре тысячи двести имён,\n"
                 "и ни одного целого.\n"
                 "\n"
                 "Я правлю их двести лет и ни разу не смогла закончить строку.\n"
                 "\n"
                 "Принеси хоть одну половину с той стороны. Одну. Мне хватит,\n"
                 "чтобы знать, что это вообще возможно.";
        DlgOption take;
        take.text = "Принесу.";
        take.next = "wholename_wait";
        take.set_quest = "wholename"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Это невозможно."));
        add(n);
    }
    {
        DlgNode n; n.id = "wholename_wait";
        n.text = "— Клочок списка. У них он валяется где попало: у них ведь\n"
                 "начала имён, а начало без конца читается легко, и они не поняли,\n"
                 "что чего-то не хватает.\n"
                 "\n"
                 "Мы поняли сразу. Конец без начала не читается вовсе.";
        n.options.push_back(bye("Найду."));
        add(n);
    }
    {
        DlgNode n; n.id = "wholename_reward";
        n.text = "Пелагея прикладывает половину к половине. Обрыв к обрыву.\n"
                 "Сходится.\n"
                 "\n"
                 "Она читает имя вслух — целиком, по слогам, как читают дети.\n"
                 "Потом ещё раз. Потом сшивает половинки ниткой, прямо по бумаге,\n"
                 "грубым швом, и вешает на шею.\n"
                 "\n"
                 "— Одно из четырёх тысяч двухсот. За двести лет.\n"
                 "\n"
                 "— Возьми себе. Я перепишу, у меня рука привычная, а тебе носить.\n"
                 "Пусть у кого-нибудь будет хоть одно целое имя.";
        DlgOption take;
        take.text = "Принять имя. [950 опыта]";
        take.set_quest = "wholename"; take.set_stage = QUEST_DONE;
        take.take_item = "half_name"; take.take_count = 1;
        take.give_item = "whole_name"; take.give_count = 1;
        take.give_gold = 400; take.give_exp = 950;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "halfscribe_shout";
        n.text = "— Перекрикивались. Первые лет тридцать.\n"
                 "\n"
                 "Слышно отлично: четыре шага, а между ними всё небо. Мы им кричали\n"
                 "имена, они нам. Записывали. Ошибались.\n"
                 "\n"
                 "Потом кто-то с их стороны крикнул: «а зачем?» — и никто не нашёлся\n"
                 "ответить. С тех пор молчим и машем по праздникам.";
        n.options.push_back(bye("Понятно."));
        add(n);
    }

    // --- Ратмир: час, который не кончается ---
    {
        DlgNode n; n.id = "soldier_root";
        n.text = "Десятник сидит на щите, воткнув меч в землю, и смотрит на запад.\n"
                 "— Держим до темноты. Ты вовремя: скоро начнётся.";

        DlgOption offer;
        offer.text = "Что начнётся?";
        offer.next = "lasthour_offer";
        offer.req_quest = "lasthour"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Знамя у меня. Знаменосец больше не поднимет.";
        done.next = "lasthour_reward";
        done.req_quest = "lasthour"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "torn_banner"; done.req_item_count = 1;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ещё не добрался до знаменосца.";
        wait.next = "lasthour_wait";
        wait.req_quest = "lasthour"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption after;
        after.text = "Ну что, стемнело?";
        after.next = "lasthour_after";
        after.req_quest = "lasthour"; after.req_stage_min = QUEST_DONE; after.req_stage_max = QUEST_DONE;
        n.options.push_back(after);

        n.options.push_back(bye("Держитесь."));
        add(n);
    }
    {
        DlgNode n; n.id = "lasthour_offer";
        n.text = "— Последний час. Он у нас один, и он идёт по кругу.\n"
                 "\n"
                 "Приказ был: держать до темноты, подмога к вечеру. Мы держали.\n"
                 "Темноты не случилось. Вечера тоже.\n"
                 "\n"
                 "Пока знамя поднято — час начинается заново: мы встаём, строимся,\n"
                 "нас сминают, и мы опять встаём. Я это помню весь. Каждый раз весь.\n"
                 "\n"
                 "Знаменосец не отдаст. Он хороший был мужик и потому не отдаст:\n"
                 "уронить знамя — это ведь позор, а он про позор помнит, а про\n"
                 "остальное уже нет.\n"
                 "\n"
                 "Сними с него знамя. Как хочешь.";
        DlgOption take;
        take.text = "Схожу к знаменосцу.";
        take.next = "lasthour_wait";
        take.set_quest = "lasthour"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Это не мой бой."));
        add(n);
    }
    {
        DlgNode n; n.id = "lasthour_wait";
        n.text = "— Он на правом крыле, где всегда стоял.\n"
                 "\n"
                 "И вот что: он не враг тебе. Он вообще уже никому не враг.\n"
                 "Просто держит древко.";
        n.options.push_back(bye("Найду."));
        add(n);
    }
    {
        DlgNode n; n.id = "lasthour_reward";
        n.text = "Ратмир берёт обрывок, разглаживает на колене и смотрит на запад\n"
                 "ещё раз — уже по-другому, как смотрят не на срок, а просто так.\n"
                 "\n"
                 "— Темнеет.\n"
                 "\n"
                 "Он говорит это спокойно, будто отмечает погоду. Потом встаёт,\n"
                 "выдёргивает меч из земли и подаёт его тебе рукоятью вперёд.\n"
                 "\n"
                 "— Держи. Мне до вечера всё равно не понадобится, а вечер вот он.\n"
                 "\n"
                 "— Ребятам скажу, что подмога пришла. Ты ведь и есть подмога,\n"
                 "просто опоздала. Ничего. Все опаздывают.";
        DlgOption take;
        take.text = "Принять клинок. [1000 опыта]";
        take.set_quest = "lasthour"; take.set_stage = QUEST_DONE;
        take.take_item = "torn_banner"; take.take_count = 1;
        take.give_item = "hour_blade"; take.give_count = 1;
        take.give_gold = 500; take.give_exp = 1000;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "lasthour_after";
        n.text = "— Стемнело. Сидим.\n"
                 "\n"
                 "Оказалось, после последнего часа ничего особенного не бывает.\n"
                 "Просто следующий. Мы уж и забыли, что так можно.";
        n.options.push_back(bye("Доброй ночи."));
        add(n);
    }

    // --- Ерофей: тот, кто остался в роще ---
    {
        DlgNode n; n.id = "grovekeeper_root";
        n.text = "Человек сидит под деревом, привалившись к стволу, и щурится\n"
                 "на свет, который не меняется.\n"
                 "— Садись. Отдых тут даром, а это редкость.";

        DlgOption rest;
        rest.text = "Посидеть у дерева.";
        rest.next = "grove_rest";
        rest.rest = true;
        n.options.push_back(rest);

        DlgOption why;
        why.text = "Почему ты не уходишь?";
        why.next = "grove_why";
        n.options.push_back(why);

        DlgOption marks;
        marks.text = "Я видел зарубки на стволе.";
        marks.next = "grove_marks";
        marks.req_note = "marks";
        n.options.push_back(marks);

        n.options.push_back(bye("Пойду."));
        add(n);
    }
    {
        DlgNode n; n.id = "grove_rest";
        n.text = "Ты садишься, и усталость уходит сразу вся, без остатка,\n"
                 "как будто её вынули. Свет не сдвинулся ни на палец.";
        n.options.push_back(bye("Хорошо тут."));
        add(n);
    }
    {
        DlgNode n; n.id = "grove_why";
        n.text = "— А куда? Везде хуже.\n"
                 "\n"
                 "Тут не темнеет, не холодает, не хочется есть. Раны заживают\n"
                 "к тому времени, как соберёшься их перевязать.\n"
                 "\n"
                 "Он поводит рукой вокруг.\n"
                 "\n"
                 "— Одна беда: раз ничего не проходит, то и ничего не проходит.\n"
                 "Понимаешь? Ни боль, ни ожидание, ни то, чего стыдно.\n"
                 "Всё как было в первый день, так и есть.\n"
                 "\n"
                 "Я тут не отдыхаю, чужак. Я тут стою на месте, и мне это подают\n"
                 "как отдых.";
        n.options.push_back(bye("Невесело."));
        add(n);
    }
    {
        DlgNode n; n.id = "grove_marks";
        n.text = "— Сто четыре. Резал каждое утро, пока не понял, что утра нет:\n"
                 "есть я, который решил, что пора резать.\n"
                 "\n"
                 "Сто четыре моих решения, а не сто четыре дня.\n"
                 "\n"
                 "После этого бросил. Считать себя — последнее дело.";
        n.options.push_back(bye("Береги себя."));
        add(n);
    }

    // --- Тихон: единственная надёжная дорога ---
    {
        DlgNode n; n.id = "pathkeeper_root";
        n.text = "На тропе стоит человек с мешком и палкой и смотрит, как ты идёшь.\n"
                 "— Идёшь правильно. Это уже много: тут почти все идут неправильно.";

        DlgOption why;
        why.text = "Куда ведёт эта тропа?";
        why.next = "path_why";
        n.options.push_back(why);

        DlgOption drift;
        drift.text = "Здесь все знают, что дрейфуют?";
        drift.next = "path_drift";
        n.options.push_back(drift);

        DlgOption trade;
        trade.text = "Что в мешке?";
        trade.open_shop = true;
        n.options.push_back(trade);

        // Изнанка: Тихон рассказывает про Край только тому, кто там уже был.
        DlgOption in_offer;
        in_offer.text = "Ты говорил про того, кто шагнул с Края.";
        in_offer.next = "inside_offer";
        in_offer.req_quest  = "inside"; in_offer.req_stage_min  = QUEST_NONE; in_offer.req_stage_max  = QUEST_NONE;
        in_offer.req_quest2 = "edgeq";  in_offer.req_stage2_min = QUEST_DONE; in_offer.req_stage2_max = QUEST_DONE;
        n.options.push_back(in_offer);

        DlgOption in_after;
        in_after.text = "Я был там. Внутри.";
        in_after.next = "inside_after";
        in_after.req_quest = "inside"; in_after.req_stage_min = QUEST_DONE; in_after.req_stage_max = QUEST_DONE;
        n.options.push_back(in_after);

        n.options.push_back(bye("Пойду дальше."));
        add(n);
    }
    {
        DlgNode n; n.id = "path_why";
        n.text = "— Домой. Единственная отсюда, которая правда домой.\n"
                 "\n"
                 "Остальные тоже куда-то ведут, и по ним даже приходят. Только\n"
                 "приходят не туда, откуда вышли, а в похожее место. Разница\n"
                 "маленькая: у кого дом на две ступени выше, у кого жена чуть\n"
                 "добрее. Живут потом и не жалуются.\n"
                 "\n"
                 "Я так не хочу. Я хочу в свой дом, даже если он хуже.";
        n.options.push_back(bye("Понимаю."));
        add(n);
    }
    {
        DlgNode n; n.id = "path_drift";
        n.text = "— Кто-то догадывается. Из здешних — я один.\n"
                 "\n"
                 "Остальным незачем. Улита ждёт утра, десятник держит до темноты,\n"
                 "Ерофей отдыхает. Скажи им — и что? Утро не придёт быстрее.\n"
                 "\n"
                 "Он поправляет мешок.\n"
                 "\n"
                 "— Я не добрый. Я просто пробовал говорить. Двоим сказал.\n"
                 "Один пошёл к Краю и шагнул, второй остался и перестал есть.\n"
                 "Больше не говорю.\n"
                 "\n"
                 "А ты говори, если считаешь нужным. Ты уйдёшь, а мне тут жить.";
        n.options.push_back(bye("Никому не скажу."));
        add(n);
    }

    // ================= Регион VI: Изнанка =================

    {
        DlgNode n; n.id = "inside_offer";
        n.text = "Тихон долго смотрит в сторону Края и наконец решается.\n"
                 "\n"
                 "— Я тебе про того, который шагнул, не всё сказал.\n"
                 "\n"
                 "Я потом ходил смотреть. Тела нет. И внизу нет — потому что\n"
                 "внизу вообще нет ничего, я проверял камнем: камень не падает,\n"
                 "он просто перестаёт быть виден.\n"
                 "\n"
                 "Значит, он не разбился. Значит, он куда-то попал.\n"
                 "\n"
                 "Он поправляет мешок и говорит уже совсем тихо:\n"
                 "\n"
                 "— Это был мой брат. Я двенадцать лет хожу мимо Края и не могу.\n"
                 "А ты можешь: тебе тут не жить.";
        DlgOption take;
        take.text = "Шагну.";
        take.next = "";
        take.set_quest = "inside"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Не сегодня."));
        add(n);
    }
    {
        DlgNode n; n.id = "inside_after";
        n.text = "— Значит, правда. Значит, он там.\n"
                 "\n"
                 "Тихон садится прямо на тропу, чего за двенадцать лет,\n"
                 "кажется, не делал ни разу.\n"
                 "\n"
                 "— Не пойду за ним. Мне тут дорогу показывать. Но теперь\n"
                 "хоть знаю, куда он делся, а это, оказывается, много.";
        n.options.push_back(bye("Бывай, Тихон."));
        add(n);
    }

    // --- Сторож Елисей: стык на честном слове ---
    {
        DlgNode n; n.id = "seamwatch_root";
        n.text = "Человек стоит, упершись ладонями в пустоту перед собой,\n"
                 "и по напряжению рук видно, что там что-то есть.\n"
                 "— Тише. Не толкни.";

        DlgOption offer;
        offer.text = "Что ты держишь?";
        offer.next = "firstjoint_offer";
        offer.req_quest = "firstjoint"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Вот расписка. Честное слово, писаное.";
        done.next = "firstjoint_reward";
        done.req_quest = "firstjoint"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "seam_word"; done.req_item_count = 1;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ещё ищу слово.";
        wait.next = "firstjoint_wait";
        wait.req_quest = "firstjoint"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption after;
        after.text = "Как руки?";
        after.next = "firstjoint_after";
        after.req_quest = "firstjoint"; after.req_stage_min = QUEST_DONE; after.req_stage_max = QUEST_DONE;
        n.options.push_back(after);

        DlgOption trade;
        trade.text = "Торгуешь чем-нибудь?";
        trade.open_shop = true;
        n.options.push_back(trade);

        n.options.push_back(bye("Держи."));
        add(n);
    }
    {
        DlgNode n; n.id = "firstjoint_offer";
        n.text = "— Стык. Первый, самый старый. Северный лоскут и южный.\n"
                 "\n"
                 "В описи сказано: крепление — слово. Я думал, ошибка писаря,\n"
                 "пока не встал сюда и не понял, что держать нечем: скобы нет,\n"
                 "узла нет, есть только то, что Мастер сказал «держать буду я».\n"
                 "\n"
                 "Он ушёл дальше. Я встал вместо него — на неделю, пока вернётся.\n"
                 "\n"
                 "Он поводит плечом, не отрывая ладоней.\n"
                 "\n"
                 "— Найди его слово писаным. Расписку, запись, что угодно.\n"
                 "Слово, положенное в стык, держит не хуже руки. Так в описи\n"
                 "и сказано, а я в опись верю: больше не во что.";
        DlgOption take;
        take.text = "Поищу расписку.";
        take.next = "firstjoint_wait";
        take.set_quest = "firstjoint"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Стой дальше."));
        add(n);
    }
    {
        DlgNode n; n.id = "firstjoint_wait";
        n.text = "— Ищи дальше по галерее. Там кто-то ходит взад-вперёд, я слышу\n"
                 "шаги вторую сотню лет. Окликать не пробовал: отпущу — разойдётся.";
        n.options.push_back(bye("Найду."));
        add(n);
    }
    {
        DlgNode n; n.id = "firstjoint_reward";
        n.text = "Елисей читает, не опуская рук. Потом кивает, и ты кладёшь\n"
                 "расписку в стык — в пустоту, где ничего нет.\n"
                 "\n"
                 "Лист не падает.\n"
                 "\n"
                 "Елисей осторожно отнимает одну ладонь. Потом вторую. Стоит,\n"
                 "растопырив пальцы, и смотрит на них, как на чужие.\n"
                 "\n"
                 "— Двести лет. Я думал, будет громче.\n"
                 "\n"
                 "Он опускает руки и морщится: плечи не слушаются.\n"
                 "\n"
                 "— Шлем возьми. Мне в нём стоять было, а ходить я в нём\n"
                 "не умею.";
        DlgOption take;
        take.text = "Принять шлем. [1100 опыта]";
        take.set_quest = "firstjoint"; take.set_stage = QUEST_DONE;
        take.take_item = "seam_word"; take.take_count = 1;
        take.give_item = "seam_helm"; take.give_count = 1;
        take.give_gold = 600; take.give_exp = 1100;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "firstjoint_after";
        n.text = "— Болят. Первый раз за двести лет болят, и это, знаешь,\n"
                 "приятно: значит, они мои.\n"
                 "\n"
                 "Стык держит. Я проверяю каждый час, но он держит.";
        n.options.push_back(bye("Отдыхай, Елисей."));
        add(n);
    }

    // --- Разметчик Пров: линии подтянуты нарочно ---
    {
        DlgNode n; n.id = "surveyor_root";
        n.text = "Человек стоит посреди галереи, чуть отвернув голову, и смотрит\n"
                 "мимо всего, что перед ним.\n"
                 "— Прямо не гляди. Прямо их не видно.";

        DlgOption offer;
        offer.text = "Что тут видно непрямо?";
        offer.next = "lines_offer";
        offer.req_quest = "lines"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Пять нитей.";
        done.next = "lines_reward";
        done.req_quest = "lines"; done.req_stage_min = 1; done.req_stage_max = 1;
        done.req_item = "line_thread"; done.req_item_count = 5;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ещё собираю нити.";
        wait.next = "lines_wait";
        wait.req_quest = "lines"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        DlgOption brother;
        brother.text = "Тебя не Тихон ли ищет?";
        brother.next = "surveyor_brother";
        brother.req_quest = "inside"; brother.req_stage_min = QUEST_DONE; brother.req_stage_max = QUEST_DONE;
        n.options.push_back(brother);

        n.options.push_back(bye("Размечай."));
        add(n);
    }
    {
        DlgNode n; n.id = "lines_offer";
        n.text = "— Линии. Вся сеть, как она есть, без чертежа и без вранья.\n"
                 "\n"
                 "Толстая линия — коротко. Тонкая — далеко. Оборванная — та,\n"
                 "по которой прошли в последний раз.\n"
                 "\n"
                 "Оборванных больше половины, и вот что меня держит тут\n"
                 "двенадцатый год: я не могу понять, сами они рвутся или их\n"
                 "рвут. Разница вся.\n"
                 "\n"
                 "Принеси пять нитей с тех, кто по ним ходит. Посмотрю на обрыв.\n"
                 "Рваное от резаного я отличу, я разметчик.";
        DlgOption take;
        take.text = "Принесу пять.";
        take.next = "lines_wait";
        take.set_quest = "lines"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Смотри дальше."));
        add(n);
    }
    {
        DlgNode n; n.id = "lines_wait";
        n.text = "— Пять. Меньше нельзя: по одной не поймёшь, по двум соврёшь.";
        n.options.push_back(bye("Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "lines_reward";
        n.text = "Пров раскладывает нити на ладони и смотрит на них мимо, боком,\n"
                 "долго. Потом сжимает кулак.\n"
                 "\n"
                 "— Резаное. Все пять.\n"
                 "\n"
                 "— Никакого износа. Их подтягивали: режут длинную и связывают\n"
                 "покороче, и мир делается теснее на этот кусок. Ровно, аккуратно,\n"
                 "по всей сети. Двести лет.\n"
                 "\n"
                 "Он садится на пол галереи.\n"
                 "\n"
                 "— Я двенадцать лет надеялся, что оно само. Само — это беда.\n"
                 "А так выходит, что кто-то до сих пор режет.\n"
                 "\n"
                 "Клинок возьми. Он тут валялся, а мне резать нечего.";
        DlgOption take;
        take.text = "Принять клинок. [1200 опыта]";
        take.set_quest = "lines"; take.set_stage = QUEST_DONE;
        take.take_item = "line_thread"; take.take_count = 5;
        take.give_item = "line_blade"; take.give_count = 1;
        take.give_gold = 700; take.give_exp = 1200;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "surveyor_brother";
        n.text = "Пров впервые поворачивается прямо.\n"
                 "\n"
                 "— Тихон. Живой, значит.\n"
                 "\n"
                 "— Я не шагал с Края, чужак, я вошёл через шов, как все.\n"
                 "Это он себе придумал, будто я сорвался, и двенадцать лет\n"
                 "об этом думает. Я знал и не поправил.\n"
                 "\n"
                 "Он снова отворачивает голову вбок.\n"
                 "\n"
                 "— Потому что если поправить, надо возвращаться. А я тут\n"
                 "линии считаю. Двенадцать лет считаю.\n"
                 "\n"
                 "— Скажи ему, что я досчитал. Теперь можно и назад.";
        n.options.push_back(bye("Скажу."));
        add(n);
    }

    // --- Первый Мастер у Первого узла ---
    {
        DlgNode n; n.id = "master_root";
        n.text = "У кривого узла стоит старик с посохом, стёртым снизу на две\n"
                 "ладони. Он не оборачивается — он тебя ждал.\n"
                 "— Долго. Я уж думал, никто не дойдёт.";

        DlgOption offer;
        offer.text = "Ты кто?";
        offer.next = "remembers_offer";
        offer.req_quest = "remembers"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption word;
        word.text = "Елисей двести лет держит твой стык.";
        word.next = "master_word";
        word.req_quest = "firstjoint"; word.req_stage_min = 1; word.req_stage_max = 1;
        n.options.push_back(word);

        DlgOption ring;
        ring.text = "Твоё кольцо у меня. Из Зала Отказа.";
        ring.next = "master_ring_talk";
        ring.req_item = "master_ring"; ring.req_item_count = 1;
        n.options.push_back(ring);

        DlgOption way;
        way.text = "Расскажи дорогу до конца.";
        way.next = "remembers_way";
        way.req_quest = "remembers"; way.req_stage_min = 1; way.req_stage_max = 1;
        n.options.push_back(way);

        DlgOption after;
        after.text = "Ты идёшь за мной?";
        after.next = "remembers_after";
        after.req_quest = "remembers"; after.req_stage_min = QUEST_DONE; after.req_stage_max = QUEST_DONE;
        n.options.push_back(after);

        n.options.push_back(bye("Мне дальше."));
        add(n);
    }
    {
        DlgNode n; n.id = "remembers_offer";
        n.text = "— Тот, кто сказал «не связывайте» и остался в меньшинстве.\n"
                 "\n"
                 "Он говорит это буднично, как называют ремесло.\n"
                 "\n"
                 "— Я снял кольцо, вышел и пошёл. Думал — до ближайшего узла,\n"
                 "посмотреть своими глазами. Посмотрел. Пошёл до следующего.\n"
                 "Так и иду.\n"
                 "\n"
                 "— На могиле моей, говорят, приписано «я ещё хожу». Не моей\n"
                 "рукой, но по правде.\n"
                 "\n"
                 "— Спрашивай, чужак. Я двести лет не с кем.";
        DlgOption take;
        take.text = "Спрошу.";
        take.next = "remembers_way";
        take.set_quest = "remembers"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Потом."));
        add(n);
    }
    {
        DlgNode n; n.id = "remembers_way";
        n.text = "— Дорога короткая, и она вся отсюда видна.\n"
                 "\n"
                 "За узлом — Сердце. Оно тянет. Не злое, не разумное: просто\n"
                 "не умеет иначе. Его можно остановить, и я тебе не советую\n"
                 "и не отговариваю — я двести лет назад уже насоветовал.\n"
                 "\n"
                 "За Сердцем — Точка Ноль. Там до всего ноль, и потому там нет\n"
                 "ничего. Идти туда незачем, кроме одного: только оттуда\n"
                 "решение и работает.\n"
                 "\n"
                 "А за Точкой — комната, где лежит лист. Я его сам писал.\n"
                 "\n"
                 "Он поворачивается наконец.\n"
                 "\n"
                 "— Три исхода. Я знаю их все и не выбрал ни одного, потому что\n"
                 "мне потом не жить, а тебе — жить. Плохой я советчик.\n"
                 "\n"
                 "— Дойдёшь до листа — я подойду. Не помогать. Просто рядом\n"
                 "постою: такое в одиночку не решают.";
        DlgOption take;
        take.text = "Понял. Иду.";
        take.set_quest = "remembers"; take.set_stage = QUEST_DONE;
        take.give_item = "walk_staff"; take.give_count = 1;
        take.give_exp = 1300;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "remembers_after";
        n.text = "— Иду. Медленно: у меня посох стёрт, а новый брать негде.\n"
                 "\n"
                 "Ты иди вперёд. Я успею.";
        n.options.push_back(bye("Успевай."));
        add(n);
    }
    {
        DlgNode n; n.id = "master_word";
        n.text = "Старик молчит очень долго.\n"
                 "\n"
                 "— Елисей. Рыжий такой, картавил.\n"
                 "\n"
                 "— Я сказал: подержи неделю. Я правда думал, что неделю.\n"
                 "\n"
                 "Он лезет за пазуху и достаёт сложенный вчетверо лист,\n"
                 "мягкий от двухсот лет.\n"
                 "\n"
                 "— Вот моё слово, писаное. Я его с собой носил, чтобы помнить,\n"
                 "что должен вернуться. Носил и не вернулся: сперва думал —\n"
                 "рано, потом — поздно, потом просто шёл.\n"
                 "\n"
                 "— Отнеси. И не говори ему, что я рядом. Пусть уж лучше думает,\n"
                 "что я не дошёл, чем что дошёл и не свернул.";
        DlgOption take;
        take.text = "Взять расписку.";
        take.give_item = "seam_word"; take.give_count = 1;
        n.options.push_back(take);
        add(n);
    }
    {
        DlgNode n; n.id = "master_ring_talk";
        n.text = "Он смотрит на кольцо и не берёт.\n"
                 "\n"
                 "— Значит, Никон дописал протокол. И записал, что я был прав.\n"
                 "\n"
                 "— Знаешь, что смешно? Я не был прав. Я был осторожен, а это\n"
                 "другое. Осторожный, который угадал, задним числом делается\n"
                 "мудрецом, но угадал-то он.\n"
                 "\n"
                 "— Оставь себе. Мне его надевать не на что: я тогда снял\n"
                 "не украшение, а право решать. Второй раз надену — опять\n"
                 "начну решать за других.";
        n.options.push_back(bye("Понимаю."));
        add(n);
    }

    // --- Развязка: три исхода, и один из них навсегда ---
    {
        DlgNode n; n.id = "finale_root";
        n.text = "Старик стоит поодаль, опершись на посох, и молчит. Он\n"
                 "не подойдёт ближе и не скажет ни слова, пока ты не скажешь\n"
                 "первым.\n"
                 "\n"
                 "На камне лежит лист, придавленный кольцом. Три строки.";

        DlgOption read;
        read.text = "Что ты сам думаешь?";
        read.next = "finale_ask";
        read.req_quest = "finale"; read.req_stage_min = 1; read.req_stage_max = 1;
        n.options.push_back(read);

        DlgOption pull;
        pull.text = "Дотянуть. Пусть все будут рядом.";
        pull.next = "";
        pull.req_quest = "finale"; pull.req_stage_min = 1; pull.req_stage_max = 1;
        pull.set_quest = "finale"; pull.set_stage = QUEST_DONE;
        pull.set_counter = "ending_choice"; pull.set_counter_value = 1;
        pull.give_exp = 2000;
        pull.ending = "pull";
        n.options.push_back(pull);

        DlgOption cut;
        cut.text = "Разрезать. Пусть мир станет большим.";
        cut.next = "";
        cut.req_quest = "finale"; cut.req_stage_min = 1; cut.req_stage_max = 1;
        cut.set_quest = "finale"; cut.set_stage = QUEST_DONE;
        cut.set_counter = "ending_choice"; cut.set_counter_value = 2;
        cut.give_exp = 2000;
        cut.ending = "cut";
        n.options.push_back(cut);

        DlgOption hold;
        hold.text = "Удержать. Ничего не менять.";
        hold.next = "";
        hold.req_quest = "finale"; hold.req_stage_min = 1; hold.req_stage_max = 1;
        hold.set_quest = "finale"; hold.set_stage = QUEST_DONE;
        hold.set_counter = "ending_choice"; hold.set_counter_value = 3;
        hold.give_exp = 2000;
        hold.ending = "hold";
        n.options.push_back(hold);

        DlgOption after_p;
        after_p.text = "Ну вот и всё.";
        after_p.next = "finale_after_pull";
        after_p.req_quest = "finale"; after_p.req_stage_min = QUEST_DONE; after_p.req_stage_max = QUEST_DONE;
        after_p.req_counter = "ending_choice"; after_p.req_counter_min = 1; after_p.req_counter_max = 1;
        n.options.push_back(after_p);

        DlgOption after_c;
        after_c.text = "Ну вот и всё.";
        after_c.next = "finale_after_cut";
        after_c.req_quest = "finale"; after_c.req_stage_min = QUEST_DONE; after_c.req_stage_max = QUEST_DONE;
        after_c.req_counter = "ending_choice"; after_c.req_counter_min = 2; after_c.req_counter_max = 2;
        n.options.push_back(after_c);

        DlgOption after_h;
        after_h.text = "Ну вот и всё.";
        after_h.next = "finale_after_hold";
        after_h.req_quest = "finale"; after_h.req_stage_min = QUEST_DONE; after_h.req_stage_max = QUEST_DONE;
        after_h.req_counter = "ending_choice"; after_h.req_counter_min = 3; after_h.req_counter_max = 3;
        n.options.push_back(after_h);

        n.options.push_back(bye("Ещё не готов."));
        add(n);
    }
    {
        DlgNode n; n.id = "finale_ask";
        n.text = "— Я думаю, что рад, что не мне.\n"
                 "\n"
                 "Он говорит это без насмешки, очень просто.\n"
                 "\n"
                 "— Дотянешь — все найдутся. Улита увидит своих, Пелагея\n"
                 "дочитает четыре тысячи двести имён за один вечер. И никто\n"
                 "никуда больше не пойдёт, никогда.\n"
                 "\n"
                 "— Разрежешь — мир станет большим и честным. И до травницы\n"
                 "твоей будет полгода пути, а до кого-то — не дойти вовсе.\n"
                 "Половина лоскутов ляжет туда, где им место, и одному место\n"
                 "в океане.\n"
                 "\n"
                 "— Удержишь — не изменится ничего. Это дороже всего: держать\n"
                 "надо вечно, и вместо тебя не встанет никто, потому что\n"
                 "сюда никто не доходит. Ты видел Елисея. Вот так двести лет.\n"
                 "\n"
                 "— Я стою здесь и не выбрал. Не бери с меня примера: не выбрать\n"
                 "— тоже выбрать, только трусливо.";
        n.options.push_back(bye("Дай подумать."));
        add(n);
    }
    {
        DlgNode n; n.id = "finale_after_pull";
        n.text = "— Всё. Теперь до меня отсюда ноль шагов, и до тебя тоже.\n"
                 "\n"
                 "Он стоит там же, где стоял, и одновременно рядом.\n"
                 "\n"
                 "— Двести лет ходил, чтобы дойти до места, где ходить нельзя.\n"
                 "Ну хоть посох поставлю.";
        n.options.push_back(bye("Ставь."));
        add(n);
    }
    {
        DlgNode n; n.id = "finale_after_cut";
        n.text = "— Всё. И мир опять большой.\n"
                 "\n"
                 "Он смотрит вдаль и щурится: раньше вдаль смотреть было\n"
                 "некуда.\n"
                 "\n"
                 "— Пойду. Далеко теперь, а посох стёрт. Ничего: я привык,\n"
                 "что далеко.";
        n.options.push_back(bye("Доброй дороги."));
        add(n);
    }
    {
        DlgNode n; n.id = "finale_after_hold";
        n.text = "— Всё как было. Ты понимаешь, что это навсегда?\n"
                 "\n"
                 "Он впервые за разговор опускает посох и садится.\n"
                 "\n"
                 "— Тогда я посижу с тобой. Не помогать — я не умею держать,\n"
                 "я умею уходить. Просто посижу.\n"
                 "\n"
                 "— Огня хватит на двоих.";
        n.options.push_back(bye("Хватит."));
        add(n);
    }

    // --- Писарь Феофан ---
    {
        DlgNode n; n.id = "scribe_root";
        n.text = "Писарь сидит на ступенях запертого архива с пустой чернильницей.\n"
                 "— Двести лет как переписал начисто и запер. Ключ отдал смотрителю.";

        DlgOption offer;
        offer.text = "А смотритель?";
        offer.next = "lists_offer";
        offer.req_quest = "lists"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption done;
        done.text = "Я прочёл опись. Каждого — половина.";
        done.next = "lists_reward";
        done.req_quest = "lists"; done.req_stage_min = 2; done.req_stage_max = 2;
        n.options.push_back(done);

        DlgOption wait;
        wait.text = "Ещё не добрался до описи.";
        wait.next = "lists_wait";
        wait.req_quest = "lists"; wait.req_stage_min = 1; wait.req_stage_max = 1;
        n.options.push_back(wait);

        n.options.push_back(bye("Ещё зайду."));
        add(n);
    }
    {
        DlgNode n; n.id = "lists_offer";
        n.text = "— На посту. Стоит и не сменяется. Я спрашивал ключ — он смотрит\n"
                 "и молчит. Он не злой, ему просто велено, а того, кто велел,\n"
                 "давно нет.\n"
                 "\n"
                 "Мне нужна опись. Не списки — опись, первый лист. Там сказано,\n"
                 "как считали. Я всю жизнь думаю, что мы посчитали неправильно.";
        DlgOption take;
        take.text = "Достану ключ и прочту.";
        take.next = "lists_wait";
        take.set_quest = "lists"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Это между вами."));
        add(n);
    }
    {
        DlgNode n; n.id = "lists_wait";
        n.text = "— Ключ у смотрителя. Другого нет, я проверял сорок лет.\n"
                 "И вот что: он не отдаст. Совсем.";
        n.options.push_back(bye("Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "lists_reward";
        n.text = "Феофан слушает, и лицо у него делается спокойное.\n"
                 "— Половина. Ну конечно.\n"
                 "Мы считали души, а надо было считать имена. Четыре тысячи двести\n"
                 "душ, и каждая записана дважды по половине — здесь и там.\n"
                 "\n"
                 "Значит, никто не погиб. Значит, всех просто разрезало.\n"
                 "Двести лет я думал, что мы ошиблись в арифметике. Мы ошиблись\n"
                 "в том, что считали.";
        DlgOption take;
        take.text = "Отдать половину имени. [480 опыта]";
        take.set_quest = "lists"; take.set_stage = QUEST_DONE;
        take.give_item = "clerk_robe"; take.give_count = 1;
        take.give_gold = 360; take.give_exp = 480;
        n.options.push_back(take);
        add(n);
    }

    // --- Перевозчик Хмурый ---
    {
        DlgNode n; n.id = "ferry_root";
        n.text = "Лодка стоит на воде, но вода стоит тоже. Перевозчик сидит в ней\n"
                 "и не гребёт.\n"
                 "— Ходить можешь. Плавать не пробуй.";
        DlgOption why;
        why.text = "Почему не плавать?";
        why.next = "ferry_why";
        n.options.push_back(why);
        DlgOption token;
        token.text = "Зачем тебе жетоны, если ты не возишь?";
        token.next = "ferry_token_talk";
        n.options.push_back(token);
        DlgOption trade;
        trade.text = "Что продаёшь?";
        trade.open_shop = true;
        n.options.push_back(trade);
        n.options.push_back(bye("Пойду по воде."));
        add(n);
    }
    {
        DlgNode n; n.id = "ferry_why";
        n.text = "— Потому что не утонешь.\n"
                 "Думаешь, хорошо? Не утонешь и не выплывешь. Так и будешь\n"
                 "стоять по грудь, пока не надоест. А надоедает не сразу.";
        n.options.push_back(bye("Ясно."));
        add(n);
    }
    {
        DlgNode n; n.id = "ferry_token_talk";
        n.text = "— Жетон не плата. Жетон — счёт.\n"
                 "Сколько роздал, столько должно вернуться. Вернулось меньше —\n"
                 "значит, кто-то остался на воде, и я иду искать.\n"
                 "\n"
                 "Пять не вернулось. Пятый год пятеро.";
        n.options.push_back(bye("Найдутся."));
        add(n);
    }

    // --- Копач Тишка: подсказки о шахтах ---
    {
        DlgNode n; n.id = "digger_root";
        n.text = "Копач сидит спиной к штольне и не оборачивается на шаги.\n"
                 "— Не спрашивай, что там. Спроси лучше, почему я снаружи.";
        DlgOption why;
        why.text = "Почему ты снаружи?";
        why.next = "digger_why";
        n.options.push_back(why);
        DlgOption hint;
        hint.text = "Что там, внизу?";
        hint.next = "digger_hint";
        n.options.push_back(hint);
        n.options.push_back(bye("Ясно."));
        add(n);
    }
    {
        DlgNode n; n.id = "digger_why";
        n.text = "— Потому что мы заложили нижнюю штольню своими руками и написали\n"
                 "наказ: не разбирай кладку. А теперь сижу и сторожу — вдруг\n"
                 "кто грамотный придёт и решит, что это иносказание.";
        n.options.push_back(bye("Не иносказание. Понял."));
        add(n);
    }
    {
        DlgNode n; n.id = "digger_hint";
        n.text = "— Соль там другая. Держит. Мышь осенью попала — весной как живая,\n"
                 "только не дышит.\n"
                 "А ещё там мокро, хотя воде взяться неоткуда. Мельник наш всё\n"
                 "считает, куда его река девается. Я ему не говорю.";
        n.options.push_back(bye("Спасибо."));
        add(n);
    }

    // --- Два Прохора: выбор без правильного ответа ---
    {
        DlgNode n; n.id = "prohor_l_root";
        n.text = "Мужик у левого двора чинит городьбу. Руки в занозах, взгляд усталый.\n"
                 "— Ты чужой. Значит, не знаешь ещё. Он тоже Прохор. И двор его —\n"
                 "тоже мой.";

        DlgOption offer;
        offer.text = "Как такое вышло?";
        offer.next = "prohor_offer";
        offer.req_quest = "doubled"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption choose_l;
        choose_l.text = "Ты настоящий. Двор твой.";
        choose_l.next = "prohor_l_chosen";
        choose_l.req_quest = "doubled"; choose_l.req_stage_min = 1; choose_l.req_stage_max = 1;
        choose_l.set_quest = "doubled"; choose_l.set_stage = QUEST_DONE;
        choose_l.set_counter = "prohor_choice"; choose_l.set_counter_value = 1;
        choose_l.give_item = "brine_ring"; choose_l.give_count = 1;
        choose_l.give_exp = 240;
        n.options.push_back(choose_l);

        DlgOption after_win;
        after_win.text = "Как хозяйство?";
        after_win.next = "prohor_l_after";
        after_win.req_quest = "doubled"; after_win.req_stage_min = QUEST_DONE; after_win.req_stage_max = QUEST_DONE;
        after_win.req_counter = "prohor_choice"; after_win.req_counter_min = 1; after_win.req_counter_max = 1;
        n.options.push_back(after_win);

        DlgOption after_lose;
        after_lose.text = "...";
        after_lose.next = "prohor_l_lost";
        after_lose.req_quest = "doubled"; after_lose.req_stage_min = QUEST_DONE; after_lose.req_stage_max = QUEST_DONE;
        after_lose.req_counter = "prohor_choice"; after_lose.req_counter_min = 2; after_lose.req_counter_max = 2;
        n.options.push_back(after_lose);

        n.options.push_back(bye("Мне надо подумать."));
        add(n);
    }
    {
        DlgNode n; n.id = "prohor_r_root";
        n.text = "Мужик у правого двора точит косу. Руки в занозах, взгляд усталый.\n"
                 "Те же руки. Тот же взгляд.\n"
                 "— Он тебе уже сказал, что я самозванец? Он всем говорит.";

        DlgOption offer;
        offer.text = "Как такое вышло?";
        offer.next = "prohor_offer";
        offer.req_quest = "doubled"; offer.req_stage_min = QUEST_NONE; offer.req_stage_max = QUEST_NONE;
        n.options.push_back(offer);

        DlgOption choose_r;
        choose_r.text = "Ты настоящий. Двор твой.";
        choose_r.next = "prohor_r_chosen";
        choose_r.req_quest = "doubled"; choose_r.req_stage_min = 1; choose_r.req_stage_max = 1;
        choose_r.set_quest = "doubled"; choose_r.set_stage = QUEST_DONE;
        choose_r.set_counter = "prohor_choice"; choose_r.set_counter_value = 2;
        choose_r.give_item = "brine_ring"; choose_r.give_count = 1;
        choose_r.give_exp = 240;
        n.options.push_back(choose_r);

        DlgOption after_win;
        after_win.text = "Как хозяйство?";
        after_win.next = "prohor_r_after";
        after_win.req_quest = "doubled"; after_win.req_stage_min = QUEST_DONE; after_win.req_stage_max = QUEST_DONE;
        after_win.req_counter = "prohor_choice"; after_win.req_counter_min = 2; after_win.req_counter_max = 2;
        n.options.push_back(after_win);

        DlgOption after_lose;
        after_lose.text = "...";
        after_lose.next = "prohor_r_lost";
        after_lose.req_quest = "doubled"; after_lose.req_stage_min = QUEST_DONE; after_lose.req_stage_max = QUEST_DONE;
        after_lose.req_counter = "prohor_choice"; after_lose.req_counter_min = 1; after_lose.req_counter_max = 1;
        n.options.push_back(after_lose);

        n.options.push_back(bye("Мне надо подумать."));
        add(n);
    }
    {
        DlgNode n; n.id = "prohor_offer";
        n.text = "— А вот так. Проснулись — а двор напротив. И в нём я.\n"
                 "Он знает, как мать звала со двора. Знает, где у меня шрам,\n"
                 "и шрам у него на том же месте.\n"
                 "\n"
                 "Я сперва думал: пусть уходит. Потом понял — он думает про меня\n"
                 "то же самое и с тем же правом. Один лишний, и никто не знает,\n"
                 "который.\n"
                 "\n"
                 "Рассуди, чужой. Мы оба примем.";
        DlgOption take;
        take.text = "Я посмотрю на обоих.";
        take.next = "";
        take.set_quest = "doubled"; take.set_stage = 1;
        n.options.push_back(take);
        n.options.push_back(bye("Это не моё дело."));
        add(n);
    }
    {
        DlgNode n; n.id = "prohor_l_chosen";
        n.text = "Он кивает, будто ждал.\n"
                 "— Ладно. Пойду скажу ему.\n"
                 "\n"
                 "Уходит к правому двору. Разговора не слышно. Через час правый\n"
                 "Прохор выходит с узлом за плечом и идёт на север, не оглядываясь.\n"
                 "Левый долго смотрит вслед, потом возвращается к городьбе.\n"
                 "\n"
                 "— Спасибо, — говорит он, не поднимая головы. — Наверное.";
        n.options.push_back(bye("Наверное."));
        add(n);
    }
    {
        DlgNode n; n.id = "prohor_r_chosen";
        n.text = "Он кивает, будто ждал.\n"
                 "— Ладно. Пойду скажу ему.\n"
                 "\n"
                 "Уходит к левому двору. Разговора не слышно. Через час левый\n"
                 "Прохор выходит с узлом за плечом и идёт на север, не оглядываясь.\n"
                 "Правый долго смотрит вслед, потом возвращается к косе.\n"
                 "\n"
                 "— Спасибо, — говорит он, не поднимая головы. — Наверное.";
        n.options.push_back(bye("Наверное."));
        add(n);
    }
    {
        DlgNode n; n.id = "prohor_l_after";
        n.text = "— Хозяйство? Хозяйство идёт.\n"
                 "Только я теперь на его двор не хожу. И на север не смотрю.";
        n.options.push_back(bye("Понимаю."));
        add(n);
    }
    {
        DlgNode n; n.id = "prohor_r_after";
        n.text = "— Идёт помаленьку. Косу вон наточил.\n"
                 "Ты не думай, я на тебя зла не держу. Ты рассудил, как умел.";
        n.options.push_back(bye("Бывай."));
        add(n);
    }
    {
        DlgNode n; n.id = "prohor_l_lost";
        n.text = "Левый двор пуст. Городьба недочинена, инструмент лежит там,\n"
                 "где его положили. Дверь не заперта.\n"
                 "\n"
                 "На столе початый хлеб.";
        n.options.push_back(bye("Уйти."));
        add(n);
    }
    {
        DlgNode n; n.id = "prohor_r_lost";
        n.text = "Правый двор пуст. Коса прислонена к стене, точило рядом.\n"
                 "Дверь не заперта.\n"
                 "\n"
                 "На столе початый хлеб.";
        n.options.push_back(bye("Уйти."));
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

        DlgOption keep;
        keep.text = "Я нашёл в обители оловянного солдатика.";
        keep.next = "keepsake_talk";
        keep.req_quest = "keepsake"; keep.req_stage_min = 1; keep.req_stage_max = 1;
        keep.req_item = "keepsake"; keep.req_item_count = 1;
        n.options.push_back(keep);

        DlgOption cinch_ask;
        cinch_ask.text = "Я нашёл отчёт Ордена. Стяжение идёт до сих пор.";
        cinch_ask.next = "cinch_talk";
        cinch_ask.req_quest = "cinch"; cinch_ask.req_stage_min = 1; cinch_ask.req_stage_max = 1;
        n.options.push_back(cinch_ask);

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
        DlgNode n; n.id = "keepsake_talk";
        n.text = "Отшельник берёт солдатика двумя пальцами и не сразу может\n"
                 "поставить его на землю: рука не слушается.\n"
                 "\n"
                 "— Келья одиннадцатая. Я его сам туда и положил.\n"
                 "\n"
                 "Он долго молчит.\n"
                 "\n"
                 "— Меня зовут Игнат. Мы должны были идти вдвоём, и я остался\n"
                 "снаружи — правило такое, чтобы было кому вспомнить.\n"
                 "Он не вышел. Я ждал сорок лет, потом ушёл сам.\n"
                 "\n"
                 "Вот и всё вспоминание, чужак. Двести лет — и оловянный солдатик.";
        DlgOption take;
        take.text = "Отдать солдатика. [600 опыта]";
        take.set_quest = "keepsake"; take.set_stage = QUEST_DONE;
        take.take_item = "keepsake"; take.take_count = 1;
        take.give_item = "portal_stone"; take.give_count = 2;
        take.give_exp = 600;
        n.options.push_back(take);
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
        DlgNode n; n.id = "cinch_talk";
        n.text = "Отшельник долго смотрит на лист, потом кладёт его в огонь.\n"
                 "— Значит, дошёл. Ладно.\n"
                 "Сеть строили, чтобы отменить расстояние. Она согласилась —\n"
                 "и с тех пор тянет. Медленно. За твою жизнь Ольховка сойдётся\n"
                 "с заставой, а через три — с тем, что за ней.\n"
                 "\n"
                 "Кончится это одним из трёх. Либо кто-то дотянет узел до конца,\n"
                 "и всё станет одной точкой — Орден этого и хотел. Либо кто-то\n"
                 "разрежет сеть, и мир снова станет огромным: до Лады будет\n"
                 "полгода пути, и она об этом не узнает. Либо кто-то удержит\n"
                 "как есть — и это труднее всего, потому что держать надо вечно.\n"
                 "\n"
                 "Я держу двадцать лет. Я устал.";
        DlgOption take;
        take.text = "Я подумаю, что с этим делать. [300 опыта]";
        take.set_quest = "cinch"; take.set_stage = QUEST_DONE;
        take.give_exp = 300;
        take.give_item = "portal_stone"; take.give_count = 2;
        n.options.push_back(take);
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
const NoteDef* Content::note(const std::string& id) const {
    auto it = notes_.find(id);
    return it == notes_.end() ? nullptr : &it->second;
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

const EndingDef* Content::ending(const std::string& id) const {
    std::map<std::string, EndingDef>::const_iterator it = endings_.find(id);
    return it == endings_.end() ? nullptr : &it->second;
}

std::string Content::quest_stage_text(const std::string& quest_id, int stage) const {
    const QuestDef* q = quest(quest_id);
    if (!q) return "";
    for (const QuestStageDef& s : q->stages)
        if (s.stage == stage) return s.text;
    return "";
}
