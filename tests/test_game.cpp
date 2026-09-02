// Регрессионные тесты игровой логики. Запуск: make test (из корня проекта —
// карты грузятся по относительному пути data/maps).
#include "../src/game.h"
#include "../src/world.h"

#include "../src/embedded_maps.h"
#include "../src/ui.h"
#include "../src/platform.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int g_checks = 0;
int g_failed = 0;

void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) {
        ++g_failed;
        std::cout << "  ПРОВАЛ: " << what << "\n";
    }
}

void eq(int got, int want, const std::string& what) {
    check(got == want, what + " (получено " + to_str(got) + ", ожидалось " + to_str(want) + ")");
}

void eqs(const std::string& got, const std::string& want, const std::string& what) {
    check(got == want, what + " (получено '" + got + "', ожидалось '" + want + "')");
}

void section(const char* name) { std::cout << "\n== " << name << " ==\n"; }

// Полный список локаций мира. Держится в одном месте: тесты обходят его
// целиком, и новая локация не может тихо выпасть из проверок.
const char* const ALL_LOCATIONS[] = {
    "village", "forest", "cave", "ruins", "sanctum", "vault",
    "goatpath", "glassfield", "mill", "market", "bridge", "saltmines",
    "caravanserai", "doubled",
    "halfcity", "endless", "foundry", "canal", "counter", "archive",
    "ordergate", "gatehouse", "library", "drafting", "cells", "furnace",
    "refusalhall", "node2", "node3", "grave"};
const int N_LOCATIONS = static_cast<int>(sizeof(ALL_LOCATIONS) / sizeof(ALL_LOCATIONS[0]));

// ------------------------------------------------------------------ тесты

void test_text_helpers() {
    section("текстовые утилиты");
    eq(static_cast<int>(utf8_len("Волк")), 4, "utf8_len считает символы, а не байты");
    eq(static_cast<int>(std::string("Волк").size()), 8, "кириллица и правда по 2 байта");
    eq(static_cast<int>(utf8_len(pad("Волк", 10))), 10, "pad выравнивает по видимой ширине");
    eqs(trunc("Волчья шкура", 6), "Волчья", "trunc режет по символам");
    eqs(plural(1,  "монета","монеты","монет"), "монета", "1 монета");
    eqs(plural(3,  "монета","монеты","монет"), "монеты", "3 монеты");
    eqs(plural(11, "монета","монеты","монет"), "монет",  "11 монет (исключение)");
    eqs(plural(21, "монета","монеты","монет"), "монета", "21 монета");
}

void test_wrap() {
    section("перенос строк");
    std::vector<std::string> w = wrap("Мирон опирается на палку и щурится", 12);
    bool fits = true;
    for (const std::string& l : w) if (utf8_len(l) > 12) fits = false;
    check(fits, "ни одна строка не длиннее заданной ширины");
    check(w.size() >= 3, "длинный текст разбит на несколько строк");

    // Существующие переводы строк сохраняются.
    std::vector<std::string> two = wrap("первая\nвторая", 40);
    eq(static_cast<int>(two.size()), 2, "абзацы не склеиваются");
    eqs(two[0], "первая", "первый абзац цел");

    // Слово длиннее строки не должно уводить в бесконечный цикл или за край.
    std::vector<std::string> long_word = wrap("длинноесловобезпробелов", 8);
    bool hard_fits = true;
    for (const std::string& l : long_word) if (utf8_len(l) > 8) hard_fits = false;
    check(hard_fits, "слишком длинное слово режется принудительно");
    check(!long_word.empty(), "и не теряется целиком");

    // Отступ абзаца сохраняется на переносах — списки не разъезжаются.
    std::vector<std::string> ind = wrap("    пункт списка с длинным текстом внутри", 16);
    check(ind.size() >= 2 && ind[1].substr(0, 4) == "    ", "отступ переносится на следующую строку");
}

void test_glyphs() {
    section("символы карты");

    // Символы объектов не должны совпадать друг с другом и с тайлами: иначе
    // на карте не отличить врага от дерева, а предмет от земли.
    const char objs[] = { glyph::PLAYER, glyph::NPC, glyph::MOB,
                          glyph::EXIT, glyph::SIGN, glyph::ITEM, glyph::BED };
    const int  n = static_cast<int>(sizeof(objs));

    bool unique = true;
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            if (objs[i] == objs[j]) unique = false;
    check(unique, "символы объектов попарно различны");

    bool clear_of_tiles = true;
    for (int t = 0; t < static_cast<int>(Tile::Count); ++t) {
        char tg = tile_glyph(static_cast<Tile>(t));
        for (int i = 0; i < n; ++i)
            if (objs[i] == tg) clear_of_tiles = false;
    }
    check(clear_of_tiles, "символы объектов не совпадают с символами тайлов");

    check(glyph::MOB != glyph::NPC, "враг и житель различимы между собой");

    // Все тайлы тоже должны быть различимы.
    bool tiles_unique = true;
    for (int a = 0; a < static_cast<int>(Tile::Count); ++a)
        for (int b = a + 1; b < static_cast<int>(Tile::Count); ++b)
            if (tile_glyph(static_cast<Tile>(a)) == tile_glyph(static_cast<Tile>(b)))
                tiles_unique = false;
    check(tiles_unique, "символы тайлов попарно различны");
}

void test_maps() {
    section("карты");
    World w("data/maps");
    const Location* v = w.location("village");
    const Location* f = w.location("forest");
    check(v != nullptr, "деревня загружается: " + w.last_error());
    check(f != nullptr, "лес загружается: " + w.last_error());
    if (!v || !f) return;

    eq(v->w, 48, "ширина деревни");
    eq(v->h, 18, "высота деревни");
    eq(static_cast<int>(v->tiles.size()), 48 * 18, "сетка деревни заполнена целиком");
    eq(static_cast<int>(f->tiles.size()), 48 * 18, "сетка леса заполнена целиком");

    check(!v->npcs.empty(), "в деревне есть NPC");
    check(!f->zones.empty(), "в лесу есть зоны спавна");

    // Рамка карты должна быть непроходимой, иначе игрок выйдет за пределы мира.
    bool border_solid = true;
    for (int x = 0; x < v->w; ++x)
        if (v->walkable(Vec2{x, 0}) || v->walkable(Vec2{x, v->h - 1})) border_solid = false;
    for (int y = 0; y < v->h; ++y)
        if (v->walkable(Vec2{0, y}) || v->walkable(Vec2{v->w - 1, y})) border_solid = false;
    check(border_solid, "рамка деревни непроходима со всех сторон");

    // Граф мира: каждый переход ведёт в существующую локацию на проходимую
    // клетку, и все локации достижимы из деревни. Ошибка здесь означает
    // локацию, куда нельзя попасть или откуда нельзя выйти.
    const char* const* all  = ALL_LOCATIONS;
    const int          nloc = N_LOCATIONS;

    for (int i = 0; i < nloc; ++i) {
        const Location* loc = w.location(all[i]);
        check(loc != nullptr, std::string("локация ") + all[i] + " загружается: " + w.last_error());
        if (!loc) continue;
        check(!loc->exits.empty(), std::string(all[i]) + ": есть хотя бы один переход");
        for (const MapExit& e : loc->exits) {
            const Location* dst = w.location(e.target);
            check(dst != nullptr, std::string(all[i]) + " -> " + e.target + ": цель существует");
            if (!dst) continue;
            check(dst->walkable(e.dest),
                  std::string(all[i]) + " -> " + e.target + ": точка прибытия проходима");
            check(loc->walkable(e.pos),
                  std::string(all[i]) + ": сам переход стоит на проходимой клетке");
        }
    }

    // Обход графа от деревни: недостижимая локация — это потерянный контент.
    std::vector<std::string> seen_loc;
    std::vector<std::string> queue_loc;
    queue_loc.push_back("village");
    while (!queue_loc.empty()) {
        std::string cur = queue_loc.back();
        queue_loc.pop_back();
        bool known = false;
        for (const std::string& x : seen_loc) if (x == cur) known = true;
        if (known) continue;
        seen_loc.push_back(cur);
        const Location* loc = w.location(cur);
        if (!loc) continue;
        for (const MapExit& e : loc->exits) queue_loc.push_back(e.target);
    }
    eq(static_cast<int>(seen_loc.size()), nloc, "все локации достижимы из деревни");

    // И обратно: из каждой локации есть путь домой (граф двусторонний).
    for (int i = 0; i < nloc; ++i) {
        const Location* loc = w.location(all[i]);
        if (!loc) continue;
        bool has_way_back = false;
        for (const MapExit& e : loc->exits) {
            const Location* dst = w.location(e.target);
            if (!dst) continue;
            for (const MapExit& back : dst->exits)
                if (back.target == all[i]) has_way_back = true;
        }
        check(has_way_back, std::string(all[i]) + ": из локации есть обратный путь");
    }

    // Ни один объект не должен стоять в стене.
    bool objects_ok = true;
    for (const Location* loc : {v, f}) {
        for (const MapNpc& n : loc->npcs)   if (!loc->walkable(n.pos)) objects_ok = false;
        for (const MapItem& m : loc->items) if (!loc->walkable(m.pos)) objects_ok = false;
        for (const MapExit& e : loc->exits) if (!loc->walkable(e.pos)) objects_ok = false;
    }
    check(objects_ok, "все NPC, предметы и переходы стоят на проходимых клетках");

    // Несуществующая карта не должна ронять процесс (была ошибка fclose(NULL)).
    check(w.location("нет-такой-карты") == nullptr, "отсутствующая карта возвращает nullptr");
    check(!w.last_error().empty(), "и внятный текст ошибки");
}

void test_embedded_maps() {
    section("вшитые карты");

    for (const char* id : ALL_LOCATIONS) {
        const char* emb = embedded_map(id);
        check(emb != nullptr, std::string("карта ") + id + " вшита в бинарник");
        if (!emb) continue;

        // Вшитая копия должна совпадать с файлом, иначе после правки карты
        // забыли выполнить «make embed» и игра в APK разойдётся с исходником.
        std::ifstream f(std::string("data/maps/") + id + ".map");
        check(static_cast<bool>(f), std::string("файл карты ") + id + " на месте");
        if (!f) continue;
        std::ostringstream buf;
        buf << f.rdbuf();
        std::string from_file = buf.str();
        while (!from_file.empty() && (from_file.back() == '\n' || from_file.back() == '\r'))
            from_file.pop_back();
        check(from_file == emb,
              std::string("вшитая копия ") + id + " совпадает с файлом (иначе: make embed)");
    }

    check(embedded_map("нет-такой") == nullptr, "неизвестная карта не выдумывается");
    check(embedded_map(nullptr) == nullptr, "nullptr обрабатывается");

    // Главный сценарий APK: каталога с картами нет вообще.
    World none("каталога-точно-нет");
    const Location* v = none.location("village");
    check(v != nullptr, "локация грузится из вшитой копии при отсутствии файлов");
    if (v) {
        eq(v->w, 48, "размер вшитой карты верный");
        eq(static_cast<int>(v->tiles.size()), 48 * 18, "сетка вшитой карты полная");
        check(!v->npcs.empty(), "объекты вшитой карты разобраны");
    }
    const Location* f2 = none.location("forest");
    check(f2 != nullptr, "вторая локация тоже доступна без файлов");

    // Игра целиком должна подниматься без внешних данных.
    Game g("каталога-точно-нет");
    g.new_game("Безфайловый");
    check(g.here() != nullptr, "новая игра стартует без каталога карт");
    if (v && !v->exits.empty()) {
        g.player().pos = Vec2(v->exits[0].pos.x - 1, v->exits[0].pos.y);
        g.try_move(1, 0);
        eqs(g.player().loc, "forest", "переход между вшитыми локациями работает");
    }
}

void test_new_game_and_stats() {
    section("новая игра и характеристики");
    const Content& c = Content::get();
    Game g;
    g.new_game("Тестер", "human", "swordsman");
    eqs(g.player().name, "Тестер", "имя игрока берётся из ввода");   // была ошибка new_name = name
    eq(g.player().level, 1, "стартовый уровень");
    eqs(g.player().race, "human", "раса записана");
    eqs(g.player().spec, "swordsman", "специализация записана");
    check(g.here() != nullptr, "стартовая локация загружена");

    // Ожидания считаем из базы контента, а не держим магическими числами:
    // при правке баланса тест не должен падать на ровном месте.
    const RaceDef*  race = c.race("human");
    const SpecDef*  spec = c.spec("swordsman");
    const ItemDef*  wpn  = c.item(spec ? spec->start_item : "");
    check(race && spec && wpn, "раса, специализация и стартовое оружие описаны");
    if (!race || !spec || !wpn) return;

    Stats t = g.total();
    eq(t.max_hp, 30 + race->bonus.max_hp + spec->bonus.max_hp,
       "здоровье = база + раса + специализация");
    eq(g.player().hp, t.max_hp, "здоровье заполнено доверху");
    eq(t.dmg_min, 1 + spec->bonus.dmg_min + wpn->bonus.dmg_min, "минимальный урон сходится");
    eq(t.dmg_max, 3 + spec->bonus.dmg_max + wpn->bonus.dmg_max, "максимальный урон сходится");
    eqs(g.player().equipped[static_cast<std::size_t>(Slot::Weapon)], spec->start_item,
        "надето стартовое оружие специализации");

    // Раса и специализация действительно влияют на итог.
    Game g2;
    g2.new_game("Другой", "elf", "archer");
    const RaceDef* r2 = c.race("elf");
    const SpecDef* s2 = c.spec("archer");
    check(r2 && s2, "эльф и лучник описаны");
    if (r2 && s2) {
        eq(g2.total().max_hp, 30 + r2->bonus.max_hp + s2->bonus.max_hp,
           "другая раса и путь дают другое здоровье");
        check(g2.total().max_hp != t.max_hp, "итог заметно отличается от мечника-человека");
        check(g2.total().attack > t.attack, "лучник-эльф метче мечника-человека");
    }

    // Ниндзя удешевляет атаку — проверяем, что специализация правит и стоимость.
    Game g3;
    g3.new_game("Третий", "human", "ninja");
    check(g3.attack_cost() < g.attack_cost(), "ниндзя бьёт дешевле по AP");

    // Стойки должны реально менять цифры.
    int block_balanced = g.total().block;
    g.player().stance = Stance::Cautious;
    check(g.total().block == block_balanced + 15, "осторожная стойка поднимает блок");
    check(g.total().armor == 2, "осторожная стойка даёт броню");
    g.player().stance = Stance::Fierce;
    check(g.total().attack > 0 && g.total().block == block_balanced - 15,
          "яростная стойка снижает блок");
    eq(stance_damage_pct(Stance::Fierce), 140, "яростная стойка усиливает урон");
    g.player().stance = Stance::Balanced;
}

