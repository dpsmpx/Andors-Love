// Регрессионные тесты игровой логики. Запуск: make test (из корня проекта —
// карты грузятся по относительному пути data/maps).
#include "../src/game.h"
#include "../src/world.h"

#include "../src/platform.h"

#include <cstdio>
#include <iostream>
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

    // Переходы должны вести друг в друга и попадать на проходимую клетку.
    check(v->exits.size() == 1 && f->exits.size() == 1, "по одному переходу в каждой локации");
    if (!v->exits.empty() && !f->exits.empty()) {
        eqs(v->exits[0].target, "forest",  "деревня ведёт в лес");
        eqs(f->exits[0].target, "village", "лес ведёт в деревню");
        check(f->walkable(v->exits[0].dest), "точка прибытия в лесу проходима");
        check(v->walkable(f->exits[0].dest), "точка прибытия в деревне проходима");
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

void test_new_game_and_stats() {
    section("новая игра и характеристики");
    Game g;
    g.new_game("Тестер");
    eqs(g.player().name, "Тестер", "имя игрока берётся из ввода");   // была ошибка new_name = name
    eq(g.player().level, 1, "стартовый уровень");
    check(g.here() != nullptr, "стартовая локация загружена");

    Stats t = g.total();
    eq(t.max_hp, 30, "стартовое здоровье");
    eq(g.player().hp, 30, "здоровье заполнено");
    // база 1-3 + кинжал 2-4
    eq(t.dmg_min, 3, "урон с кинжалом, минимум");
    eq(t.dmg_max, 7, "урон с кинжалом, максимум");
    eq(g.attack_cost(), 3, "кинжал удешевляет атаку на 1 AP");

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
    g.new_game("Тестер");
    int armor0 = g.total().armor;

    g.add_item("leather_armor", 1);
    check(g.equip("leather_armor"), "кожаный доспех надевается");
    eq(g.total().armor, armor0 + 2, "броня выросла");
    eq(g.count_item("leather_armor"), 0, "надетое ушло из сумки");

    // Смена оружия должна возвращать прежнее в сумку, а не терять его.
    g.add_item("axe", 1);
    check(g.equip("axe"), "топор надевается");
    eq(g.count_item("dagger"), 1, "прежнее оружие вернулось в сумку");
    eq(g.attack_cost(), 5, "топор дороже по AP");

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
    const char* shops[] = {"shop_smith", "shop_general", "shop_herbs"};
    for (const char* sid : shops) {
        const ShopDef* s = c.shop(sid);
        check(s != nullptr, std::string("магазин ") + sid + " существует");
        if (!s) continue;
        for (const std::string& gid : s->goods)
            check(c.item(gid) != nullptr, "товар " + gid + " есть в базе предметов");
    }

    const char* npcs[] = {"elder", "herbalist", "smith", "trader", "hermit"};
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
                           "elder_reward","elder_after","herbalist_root","herb_offer",
                           "herb_taken","herb_progress","herb_reward","smith_root",
                           "smith_offer","smith_taken","smith_progress","smith_reward",
                           "trader_root","trader_talk","hermit_root","hermit_rest","hermit_hint"};
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
            if (!o.req_quest.empty())
                check(c.quest(o.req_quest) != nullptr,
                      std::string(nid) + ": квест '" + o.req_quest + "' существует");
            if (!o.set_quest.empty())
                check(c.quest(o.set_quest) != nullptr,
                      std::string(nid) + ": назначаемый квест '" + o.set_quest + "' существует");
        }
    }

    // Добыча врагов должна ссылаться на существующие предметы.
    const char* mobs[] = {"rat", "wolf", "bandit", "wolf_alpha"};
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
    for (const char* lid : {"village", "forest"}) {
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
    }

    // Амулет должен быть добываем — иначе квест Лады непроходим.
    const EnemyDef* alpha = c.enemy("wolf_alpha");
    bool amulet_droppable = false;
    if (alpha)
        for (const Drop& d : alpha->drops)
            if (d.item == "amulet" && d.percent >= 100) amulet_droppable = true;
    check(amulet_droppable, "амулет гарантированно падает с вожака — квест проходим");
}