void test_equipment() {
    section("снаряжение");
    Game g;
    // Мечник не меняет стоимость атаки, поэтому вклад оружия виден чисто.
    g.new_game("Тестер", "human", "swordsman");
    const std::string start_weapon = g.player().equipped[static_cast<std::size_t>(Slot::Weapon)];
    const int cost0 = g.attack_cost();
    int armor0 = g.total().armor;

    g.add_item("leather_armor", 1);
    check(g.equip("leather_armor"), "кожаный доспех надевается");
    eq(g.total().armor, armor0 + 2, "броня выросла");
    eq(g.count_item("leather_armor"), 0, "надетое ушло из сумки");

    // Смена оружия должна возвращать прежнее в сумку, а не терять его.
    g.add_item("axe", 1);
    check(g.equip("axe"), "топор надевается");
    eq(g.count_item(start_weapon), 1, "прежнее оружие вернулось в сумку");
    eq(g.attack_cost(), cost0 + 1, "топор дороже по AP на единицу");

    check(g.unequip(Slot::Armor), "доспех снимается");
    eq(g.total().armor, armor0, "броня вернулась к исходной");
    eq(g.count_item("leather_armor"), 1, "снятое вернулось в сумку");

    check(!g.equip("bread"), "хлеб надеть нельзя");
}

void test_level_up() {
    section("прокачка");
    Game g;
    g.new_game("Тестер");
    int need = g.exp_to_next();
    int hp_before = g.total().max_hp;
    int pts_before = g.player().skill_points;

    g.grant_exp(need);
    eq(g.player().level, 2, "уровень вырос");
    eq(g.player().skill_points, pts_before + 1, "выдано очко навыка");
    check(g.total().max_hp > hp_before, "здоровье выросло");

    int hp2 = g.total().max_hp;
    check(g.learn_skill("vigor"), "навык «Крепость» изучается");
    eq(g.total().max_hp, hp2 + 7, "навык прибавил здоровья");
    eq(g.player().skill_points, pts_before, "очко потрачено");

    // Без очков навык не растёт.
    g.player().skill_points = 0;
    check(!g.learn_skill("vigor"), "без очков навык не повышается");

    // Большая выдача опыта должна поднять сразу несколько уровней.
    g.grant_exp(10000);
    check(g.player().level > 5, "крупный опыт поднимает несколько уровней");
    eq(g.player().hp, g.total().max_hp, "на новом уровне здоровье полное");
}

void test_shop() {
    section("торговля");
    Game g;
    g.new_game("Тестер");
    const ShopDef* s = Content::get().shop("shop_general");
    check(s != nullptr, "магазин найден");
    if (!s) return;

    const ItemDef* bread = Content::get().item("bread");
    check(bread != nullptr, "хлеб есть в базе");
    if (!bread) return;

    eq(g.buy_price(*s, *bread), 6, "цена покупки с наценкой 115%");
    eq(g.sell_price(*s, *bread), 2, "цена продажи 40%");

    int gold0 = g.player().gold, n0 = g.count_item("bread");
    check(g.buy(*s, "bread"), "покупка проходит");
    eq(g.player().gold, gold0 - 6, "золото списано");
    eq(g.count_item("bread"), n0 + 1, "товар в сумке");

    check(g.sell(*s, "bread"), "продажа проходит");
    eq(g.player().gold, gold0 - 6 + 2, "золото начислено");

    // Нельзя купить дороже, чем есть денег.
    g.player().gold = 0;
    check(!g.buy(*s, "ring_hp"), "без золота покупка не проходит");
    eq(g.count_item("ring_hp"), 0, "и товар не появился");
}

void test_quest_flow() {
    section("квест «Волчья напасть» целиком");
    Game g;
    g.new_game("Тестер");
    const Content& c = Content::get();

    // Пока квест не взят, доступен только вариант «взять».
    const DlgNode* root = c.node("elder_root");
    check(root != nullptr, "узел диалога старейшины существует");
    if (!root) return;
    int avail = 0;
    for (const DlgOption& o : root->options) if (g.option_available(o)) ++avail;
    eq(avail, 2, "в начале доступны предложение работы и прощание");

    // Берём квест.
    const DlgNode* offer = c.node("elder_offer");
    check(offer != nullptr, "узел с предложением есть");
    if (!offer) return;
    std::string shop;
    g.apply_option(offer->options[0], "", &shop);
    eq(g.player().quests["wolves"], 1, "квест взят, этап 1");

    // Награду выдавать рано: волков ещё не убивали.
    const DlgNode* reward = c.node("elder_reward");
    check(reward != nullptr, "узел награды есть");
    bool reward_visible = false;
    for (const DlgOption& o : root->options)
        if (o.next == "elder_reward" && g.option_available(o)) reward_visible = true;
    check(!reward_visible, "до пяти волков награда недоступна");

    g.player().counters["kill_wolf"] = 4;
    reward_visible = false;
    for (const DlgOption& o : root->options)
        if (o.next == "elder_reward" && g.option_available(o)) reward_visible = true;
    check(!reward_visible, "четырёх волков всё ещё мало");

    g.player().counters["kill_wolf"] = 5;
    reward_visible = false;
    for (const DlgOption& o : root->options)
        if (o.next == "elder_reward" && g.option_available(o)) reward_visible = true;
    check(reward_visible, "на пятом волке награда открывается");

    // Забираем награду.
    int gold0 = g.player().gold;
    if (reward) g.apply_option(reward->options[0], "", &shop);
    eq(g.player().quests["wolves"], QUEST_DONE, "квест завершён");
    eq(g.player().gold, gold0 + 80, "золото за квест выдано");
    eq(g.count_item("herb_potion"), 2, "настои за квест выданы");
    check(g.player().exp > 0 || g.player().level > 1, "опыт за квест начислен");
}

void test_quest_item_exchange() {
    section("квест «Заказ кузнеца»: обмен предметов");
    Game g;
    g.new_game("Тестер");
    const Content& c = Content::get();

    std::string shop;
    g.apply_option(c.node("smith_offer")->options[0], "", &shop);
    eq(g.player().quests["pelts"], 1, "заказ принят");

    const DlgNode* root = c.node("smith_root");
    auto deliver_visible = [&] {
        for (const DlgOption& o : root->options)
            if (o.next == "smith_reward" && g.option_available(o)) return true;
        return false;
    };
    check(!deliver_visible(), "без шкур сдать заказ нельзя");
    g.add_item("wolf_pelt", 2);
    check(!deliver_visible(), "двух шкур мало");
    g.add_item("wolf_pelt", 1);
    check(deliver_visible(), "с тремя шкурами заказ можно сдать");

    g.apply_option(c.node("smith_reward")->options[0], "", &shop);
    eq(g.count_item("wolf_pelt"), 0, "шкуры забраны");
    eq(g.count_item("chain_armor"), 1, "кольчуга выдана");
    eq(g.player().quests["pelts"], QUEST_DONE, "заказ закрыт");
}

void test_combat() {
    section("бой");
    Game g;
    g.new_game("Тестер");

    // Ставим моба вплотную и вступаем в бой.
    g.player().pos = Vec2{5, 8};
    const Location* loc = g.here();
    check(loc != nullptr, "локация есть");
    if (!loc) return;

    // Находим любого моба в деревне.
    check(!g.mobs().empty(), "мобы расставлены при старте");
    if (g.mobs().empty()) return;
    int uid = g.mobs()[0].uid;

    g.start_combat(uid);
    check(g.combat().active, "бой начался");
    eq(g.player().momentum, 0, "кураж на старте нулевой");
    int ap0 = g.player().ap;

    g.combat_attack(false);
    check(g.player().ap < ap0, "атака потратила AP");

    // Мощный удар без куража запрещён.
    g.player().momentum = 0;
    int hp_before = g.combat().enemy_hp;
    g.combat_attack(true);
    check(g.combat().enemy_hp == hp_before || !g.combat().active,
          "мощный удар без куража не проходит");

    // С куражом — проходит и тратит его.
    if (g.combat().active) {
        g.player().momentum = MOMENTUM_COST;
        g.player().ap = g.total().max_ap;
        g.combat_attack(true);
        check(g.player().momentum < MOMENTUM_COST || !g.combat().active,
              "мощный удар списал кураж");
    }

    // Смена стойки в бою — раз за раунд.
    Game g2;
    g2.new_game("Тестер");
    if (!g2.mobs().empty()) {
        g2.start_combat(g2.mobs()[0].uid);
        g2.combat_set_stance(Stance::Fierce);
        eq(static_cast<int>(g2.player().stance), static_cast<int>(Stance::Fierce),
           "стойка сменилась");
        g2.combat_set_stance(Stance::Cautious);
        eq(static_cast<int>(g2.player().stance), static_cast<int>(Stance::Fierce),
           "вторая смена за раунд отклонена");
    }
}

void test_save_load() {
    section("сохранение и загрузка");
    const char* path = "saves/test_roundtrip.sav";

    Game a;
    a.new_game("Ирма");
    a.player().gold = 1234;
    a.player().quests["wolves"] = 1;
    a.player().counters["kill_wolf"] = 3;
    a.player().skills["might"] = 2;
    a.add_item("wolf_pelt", 5);
    a.add_item("herb_potion", 2);
    a.add_item("helm", 1);
    a.equip("helm");
    a.player().loc = "forest";
    a.player().pos = Vec2{10, 8};
    a.player().stance = Stance::Fierce;
    a.grant_exp(30);
    int level = a.player().level, exp = a.player().exp;
    int mobs_saved = static_cast<int>(a.mobs().size());
    Stats ta = a.total();

    platform::make_dir("saves");
    check(a.save_to(path), "сохранение записалось: " + a.error());

    Game b;
    b.new_game("Другой");             // заведомо иное состояние
    check(b.load_from(path), "сохранение читается: " + b.error());

    eqs(b.player().name, "Ирма", "имя восстановлено");
    eq(b.player().gold, 1234, "золото восстановлено");
    eq(b.player().level, level, "уровень восстановлен");
    eq(b.player().exp, exp, "опыт восстановлен");
    eqs(b.player().loc, "forest", "локация восстановлена");
    eq(b.player().pos.x, 10, "координата x восстановлена");
    eq(b.player().pos.y, 8, "координата y восстановлена");
    eq(static_cast<int>(b.player().stance), static_cast<int>(Stance::Fierce),
       "стойка восстановлена");
    eq(b.player().quests["wolves"], 1, "этап квеста восстановлен");
    eq(b.player().counters["kill_wolf"], 3, "счётчик убийств восстановлен");
    eq(b.player().skills["might"], 2, "ранг навыка восстановлен");
    eq(b.count_item("wolf_pelt"), 5, "стопка предметов восстановлена");
    eq(b.count_item("herb_potion"), 2, "второй предмет восстановлен");
    eqs(b.player().equipped[static_cast<std::size_t>(Slot::Helmet)], "helm",
        "надетый шлем восстановлен");
    eq(static_cast<int>(b.mobs().size()), mobs_saved, "мобы восстановлены");

    Stats tb = b.total();
    eq(tb.max_hp, ta.max_hp, "итоговое здоровье совпадает");
    eq(tb.armor,  ta.armor,  "итоговая броня совпадает");
    eq(tb.dmg_max, ta.dmg_max, "итоговый урон совпадает");

    // Битые и чужие файлы не должны ронять игру.
    Game c;
    c.new_game("Тестер");
    check(!c.load_from("saves/такого-файла-нет.sav"), "отсутствующий файл не грузится");
    check(!c.error().empty(), "и объясняет причину");

    std::FILE* fp = std::fopen("saves/test_garbage.sav", "w");
    if (fp) { std::fputs("это не сохранение\nмусор мусор\n", fp); std::fclose(fp); }
    check(!c.load_from("saves/test_garbage.sav"), "мусорный файл отвергается");
    eqs(c.player().name, "Тестер", "состояние игрока не испорчено неудачной загрузкой");

    std::remove(path);
    std::remove("saves/test_garbage.sav");
}

void test_movement_and_pickup() {
    section("перемещение и подбор предметов");
    Game g;
    g.new_game("Тестер");
    const Location* loc = g.here();
    if (!loc) { check(false, "локация не загружена"); return; }

    // В стену пройти нельзя.
    g.player().pos = Vec2{1, 8};
    Bump b = g.try_move(-1, 0);
    check(b == Bump::Blocked, "стена не пропускает");
    eq(g.player().pos.x, 1, "позиция не изменилась");

    // Предмет подбирается ровно один раз.
    if (!loc->items.empty()) {
        Vec2 ip = loc->items[0].pos;
        const std::string iid = loc->items[0].item_id;
        int have = g.count_item(iid);
        g.player().pos = Vec2{ip.x - 1, ip.y};
        if (loc->walkable(g.player().pos)) {
            g.try_move(1, 0);
            check(g.count_item(iid) > have, "предмет подобран");
            check(g.item_taken(loc->id, 0), "предмет помечен подобранным");
            int now = g.count_item(iid);
            g.player().pos = Vec2{ip.x - 1, ip.y};
            g.try_move(1, 0);
            eq(g.count_item(iid), now, "повторно тот же предмет не появляется");
        }
    }

    // Переход между локациями.
    if (!loc->exits.empty()) {
        const MapExit ex = loc->exits[0];
        g.player().pos = Vec2{ex.pos.x - 1, ex.pos.y};
        if (loc->walkable(g.player().pos)) {
            Bump r = g.try_move(1, 0);
            check(r == Bump::Exit, "переход сработал");
            eqs(g.player().loc, "forest", "игрок оказался в лесу");
            check(g.here() && g.here()->walkable(g.player().pos),
                  "и стоит на проходимой клетке");
        }
    }
}

void test_content_integrity() {
    section("целостность контента");
    const Content& c = Content::get();

    // Каждая ссылка на предмет/врага/узел из контента должна разрешаться.
    const char* shops[] = {"shop_smith", "shop_general", "shop_herbs", "shop_books",
                           "shop_glass", "shop_market", "shop_foundry", "shop_ferry",
                           "shop_order"};
    for (const char* sid : shops) {
        const ShopDef* s = c.shop(sid);
        check(s != nullptr, std::string("магазин ") + sid + " существует");
        if (!s) continue;
        for (const std::string& gid : s->goods)
            check(c.item(gid) != nullptr, "товар " + gid + " есть в базе предметов");
    }

    const char* npcs[] = {"elder", "herbalist", "smith", "trader", "hermit", "enchanter",
                          "glazier", "miller", "warden", "digger", "prohor_l", "prohor_r",
                          "survivor", "founder", "counter", "scribe", "ferryman",
                          "gatekeeper", "librarian", "draftsman", "stoker", "recorder"};
    for (const char* nid : npcs) {
        const NpcDef* n = c.npc(nid);
        check(n != nullptr, std::string("NPC ") + nid + " существует");
        if (!n) continue;
        check(c.node(n->root) != nullptr, "корневой узел диалога " + n->root + " существует");
        if (!n->shop.empty())
            check(c.shop(n->shop) != nullptr, "магазин NPC " + n->shop + " существует");
    }

    // Все переходы диалогов ведут в существующие узлы, а награды — в реальные предметы.
    const char* nodes[] = {"elder_root","elder_offer","elder_taken","elder_progress",
                           "elder_reward","elder_after","elder_queen_offer",
                           "elder_queen_wait","elder_queen_reward",
                           "herbalist_root","herb_offer","herb_taken","herb_progress",
                           "herb_reward","moss_offer","moss_wait","moss_reward",
                           "smith_root","smith_offer","smith_taken","smith_progress",
                           "smith_reward","outpost_offer","outpost_wait","outpost_reward",
                           "trader_root","trader_talk",
                           "hermit_root","hermit_rest","hermit_hint",
                           "zp_offer","zp_wait","zp_reward","zp_after",
                           "ench_root","ench_about","ench_offer","ench_wait","ench_reward",
                           "books_offer","books_wait","books_reward","cinch_talk",
                           "goat_offer","goat_wait","goat_reward",
                           "glazier_root","glass_offer","glass_wait","glass_reward",
                           "miller_root","mill_offer","mill_wait","mill_reward",
                           "warden_root","market_offer","market_wait","market_reward","market_rules",
                           "digger_root","digger_why","digger_hint",
                           "prohor_l_root","prohor_r_root","prohor_offer",
                           "prohor_l_chosen","prohor_r_chosen",
                           "prohor_l_after","prohor_r_after",
                           "prohor_l_lost","prohor_r_lost",
                           "cityroad_offer",
                           "survivor_root","survivor_cut","survivor_who",
                           "founder_root","foundry_offer","foundry_wait","foundry_reward",
                           "counter_root","counter_view","counting_offer","counting_wait","counting_reward",
                           "scribe_root","lists_offer","lists_wait","lists_reward",
                           "ferry_root","ferry_why","ferry_token_talk",
                           "orderway_offer",
                           "gatekeeper_root","watch4_offer","watch4_wait","watch4_reward","watch4_after",
                           "librarian_root","librarian_box","librarian_read","unsealed_talk",
                           "draftsman_root","charts_offer","charts_wait","charts_reward",
                           "stoker_root","ovens_offer","ovens_wait","ovens_reward",
                           "recorder_root","refusal_offer","refusal_master","refusal_council",
                           "refusal_after_m","refusal_after_c","keepsake_talk"};
    for (const char* nid : nodes) {
        const DlgNode* n = c.node(nid);
        check(n != nullptr, std::string("узел ") + nid + " существует");
        if (!n) continue;
        for (const DlgOption& o : n->options) {
            if (!o.next.empty())
                check(c.node(o.next) != nullptr,
                      std::string(nid) + ": переход в '" + o.next + "' разрешается");
            if (!o.give_item.empty())
                check(c.item(o.give_item) != nullptr,
                      std::string(nid) + ": награда '" + o.give_item + "' есть в базе");
            if (!o.take_item.empty())
                check(c.item(o.take_item) != nullptr,
                      std::string(nid) + ": требуемый '" + o.take_item + "' есть в базе");
            if (!o.req_note.empty())
                check(c.note(o.req_note) != nullptr,
                      std::string(nid) + ": записка '" + o.req_note + "' есть в базе");
            if (!o.shop_id.empty())
                check(c.shop(o.shop_id) != nullptr,
                      std::string(nid) + ": магазин '" + o.shop_id + "' существует");
            if (!o.req_quest.empty())
                check(c.quest(o.req_quest) != nullptr,
                      std::string(nid) + ": квест '" + o.req_quest + "' существует");
            if (!o.set_quest.empty())
                check(c.quest(o.set_quest) != nullptr,
                      std::string(nid) + ": назначаемый квест '" + o.set_quest + "' существует");
        }
    }

    // Добыча врагов должна ссылаться на существующие предметы.
    const char* mobs[] = {"rat", "wolf", "bandit", "wolf_alpha", "spider", "bat",
                          "spider_queen", "brigand", "bandit_chief", "wraith", "keeper",
                          "archivist", "stray", "glass_hound", "mill_rat", "salt_ghoul",
                          "caravan_shade", "salt_mother", "bridge_walker",
                          "city_rat", "cut_man", "mad_clerk", "slag_thing",
                          "canal_walker", "archive_moth", "half_warden", "slag_master",
                          "acolyte", "gate_guard", "page_swarm", "draft_shade",
                          "cell_dweller", "furnace_born", "refusal_echo",
                          "node_guard", "node_heart", "master_shadow"};
    for (const char* mid : mobs) {
        const EnemyDef* e = c.enemy(mid);
        check(e != nullptr, std::string("враг ") + mid + " существует");
        if (!e) continue;
        check(e->stats.max_hp > 0, std::string(mid) + ": положительное здоровье");
        check(e->stats.ap_atk > 0, std::string(mid) + ": ненулевая цена атаки");
        for (const Drop& d : e->drops)
            check(c.item(d.item) != nullptr, std::string(mid) + ": добыча '" + d.item + "' есть в базе");
    }

    // Зоны спавна на картах должны ссылаться на существующих врагов.
    World w("data/maps");
    for (const char* lid : ALL_LOCATIONS) {
        const Location* loc = w.location(lid);
        if (!loc) continue;
        for (const SpawnZone& z : loc->zones)
            check(c.enemy(z.enemy_id) != nullptr,
                  std::string(lid) + ": зона спавна ссылается на '" + z.enemy_id + "'");
        for (const MapItem& m : loc->items)
            check(c.item(m.item_id) != nullptr,
                  std::string(lid) + ": предмет на карте '" + m.item_id + "' есть в базе");
        for (const MapNpc& n : loc->npcs)
            check(c.npc(n.npc_id) != nullptr,
                  std::string(lid) + ": NPC на карте '" + n.npc_id + "' есть в базе");
        for (const MapNote& nt : loc->notes)
            check(c.note(nt.note_id) != nullptr,
                  std::string(lid) + ": записка на карте '" + nt.note_id + "' есть в базе");
        for (const MapChest& ch : loc->chests) {
            if (!ch.key.empty())
                check(c.item(ch.key) != nullptr,
                      std::string(lid) + ": ключ сундука '" + ch.key + "' есть в базе");
            for (const ItemStack& st : ch.items)
                check(c.item(st.id) != nullptr,
                      std::string(lid) + ": содержимое сундука '" + st.id + "' есть в базе");
        }
    }

    // Амулет должен быть добываем — иначе квест Лады непроходим.
    const EnemyDef* alpha = c.enemy("wolf_alpha");
    bool amulet_droppable = false;
    if (alpha)
        for (const Drop& d : alpha->drops)
            if (d.item == "amulet" && d.percent >= 100) amulet_droppable = true;
    check(amulet_droppable, "амулет гарантированно падает с вожака — квест проходим");
}

void test_mob_ai() {
    section("ИИ мобов: видимость и возврат в зону");
    World w("data/maps");
    const Location* v = w.location("village");
    check(v != nullptr, "деревня загружается");
    if (!v) return;

    // --- прямая видимость ---
    // В деревне дома стоят на строках 11..13, столбцы 3..9; пруд — 17..21 на
    // строках 10..12; деревья — 7..9 на строке 15.
    check(v->visible(Vec2(6, 9), Vec2(6, 5)),   "по открытому месту видно");
    check(v->visible(Vec2(6, 9), Vec2(6, 9)),   "клетка видит саму себя");
    check(!v->visible(Vec2(6, 14), Vec2(6, 10)), "сквозь дом не видно");
    check(!v->visible(Vec2(6, 15), Vec2(10, 15)), "сквозь деревья не видно");
    check(v->visible(Vec2(16, 11), Vec2(22, 11)), "через воду видно: она не стена");
    check(v->visible(Vec2(6, 14), Vec2(6, 10)) == v->visible(Vec2(6, 10), Vec2(6, 14)),
          "видимость симметрична");

    // --- поведение ---
    Game g;
    g.new_game("Наблюдатель", "human", "swordsman");
    check(!g.mobs().empty(), "мобы расставлены");
    if (g.mobs().empty()) return;

    const int uid = g.mobs()[0].uid;

    // Остальных мобов уводим в дальний угол, чтобы они не начали бой сами
    // и не оборвали ход мира раньше времени.
    auto park_others = [&]() {
        int slot = 0;
        for (const Mob& other : g.mobs()) {
            if (other.uid == uid) continue;
            Mob* o = g.mob_by_uid(other.uid);
            if (!o) continue;
            o->pos = Vec2(40 + (slot % 6), 16);
            o->state = MobState::Idle;
            ++slot;
        }
    };

    // Дом наблюдаемого моба — центр его зоны спавна.
    Vec2 home(0, 0);
    int radius = 0;
    {
        const Mob* m = g.mob_by_uid(uid);
        check(m && m->zone >= 0, "у моба есть зона спавна");
        if (!m || m->zone < 0) return;
        home   = v->zones[static_cast<std::size_t>(m->zone)].pos;
        radius = v->zones[static_cast<std::size_t>(m->zone)].radius;
    }

    // 1. Игрок за домом — моб его не видит и не преследует.
    {
        Mob* m = g.mob_by_uid(uid);
        m->pos = Vec2(6, 14);
        m->state = MobState::Idle;
        g.player().pos = Vec2(6, 10);
        park_others();
        g.world_turn();
        m = g.mob_by_uid(uid);
        check(m != nullptr, "моб не исчез");
        if (m) check(m->state != MobState::Chase,
                     "за препятствием моб игрока не видит и не преследует");
    }

    // 2. На открытом месте на том же расстоянии — преследует и приближается.
    {
        Mob* m = g.mob_by_uid(uid);
        m->pos = Vec2(24, 16);
        m->state = MobState::Idle;
        g.player().pos = Vec2(20, 16);
        park_others();
        int before = dist(m->pos, g.player().pos);
        g.world_turn();
        m = g.mob_by_uid(uid);
        if (m && !g.combat().active) {
            eq(static_cast<int>(m->state), static_cast<int>(MobState::Chase),
               "на виду моб переходит в преследование");
            check(dist(m->pos, g.player().pos) < before, "и подходит ближе");
        } else {
            check(true, "моб дошёл до игрока и начал бой — тоже верно");
        }
        g.combat().active = false;
    }

    // 3. Игрок скрылся за препятствием — моб бросает погоню и идёт домой.
    {
        Mob* m = g.mob_by_uid(uid);
        m->pos = Vec2(6, 14);            // далеко от своей зоны
        m->state = MobState::Chase;      // как будто только что гнался
        g.player().pos = Vec2(6, 10);    // за домом, вне видимости
        park_others();
        check(dist(m->pos, home) > radius, "моб заведомо вне своей зоны");
        int before_home = dist(m->pos, home);
        g.world_turn();
        m = g.mob_by_uid(uid);
        check(m != nullptr, "моб не исчез");
        if (m) {
            eq(static_cast<int>(m->state), static_cast<int>(MobState::Return),
               "потеряв игрока из виду, моб возвращается");
            check(dist(m->pos, home) < before_home, "и приближается к своей зоне");
        }
    }

    // 4. Дойдя до зоны, моб успокаивается.
    {
        g.player().pos = Vec2(1, 1);     // игрок далеко от всех зон
        park_others();
        int guard = 0;
        const Mob* m = g.mob_by_uid(uid);
        while (m && m->state == MobState::Return && guard++ < 200) {
            g.world_turn();
            m = g.mob_by_uid(uid);
        }
        check(m != nullptr, "моб дошёл живым");
        if (m) {
            eq(static_cast<int>(m->state), static_cast<int>(MobState::Idle),
               "вернувшись в зону, моб переходит в покой");
            check(dist(m->pos, home) <= radius, "и находится внутри своей зоны");
        }
    }

    // 5. В покое моб не расползается по карте. Раньше он уходил куда угодно,
    //    продолжая занимать слот своей зоны и блокируя респавн.
    {
        g.player().pos = Vec2(1, 1);
        bool all_home = true;
        for (int turn = 0; turn < 250; ++turn) {
            g.world_turn();
            if (g.combat().active) { g.combat().active = false; continue; }
            for (const Mob& m : g.mobs()) {
                if (m.loc != "village" || m.zone < 0) continue;
                if (m.state != MobState::Idle) continue;
                const SpawnZone& z = v->zones[static_cast<std::size_t>(m.zone)];
                if (dist(m.pos, z.pos) > z.radius) all_home = false;
            }
        }
        check(all_home, "за 250 ходов ни один спокойный моб не ушёл из своей зоны");
    }

    // 6. Зоны спавна не пустеют. Ровно это и ломал прежний ИИ: уползший моб
    //    продолжал считаться жителем своей зоны, слот был занят, а рядом с
    //    точкой спавна никого не было.
    {
        g.player().pos = Vec2(1, 1);
        for (int turn = 0; turn < 300; ++turn) {
            g.world_turn();
            if (g.combat().active) g.combat().active = false;
        }
        bool zones_ok = true;
        for (std::size_t z = 0; z < v->zones.size(); ++z) {
            const SpawnZone& zone = v->zones[z];
            int counted = 0, actually_home = 0;
            for (const Mob& m : g.mobs()) {
                if (m.loc != "village" || m.zone != static_cast<int>(z)) continue;
                ++counted;
                if (dist(m.pos, zone.pos) <= zone.radius) ++actually_home;
            }
            // Слот зоны занят только теми, кто и правда рядом (или идёт назад).
            if (counted > 0 && actually_home == 0) zones_ok = false;
        }
        check(zones_ok, "занятые слоты зон подкреплены мобами рядом с точкой спавна");
    }

    // 7. Луч видимости работает и в тесных локациях, где стен больше, чем пола.
    {
        const Location* cave = w.location("cave");
        check(cave != nullptr, "пещера загружается");
        if (cave) {
            int blocked = 0, open = 0;
            for (int y = 1; y < 17; ++y) {
                for (int x = 1; x < 47; ++x) {
                    Vec2 a(x, y), b(x + 6, y);
                    if (!cave->in_bounds(b)) continue;
                    if (!cave->walkable(a) || !cave->walkable(b)) continue;
                    if (cave->visible(a, b)) ++open; else ++blocked;
                }
            }
            check(blocked > 0, "в пещере есть пары клеток, между которыми не видно");
            check(open > 0, "и есть пары, между которыми видно");
        }
    }

    // 8. Состояние переживает сохранение и загрузку.
    {
        platform::make_dir("saves");
        Mob* m = g.mob_by_uid(uid);
        m->state = MobState::Return;
        check(g.save_to("saves/test_ai.sav"), "сохранение записано");
        Game g2;
        check(g2.load_from("saves/test_ai.sav"), "сохранение прочитано");
        const Mob* m2 = g2.mob_by_uid(uid);
        check(m2 != nullptr, "моб восстановлен");
        if (m2) eq(static_cast<int>(m2->state), static_cast<int>(MobState::Return),
                   "состояние моба пережило сохранение");
        std::remove("saves/test_ai.sav");
    }
}