// Сквозная проверка: можно ли вообще пройти игру. Бьём реальной боевой
// механикой, а не подкручиваем счётчики, — иначе тест не доказывает
// проходимость.
void test_playthrough() {
    section("сквозное прохождение: квесты через реальный бой");
    Game g;
    g.new_game("Герой");

    // Берём все три квеста у жителей.
    const Content& c = Content::get();
    std::string shop;
    g.apply_option(c.node("elder_offer")->options[0], "", &shop);
    g.apply_option(c.node("herb_offer")->options[0],  "", &shop);
    g.apply_option(c.node("smith_offer")->options[0], "", &shop);
    eq(g.player().quests["wolves"], 1, "квест на волков взят");
    eq(g.player().quests["amulet"], 1, "квест на амулет взят");
    eq(g.player().quests["pelts"],  1, "заказ кузнеца взят");

    // Уходим в лес через настоящий переход.
    const Location* v = g.here();
    check(v && !v->exits.empty(), "в деревне есть переход");
    if (!v || v->exits.empty()) return;
    g.player().pos = Vec2{v->exits[0].pos.x - 1, v->exits[0].pos.y};
    g.try_move(1, 0);
    eqs(g.player().loc, "forest", "герой в лесу");
    check(!g.mobs().empty(), "в лесу есть кому сопротивляться");

    // Побеждаем противника честной боевой механикой.
    auto fight = [&](int uid) {
        g.start_combat(uid);
        for (int guard = 0; guard < 400 && g.combat().active; ++guard) {
            // Герой не должен умереть — тест про проходимость, не про баланс.
            g.player().hp = g.total().max_hp;
            if (g.player().ap < g.attack_cost()) g.player().ap = g.total().max_ap;
            g.combat_attack(false);
        }
        return !g.combat().active;
    };

    // Бьём волков, пока счётчик не дойдёт до пяти.
    int guard = 0;
    while (g.player().counters["kill_wolf"] < 5 && guard++ < 60) {
        int uid = -1;
        for (const Mob& m : g.mobs())
            if (m.loc == "forest" && (m.enemy_id == "wolf" || m.enemy_id == "wolf_alpha")) {
                uid = m.uid;
                break;
            }
        if (uid < 0) { g.world_turn(); continue; }   // ждём респавна
        check(fight(uid), "бой завершается, а не зацикливается");
    }
    check(g.player().counters["kill_wolf"] >= 5, "пять волков перебиты в бою");

    // Вожак должен быть побеждён и отдать амулет.
    int alpha = -1;
    for (const Mob& m : g.mobs())
        if (m.enemy_id == "wolf_alpha") alpha = m.uid;
    if (alpha >= 0) fight(alpha);
    eq(g.count_item("amulet"), 1, "амулет добыт с вожака");

    // Возвращаемся и сдаём все три квеста.
    bool can_finish_wolves = false;
    for (const DlgOption& o : c.node("elder_root")->options)
        if (o.next == "elder_reward" && g.option_available(o)) can_finish_wolves = true;
    check(can_finish_wolves, "старейшина принимает работу");
    g.apply_option(c.node("elder_reward")->options[0], "", &shop);
    eq(g.player().quests["wolves"], QUEST_DONE, "квест на волков закрыт");

    bool can_finish_amulet = false;
    for (const DlgOption& o : c.node("herbalist_root")->options)
        if (o.next == "herb_reward" && g.option_available(o)) can_finish_amulet = true;
    check(can_finish_amulet, "Лада принимает амулет");
    g.apply_option(c.node("herb_reward")->options[0], "", &shop);
    eq(g.player().quests["amulet"], QUEST_DONE, "квест на амулет закрыт");
    eq(g.count_item("amulet"), 0, "амулет отдан хозяйке");
    eq(g.count_item("ring_hp"), 1, "кольцо жизни получено");

    check(g.count_item("wolf_pelt") >= 3, "с волков набралось три шкуры на заказ");
    if (g.count_item("wolf_pelt") >= 3) {
        g.apply_option(c.node("smith_reward")->options[0], "", &shop);
        eq(g.player().quests["pelts"], QUEST_DONE, "заказ кузнеца закрыт");
        eq(g.count_item("chain_armor"), 1, "кольчуга получена");
    }

    check(g.player().level >= 3, "к концу трёх квестов герой заметно вырос");
    std::cout << "  (итог прохождения: уровень " << g.player().level
              << ", золота " << g.player().gold
              << ", волков убито " << g.player().counters["kill_wolf"] << ")\n";
}

} // namespace

int main() {
    std::cout << "Тесты «Любви Эндора»\n";
    test_text_helpers();
    test_wrap();
    test_maps();
    test_new_game_and_stats();
    test_equipment();
    test_level_up();
    test_shop();
    test_quest_flow();
    test_quest_item_exchange();
    test_combat();
    test_save_load();
    test_movement_and_pickup();
    test_content_integrity();
    test_playthrough();

    std::cout << "\n----------------------------------------\n";
    std::cout << "Проверок: " << g_checks << ", провалов: " << g_failed << "\n";
    if (g_failed == 0) std::cout << "ВСЁ ЗЕЛЁНОЕ\n";
    return g_failed == 0 ? 0 : 1;
}