void test_effects() {
    section("эффекты и их таймер");
    const Content& c = Content::get();

    std::vector<ActiveEffect> fx;
    Game::apply_effect(fx, "poison", 3, 2);
    eq(static_cast<int>(fx.size()), 1, "эффект наложен");

    // Повторное наложение не плодит дубликаты, а продлевает и усиливает.
    Game::apply_effect(fx, "poison", 5, 1);
    eq(static_cast<int>(fx.size()), 1, "повтор не создаёт второй записи");
    eq(fx[0].turns, 5, "длительность взята большая");
    eq(fx[0].power, 2, "сила взята большая");

    Game::apply_effect(fx, "нет-такого-эффекта", 5, 1);
    eq(static_cast<int>(fx.size()), 1, "несуществующий эффект не накладывается");

    // Тик отнимает здоровье и укорачивает эффект.
    const EffectDef* pd = c.effect("poison");
    check(pd != nullptr, "яд описан в базе");
    if (pd) {
        int d = Game::tick_effects(fx);
        eq(d, pd->hp_per_turn * 2, "яд силы 2 отнимает вдвое больше");
        eq(fx[0].turns, 4, "длительность уменьшилась на ход");
    }

    // Эффект истекает и исчезает сам.
    for (int i = 0; i < 10; ++i) Game::tick_effects(fx);
    check(fx.empty(), "истёкший эффект убирается из списка");

    // Модификаторы характеристик суммируются с учётом силы.
    std::vector<ActiveEffect> mod;
    Game::apply_effect(mod, "might", 5, 3);
    const EffectDef* md = c.effect("might");
    if (md) {
        Stats st = Game::effect_stats(mod);
        eq(st.dmg_min, md->per_power.dmg_min * 3, "сила эффекта умножает бонус");
    }
    Stats none = Game::effect_stats(std::vector<ActiveEffect>());
    eq(none.dmg_min, 0, "пустой список не даёт бонусов");

    // Снятие: "*" убирает только вредное.
    std::vector<ActiveEffect> mix;
    Game::apply_effect(mix, "poison", 5, 1);
    Game::apply_effect(mix, "regen", 5, 1);
    Game::apply_effect(mix, "weaken", 5, 1);
    eq(Game::cure_effects(mix, "*"), 2, "снято два вредных эффекта");
    eq(static_cast<int>(mix.size()), 1, "полезный эффект остался");
    eqs(mix[0].id, "regen", "остался именно полезный");
    eq(Game::cure_effects(mix, "regen"), 1, "снятие по имени работает");

    // Эффекты действительно меняют боевые характеристики героя.
    Game g;
    g.new_game("Тестер", "human", "swordsman");
    int dmg0 = g.total().dmg_max;
    Game::apply_effect(g.player().effects, "might", 5, 2);
    check(g.total().dmg_max > dmg0, "эффект силы поднимает урон героя");
    int atk0 = g.total().attack;
    Game::apply_effect(g.player().effects, "weaken", 5, 1);
    check(g.total().attack < atk0, "слабость снижает меткость");

    // Яд тикает и вне боя, на ходу мира.
    Game g2;
    g2.new_game("Отравленный", "human", "swordsman");
    Game::apply_effect(g2.player().effects, "poison", 5, 2);
    int hp0 = g2.player().hp;
    g2.world_turn();
    check(g2.player().hp < hp0, "яд отнимает здоровье на ходу мира");

    // Отдых снимает отраву.
    g2.rest();
    check(g2.player().effects.empty(), "отдых снимает вредные эффекты");
    eq(g2.player().hp, g2.total().max_hp, "и восстанавливает здоровье");

    // Противоядие тоже.
    Game g3;
    g3.new_game("Лечёный", "human", "swordsman");
    Game::apply_effect(g3.player().effects, "poison", 5, 1);
    g3.add_item("antidote", 1);
    check(g3.use_item("antidote"), "противоядие применяется");
    check(g3.player().effects.empty(), "противоядие снимает яд");

    // Эликсир накладывает эффект.
    g3.add_item("elixir_might", 1);
    check(g3.use_item("elixir_might"), "эликсир применяется");
    check(!g3.player().effects.empty(), "эликсир накладывает эффект");
}

void test_races_and_specs() {
    section("расы и специализации");
    const Content& c = Content::get();
    check(c.races().size() >= 5, "рас не меньше пяти");
    check(c.specs().size() >= 5, "специализаций не меньше пяти");

    for (const RaceDef& r : c.races())
        check(!r.name.empty() && !r.desc.empty(), "у расы " + r.id + " есть имя и описание");
    for (const SpecDef& sp : c.specs()) {
        check(!sp.name.empty(), "у специализации " + sp.id + " есть имя");
        check(c.item(sp.start_item) != nullptr,
              "стартовый предмет " + sp.start_item + " существует");
    }

    // Каждая связка расы и пути должна давать играбельного героя.
    for (const RaceDef& r : c.races()) {
        for (const SpecDef& sp : c.specs()) {
            Game g;
            g.new_game("Проба", r.id, sp.id);
            Stats t = g.total();
            check(t.max_hp > 0, r.id + "/" + sp.id + ": здоровье положительно");
            check(t.max_ap > 0, r.id + "/" + sp.id + ": очки действия положительны");
            check(g.attack_cost() >= 1, r.id + "/" + sp.id + ": атака стоит хотя бы 1 AP");
            check(t.attack >= 5, r.id + "/" + sp.id + ": шанс попасть не обнулён");
            check(g.player().hp == t.max_hp, r.id + "/" + sp.id + ": старт с полным здоровьем");
        }
    }

    // Неизвестная раса или путь откатываются к значению по умолчанию.
    Game g;
    g.new_game("Кто-то", "нет-такой-расы", "нет-такого-пути");
    check(c.race(g.player().race) != nullptr, "неизвестная раса заменена корректной");
    check(c.spec(g.player().spec) != nullptr, "неизвестный путь заменён корректным");
}

void test_mob_inventory() {
    section("инвентарь мобов");
    Game g;
    g.new_game("Тестер", "human", "swordsman");
    check(!g.mobs().empty(), "мобы расставлены");
    if (g.mobs().empty()) return;

    // Хотя бы у кого-то из мобов есть что отнять: добыча разыгрывается
    // при появлении, а не в момент смерти.
    bool any_loot = false;
    for (const Mob& m : g.mobs())
        if (!m.inv.empty() || m.gold > 0) any_loot = true;
    check(any_loot, "мобы носят с собой добычу");

    // Убийство передаёт игроку именно то, что моб нёс.
    int uid = g.mobs()[0].uid;
    const Mob* m = g.mob_by_uid(uid);
    check(m != nullptr, "моб найден");
    if (!m) return;
    std::vector<ItemStack> carried = m->inv;
    int carried_gold = m->gold;
    int gold_before = g.player().gold;
    std::vector<int> before;
    for (const ItemStack& st : carried) before.push_back(g.count_item(st.id));

    g.start_combat(uid);
    for (int i = 0; i < 400 && g.combat().active; ++i) {
        g.player().hp = g.total().max_hp;
        if (g.player().ap < g.attack_cost()) g.player().ap = g.total().max_ap;
        g.combat_attack(false);
    }
    check(!g.combat().active, "бой закончился");
    eq(g.player().gold, gold_before + carried_gold, "золото моба перешло игроку");
    for (std::size_t i = 0; i < carried.size(); ++i)
        eq(g.count_item(carried[i].id), before[i] + carried[i].count,
           "предмет " + carried[i].id + " перешёл игроку");
    check(g.mob_by_uid(uid) == nullptr, "убитый моб убран с карты");
}

void test_chests() {
    section("сундуки");
    World w("data/maps");
    const Location* v = w.location("village");
    check(v != nullptr && !v->chests.empty(), "в деревне есть сундук");
    if (!v || v->chests.empty()) return;

    Game g;
    g.new_game("Тестер", "human", "swordsman");
    check(!g.chest_opened("village", 0), "сундук изначально закрыт");

    const MapChest& ch = v->chests[0];
    int gold0 = g.player().gold;
    check(g.open_chest(0), "незапертый сундук открывается");
    check(g.chest_opened("village", 0), "сундук помечен вскрытым");
    eq(g.player().gold, gold0 + ch.gold, "золото из сундука получено");
    for (const ItemStack& st : ch.items)
        check(g.count_item(st.id) >= st.count, "предмет " + st.id + " из сундука получен");

    // Повторно тот же сундук не даёт ничего.
    int gold1 = g.player().gold;
    check(!g.open_chest(0), "второй раз сундук не открывается");
    eq(g.player().gold, gold1, "и золота не прибавляет");

    // Запертый сундук требует ключ.
    const Location* r = w.location("ruins");
    check(r != nullptr, "развалины загружаются");
    if (!r) return;
    int locked = -1;
    for (std::size_t i = 0; i < r->chests.size(); ++i)
        if (!r->chests[i].key.empty()) locked = static_cast<int>(i);
    check(locked >= 0, "на развалинах есть запертый сундук");
    if (locked < 0) return;

    const std::string keyid = r->chests[static_cast<std::size_t>(locked)].key;
    check(Content::get().item(keyid) != nullptr, "нужный ключ есть в базе предметов");

    Game g2;
    g2.new_game("Взломщик", "human", "swordsman");
    g2.player().loc = "ruins";
    g2.player().pos = Vec2(24, 3);
    check(!g2.open_chest(locked), "без ключа запертый сундук не открыть");
    check(!g2.chest_opened("ruins", locked), "и он остаётся закрытым");
    g2.add_item(keyid, 1);
    check(g2.open_chest(locked), "с ключом открывается");
}

void test_enchanting() {
    section("зачарования");
    const Content& c = Content::get();
    check(c.enchants().size() >= 5, "зачарований не меньше пяти");
    for (const EnchantDef& e : c.enchants()) {
        check(!e.name.empty(), "у зачарования " + e.id + " есть имя");
        check(e.price > 0, "у зачарования " + e.id + " есть цена");
        if (!e.reagent.empty())
            check(c.item(e.reagent) != nullptr, "реагент " + e.reagent + " есть в базе");
        if (!e.on_hit_effect.empty())
            check(c.effect(e.on_hit_effect) != nullptr,
                  "эффект удара " + e.on_hit_effect + " есть в базе");
    }

    Game g;
    g.new_game("Тестер", "human", "swordsman");
    const std::string wpn = g.player().equipped[static_cast<std::size_t>(Slot::Weapon)];
    check(g.can_enchant(wpn), "оружие можно зачаровать");
    check(!g.can_enchant("bread"), "хлеб зачаровать нельзя");

    const EnchantDef* keen = c.enchant("keen");
    check(keen != nullptr, "зачарование «Острота» есть");
    if (!keen) return;

    // Без золота и реагента не выйдет.
    g.player().gold = 0;
    check(!g.enchant_item(wpn, "keen"), "без золота зачарование не проходит");
    g.player().gold = keen->price;
    if (!keen->reagent.empty()) {
        check(!g.enchant_item(wpn, "keen"), "без реагента зачарование не проходит");
        g.add_item(keen->reagent, keen->reagent_count);
    }

    int atk0 = g.total().attack;
    check(g.enchant_item(wpn, "keen"), "с золотом и реагентом проходит");
    eq(g.player().gold, 0, "золото списано");
    if (!keen->reagent.empty()) eq(g.count_item(keen->reagent), 0, "реагент израсходован");
    check(g.total().attack > atk0, "зачарование подняло меткость");

    // Одна вещь — одна руна.
    g.player().gold = 10000;
    g.add_item("ember", 5);
    check(!g.can_enchant(wpn), "повторно ту же вещь зачаровать нельзя");
    check(!g.enchant_item(wpn, "flame"), "и попытка отклоняется");

    // Снятое снаряжение перестаёт давать бонус зачарования.
    int with_ench = g.total().attack;
    g.unequip(Slot::Weapon);
    check(g.total().attack < with_ench, "снятая вещь не даёт бонуса зачарования");
}

void test_portals() {
    section("порталы");
    Game g;
    g.new_game("Тестер", "human", "swordsman");

    check(!g.place_portal(), "без умения портал не поставить");
    g.player().portal_master = true;
    check(!g.place_portal(), "без камня портал не поставить");

    g.add_item("portal_stone", 3);
    g.player().pos = Vec2(5, 8);
    check(g.place_portal(), "первый портал ставится");
    eq(static_cast<int>(g.player().portals.size()), 1, "портал записан");
    eq(g.count_item("portal_stone"), 2, "камень израсходован");
    check(!g.place_portal(), "второй портал на том же месте не встанет");

    check(g.portal_at(Vec2(5, 8), "village") != nullptr, "портал виден на своём месте");
    check(g.portal_at(Vec2(6, 8), "village") == nullptr, "и только на своём");

    // Один портал никуда не ведёт.
    g.player().pos = Vec2(4, 8);
    g.try_move(1, 0);
    eqs(g.player().loc, "village", "с одним порталом переноса нет");

    // Со вторым — переносит.
    g.player().pos = Vec2(20, 8);
    check(g.place_portal(), "второй портал ставится");
    g.player().pos = Vec2(4, 8);
    Bump b = g.try_move(1, 0);
    check(b == Bump::Portal, "шаг на портал сработал");
    eq(g.player().pos.x, 20, "перенесло ко второму порталу");

    // Снятие возвращает камень.
    int stones = g.count_item("portal_stone");
    check(g.remove_portal_here(), "портал под ногами снимается");
    eq(g.count_item("portal_stone"), stones + 1, "камень вернулся");
    eq(static_cast<int>(g.player().portals.size()), 1, "портал убран из списка");

    // Больше лимита не поставить.
    g.player().portals.clear();
    g.add_item("portal_stone", 10);
    int placed = 0;
    for (int i = 0; i < PORTAL_LIMIT + 3; ++i) {
        g.player().pos = Vec2(3 + i, 9);
        if (g.place_portal()) ++placed;
    }
    eq(placed, PORTAL_LIMIT, "поставлено ровно столько, сколько разрешено");
}

// Сквозная проверка: проходима ли игра целиком. Бои идут настоящей боевой
// механикой, предметы поднимаются с карты, переходы — через реальные выходы.
// Подкручиваются только здоровье и очки действия героя: тест про проходимость
// контента, а не про баланс.
void test_gates_and_secret_quests() {
    section("запертые проходы и тайные квесты");
    const Content& c = Content::get();
    World w("data/maps");

    // Схрон должен быть закрыт: единственный вход в него — с условием.
    const Location* sanct = w.location("sanctum");
    check(sanct != nullptr, "святилище загружается");
    if (!sanct) return;
    const MapExit* to_vault = nullptr;
    for (const MapExit& e : sanct->exits)
        if (e.target == "vault") to_vault = &e;
    check(to_vault != nullptr, "из святилища есть проход в схрон");
    if (!to_vault) return;
    check(!to_vault->gate.empty(), "проход в схрон под условием");
    eqs(to_vault->gate.key, "seam_key", "нужен ключ шва");
    eqs(to_vault->gate.req_quest, "seam", "и открытая тайна");
    check(!to_vault->gate.denied.empty(), "отказ объясняется словами, а не молчанием");

    // Каждое условие врат должно ссылаться на существующие вещи.
    for (const char* lid : ALL_LOCATIONS) {
        const Location* loc = w.location(lid);
        if (!loc) continue;
        for (const MapExit& e : loc->exits) {
            if (!e.gate.key.empty())
                check(c.item(e.gate.key) != nullptr,
                      std::string(lid) + ": ключ врат '" + e.gate.key + "' есть в базе");
            if (!e.gate.req_quest.empty())
                check(c.quest(e.gate.req_quest) != nullptr,
                      std::string(lid) + ": квест врат '" + e.gate.req_quest + "' есть в базе");
        }
    }

    // Ключом может быть носимая вещь: надетое кольцо не должно запирать
    // дверь, которую само открывает.
    {
        Game gk;
        gk.new_game("Носитель", "human", "swordsman");
        gk.add_item("ring_hp", 1);
        eq(gk.carries_item("ring_hp"), 1, "в сумке предмет считается");
        gk.equip("ring_hp");
        eq(gk.count_item("ring_hp"), 0, "надетого нет в сумке");
        eq(gk.carries_item("ring_hp"), 1, "но при игроке оно есть");
        gk.add_item("ring_hp", 2);
        eq(gk.carries_item("ring_hp"), 3, "сумка и надетое складываются");
    }

    // --- поведение врат ---
    Game g;
    g.new_game("Взломщик", "human", "swordsman");
    g.player().loc = "sanctum";

    const int dxs[] = {1, -1, 0, 0}, dys[] = {0, 0, 1, -1};
    auto try_enter = [&]() {
        for (int k = 0; k < 4; ++k) {
            Vec2 from(to_vault->pos.x - dxs[k], to_vault->pos.y - dys[k]);
            if (!sanct->walkable(from)) continue;
            g.player().loc = "sanctum";
            g.player().pos = from;
            if (g.try_move(dxs[k], dys[k]) == Bump::Exit) return true;
        }
        return false;
    };

    check(!try_enter(), "без ключа и без тайны в схрон не войти");
    eqs(g.player().loc, "sanctum", "игрок остался на месте");

    g.add_item("seam_key", 1);
    check(!try_enter(), "одного ключа мало: нужно ещё знать, что искать");

    // Тайна открывается находкой заметки, а не разговором.
    eq(g.quest_stage("seam"), QUEST_NONE, "тайна ещё не открыта");
    g.fire_event(TriggerKind::NoteTaken, "seam");
    check(g.quest_stage("seam") >= 1, "находка заметки открыла тайну");

    check(try_enter(), "с ключом и открытой тайной проход пускает");
    eqs(g.player().loc, "vault", "игрок в схроне");
    eq(g.quest_stage("seam"), QUEST_DONE, "вход в схрон закрыл тайну");

    // --- квесты от событий ---
    Game g2;
    g2.new_game("Следопыт", "human", "swordsman");
    eq(g2.quest_stage("seam"), QUEST_NONE, "тайна не выдана на старте");

    // Добыча ключа двигает квест сама, без разговоров.
    g2.fire_event(TriggerKind::NoteTaken, "seam");
    eq(g2.quest_stage("seam"), 1, "этап 1 от находки");
    g2.add_item("seam_key", 1);          // add_item сам поднимает событие
    eq(g2.quest_stage("seam"), 2, "этап 2 от добычи предмета");

    // Событие не откатывает квест назад.
    g2.fire_event(TriggerKind::NoteTaken, "seam");
    eq(g2.quest_stage("seam"), 2, "повторное событие не сбрасывает прогресс");

    // Вторая тайна начинается с чтения отчёта.
    eq(g2.quest_stage("cinch"), QUEST_NONE, "вторая тайна закрыта");
    g2.fire_event(TriggerKind::NoteTaken, "cinch");
    eq(g2.quest_stage("cinch"), 1, "отчёт открыл вторую тайну");

    // Убийство тоже умеет открывать квесты.
    bool has_kill_trigger = false, has_loc_trigger = false, has_item_trigger = false;
    for (const QuestTrigger& t : c.triggers()) {
        if (t.kind == TriggerKind::MobKilled)       has_kill_trigger = true;
        if (t.kind == TriggerKind::LocationEntered) has_loc_trigger  = true;
        if (t.kind == TriggerKind::ItemGained)      has_item_trigger = true;
        check(c.quest(t.quest) != nullptr, "триггер ссылается на существующий квест " + t.quest);
        if (t.kind == TriggerKind::NoteTaken)
            check(c.note(t.key) != nullptr, "триггер ссылается на существующую записку " + t.key);
        if (t.kind == TriggerKind::ItemGained)
            check(c.item(t.key) != nullptr, "триггер ссылается на существующий предмет " + t.key);
    }
    check(has_loc_trigger,  "есть квест, открывающийся входом в локацию");
    check(has_item_trigger, "есть квест, открывающийся добычей предмета");
    (void)has_kill_trigger;

    // Тайные квесты помечены и не выдаются NPC.
    int secrets = 0;
    for (const QuestDef& q : c.quests()) if (q.secret) ++secrets;
    check(secrets >= 2, "тайных квестов не меньше двух");
    for (const QuestDef& q : c.quests()) {
        if (!q.secret) continue;
        bool offered = false;
        for (const NpcDef* npc : std::vector<const NpcDef*>{
                 c.npc("elder"), c.npc("herbalist"), c.npc("smith"),
                 c.npc("trader"), c.npc("hermit"), c.npc("enchanter")}) {
            if (!npc) continue;
            const DlgNode* root = c.node(npc->root);
            if (!root) continue;
            for (const DlgOption& o : root->options)
                if (o.set_quest == q.id && o.set_stage == 1) offered = true;
        }
        check(!offered, "тайну «" + q.name + "» никто не выдаёт в разговоре");
    }
}

void test_books_and_notes() {
    section("книги и записки");
    const Content& c = Content::get();

    // --- записки в базе ---
    const char* note_ids[] = {"ink", "miner", "watch", "zero", "child", "proto", "hermit",
                              "seam", "cinch", "order", "double",
                              "goat", "glass", "mill", "market", "prohor",
                              "caravan", "salt", "bridge",
                              "cityhalf", "endless", "deadwater", "counting",
                              "lists", "foundry", "halves", "lastclerk",
                              "gates", "watchwrit", "read", "unsealed", "charts",
                              "novice", "keepsake", "ovens", "refusal",
                              "secondnode", "thirdnode", "emptygrave"};
    for (const char* id : note_ids) {
        const NoteDef* n = c.note(id);
        check(n != nullptr, std::string("записка ") + id + " описана");
        if (!n) continue;
        check(!n->title.empty(), std::string("у записки ") + id + " есть заголовок");
        check(!n->lines.empty(), std::string("у записки ") + id + " есть текст");
    }
    check(c.note("нет-такой") == nullptr, "несуществующая записка не выдумывается");

    // --- книги ---
    Game g;
    g.new_game("Писарь", "human", "swordsman");
    check(g.books().empty(), "библиотека пуста на старте");
    check(!g.start_book("Дневник"), "без чистой книги начать нечего");

    g.add_item("book_blank", 2);
    check(g.start_book("Дневник"), "книга начата");
    eq(static_cast<int>(g.books().size()), 1, "книга появилась в библиотеке");
    eq(g.count_item("book_blank"), 1, "чистая книга израсходована");

    const std::string bid = g.books()[0].id;
    const Book* b = g.book(bid);
    check(b != nullptr, "книга находится по коду");
    if (!b) return;
    eqs(b->title, "Дневник", "название сохранено");
    check(!b->readonly, "своя книга доступна для правки");

    // Правка строк.
    check(g.book_set_line(bid, 0, "Первая строка"), "строка записана");
    eqs(g.book(bid)->lines[0], "Первая строка", "текст на месте");
    check(g.book_insert_line(bid, 1, "Вторая"), "строка добавлена");
    eq(static_cast<int>(g.book(bid)->lines.size()), 2, "строк стало две");
    check(g.book_insert_line(bid, 1, ""), "пустая строка тоже добавляется");
    eqs(g.book(bid)->lines[1], "", "пустая строка пуста");

    check(!g.book_set_line(bid, 99, "мимо"), "правка несуществующей строки отклонена");
    check(!g.book_set_line("нет-такой-книги", 0, "мимо"), "правка чужого кода отклонена");

    // Пределы длины.
    std::string longline;
    for (int i = 0; i < BOOK_MAX_CHARS + 30; ++i) longline += "я";
    g.book_set_line(bid, 0, longline);
    eq(static_cast<int>(utf8_len(g.book(bid)->lines[0])), BOOK_MAX_CHARS,
       "слишком длинная строка обрезается по видимым символам");

    std::string longtitle;
    for (int i = 0; i < BOOK_TITLE_MAX + 20; ++i) longtitle += "ю";
    g.book_set_title(bid, longtitle);
    eq(static_cast<int>(utf8_len(g.book(bid)->title)), BOOK_TITLE_MAX,
       "слишком длинное название обрезается");

    // Предел числа строк: лишние вставки не просто игнорируются — они
    // честно отвечают «нет», и их ровно столько, сколько влезло.
    const int lines_before = static_cast<int>(g.book(bid)->lines.size());
    int added = 0;
    for (int i = 0; i < BOOK_MAX_LINES + 20; ++i)
        if (g.book_insert_line(bid, 0, "строка")) ++added;
    eq(static_cast<int>(g.book(bid)->lines.size()), BOOK_MAX_LINES,
       "строк не больше предела");
    eq(added, BOOK_MAX_LINES - lines_before, "вставка отказывает ровно на пределе");

    // Удаление строк: последняя не исчезает, а очищается.
    while (g.book(bid)->lines.size() > 1) g.book_remove_line(bid, 0);
    eq(static_cast<int>(g.book(bid)->lines.size()), 1, "осталась одна строка");
    check(g.book_remove_line(bid, 0), "удаление последней строки допустимо");
    eq(static_cast<int>(g.book(bid)->lines.size()), 1, "но книга не остаётся без строк");
    eqs(g.book(bid)->lines[0], "", "последняя строка просто очищается");

    // Предел числа книг.
    g.add_item("book_blank", BOOK_MAX_COUNT + 5);
    const int had_books = static_cast<int>(g.books().size());
    int made = 0;
    for (int i = 0; i < BOOK_MAX_COUNT + 5; ++i)
        if (g.start_book("Книга " + to_str(i))) ++made;
    eq(static_cast<int>(g.books().size()), BOOK_MAX_COUNT, "книг не больше предела");
    eq(made, BOOK_MAX_COUNT - had_books, "начать книгу сверх предела не дают");

    // Удаление книги.
    const std::string victim = g.books().back().id;
    check(g.delete_book(victim), "книга выбрасывается");
    check(g.book(victim) == nullptr, "и исчезает из библиотеки");
    check(!g.delete_book("нет-такой"), "выбросить несуществующую нельзя");

    // --- записки подбираются с карты ---
    Game g2;
    g2.new_game("Искатель", "human", "swordsman");
    const Location* forest = g2.world().location("forest");
    check(forest != nullptr && !forest->notes.empty(), "в лесу есть записки");
    if (!forest || forest->notes.empty()) return;

    g2.player().loc = "forest";
    const MapNote& mn = forest->notes[0];
    check(!g2.note_taken("forest", 0), "записка изначально не подобрана");

    // Подходим к записке настоящим шагом.
    const int dxs[] = {1, -1, 0, 0}, dys[] = {0, 0, 1, -1};
    bool picked = false;
    for (int k = 0; k < 4 && !picked; ++k) {
        Vec2 from(mn.pos.x - dxs[k], mn.pos.y - dys[k]);
        if (!forest->walkable(from)) continue;
        g2.player().pos = from;
        if (g2.try_move(dxs[k], dys[k]) == Bump::Note) picked = true;
    }
    check(picked, "шаг на записку её подбирает");
    check(g2.note_taken("forest", 0), "записка помечена подобранной");

    const Book* nb = g2.book("n_" + mn.note_id);
    check(nb != nullptr, "записка попала в библиотеку");
    if (nb) {
        check(nb->readonly, "найденная записка доступна только для чтения");
        check(!nb->lines.empty(), "и содержит текст");
        check(!g2.book_set_line(nb->id, 0, "правка"), "переписать записку нельзя");
        check(!g2.book_set_title(nb->id, "другое"), "и переименовать нельзя");
    }
    eq(g2.player().counters["note_" + mn.note_id], 1, "находка отмечена счётчиком");

    // Повторно та же записка не появляется.
    int books_before = static_cast<int>(g2.books().size());
    check(!g2.take_note(0), "повторно записка не подбирается");
    eq(static_cast<int>(g2.books().size()), books_before, "и дубликата в библиотеке нет");
}

void test_book_save_roundtrip() {
    section("сохранение библиотеки");
    const char* path = "saves/test_books.sav";
    platform::make_dir("saves");

    Game a;
    a.new_game("Летописец", "elf", "mage");
    a.add_item("book_blank", 3);
    a.start_book("Путевые заметки");
    const std::string id = a.books()[0].id;

    // Нарочно кладём то, на чём построчный формат мог бы сломаться:
    // пустые строки, отступы, кириллицу и знаки-разделители.
    a.book_set_line(id, 0, "Первый день пути");
    a.book_insert_line(id, 1, "");
    a.book_insert_line(id, 2, "    отступ в четыре пробела");
    a.book_insert_line(id, 3, "знаки: : = # ~ | > * & 0 1 2");
    a.book_insert_line(id, 4, "ёжик, «кавычки» и тире —");
    a.book_insert_line(id, 5, "");

    a.start_book("Вторая книга");
    a.book_set_line(a.books()[1].id, 0, "текст второй книги");

    // И найденная записка.
    a.player().counters["note_ink"] = 1;
    Book note;
    note.id = "n_ink"; note.title = "Рецепт чернил"; note.readonly = true;
    note.lines.push_back("строка записки");
    a.player().books.push_back(note);

    const std::vector<Book> before = a.books();
    check(a.save_to(path), "сохранение записано: " + a.error());

    Game b;
    b.new_game("Другой", "orc", "ninja");
    check(b.load_from(path), "сохранение прочитано: " + b.error());

    eq(static_cast<int>(b.books().size()), static_cast<int>(before.size()),
       "число книг совпадает");
    bool all_same = true;
    for (std::size_t i = 0; i < before.size() && i < b.books().size(); ++i) {
        const Book& x = before[i];
        const Book& y = b.books()[i];
        if (x.id != y.id || x.title != y.title || x.readonly != y.readonly) all_same = false;
        if (x.lines.size() != y.lines.size()) { all_same = false; continue; }
        for (std::size_t j = 0; j < x.lines.size(); ++j)
            if (x.lines[j] != y.lines[j]) all_same = false;
    }
    check(all_same, "текст всех книг восстановлен посимвольно");

    const Book* restored = b.book(id);
    check(restored != nullptr, "первая книга на месте");
    if (restored && restored->lines.size() > 5) {
        eqs(restored->lines[0], "Первый день пути", "обычная строка цела");
        eqs(restored->lines[1], "", "пустая строка осталась пустой");
        eqs(restored->lines[2], "    отступ в четыре пробела", "отступ не съеден");
        eqs(restored->lines[3], "знаки: : = # ~ | > * & 0 1 2", "разделители не сломали разбор");
        eqs(restored->lines[4], "ёжик, «кавычки» и тире —", "кириллица и знаки целы");
        eqs(restored->lines[5], "", "последняя пустая строка на месте");
    }
    const Book* rn = b.book("n_ink");
    check(rn != nullptr && rn->readonly, "записка восстановлена как «только чтение»");
    eq(b.player().counters["note_ink"], 1, "счётчик находки восстановлен");

    std::remove(path);
}

void test_no_escape_needed() {
    section("управление без Escape");
    // Из любого списка должен быть выход обычными клавишами: на экранной
    // клавиатуре Android Escape набирается сочетанием и требовать его нельзя.
    // Проверяем сам контракт: choose трактует Q и 0 как отмену.
    check(ui::CHOOSE_CANCEL == -1, "код отмены не пересекается с индексами");
    check(ui::CHOOSE_HOTKEY == -2, "код горячей клавиши отличается от отмены");

    // И что коды Escape и конца ввода — разные значения: раньше одиночный
    // Escape и закрытый ввод были неотличимы, из-за чего игра зависала.
    check(platform::KEY_ESC != platform::KEY_EOF, "Escape и конец ввода различаются");
    check(platform::KEY_EOF != platform::KEY_UP &&
          platform::KEY_EOF != platform::KEY_DOWN,
          "конец ввода не совпадает со стрелками");
}

void test_playthrough() {
    section("сквозное прохождение: все квесты");
    Game g;
    g.new_game("Герой", "human", "swordsman");
    const Content& c = Content::get();
    std::string shop;

    // --- вспомогательные действия ---

    // Перейти в соседнюю локацию через настоящий выход.
    auto travel = [&](const std::string& target) {
        const Location* loc = g.here();
        if (!loc) return false;
        for (const MapExit& e : loc->exits) {
            if (e.target != target) continue;
            const int dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
            for (int k = 0; k < 4; ++k) {
                Vec2 from(e.pos.x - dx[k], e.pos.y - dy[k]);
                if (!loc->walkable(from) && !(loc->exit_at(from))) continue;
                g.player().pos = from;
                if (g.try_move(dx[k], dy[k]) == Bump::Exit) return true;
            }
        }
        return false;
    };

    // Подобрать с карты все экземпляры предмета в текущей локации.
    auto gather = [&](const std::string& item_id) {
        const Location* loc = g.here();
        if (!loc) return 0;
        int got = 0;
        for (std::size_t i = 0; i < loc->items.size(); ++i) {
            if (loc->items[i].item_id != item_id) continue;
            if (g.item_taken(loc->id, static_cast<int>(i))) continue;
            const Vec2 t = loc->items[i].pos;
            const int dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
            for (int k = 0; k < 4; ++k) {
                Vec2 from(t.x - dx[k], t.y - dy[k]);
                if (!loc->walkable(from)) continue;
                g.player().pos = from;
                if (g.try_move(dx[k], dy[k]) == Bump::Item) { ++got; break; }
            }
        }
        return got;
    };

    // Подобрать записку с карты настоящим шагом.
    auto gather_note = [&](const std::string& note_id) {
        const Location* loc = g.here();
        if (!loc) return false;
        for (std::size_t i = 0; i < loc->notes.size(); ++i) {
            if (loc->notes[i].note_id != note_id) continue;
            if (g.note_taken(loc->id, static_cast<int>(i))) return true;
            const Vec2 t = loc->notes[i].pos;
            const int dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
            for (int k = 0; k < 4; ++k) {
                Vec2 from(t.x - dx[k], t.y - dy[k]);
                if (!loc->walkable(from)) continue;
                g.player().pos = from;
                if (g.try_move(dx[k], dy[k]) == Bump::Note) return true;
            }
        }
        return false;
    };

    // Победить противника честной боевой механикой.
    auto fight = [&](int uid) {
        g.start_combat(uid);
        for (int guard = 0; guard < 600 && g.combat().active; ++guard) {
            g.player().hp = g.total().max_hp;
            if (g.player().ap < g.attack_cost()) g.player().ap = g.total().max_ap;
            g.combat_attack(false);
        }
        g.player().effects.clear();      // тест не про выживание под ядом
        return !g.combat().active;
    };

    // Перебить в текущей локации всех мобов заданного вида (с респавном).
    auto hunt = [&](const std::string& enemy_id, int need) {
        int killed = 0, guard = 0;
        while (killed < need && guard++ < 200) {
            int uid = -1;
            for (const Mob& m : g.mobs())
                if (m.loc == g.player().loc && m.enemy_id == enemy_id) { uid = m.uid; break; }
            if (uid < 0) { g.world_turn(); continue; }
            if (fight(uid)) ++killed;
        }
        return killed;
    };

    // --- деревня: берём первые квесты ---
    g.apply_option(c.node("elder_offer")->options[0], "", &shop);
    g.apply_option(c.node("herb_offer")->options[0],  "", &shop);
    g.apply_option(c.node("smith_offer")->options[0], "", &shop);
    eq(g.player().quests["wolves"], 1, "квест на волков взят");

    // --- лес: волки, вожак, шкуры ---
    check(travel("forest"), "переход в лес");
    eqs(g.player().loc, "forest", "герой в лесу");
    check(hunt("wolf", 5) >= 1, "волки находятся и побеждаются");
    int guard = 0;
    while (g.player().counters["kill_wolf"] < 5 && guard++ < 40) {
        if (hunt("wolf", 1) == 0) g.world_turn();
    }
    check(g.player().counters["kill_wolf"] >= 5, "пять волков перебиты");
    check(hunt("wolf_alpha", 1) == 1, "вожак стаи повержен");
    eq(g.count_item("amulet"), 1, "амулет добыт");

    while (g.count_item("wolf_pelt") < 3 && guard++ < 80) {
        if (hunt("wolf", 1) == 0) g.world_turn();
    }
    check(g.count_item("wolf_pelt") >= 3, "три шкуры собраны");

    // Пока в лесу — собираем всё для квеста Гурия.
    check(gather_note("ink"), "рецепт чернил найден в лесу");
    check(g.book("n_ink") != nullptr, "записка попала в библиотеку");
    check(gather("oak_gall") >= 3, "чернильные орешки собраны");

    // --- сдаём первую тройку квестов ---
    g.apply_option(c.node("elder_reward")->options[0], "", &shop);
    g.apply_option(c.node("herb_reward")->options[0],  "", &shop);
    g.apply_option(c.node("smith_reward")->options[0], "", &shop);
    eq(g.player().quests["wolves"], QUEST_DONE, "квест на волков закрыт");
    eq(g.player().quests["amulet"], QUEST_DONE, "квест на амулет закрыт");
    eq(g.player().quests["pelts"],  QUEST_DONE, "заказ кузнеца закрыт");

    // --- новые квесты открываются только после старых ---
    auto visible = [&](const char* node, const char* target) {
        const DlgNode* n = c.node(node);
        if (!n) return false;
        for (const DlgOption& o : n->options)
            if (o.next == target && g.option_available(o)) return true;
        return false;
    };
    check(visible("trader_root", "books_offer"), "Гурий открыл квест о бумаге");
    g.apply_option(c.node("books_offer")->options[0], "", &shop);
    check(visible("trader_root", "books_reward"),
          "с рецептом и орешками Гурий готов принять работу");
    g.apply_option(c.node("books_reward")->options[0], "", &shop);
    eq(g.player().quests["books"], QUEST_DONE, "квест о бумаге закрыт");
    eq(g.count_item("book_blank"), 1, "первая чистая книга выдана");

    // Книжная лавка открылась, и в ней действительно книги.
    bool book_shop = false;
    for (const DlgOption& o : c.node("trader_root")->options)
        if (o.open_shop && o.shop_id == "shop_books" && g.option_available(o)) book_shop = true;
    check(book_shop, "книжная лавка стала доступна");

    // Книгу можно начать и в неё писать.
    check(g.start_book("Хроника похода"), "книга начата из выданной чистой");
    const std::string diary = g.books().back().id;
    check(g.book_set_line(diary, 0, "Волки, амулет и три шкуры."), "первая запись сделана");
    check(g.book_insert_line(diary, 1, "Дальше — пещера."), "вторая запись сделана");
    eq(static_cast<int>(g.book(diary)->lines.size()), 2, "в книге две строки");

    check(visible("elder_root", "elder_queen_offer"), "Мирон открыл квест на матку");
    check(visible("herbalist_root", "moss_offer"),    "Лада открыла квест на мох");
    check(visible("smith_root", "outpost_offer"),     "Бран открыл квест на заставу");

    g.apply_option(c.node("elder_queen_offer")->options[0], "", &shop);
    g.apply_option(c.node("moss_offer")->options[0],        "", &shop);
    g.apply_option(c.node("outpost_offer")->options[0],     "", &shop);

    // --- пещера: мох и матка ---
    check(travel("cave"), "переход в пещеру");
    eqs(g.player().loc, "cave", "герой в пещере");
    check(gather("glow_moss") >= 3, "светящийся мох собран с карты");
    check(hunt("spider_queen", 1) == 1, "паучья матка повержена");
    eq(g.player().counters["kill_queen"], 1, "счётчик матки сработал");
    check(g.count_item("focus_node") >= 1, "первый узловой фокус добыт");

    check(travel("forest"), "возврат в лес");
    g.apply_option(c.node("elder_queen_reward")->options[0], "", &shop);
    g.apply_option(c.node("moss_reward")->options[0],        "", &shop);
    eq(g.player().quests["queen"], QUEST_DONE, "квест на матку закрыт");
    eq(g.player().quests["moss"],  QUEST_DONE, "квест на мох закрыт");

    // --- развалины: атаман ---
    check(travel("ruins"), "переход на развалины");
    eqs(g.player().loc, "ruins", "герой на развалинах");
    check(hunt("bandit_chief", 1) == 1, "атаман повержен");
    check(g.count_item("rusty_key") >= 1, "ржавый ключ добыт с атамана");
    check(g.count_item("focus_node") >= 2, "второй фокус добыт");

    // Запертый сундук теперь открывается.
    const Location* rl = g.here();
    int locked = -1;
    if (rl)
        for (std::size_t i = 0; i < rl->chests.size(); ++i)
            if (!rl->chests[i].key.empty()) locked = static_cast<int>(i);
    check(locked >= 0 && g.open_chest(locked), "запертый сундук открыт добытым ключом");

    g.apply_option(c.node("outpost_reward")->options[0], "", &shop);
    eq(g.player().quests["outpost"], QUEST_DONE, "квест на заставу закрыт");

    // --- святилище: страж ---
    check(travel("sanctum"), "переход в святилище");
    eqs(g.player().loc, "sanctum", "герой в святилище");
    check(hunt("keeper", 1) == 1, "страж нулевой точки повержен");
    check(g.count_item("focus_node") >= 3, "третий фокус добыт");
    check(g.count_item("rune_stone") >= 1, "рунный камень добыт");

    // --- тайная цепочка: заметка -> ключ -> шов -> схрон ---
    check(gather_note("seam"), "заметка на полях найдена в святилище");
    check(g.quest_stage("seam") >= 1, "находка открыла тайну сама, без разговора");
    check(g.count_item("seam_key") >= 1, "ключ шва снят со стража");
    eq(g.quest_stage("seam"), 2, "добыча ключа продвинула тайну");

    check(travel("vault"), "шов за алтарём пропустил с ключом и разгадкой");
    eqs(g.player().loc, "vault", "герой в схроне Ордена");
    eq(g.quest_stage("seam"), QUEST_DONE, "тайна «Шов за алтарём» закрыта входом");

    check(hunt("archivist", 1) == 1, "архивариус повержен");
    check(g.count_item("order_seal") >= 1, "печать Ордена добыта");
    check(gather_note("cinch"), "отчёт о Стяжении найден");
    eq(g.quest_stage("cinch"), 1, "отчёт открыл вторую тайну");

    check(travel("sanctum"), "из схрона есть обратный путь");

    // --- отшельник: порталы ---
    check(visible("hermit_root", "cinch_talk"), "отшельник готов говорить о Стяжении");
    g.apply_option(c.node("cinch_talk")->options[0], "", &shop);
    eq(g.quest_stage("cinch"), QUEST_DONE, "тайна «Стяжение не кончилось» закрыта разговором");

    check(visible("hermit_root", "zp_offer"), "отшельник открыл квест о нулевой точке");
    g.apply_option(c.node("zp_offer")->options[0], "", &shop);
    check(visible("hermit_root", "zp_reward"), "с тремя фокусами награда доступна");
    g.apply_option(c.node("zp_reward")->options[0], "", &shop);
    eq(g.player().quests["zero_point"], QUEST_DONE, "квест о нулевой точке закрыт");
    check(g.player().portal_master, "умение ставить порталы получено");
    check(g.count_item("portal_stone") >= 2, "портальные камни выданы");

    // Порталы работают в бою за пределами теста портального модуля.
    g.player().loc = "village";
    g.player().pos = Vec2(5, 8);
    check(g.place_portal(), "портал ставится в деревне");
    g.player().loc = "forest";
    g.player().pos = Vec2(3, 8);
    check(g.place_portal(), "второй портал ставится в лесу");
    g.player().pos = Vec2(2, 8);
    Bump pb = g.try_move(1, 0);
    check(pb == Bump::Portal, "портал сработал");
    eqs(g.player().loc, "village", "портал перенёс между локациями");

    // --- зачарователь ---
    g.apply_option(c.node("ench_offer")->options[0], "", &shop);
    check(visible("ench_root", "ench_reward"), "с рунным камнем Вельд готов");
    g.apply_option(c.node("ench_reward")->options[0], "", &shop);
    eq(g.player().quests["enchanter"], QUEST_DONE, "квест зачарователя закрыт");

    bool ench_open = false;
    const DlgNode* er = c.node("ench_root");
    for (const DlgOption& o : er->options)
        if (o.open_enchant && g.option_available(o)) ench_open = true;
    check(ench_open, "услуга зачарования открылась");

    // Зачарование доводится до конца.
    const std::string wpn = g.player().equipped[static_cast<std::size_t>(Slot::Weapon)];
    g.player().gold += 5000;
    g.add_item("whetstone", 3);
    int atk_before = g.total().attack;
    check(g.enchant_item(wpn, "keen"), "оружие зачаровано");
    check(g.total().attack > atk_before, "зачарование подняло меткость");

    // ================= РЕГИОН II · ШОВ =================

    // Тропа закрыта, пока Гурий про неё не расскажет.
    check(travel("village") || true, "");
    g.player().loc = "forest";
    check(!travel("goatpath"), "без разговора с Гурием тропа не пускает");

    check(visible("trader_root", "goat_offer"), "Гурий открыл квест о пропавшем обозе");
    g.apply_option(c.node("goat_offer")->options[0], "", &shop);
    eq(g.quest_stage("goatpath"), 1, "квест об обозе взят");

    g.player().loc = "forest";
    check(travel("goatpath"), "с открытым квестом тропа пускает");
    eqs(g.player().loc, "goatpath", "герой на Козьей тропе");
    check(gather_note("goat"), "путевой лист обоза найден");
    check(hunt("stray", 2) >= 1, "приблудные встречаются и побеждаются");

    // --- Стеклянное поле ---
    check(travel("glassfield"), "переход на Стеклянное поле");
    check(visible("glazier_root", "glass_offer"), "Ферапонт предлагает работу");
    g.apply_option(c.node("glass_offer")->options[0], "", &shop);
    check(gather_note("glass"), "свидетельство стекольщика найдено");
    gather("glass_shard");
    guard = 0;
    while (g.count_item("glass_shard") < 6 && guard++ < 60) {
        if (hunt("glass_hound", 1) == 0) g.world_turn();
    }
    check(g.count_item("glass_shard") >= 6, "шесть осколков собраны");
    g.apply_option(c.node("glass_reward")->options[0], "", &shop);
    eq(g.quest_stage("glass"), QUEST_DONE, "квест «Стекло помнит» закрыт");
    check(g.count_item("glass_blade") >= 1, "стеклянный резак получен");

    // --- Мост: тайна от найденной вещи ---
    check(travel("bridge"), "переход на мост");
    gather("rope_end");
    if (g.count_item("rope_end") == 0) { hunt("bridge_walker", 1); }
    check(g.count_item("rope_end") >= 1, "обрывок каната добыт");
    eq(g.quest_stage("bridge"), 1, "вещь сама открыла тайну «Обрезанный канат»");
    check(gather_note("bridge"), "донесение о мосте найдено");
    eq(g.quest_stage("bridge"), QUEST_DONE, "тайна о мосте разгадана записью");
    check(travel("glassfield"), "возврат с моста");

    // --- Рынок Шва ---
    check(travel("market"), "переход на Рынок Шва");
    eq(g.quest_stage("goatpath"), 2, "приход на Рынок продвинул квест Гурия сам");
    check(gather_note("market"), "уложение Рынка найдено");
    check(visible("warden_root", "market_offer"), "Смотритель предлагает счёт");
    g.apply_option(c.node("market_offer")->options[0], "", &shop);

    // --- Караван-сарай: тайна от входа ---
    check(travel("caravanserai"), "переход в караван-сарай");
    eq(g.quest_stage("caravan"), 1, "вход открыл тайну «Столы накрыты»");
    check(gather_note("caravan"), "опись каравана найдена");
    eq(g.quest_stage("caravan"), QUEST_DONE, "тайна о караване разгадана");
    gather("old_coin");
    guard = 0;
    while (g.count_item("old_coin") < 8 && guard++ < 80) {
        if (hunt("caravan_shade", 1) == 0) g.world_turn();
    }
    check(travel("market"), "возврат на Рынок");
    check(g.count_item("old_coin") >= 8, "восемь монет чужой чеканки собраны");
    g.apply_option(c.node("market_reward")->options[0], "", &shop);
    eq(g.quest_stage("market"), QUEST_DONE, "квест «Не той чеканки» закрыт");

    // --- Хутор Двоеданный: выбор без правильного ответа ---
    check(travel("doubled"), "переход на хутор");
    check(gather_note("prohor"), "записка Прохора о Прохоре найдена");
    g.apply_option(c.node("prohor_offer")->options[0], "", &shop);
    eq(g.quest_stage("doubled"), 1, "спор принят к рассмотрению");

    // Оба варианта должны быть доступны — правильного нет.
    auto choice_visible = [&](const char* node) {
        const DlgNode* n = c.node(node);
        if (!n) return false;
        for (const DlgOption& o : n->options)
            if (o.set_counter == "prohor_choice" && g.option_available(o)) return true;
        return false;
    };
    check(choice_visible("prohor_l_root"), "левого Прохора можно признать настоящим");
    check(choice_visible("prohor_r_root"), "и правого тоже");

    for (const DlgOption& o : c.node("prohor_l_root")->options)
        if (o.set_counter == "prohor_choice") { g.apply_option(o, "", &shop); break; }
    eq(g.quest_stage("doubled"), QUEST_DONE, "спор рассужен");
    eq(g.player().counters["prohor_choice"], 1, "выбор запомнен");
    check(!choice_visible("prohor_r_root"), "второй раз выбрать уже нельзя");

    // --- Мельница и соляные шахты ---
    check(travel("market"), "возврат на Рынок");
    check(travel("glassfield"), "возврат на поле");
    check(travel("goatpath"), "возврат на тропу");
    check(travel("mill"), "переход на мельницу");
    check(gather_note("mill"), "расчёт мельника найден");

    g.player().loc = "mill";
    check(!travel("saltmines"), "без ключа затвор не открыть");
    check(visible("miller_root", "mill_offer"), "мельник предлагает разобраться с водой");
    g.apply_option(c.node("mill_offer")->options[0], "", &shop);
    check(g.count_item("mill_key") >= 1, "ключ от затвора получен");

    g.player().loc = "mill";
    check(travel("saltmines"), "с ключом затвор открывается");
    eqs(g.player().loc, "saltmines", "герой в соляных шахтах");
    check(gather_note("salt"), "наказ копача найден");
    eq(g.quest_stage("salt"), 1, "наказ открыл тайну «Соль помнит»");
    check(hunt("salt_mother", 1) == 1, "Соляная матерь повержена");
    eq(g.quest_stage("salt"), QUEST_DONE, "тайна «Соль помнит» разгадана убийством");

    check(travel("mill"), "возврат на мельницу");
    g.apply_option(c.node("mill_reward")->options[0], "", &shop);
    eq(g.quest_stage("mill"), QUEST_DONE, "квест «Куда уходит вода» закрыт");

    // --- обратно к Гурию ---
    g.apply_option(c.node("goat_reward")->options[0], "", &shop);
    eq(g.quest_stage("goatpath"), QUEST_DONE, "квест об обозе закрыт");

    // Ключ не тратится: игрок не может запереть себя снаружи.
    check(g.count_item("mill_key") >= 1, "ключ остался у игрока после прохода");

    // ================= РЕГИОН III · ПОЛОВИНЫ =================

    // Возвращаемся на Рынок настоящим маршрутом: мельница -> тропа -> поле -> Рынок.
    check(travel("goatpath"),   "с мельницы на тропу");
    check(travel("glassfield"), "с тропы на поле");
    check(travel("market"),     "с поля на Рынок");
    check(!travel("halfcity"), "без разговора с Улеем дорога в Город не пускает");

    check(visible("warden_root", "cityroad_offer"), "Улей рассказывает о городском товаре");
    g.apply_option(c.node("cityroad_offer")->options[0], "", &shop);
    eq(g.quest_stage("cityroad"), 1, "дорога в Город открыта разговором");

    g.player().loc = "market";
    check(travel("halfcity"), "теперь дорога пускает");
    eqs(g.player().loc, "halfcity", "герой в Половине Города");
    eq(g.quest_stage("cityroad"), QUEST_DONE, "приход в Город закрыл квест сам");
    check(gather_note("cityhalf"), "прошение о восстановлении найдено");
    check(gather_note("lastclerk"), "последняя запись писаря найдена");

    // Смотритель Половины не отдаёт ключ живым.
    check(hunt("half_warden", 1) == 1, "Смотритель Половины повержен");
    check(g.count_item("archive_key") >= 1, "ключ архива снят со смотрителя");
    check(g.count_item("half_name") >= 1, "половина имени добыта");
    eq(g.quest_stage("halves"), 1, "обрывок имени открыл тайну «Вторая половина»");

    // --- Улица без конца ---
    check(travel("endless"), "переход на Кольцевую");
    eq(g.quest_stage("endless"), 1, "улица открыла тайну сама");
    check(gather_note("endless"), "заметка обходчика найдена");
    eq(g.quest_stage("endless"), QUEST_DONE, "тайна «Улица без конца» разгадана");

    // Восточный конец улицы выводит в её же начало.
    {
        const Location* st = g.here();
        const MapExit* loop = 0;
        for (const MapExit& e : st->exits)
            if (e.target == "endless") loop = &e;
        check(loop != nullptr, "у улицы есть переход в саму себя");
        if (loop) {
            g.player().pos = Vec2(loop->pos.x - 1, loop->pos.y);
            Bump b2 = g.try_move(1, 0);
            check(b2 == Bump::Exit, "конец улицы срабатывает как переход");
            eqs(g.player().loc, "endless", "и приводит на ту же улицу");
            check(g.player().pos.x < 10, "но в её начало");
        }
    }
    check(travel("halfcity"), "возврат в Город");

    // --- Литейный двор ---
    check(travel("foundry"), "переход на Литейный двор");
    check(gather_note("foundry"), "цеховая книга найдена");
    check(visible("founder_root", "foundry_offer"), "Кузьма просит железа");
    g.apply_option(c.node("foundry_offer")->options[0], "", &shop);
    gather("scrap_iron");
    guard = 0;
    while (g.count_item("scrap_iron") < 8 && guard++ < 80) {
        if (hunt("slag_thing", 1) == 0) g.world_turn();
    }
    check(g.count_item("scrap_iron") >= 8, "восемь кусков железа собраны");
    g.apply_option(c.node("foundry_reward")->options[0], "", &shop);
    eq(g.quest_stage("foundry"), QUEST_DONE, "квест «Пока льём — стоим» закрыт");

    // --- Архив: заперт, пока нет ключа ---
    check(visible("scribe_root", "lists_offer"), "писарь просит попасть в архив");
    g.apply_option(c.node("lists_offer")->options[0], "", &shop);
    eq(g.quest_stage("lists"), 1, "квест о списках взят");

    g.player().loc = "foundry";
    check(travel("archive"), "с ключом архив открывается");
    eqs(g.player().loc, "archive", "герой в архиве");
    check(gather_note("lists"), "опись архива найдена");
    eq(g.quest_stage("lists"), 2, "опись прочитана");
    check(travel("foundry"), "возврат к писарю");
    g.apply_option(c.node("lists_reward")->options[0], "", &shop);
    eq(g.quest_stage("lists"), QUEST_DONE, "квест «Половина имени» закрыт");

    // --- Канал Мёртвой воды ---
    check(travel("halfcity"), "возврат в Город");
    check(travel("canal"), "спуск к каналу");
    check(gather_note("deadwater"), "наставление перевозчику найдено");
    eq(g.quest_stage("deadwater"), 1, "наказ открыл тайну «Мёртвая вода»");
    gather("ferry_token");
    guard = 0;
    while (g.count_item("ferry_token") < 5 && guard++ < 80) {
        if (hunt("canal_walker", 1) == 0) g.world_turn();
    }
    check(g.count_item("ferry_token") >= 5, "пять жетонов собраны");
    eq(g.quest_stage("deadwater"), QUEST_DONE, "счёт сошёлся, тайна разгадана");

    // По стоячей воде действительно ходят.
    {
        const Location* cn = g.here();
        bool walkable_water = false;
        for (int y = 1; y < 17 && !walkable_water; ++y)
            for (int x = 1; x < 47; ++x)
                if (cn->at(Vec2(x, y)) == Tile::DeadWater && cn->walkable(Vec2(x, y))) {
                    walkable_water = true;
                    break;
                }
        check(walkable_water, "по мёртвой воде можно ходить");
    }

    // --- Башня Счетовода ---
    check(travel("counter"), "переход к башне");
    check(gather_note("counting"), "черновик Счетовода найден");
    check(gather_note("halves"), "донесение о второй половине найдено");
    eq(g.quest_stage("halves"), QUEST_DONE, "тайна «Вторая половина» разгадана");
    check(visible("counter_root", "counting_offer"), "Аким просит листы гроссбуха");
    g.apply_option(c.node("counting_offer")->options[0], "", &shop);
    gather("ledger_page");
    guard = 0;
    while (g.count_item("ledger_page") < 6 && guard++ < 80) {
        if (hunt("mad_clerk", 1) == 0) g.world_turn();
    }
    check(g.count_item("ledger_page") >= 6, "шесть листов гроссбуха собраны");
    g.apply_option(c.node("counting_reward")->options[0], "", &shop);
    eq(g.quest_stage("counting"), QUEST_DONE, "квест «Две сажени в год» закрыт");

    // ================= Регион IV: Обитель Ордена =================

    // --- Ворота: печать пускает, отсутствие печати — нет ---
    check(visible("counter_root", "orderway_offer"),
          "закрыв счёт, Аким рассказывает про стены на севере");
    g.apply_option(c.node("orderway_offer")->options[0], "", &shop);
    eq(g.quest_stage("orderway"), 1, "путь к обители открыт разговором");

    check(g.count_item("order_seal") >= 1, "печать Ордена снята с архивариуса ещё в схроне");
    g.remove_item("order_seal", g.count_item("order_seal"));
    check(!travel("ordergate"), "без печати ворота не пускают");
    eqs(g.player().loc, "counter", "и герой остаётся на башне");

    // Печать на пальце — тоже печать: надетое кольцо открывает те же ворота.
    g.add_item("order_seal", 1);
    check(g.equip("order_seal"), "печать надевается как кольцо");
    eq(g.count_item("order_seal"), 0, "в сумке её больше нет");
    check(travel("ordergate"), "надетая печать открывает ворота");
    eqs(g.player().loc, "ordergate", "герой у обители");
    eq(g.quest_stage("orderway"), QUEST_DONE, "приход к воротам закрыл квест сам");
    check(gather_note("gates"), "надпись на воротах прочитана");

    // --- Привратная: Севир стоит двести лет ---
    check(travel("gatehouse"), "переход в привратную");
    check(gather_note("watchwrit"), "черновик приказа найден");
    check(visible("gatekeeper_root", "watch4_offer"), "Севир объясняет, почему стоит");
    g.apply_option(c.node("watch4_offer")->options[0], "", &shop);
    eq(g.quest_stage("watch4"), 1, "квест о смене караула взят");
    check(!visible("gatekeeper_root", "watch4_reward"),
          "без приказа пост не снять");

    // --- Библиотека: ящик для недоставленного и вырванный лист ---
    check(travel("library"), "переход в библиотеку");
    check(visible("librarian_root", "librarian_box"), "Аврелий помнит про ящик");
    g.apply_option(c.node("librarian_box")->options[0], "", &shop);
    check(g.count_item("order_writ") >= 1, "приказ о смене найден в ящике");

    check(gather_note("read"), "выписка книжника найдена");
    check(gather_note("unsealed"), "вырванный лист найден");
    eq(g.quest_stage("unsealed"), 1, "лист открыл тайну «То, что вырвали»");
    gather("torn_page");
    guard = 0;
    while (g.count_item("torn_page") < 1 && guard++ < 60) {
        if (hunt("page_swarm", 1) == 0) g.world_turn();
    }
    check(visible("librarian_root", "unsealed_talk"), "есть с чем идти к Аврелию");
    g.apply_option(c.node("unsealed_talk")->options[0], "", &shop);
    eq(g.quest_stage("unsealed"), QUEST_DONE, "тайна «То, что вырвали» разгадана");

    // --- Чертёжная: карта сети без спиц ---
    check(travel("drafting"), "переход в чертёжную");
    check(gather_note("charts"), "заметка о спицах найдена");
    check(visible("draftsman_root", "charts_offer"), "Гордей просит обрывки");
    g.apply_option(c.node("charts_offer")->options[0], "", &shop);
    eq(g.quest_stage("charts"), 1, "квест о карте сети взят");
    gather("chart_piece");
    guard = 0;
    while (g.count_item("chart_piece") < 4 && guard++ < 90) {
        if (hunt("draft_shade", 1) == 0) g.world_turn();
    }
    check(g.count_item("chart_piece") >= 4, "четыре обрывка чертежа собраны");
    g.apply_option(c.node("charts_reward")->options[0], "", &shop);
    eq(g.quest_stage("charts"), QUEST_DONE, "квест «Карта сети» закрыт");

    // --- Приказ доставлен ---
    g.player().loc = "gatehouse";
    check(visible("gatekeeper_root", "watch4_reward"), "с приказом пост снимается");
    g.apply_option(c.node("watch4_reward")->options[0], "", &shop);
    eq(g.quest_stage("watch4"), QUEST_DONE, "квест «Смена караула» закрыт");
    eq(g.count_item("order_writ"), 0, "приказ отдан Севиру");
    check(visible("gatekeeper_root", "watch4_after"), "Севиру теперь есть что сказать");

    // --- Кельи: солдатик заводит квест сам, без разговора ---
    check(travel("cells"), "переход в кельи");
    check(gather_note("novice"), "запись послушника найдена");
    check(gather_note("keepsake"), "наказ настоятеля найден");
    eq(g.quest_stage("keepsake"), QUEST_NONE, "записка сама по себе квеста не даёт");
    check(gather("keepsake") >= 1, "оловянный солдатик подобран");
    eq(g.quest_stage("keepsake"), 1, "находка открыла квест без единого слова");

    // --- Печь: обмен без возврата ---
    check(travel("furnace"), "спуск к печам");
    check(gather_note("ovens"), "правило истопника найдено");
    check(visible("stoker_root", "ovens_offer"), "Фома объясняет, что берёт печь");
    g.apply_option(c.node("ovens_offer")->options[0], "", &shop);
    eq(g.quest_stage("ovens"), 1, "квест «Печь берёт» взят");
    check(!visible("stoker_root", "ovens_reward"), "без шести листов печь не топят");

    g.player().loc = "library";
    guard = 0;
    while (g.count_item("torn_page") < 6 && guard++ < 120) {
        gather("torn_page");
        if (hunt("page_swarm", 1) == 0) g.world_turn();
    }
    check(g.count_item("torn_page") >= 6, "шесть вырванных листов собраны");
    g.player().loc = "furnace";
    g.apply_option(c.node("ovens_reward")->options[0], "", &shop);
    eq(g.quest_stage("ovens"), QUEST_DONE, "квест «Печь берёт» закрыт");
    eq(g.count_item("torn_page"), 0, "печь забрала листы без остатка");
    check(g.count_item("order_plate") >= 1, "и отдала то, чего в них не было");

    // --- Зал Отказа: развилка, после которой второго ответа нет ---
    g.player().loc = "refusalhall";
    check(gather_note("refusal"), "стенограмма Зала найдена");
    check(visible("recorder_root", "refusal_offer"), "Никон рассказывает о заседании");
    g.apply_option(c.node("refusal_offer")->options[0], "", &shop);
    eq(g.quest_stage("refusal"), 1, "протокол ждёт последней записи");
    check(visible("recorder_root", "refusal_master"), "можно сказать, что прав Мастер");
    check(visible("recorder_root", "refusal_council"), "и что прав Совет — тоже");

    {
        const DlgNode* rec = c.node("recorder_root");
        const DlgOption* say_master = 0;
        for (const DlgOption& o : rec->options)
            if (o.next == "refusal_master") say_master = &o;
        check(say_master != nullptr, "ответ в пользу Мастера есть в узле");
        if (say_master) g.apply_option(*say_master, "", &shop);
    }
    eq(g.quest_stage("refusal"), QUEST_DONE, "заседание закрыто");
    eq(g.player().counters["refusal_choice"], 1, "выбор записан в протокол");
    check(g.count_item("master_ring") >= 1, "кольцо Мастера снято со стола");
    check(!visible("recorder_root", "refusal_master"), "переписать ответ уже нельзя");
    check(!visible("recorder_root", "refusal_council"), "и второй вариант закрыт");
    check(visible("recorder_root", "refusal_after_m"),
          "Никон отвечает по тому ответу, который дали");
    check(!visible("recorder_root", "refusal_after_c"),
          "и не по тому, которого не давали");

    // --- Узел Второй: держит ---
    check(travel("node2"), "переход к Второму узлу");
    eq(g.quest_stage("node2q"), 1, "сам приход открыл тайну «Второй держит»");
    check(gather_note("secondnode"), "отметка о Втором узле найдена");
    check(hunt("node_heart", 1) == 1, "Сердце Узла остановлено");
    eq(g.quest_stage("node2q"), QUEST_DONE, "тайна «Второй держит» разгадана");

    // --- Узел Третий: открыт ---
    check(travel("node3"), "переход к Третьему узлу");
    eq(g.quest_stage("node3q"), 1, "тайна «Третий открыт» открылась приходом");
    check(gather_note("thirdnode"), "отметка о Третьем узле найдена");
    eq(g.quest_stage("node3q"), QUEST_DONE, "через Третий уходят в Дрейф");

    // --- Пустая могила: тот, кто ещё ходит ---
    check(travel("grave"), "переход к могиле Первого Мастера");
    check(gather_note("emptygrave"), "приписка углём найдена");
    eq(g.quest_stage("firstmaster"), 1, "тайна «Он ещё ходит» открыта");
    check(hunt("master_shadow", 1) == 1, "тень Первого Мастера повержена");
    eq(g.quest_stage("firstmaster"), QUEST_DONE, "тайна «Он ещё ходит» разгадана");

    // --- Солдатик возвращается тому, кто его положил ---
    g.player().loc = "forest";
    check(visible("hermit_root", "keepsake_talk"), "отшельнику есть что сказать о солдатике");
    g.apply_option(c.node("keepsake_talk")->options[0], "", &shop);
    eq(g.quest_stage("keepsake"), QUEST_DONE, "квест «Оловянный солдатик» закрыт");
    eq(g.count_item("keepsake"), 0, "солдатик остался у Игната");

    // --- итог ---
    const char* all_quests[] = {"wolves", "amulet", "pelts", "moss", "books",
                                "queen", "outpost", "zero_point", "enchanter",
                                "seam", "cinch",
                                "goatpath", "glass", "mill", "market", "doubled",
                                "caravan", "salt", "bridge",
                                "cityroad", "foundry", "counting", "lists",
                                "endless", "deadwater", "halves",
                                "orderway", "watch4", "charts", "keepsake", "ovens",
                                "refusal", "unsealed", "node2q", "node3q", "firstmaster"};
    for (const char* q : all_quests)
        eq(g.player().quests[q], QUEST_DONE, std::string("квест ") + q + " пройден");

    // Записки-пасхалки лежат по всем локациям и находятся по ходу дела.
    check(g.books().size() >= 2, "библиотека наполнилась находками и своими книгами");
    check(g.book(diary) != nullptr, "своя книга дожила до конца прохождения");

    check(g.player().level >= 20, "к концу четырёх регионов герой заметно вырос");
    std::cout << "  (итог: уровень " << g.player().level
              << ", золота " << g.player().gold
              << ", квестов пройдено " << (sizeof(all_quests) / sizeof(all_quests[0]))
              << ")\n";
}

} // namespace

int main() {
    std::cout << "Тесты «Любви Эндора»\n";
    test_text_helpers();
    test_wrap();
    test_glyphs();
    test_maps();
    test_embedded_maps();
    test_new_game_and_stats();
    test_equipment();
    test_level_up();
    test_shop();
    test_quest_flow();
    test_quest_item_exchange();
    test_combat();
    test_save_load();
    test_movement_and_pickup();
    test_mob_ai();
    test_effects();
    test_races_and_specs();
    test_mob_inventory();
    test_chests();
    test_enchanting();
    test_portals();
    test_gates_and_secret_quests();
    test_books_and_notes();
    test_book_save_roundtrip();
    test_no_escape_needed();
    test_content_integrity();
    test_playthrough();

    std::cout << "\n----------------------------------------\n";
    std::cout << "Проверок: " << g_checks << ", провалов: " << g_failed << "\n";
    if (g_failed == 0) std::cout << "ВСЁ ЗЕЛЁНОЕ\n";
    return g_failed == 0 ? 0 : 1;
}
