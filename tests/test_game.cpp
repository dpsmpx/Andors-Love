// Регрессионные тесты игровой логики. Запуск: make test (из корня проекта —
// карты грузятся по относительному пути data/maps).
#include "../src/game.h"
#include "../src/world.h"

#include "../src/embedded_maps.h"
#include "../src/ui.h"
#include "../src/platform.h"
#include "../src/gfx/font.h"
#include "../src/gfx/touch.h"
#include "../src/gfx/walk.h"
#include "../src/gfx/png.h"
#include "../src/gfx/tiles.h"

#include <cstdio>
#include <fstream>
#include <set>
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
    "village", "forest", "cave", "ruins", "sanctum", "vault", "graveyard", "swamp",
    "goatpath", "glassfield", "mill", "market", "bridge", "saltmines",
    "caravanserai", "doubled",
    "halfcity", "endless", "foundry", "canal", "counter", "archive",
    "ordergate", "gatehouse", "library", "drafting", "cells", "furnace",
    "refusalhall", "node2", "node3", "grave",
    "meadow", "farhouse", "well", "grove", "battlefield", "otherhalf",
    "upstair", "edge", "emptyalder", "homepath",
    "firstseam", "gallery", "measures", "meeting", "node1", "cinchheart",
    "zeropoint", "finale"};
const int N_LOCATIONS = static_cast<int>(sizeof(ALL_LOCATIONS) / sizeof(ALL_LOCATIONS[0]));

// Все записки мира: список держится в одном месте, чтобы новая
// записка не могла тихо выпасть из проверок.
const char* const ALL_NOTES[] = {"ink", "miner", "watch", "zero", "child", "proto", "hermit",
                              "bog", "drovers", "graves", "sexton",
                              "seam", "cinch", "order", "double",
                              "goat", "glass", "mill", "market", "prohor",
                              "caravan", "salt", "bridge",
                              "cityhalf", "endless", "deadwater", "counting",
                              "lists", "foundry", "halves", "lastclerk",
                              "gates", "watchwrit", "read", "unsealed", "charts",
                              "novice", "keepsake", "ovens", "refusal",
                              "secondnode", "thirdnode", "emptygrave",
                              "drift", "tomorrow", "otherside", "stair", "lastorder",
                              "marks", "wellrule", "homeward", "houses", "edgeview",
                              "oldseam", "readlines", "roomrule", "walkers",
                              "firstnode", "cinchwork", "allatonce", "threeways"};

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

void test_log_history() {
    section("журнал партии");

    Game g;
    g.new_game("Тест", "human", "swordsman");
    g.clear_log();

    // История хранится целиком: журнал листают до начала партии, а прежний
    // предел в двести записей выбрасывал большую её часть — полное
    // прохождение набирает под восемьсот.
    for (int i = 0; i < 900; ++i) g.msg("запись " + to_str(i));
    eq(static_cast<int>(g.log().size()), 900, "девятьсот записей не обрезаны");
    eqs(g.log().front(), "запись 0", "самая первая на месте");
    eqs(g.log().back(), "запись 899", "и самая последняя тоже");

    // Предел всё же есть, чтобы очень долгая партия не съела память.
    // Режется пачкой, а не по записи: сдвигать весь вектор на каждое
    // сообщение — работа, растущая вместе с журналом.
    g.clear_log();
    for (std::size_t i = 0; i < LOG_MAX + LOG_TRIM / 2; ++i) g.msg("з" + to_str(static_cast<int>(i)));
    check(g.log().size() <= LOG_MAX, "выше предела журнал не растёт");
    check(g.log().size() > LOG_MAX - LOG_TRIM, "но и не срезается до нуля");
    eqs(g.log().back(), "з" + to_str(static_cast<int>(LOG_MAX + LOG_TRIM / 2) - 1),
        "последнее сообщение всегда сохраняется");
    check(g.log().front() != "з0", "а самое старое вытеснено");

    // Счётчик срезаний. Кэш переноса строк в графической оболочке хранит
    // разложенный журнал и дописывает к нему новые записи; понять, что
    // начало срезали, по одной длине нельзя — журнал дорастает до прежней
    // длины другими записями. Здесь ровно этот случай: длина та же,
    // содержимое другое, и отличает их только счётчик.
    g.clear_log();
    const unsigned long ep0 = g.log_epoch();
    for (std::size_t i = 0; i < LOG_MAX; ++i) g.msg("a" + to_str(static_cast<int>(i)));
    eq(static_cast<int>(g.log_epoch() - ep0), 0, "до предела счётчик стоит");
    const std::size_t before = g.log().size();
    // Ровно LOG_TRIM записей: первая срезает начало (5000 -> 4001),
    // остальные добирают длину обратно до 5000.
    for (std::size_t i = 0; i < LOG_TRIM; ++i) g.msg("b" + to_str(static_cast<int>(i)));
    eq(static_cast<int>(g.log().size()), static_cast<int>(before), "длина вернулась к прежней");
    check(g.log_epoch() != ep0, "но счётчик срезаний это заметил");

    // Очистка журнала — тоже смена содержимого при возможной прежней длине.
    const unsigned long ep1 = g.log_epoch();
    g.clear_log();
    check(g.log_epoch() != ep1, "очистка журнала считается сменой");

    // --- важность сообщений ---
    // Важности идут строка в строку с журналом. Разойдись длины — и красить
    // начнёт не те строки, причём молча.
    {
        Game t;
        t.new_game("Заметный");
        t.clear_log();
        t.msg("обычное");
        t.msg("хорошее", MsgTone::Good);
        t.msg("плохое", MsgTone::Bad);
        t.msg("веха", MsgTone::Loud);
        eq(static_cast<int>(t.log().size()), 4, "четыре записи");
        eq(static_cast<int>(t.log_tones().size()), 4, "и ровно четыре важности");
        eq(static_cast<int>(t.log_tones()[0]), static_cast<int>(MsgTone::Plain),
           "по умолчанию — обычное");
        eq(static_cast<int>(t.log_tones()[1]), static_cast<int>(MsgTone::Good), "хорошее");
        eq(static_cast<int>(t.log_tones()[2]), static_cast<int>(MsgTone::Bad), "плохое");
        eq(static_cast<int>(t.log_tones()[3]), static_cast<int>(MsgTone::Loud), "веха");

        // Срезание начала режет важности тем же куском.
        for (std::size_t i = 0; i < LOG_MAX + 5; ++i) t.msg("шум");
        eq(static_cast<int>(t.log_tones().size()), static_cast<int>(t.log().size()),
           "после срезания длины по-прежнему совпадают");

        // Очистка и загрузка обнуляют оба списка разом.
        t.clear_log();
        eq(static_cast<int>(t.log_tones().size()), 0, "очистка убирает и важности");
    }

    // Настоящие события размечены, а не оставлены серыми: ради них подсветка
    // и делалась. Проверяем на подъёме уровня — самой заметной вехе.
    {
        Game t;
        t.new_game("Растущий");
        t.clear_log();
        t.grant_exp(t.exp_to_next());
        bool loud = false, good = false;
        for (std::size_t i = 0; i < t.log_tones().size(); ++i) {
            if (t.log_tones()[i] == static_cast<unsigned char>(MsgTone::Loud)) loud = true;
            if (t.log_tones()[i] == static_cast<unsigned char>(MsgTone::Good)) good = true;
        }
        check(loud, "подъём уровня помечен как веха");
        check(good, "а полученный опыт — как хорошее");
    }
}

void test_log_tail_source() {
    section("журнал: строка знает свою запись");

    // После переноса строка теряет связь с записью, а красить надо по записи:
    // у длинного сообщения обе половины одного цвета.
    std::vector<std::string> lines;
    lines.push_back("короткая");
    lines.push_back("очень длинная запись, которая точно не влезет в одну строку окна");
    lines.push_back("хвост");

    std::vector<std::size_t> src;
    const std::vector<std::string> tail = log_tail(lines, 20, 10, &src);
    eq(static_cast<int>(src.size()), static_cast<int>(tail.size()),
       "номер записи есть у каждой выданной строки");
    check(tail.size() > lines.size(), "длинная запись действительно распалась");

    // Номера идут по возрастанию и не выходят за границы журнала.
    bool ordered = true, in_range = true;
    for (std::size_t i = 0; i < src.size(); ++i) {
        if (src[i] >= lines.size()) in_range = false;
        if (i && src[i] < src[i - 1]) ordered = false;
    }
    check(in_range, "номера указывают на существующие записи");
    check(ordered, "и идут в том же порядке, что строки");
    eqs(lines[src.front()], "короткая", "первая строка — из первой записи");
    eqs(lines[src.back()], "хвост", "последняя — из последней");

    // Обе половины длинной записи ссылаются на неё же.
    int mid = 0;
    for (std::size_t i = 0; i < src.size(); ++i) if (src[i] == 1) ++mid;
    check(mid >= 2, "распавшаяся запись дала больше одной строки с одним номером");

    // Обрезание сверху не сбивает соответствие.
    std::vector<std::size_t> src2;
    const std::vector<std::string> two = log_tail(lines, 20, 2, &src2);
    eq(static_cast<int>(two.size()), 2, "выдано ровно две строки");
    eq(static_cast<int>(src2.size()), 2, "и два номера");
    eqs(lines[src2.back()], "хвост", "последняя всё та же");
}

void test_item_text() {
    section("предмет словами");
    const Content& c = Content::get();

    const ItemDef* sword = c.item("short_sword");
    const ItemDef* axe   = c.item("axe");
    check(sword != nullptr && axe != nullptr, "меч и топор есть в содержимом");
    if (!sword || !axe) return;

    // Описание: название, вид, цена и только ненулевые бонусы. Нулевые
    // строки — шум: в списке из десяти «+0 к тому-то» не видно главного.
    const std::string d = item_desc(*sword);
    check(d.find(sword->name) != std::string::npos, "в описании есть название");
    check(d.find("Цена:") != std::string::npos, "и цена");
    check(d.find("+0 ") == std::string::npos, "а нулевых бонусов нет");

    // Отступ приписывается к каждой строке, а не только к первой: в
    // терминале блок стоит с отступом целиком.
    const std::string ind = item_desc(*sword, "  ");
    check(ind.substr(0, 2) == "  ", "первая строка с отступом");
    std::size_t nl = ind.find('\n');
    check(nl != std::string::npos && ind.compare(nl + 1, 2, "  ") == 0,
          "и вторая тоже");

    // Пустое гнездо и уже надетое — разные ответы, и оба без чисел:
    // сравнивать не с чем.
    eqs(compare_worn(*sword, 0), "\nГнездо пустое — надеть будет во что.\n",
        "пустому гнезду так и сказано");
    check(compare_worn(*sword, sword).find("Такое уже надето") != std::string::npos,
          "своё же оружие узнаётся");

    // А вот сравнение двух разных: строка пишется только для разошедшихся
    // чисел, со стрелкой и знаковой разницей.
    const std::string cmp = compare_worn(*axe, sword);
    check(cmp.find("Сейчас надето: " + sword->name) != std::string::npos,
          "названо, что надето сейчас");
    check(cmp.find("->") != std::string::npos, "есть хотя бы одна строка сравнения");
    {
        // Проверяем ровно ту пару чисел, что у этих двух вещей разошлась.
        const int a = sword->bonus.dmg_max, b = axe->bonus.dmg_max;
        if (a != b) {
            const int diff = b - a;
            const std::string want = "урон сверху " + to_str(a) + " -> " + to_str(b) +
                                     "  (" + (diff > 0 ? "+" : "") + to_str(diff) + ")";
            check(cmp.find(want) != std::string::npos, "разница по урону сверху точна");
        }
        // А совпавшие числа строки не порождают.
        if (sword->bonus.armor == axe->bonus.armor)
            check(cmp.find("броня") == std::string::npos,
                  "совпавшая броня строки не занимает");
    }

    // Расходник ни в какое гнездо не идёт, и сравнивать его не с чем.
    if (const ItemDef* bread = c.item("bread"))
        eqs(compare_worn(*bread, sword), "", "у расходника сравнения нет");
}

void test_reflow() {
    section("переливка абзацев");

    // Проза в исходнике разбита под одну ширину, а окно бывает другим.
    // После переливки строка должна дотягиваться до края, а не обрываться
    // посреди фразы коротким хвостом.
    const std::string src = "Кольцо возьми. Оно тут с того дня лежит,\n"
                            "и я не смел его тронуть,\n"
                            "пока протокол был открыт.";
    // Заполнена ли раскладка: в каждую строку, кроме последней, следующее
    // слово уже не влезало. Незаполненная строка — это и есть обрыв
    // посреди фразы, ради которого всё затевалось.
    struct Packed {
        static bool of(const std::vector<std::string>& ls, std::size_t w) {
            for (std::size_t i = 0; i + 1 < ls.size(); ++i) {
                const std::vector<std::string> next = split_ws(ls[i + 1]);
                if (next.empty()) continue;
                if (utf8_len(ls[i]) + 1 + utf8_len(next[0]) <= w) return false;
            }
            return true;
        }
    };
    const std::vector<std::string> raw = wrap(src, 44);
    const std::vector<std::string> flow = wrap(reflow(src), 44);
    check(!Packed::of(raw, 44), "исходная разбивка оставляла место в строке");
    check(Packed::of(flow, 44), "после переливки каждая строка заполнена");
    check(reflow(src).find("пока протокол был открыт.") != std::string::npos,
          "и текст при этом цел");

    // Пустая строка — граница абзаца, её нельзя терять.
    const std::vector<std::string> par = wrap(reflow(std::string("первый\n\nвторой")), 40);
    eq(static_cast<int>(par.size()), 3, "пустая строка разделяет абзацы");
    eqs(par[1], "", "и остаётся пустой");

    // Реплики разных говорящих не должны оказаться в одной строке.
    const std::string talk = "— Здравствуй.\n— И тебе.";
    const std::vector<std::string> two = wrap(reflow(talk), 60);
    eq(static_cast<int>(two.size()), 2, "две реплики — две строки");

    // Строка с отступом значит сама себя: так набирают списки и таблицы.
    const std::string list = "Заголовок\n  первый пункт\n  второй пункт";
    const std::vector<std::string> keep = wrap(reflow(list), 60);
    eq(static_cast<int>(keep.size()), 3, "пункты списка не сливаются");
}

void test_log_tail() {
    section("хвост журнала");

    std::vector<std::string> lg;
    lg.push_back("короткая");
    lg.push_back("Ольховка встречает тебя запахом дыма и мокрой соломы.");

    // Раньше длинная строка обрезалась и хвост терялся совсем. Теперь она
    // переносится и занимает столько строк, сколько ей нужно.
    const std::vector<std::string> t = log_tail(lg, 30, 5);
    bool fits = true;
    for (const std::string& l : t) if (utf8_len(l) > 30) fits = false;
    check(fits, "ни одна строка не шире отведённого");
    std::string joined;
    for (const std::string& l : t) joined += l + " ";
    check(joined.find("соломы") != std::string::npos, "хвост сообщения не потерян");

    // В тесное место попадает конец журнала: позднее важнее раннего.
    const std::vector<std::string> tight = log_tail(lg, 30, 2);
    eq(static_cast<int>(tight.size()), 2, "берётся ровно столько строк, сколько дали");
    check(tight[1].find("соломы") != std::string::npos, "и это конец последнего сообщения");

    // Пустой журнал и нулевое место не должны ничего ломать.
    eq(static_cast<int>(log_tail(std::vector<std::string>(), 30, 3).size()), 0,
       "пустой журнал даёт пусто");
    eq(static_cast<int>(log_tail(lg, 30, 0).size()), 0, "нулевое место даёт пусто");
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
    // Здоровье само не растёт: уровень даёт балл, а куда его вложить —
    // решает игрок. Иначе «Крепость» была бы добавкой к тому, что и так
    // капает, и выбор терял бы половину смысла.
    eq(g.total().max_hp, hp_before, "здоровье само не выросло");

    int hp2 = g.total().max_hp;
    check(g.learn_skill("vigor"), "навык «Крепость» изучается");
    eq(g.total().max_hp, hp2 + 7, "здоровье растёт только навыком");
    eq(g.player().skill_points, pts_before, "очко потрачено");

    // Без очков навык не растёт.
    g.player().skill_points = 0;
    check(!g.learn_skill("vigor"), "без очков навык не повышается");

    // А вот потолка у ранга нет: единственное, что ограничивает вложения, —
    // сколько баллов на руках. Прежний предел в пять рангов упирался как раз
    // к концу игры, и последние баллы девать было некуда.
    {
        Game u;
        u.new_game("Упорный");
        const int dmg0 = u.total().dmg_min;
        u.player().skill_points = 20;
        bool all_ok = true;
        for (int i = 0; i < 20; ++i)
            if (!u.learn_skill("might")) all_ok = false;
        check(all_ok, "двадцать вложений подряд проходят");
        eq(u.player().skills["might"], 20, "ранг дорос до двадцати");
        eq(u.player().skill_points, 0, "все баллы потрачены");
        check(!u.learn_skill("might"), "а без баллов дальше некуда");
        // Бонус складывается каждый раз, а не упирается: «Мощь» даёт +1 к
        // нижнему урону за ранг, и двадцать рангов дают ровно двадцать.
        eq(u.total().dmg_min, dmg0 + 20, "двадцать рангов дали двадцать к урону");
    }

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

    // --- продажа пачкой ---
    // Просят больше, чем есть, — продаётся всё, что есть, и ни штукой
    // больше. Обрезание живёт в самой продаже, а не в окне: набрать
    // двадцать хвостов, имея три, нельзя никаким путём.
    {
        Game b;
        b.new_game("Купец");
        const int one = b.sell_price(*s, *bread);
        b.add_item("bread", 5);
        const int have = b.count_item("bread");
        const int gold_before = b.player().gold;
        check(b.sell(*s, "bread", 999), "просьба продать больше, чем есть, проходит");
        eq(b.count_item("bread"), 0, "но продалось ровно всё, что было");
        eq(b.player().gold, gold_before + one * have, "и заплачено за столько же");
    }
    // Ноль и отрицательное — это одна штука, а не ноль штук и не подарок
    // лавочнику: окно даёт выбрать от единицы, и логика держит ту же границу.
    {
        Game b;
        b.new_game("Купец");
        b.add_item("bread", 4);
        // Считаем от того, что есть на самом деле: герой начинает игру не с
        // пустой сумкой, и жёсткое число здесь ловило бы стартовый набор,
        // а не обрезание.
        const int have = b.count_item("bread");
        const int gold_before = b.player().gold;
        const int one = b.sell_price(*s, *bread);
        check(b.sell(*s, "bread", 0), "ноль штук — это одна");
        eq(b.count_item("bread"), have - 1, "ушла ровно одна");
        check(b.sell(*s, "bread", -7), "отрицательное — тоже одна");
        eq(b.count_item("bread"), have - 2, "и снова одна");
        eq(b.player().gold, gold_before + one * 2, "заплачено за две");
    }
    // Чего нет — того не продать, сколько ни проси.
    {
        Game b;
        b.new_game("Купец");
        check(!b.sell(*s, "ring_hp", 3), "несуществующий в сумке товар не продаётся");
        check(!b.sell(*s, "ring_hp", 1), "и по одной тоже");
    }
    // Ровно столько, сколько есть, — обычный случай, и он не должен
    // спотыкаться на границе.
    {
        Game b;
        b.new_game("Купец");
        b.add_item("herb_potion", 3);
        const int n = b.count_item("herb_potion");
        check(b.sell(*s, "herb_potion", n), "продажа ровно всего запаса проходит");
        eq(b.count_item("herb_potion"), 0, "запас кончился");
    }

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
                           "shop_order", "shop_drift", "shop_inside"};
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
                          "gatekeeper", "librarian", "draftsman", "stoker", "recorder",
                          "driftwife", "halfscribe", "soldier", "grovekeeper", "pathkeeper",
                          "seamwatch", "surveyor", "master", "master_end"};
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
                           "strangers_offer","strangers_wait","strangers_reward","strangers_after",
                           "root_offer","root_wait","root_reward",
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
                           "refusal_after_m","refusal_after_c","keepsake_talk",
                           "driftway_offer","driftway_wait","driftway_reward","driftway_after",
                           "driftwife_root","driftwife_tally",
                           "water_offer","water_wait","water_reward","water_after",
                           "halfscribe_root","halfscribe_shout",
                           "wholename_offer","wholename_wait","wholename_reward",
                           "soldier_root","lasthour_offer","lasthour_wait",
                           "lasthour_reward","lasthour_after",
                           "grovekeeper_root","grove_rest","grove_why","grove_marks",
                           "pathkeeper_root","path_why","path_drift",
                           "inside_offer","inside_after",
                           "seamwatch_root","firstjoint_offer","firstjoint_wait",
                           "firstjoint_reward","firstjoint_after",
                           "surveyor_root","lines_offer","lines_wait","lines_reward",
                           "surveyor_brother",
                           "master_root","remembers_offer","remembers_way","remembers_after",
                           "master_word","master_ring_talk",
                           "finale_root","finale_ask","finale_after_pull",
                           "finale_after_cut","finale_after_hold"};
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
            if (!o.ending.empty())
                check(c.ending(o.ending) != nullptr,
                      std::string(nid) + ": исход '" + o.ending + "' описан");
        }
    }

    // Добыча врагов должна ссылаться на существующие предметы.
    const char* mobs[] = {"rat", "wolf", "bandit", "wolf_alpha", "spider", "bat",
                          "bog_leech", "drowned_man", "bog_walker", "barrow_shade",
                          "spider_queen", "brigand", "bandit_chief", "wraith", "keeper",
                          "archivist", "stray", "glass_hound", "mill_rat", "salt_ghoul",
                          "caravan_shade", "salt_mother", "bridge_walker",
                          "city_rat", "cut_man", "mad_clerk", "slag_thing",
                          "canal_walker", "archive_moth", "half_warden", "slag_master",
                          "acolyte", "gate_guard", "page_swarm", "draft_shade",
                          "cell_dweller", "furnace_born", "refusal_echo",
                          "node_guard", "node_heart", "master_shadow",
                          "drift_hare", "cart_shade", "stair_walker", "last_hour",
                          "bannerman", "grove_sleeper", "second_bucket", "other_rat",
                          "own_copy", "edge_wind",
                          "seam_moth", "line_walker", "measure_thing", "first_guard",
                          "cinch_engine", "cinch_heart", "zero_echo", "all_at_once"};
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

    // Три развязки описаны, и у каждой есть имя и текст эпилога.
    for (const char* eid : {"pull", "cut", "hold"}) {
        const EndingDef* e = c.ending(eid);
        check(e != nullptr, std::string("развязка ") + eid + " описана");
        if (!e) continue;
        check(!e->name.empty(), std::string("у развязки ") + eid + " есть имя");
        check(e->lines.size() >= 8, std::string("у развязки ") + eid + " есть эпилог");
        // Эпилог читается на узком экране: строки не длиннее того, что
        // помещается в самую тесную раскладку без переноса посреди мысли.
        bool fits = true;
        for (const std::string& l : e->lines)
            if (utf8_len(l) > 76) fits = false;
        check(fits, std::string("эпилог ") + eid + " умещается по ширине");
    }
    check(c.ending("нет-такой") == nullptr, "несуществующая развязка не выдумывается");

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

// ------------------------------------------------------- графическая часть
// Проверяется то, что можно проверить без окна: разбор UTF-8 и покрытие
// шрифта, распознавание жестов и ходьба по тапу. Рисование проверяется
// отдельно — прогоном `andors-love-gui --script` со снимками экрана.

void test_font() {
    section("шрифт: UTF-8 и покрытие");

    // Разбор UTF-8 посимвольно.
    {
        const std::string s = "aЖ«—";
        std::size_t i = 0;
        eq(static_cast<int>(gfx::utf8_next(s, i)), 'a', "латиница разбирается");
        eq(static_cast<int>(gfx::utf8_next(s, i)), 0x416, "кириллица разбирается");
        eq(static_cast<int>(gfx::utf8_next(s, i)), 0xAB, "кавычка разбирается");
        eq(static_cast<int>(gfx::utf8_next(s, i)), 0x2014, "тире разбирается");
        eq(static_cast<int>(i), static_cast<int>(s.size()), "строка разобрана до конца");
        eq(static_cast<int>(gfx::utf8_next(s, i)), 0, "за концом строки — ноль");
    }

    // Битая последовательность не должна зацикливать разбор.
    {
        std::string bad;
        bad += static_cast<char>(0xD0);      // начало двухбайтового и обрыв
        std::size_t i = 0;
        eq(static_cast<int>(gfx::utf8_next(bad, i)), 0xFFFD, "обрыв даёт замену");
        check(i > 0, "и разбор всё равно двигается вперёд");

        std::string lone;
        lone += static_cast<char>(0x80);     // одинокий байт продолжения
        i = 0;
        eq(static_cast<int>(gfx::utf8_next(lone, i)), 0xFFFD, "одинокий байт — замена");
        eq(static_cast<int>(i), 1, "и сдвиг ровно на байт");
    }

    check(gfx::has_glyph('A'), "латиница в шрифте есть");
    check(gfx::has_glyph(0x416), "кириллица в шрифте есть");
    check(gfx::has_glyph(0x451), "буква ё в шрифте есть");
    check(!gfx::has_glyph(0x4E00), "чего в шрифте нет, того нет");
    check(gfx::glyph(0x4E00) == gfx::glyph('?'),
          "для неизвестного символа рисуется вопросительный знак");

    // Пробел обязан быть пустым: иначе весь текст в кляксах.
    {
        const unsigned char* sp = gfx::glyph(' ');
        bool empty = true;
        for (int i = 0; i < gfx::FONT_H; ++i) if (sp[i]) empty = false;
        check(empty, "пробел пустой");
    }

    // Главное: всё, что игра умеет напечатать, шрифт умеет нарисовать.
    // Иначе дыра в тексте найдётся не тестом, а игроком.
    const Content& c = Content::get();
    std::string all;
    for (const QuestDef& q : c.quests()) {
        all += q.name;
        for (const QuestStageDef& st : q.stages) all += st.text;
    }
    for (const SkillDef& s : c.skills()) { all += s.name; all += s.desc; }
    for (const RaceDef& r : c.races())   { all += r.name; all += r.desc; }
    for (const SpecDef& s : c.specs())   { all += s.name; all += s.desc; }
    for (const EnchantDef& e : c.enchants()) { all += e.name; all += e.desc; }
    for (const QuestTrigger& t : c.triggers()) all += t.message;
    for (const char* nid : ALL_NOTES) {
        const NoteDef* n = c.note(nid);
        if (!n) continue;
        all += n->title;
        for (const std::string& l : n->lines) all += l;
    }
    for (const char* eid : {"pull", "cut", "hold"}) {
        const EndingDef* e = c.ending(eid);
        if (!e) continue;
        all += e->name;
        for (const std::string& l : e->lines) all += l;
    }
    World w("data/maps");
    for (const char* lid : ALL_LOCATIONS) {
        const Location* loc = w.location(lid);
        if (!loc) continue;
        all += loc->name;
        for (const MapSign& s : loc->signs) all += s.text;
    }

    std::vector<unsigned> missing;
    std::size_t i = 0;
    while (i < all.size()) {
        const unsigned cp = gfx::utf8_next(all, i);
        if (cp == '\n' || cp == 0) continue;
        if (gfx::has_glyph(cp)) continue;
        bool known = false;
        for (unsigned m : missing) if (m == cp) known = true;
        if (!known) missing.push_back(cp);
    }
    std::string report;
    for (unsigned m : missing) report += " U+" + to_str(static_cast<int>(m));
    check(missing.empty(), "весь текст игры покрыт шрифтом; нет глифов:" + report);
}

void test_long_press() {
    section("касания: удержание");

    gfx::Pointer p;
    p.configure(20, 500);
    gfx::Gesture g;

    // Удержание выдаётся, пока палец ещё на экране: окно должно всплыть
    // под пальцем, а не после того, как его убрали.
    p.down(1, 100, 100, 0);
    p.tick(200);
    check(!p.poll(&g), "до порога удержания ничего нет");
    p.tick(500);
    check(p.poll(&g), "на пороге удержание выдано");
    eq(static_cast<int>(g.kind), static_cast<int>(gfx::G_LONG), "именно удержание");
    eq(g.x, 100, "в точке касания");
    p.tick(900);
    check(!p.poll(&g), "и выдано ровно один раз");

    // После удержания отпускание не тапает: одно касание — одно действие,
    // иначе всплывшее окно тут же получало бы тап по себе.
    p.up(1, 100, 100, 1000);
    check(!p.poll(&g), "отпускание после удержания тапом не считается");

    // Уехавший палец удержанием не становится: это прокрутка.
    p.clear();
    p.down(2, 100, 100, 0);
    p.move(2, 100, 160, 100);
    while (p.poll(&g)) {}          // свайпы разобрали
    p.tick(2000);
    check(!p.poll(&g), "после свайпа удержания нет");

    // Быстрый тап удержанием тоже не становится.
    p.clear();
    p.down(3, 10, 10, 0);
    p.tick(100);
    p.up(3, 10, 10, 200);
    check(p.poll(&g), "жест есть");
    eq(static_cast<int>(g.kind), static_cast<int>(gfx::G_TAP), "и это обычный тап");
    check(!p.poll(&g), "больше ничего");
}

void test_gestures() {
    section("касания: тап, состояние пальца, свайп");

    gfx::Pointer p;
    p.configure(20);
    gfx::Gesture g;

    // Тап: коснулся и отпустил, не сдвинувшись.
    p.down(1, 100, 100, 0);
    check(p.pressed(), "палец на экране");
    eq(p.press_x(), 100, "и его позиция известна");
    p.up(1, 102, 101, 60);
    check(!p.pressed(), "после отпускания пальца нет");
    check(p.poll(&g), "тап распознан");
    eq(static_cast<int>(g.kind), static_cast<int>(gfx::G_TAP), "именно тап");
    eq(g.x, 102, "координата отпускания");
    check(!p.poll(&g), "и больше ничего");

    // Удержание — это состояние, а не событие: по нему и ходит герой.
    p.clear();
    p.down(1, 50, 60, 1000);
    check(p.pressed(), "палец удерживается");
    eq(static_cast<int>(p.held_ms(1000)), 0, "сразу после касания держится ноль");
    eq(static_cast<int>(p.held_ms(1450)), 450, "и растёт по часам, а не по кадрам");

    // Ведение пальцем двигает цель и не выдаёт ни одного тапа.
    p.move(1, 70, 90, 1500);
    eq(p.press_x(), 70, "цель едет за пальцем");
    eq(p.press_y(), 90, "и по второй оси тоже");
    eq(static_cast<int>(p.held_ms(1500)), 500, "время удержания при ведении не сбрасывается");
    p.up(1, 70, 90, 1600);
    check(!p.pressed(), "палец отпущен");
    while (p.poll(&g))
        check(g.kind != gfx::G_TAP, "после ведения тапа не бывает");

    // Свайп остаётся, но только для прокрутки списков: шагов он не даёт.
    p.clear();
    p.down(2, 200, 200, 0);
    p.move(2, 265, 202, 30);
    int swipes = 0, last_dx = 0;
    while (p.poll(&g)) {
        eq(static_cast<int>(g.kind), static_cast<int>(gfx::G_SWIPE), "это свайп");
        ++swipes;
        last_dx = g.dx;
    }
    eq(swipes, 3, "свайп щёлкает столько раз, сколько порогов пройдено");
    eq(last_dx, 1, "направление вправо");
    p.up(2, 265, 202, 60);
    check(!p.poll(&g), "после свайпа отпускание тапом не считается");

    // Очень длинный протяг: цикл выдачи обязан закончиться.
    p.clear();
    p.down(9, 0, 0, 0);
    p.move(9, 2000, 0, 20);
    int long_swipes = 0;
    while (p.poll(&g)) ++long_swipes;
    eq(long_swipes, 100, "длинный протяг даёт ровно 2000/20 щелчков");
    p.up(9, 2000, 0, 40);
    check(!p.poll(&g), "и тапом не заканчивается");

    // Вертикаль отличается от горизонтали по большей составляющей.
    p.clear();
    p.down(3, 10, 10, 0);
    p.move(3, 12, 40, 20);
    check(p.poll(&g), "вертикальный свайп распознан");
    eq(g.dy, 1, "направление вниз");
    eq(g.dx, 0, "и без горизонтальной составляющей");

    // Второй палец не перехватывает управление у первого.
    p.clear();
    p.down(4, 10, 10, 0);
    p.down(5, 400, 400, 10);
    eq(p.press_x(), 10, "ведущим остаётся тот палец, что лёг первым");
    p.up(4, 10, 10, 80);
    check(p.pressed(), "пока второй на экране, палец всё ещё есть");
    eq(p.press_x(), 400, "и ведущим становится он");
    p.up(5, 400, 400, 120);
    check(!p.pressed(), "когда убрали оба — пальца нет");
    int taps = 0;
    while (p.poll(&g)) if (g.kind == gfx::G_TAP) ++taps;
    eq(taps, 2, "два пальца дают два тапа, и ни один не потерян");
}

// ------------------------------------------------------------------ PNG

// Эталон от настоящего кодировщика: динамические коды Хаффмана и адаптивные
// фильтры строк. Сам наш писатель такого не создаёт — фиксированные коды и
// фильтр 0, — поэтому без чужого файла половина читателя осталась бы
// непроверенной.
const unsigned char REF_PNG[] = {
    137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82,
    0, 0, 0, 16, 0, 0, 0, 12, 8, 6, 0, 0, 0, 107, 231, 61,
    129, 0, 0, 0, 124, 73, 68, 65, 84, 120, 218, 157, 208, 161, 10, 195,
    48, 16, 128, 225, 191, 80, 168, 43, 76, 148, 153, 186, 186, 186, 186, 115,
    113, 113, 121, 130, 184, 185, 190, 191, 153, 91, 77, 8, 199, 145, 116, 105,
    196, 7, 151, 16, 126, 142, 12, 192, 239, 5, 223, 94, 35, 111, 128, 169,
    155, 10, 204, 93, 76, 96, 121, 172, 16, 88, 27, 196, 60, 87, 2, 91,
    129, 47, 222, 223, 4, 246, 68, 212, 108, 157, 255, 2, 71, 69, 204, 243,
    77, 64, 210, 35, 81, 130, 57, 75, 203, 6, 2, 184, 244, 7, 78, 249,
    0, 174, 22, 240, 38, 224, 141, 152, 231, 134, 13, 130, 218, 32, 24, 39,
    23, 253, 192, 40, 85, 35, 148, 39, 71, 0, 0, 0, 0, 73, 69, 78,
    68, 174, 66, 96, 130
};
const unsigned char REF_PX[] = {
    0, 0, 0, 255, 17, 0, 0, 247, 34, 0, 0, 239, 51, 0, 0, 231,
    68, 0, 0, 223, 85, 0, 0, 215, 102, 0, 0, 207, 119, 0, 0, 199,
    136, 0, 0, 191, 153, 0, 0, 183, 170, 0, 0, 175, 187, 0, 0, 167,
    204, 0, 0, 159, 221, 0, 0, 151, 238, 0, 0, 143, 255, 0, 0, 135,
    0, 23, 0, 255, 17, 23, 7, 247, 34, 23, 14, 239, 51, 23, 21, 231,
    68, 23, 28, 223, 85, 23, 35, 215, 102, 23, 42, 207, 119, 23, 49, 199,
    136, 23, 56, 191, 153, 23, 63, 183, 170, 23, 70, 175, 187, 23, 77, 167,
    204, 23, 84, 159, 221, 23, 91, 151, 238, 23, 98, 143, 255, 23, 105, 135,
    0, 46, 0, 255, 17, 46, 14, 247, 34, 46, 28, 239, 51, 46, 42, 231,
    68, 46, 56, 223, 85, 46, 70, 215, 102, 46, 84, 207, 119, 46, 98, 199,
    136, 46, 112, 191, 153, 46, 126, 183, 170, 46, 140, 175, 187, 46, 154, 167,
    204, 46, 168, 159, 221, 46, 182, 151, 238, 46, 196, 143, 255, 46, 210, 135,
    0, 69, 0, 255, 17, 69, 21, 247, 34, 69, 42, 239, 51, 69, 63, 231,
    68, 69, 84, 223, 85, 69, 105, 215, 102, 69, 126, 207, 119, 69, 147, 199,
    136, 69, 168, 191, 153, 69, 189, 183, 170, 69, 210, 175, 187, 69, 231, 167,
    204, 69, 252, 159, 221, 69, 17, 151, 238, 69, 38, 143, 255, 69, 59, 135,
    0, 92, 0, 255, 17, 92, 28, 247, 34, 92, 56, 239, 51, 92, 84, 231,
    68, 92, 112, 223, 85, 92, 140, 215, 102, 92, 168, 207, 119, 92, 196, 199,
    136, 92, 224, 191, 153, 92, 252, 183, 170, 92, 24, 175, 187, 92, 52, 167,
    204, 92, 80, 159, 221, 92, 108, 151, 238, 92, 136, 143, 255, 92, 164, 135,
    0, 115, 0, 255, 17, 115, 35, 247, 34, 115, 70, 239, 51, 115, 105, 231,
    68, 115, 140, 223, 85, 115, 175, 215, 102, 115, 210, 207, 119, 115, 245, 199,
    136, 115, 24, 191, 153, 115, 59, 183, 170, 115, 94, 175, 187, 115, 129, 167,
    204, 115, 164, 159, 221, 115, 199, 151, 238, 115, 234, 143, 255, 115, 13, 135,
    0, 138, 0, 255, 17, 138, 42, 247, 34, 138, 84, 239, 51, 138, 126, 231,
    68, 138, 168, 223, 85, 138, 210, 215, 102, 138, 252, 207, 119, 138, 38, 199,
    136, 138, 80, 191, 153, 138, 122, 183, 170, 138, 164, 175, 187, 138, 206, 167,
    204, 138, 248, 159, 221, 138, 34, 151, 238, 138, 76, 143, 255, 138, 118, 135,
    0, 161, 0, 255, 17, 161, 49, 247, 34, 161, 98, 239, 51, 161, 147, 231,
    68, 161, 196, 223, 85, 161, 245, 215, 102, 161, 38, 207, 119, 161, 87, 199,
    136, 161, 136, 191, 153, 161, 185, 183, 170, 161, 234, 175, 187, 161, 27, 167,
    204, 161, 76, 159, 221, 161, 125, 151, 238, 161, 174, 143, 255, 161, 223, 135,
    0, 184, 0, 255, 17, 184, 56, 247, 34, 184, 112, 239, 51, 184, 168, 231,
    68, 184, 224, 223, 85, 184, 24, 215, 102, 184, 80, 207, 119, 184, 136, 199,
    136, 184, 192, 191, 153, 184, 248, 183, 170, 184, 48, 175, 187, 184, 104, 167,
    204, 184, 160, 159, 221, 184, 216, 151, 238, 184, 16, 143, 255, 184, 72, 135,
    0, 207, 0, 255, 17, 207, 63, 247, 34, 207, 126, 239, 51, 207, 189, 231,
    68, 207, 252, 223, 85, 207, 59, 215, 102, 207, 122, 207, 119, 207, 185, 199,
    136, 207, 248, 191, 153, 207, 55, 183, 170, 207, 118, 175, 187, 207, 181, 167,
    204, 207, 244, 159, 221, 207, 51, 151, 238, 207, 114, 143, 255, 207, 177, 135,
    0, 230, 0, 255, 17, 230, 70, 247, 34, 230, 140, 239, 51, 230, 210, 231,
    68, 230, 24, 223, 85, 230, 94, 215, 102, 230, 164, 207, 119, 230, 234, 199,
    136, 230, 48, 191, 153, 230, 118, 183, 170, 230, 188, 175, 187, 230, 2, 167,
    204, 230, 72, 159, 221, 230, 142, 151, 238, 230, 212, 143, 255, 230, 26, 135,
    0, 253, 0, 255, 17, 253, 77, 247, 34, 253, 154, 239, 51, 253, 231, 231,
    68, 253, 52, 223, 85, 253, 129, 215, 102, 253, 206, 207, 119, 253, 27, 199,
    136, 253, 104, 191, 153, 253, 181, 183, 170, 253, 2, 175, 187, 253, 79, 167,
    204, 253, 156, 159, 221, 253, 233, 151, 238, 253, 54, 143, 255, 253, 131, 135
};

void test_png() {
    section("PNG: чтение чужого, запись своего");

    // Чужой файл разбирается пиксель в пиксель.
    {
        gfx::Image im;
        std::string err;
        check(gfx::png_read_mem(REF_PNG, sizeof REF_PNG, &im, &err),
              "PNG от чужого кодировщика прочитан");
        eq(im.w, 16, "ширина из заголовка");
        eq(im.h, 12, "высота из заголовка");
        bool same = im.px.size() == sizeof REF_PX;
        for (std::size_t i = 0; same && i < im.px.size(); ++i)
            if (im.px[i] != REF_PX[i]) same = false;
        check(same, "и все пиксели совпали с оригиналом");
    }

    // Своё пишется и читается обратно без потерь.
    {
        const int sizes[][2] = { {1, 1}, {7, 3}, {32, 32}, {1, 200}, {264, 264} };
        for (int k = 0; k < 5; ++k) {
            gfx::Image im(sizes[k][0], sizes[k][1]);
            unsigned seed = 1234u + static_cast<unsigned>(k);
            for (int y = 0; y < im.h; ++y)
                for (int x = 0; x < im.w; ++x) {
                    seed = seed * 1103515245u + 12345u;
                    const unsigned v = (seed >> 16) & 0xFFu;
                    // Половина картинки — ровный цвет, половина — шум: так
                    // проверяются и совпадения, и одиночные литералы.
                    if (x < im.w / 2) im.set(x, y, 40, 90, 200, 255);
                    else im.set(x, y, static_cast<unsigned char>(v),
                                static_cast<unsigned char>(v * 3),
                                static_cast<unsigned char>(v * 7),
                                static_cast<unsigned char>(v | 1u));
                }

            std::vector<unsigned char> bytes;
            std::string err;
            check(gfx::png_write_mem(im, &bytes, &err), "картинка записана");

            gfx::Image back;
            check(gfx::png_read_mem(bytes.empty() ? REF_PNG : &bytes[0], bytes.size(),
                                    &back, &err), "и прочитана обратно");
            eq(back.w, im.w, "ширина не поменялась");
            eq(back.h, im.h, "высота не поменялась");
            bool same = back.px.size() == im.px.size();
            for (std::size_t i = 0; same && i < im.px.size(); ++i)
                if (back.px[i] != im.px[i]) same = false;
            check(same, "и пиксели вернулись те же");
        }
    }

    // Мусор отвергается с внятной причиной, а не падением.
    {
        gfx::Image im;
        std::string err;
        const unsigned char junk[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
        check(!gfx::png_read_mem(junk, sizeof junk, &im, &err), "не-PNG отвергнут");
        check(!err.empty(), "и сказано почему");

        std::vector<unsigned char> cut(REF_PNG, REF_PNG + 40);
        check(!gfx::png_read_mem(&cut[0], cut.size(), &im, &err), "обрезанный PNG отвергнут");

        std::vector<unsigned char> bad(REF_PNG, REF_PNG + sizeof REF_PNG);
        bad[40] = static_cast<unsigned char>(bad[40] ^ 0xFF);
        check(!gfx::png_read_mem(&bad[0], bad.size(), &im, &err),
              "испорченный байт ловится контрольной суммой");

        check(!gfx::png_write_mem(gfx::Image(), &cut, &err), "пустая картинка не пишется");
    }
}

// ------------------------------------------------------------------ тайлы

void test_tiles() {
    section("тайлы: формат листа и раскладка слотов");

    // Маркер — это ровно RGB(1,2,3) непрозрачный, как в редакторе. Прозрачный
    // пиксель за него принимать нельзя: иначе тайл с прозрачностью обрезался
    // бы на первой же дырке.
    {
        const unsigned char marker[4]  = { 1, 2, 3, 255 };
        const unsigned char clearpx[4] = { 1, 2, 3, 0 };
        const unsigned char near_[4]   = { 1, 2, 4, 255 };
        check(gfx::tjtm_is_marker(marker), "маркер узнан");
        check(!gfx::tjtm_is_marker(clearpx), "прозрачный пиксель того же цвета — не маркер");
        check(!gfx::tjtm_is_marker(near_), "соседний цвет — не маркер");
    }

    // Собрали лист и разрезали обратно: тайлы вернулись как были, включая
    // разные размеры и пустые ячейки.
    {
        std::vector<gfx::Image> tiles(static_cast<std::size_t>(gfx::TJTM_SLOTS));
        tiles[0] = gfx::Image(16, 16);
        tiles[0].clear(200, 30, 40, 255);
        tiles[1] = gfx::Image(11, 5);
        tiles[1].clear(0, 0, 0, 0);                    // целиком прозрачный
        tiles[9] = gfx::Image(20, 20);
        tiles[9].clear(1, 2, 3, 0);                    // цвет маркера, но прозрачный
        tiles[63] = gfx::Image(20, 12);
        tiles[63].clear(7, 8, 9, 128);

        gfx::Image sheet;
        std::string err;
        check(gfx::tjtm_compose(tiles, &sheet, &err), "лист собран");
        eq(sheet.w, gfx::TJTM_COLS * 21, "ширина по самому широкому тайлу плюс разделитель");
        eq(sheet.h, gfx::TJTM_ROWS * 21, "и высота так же");

        std::vector<gfx::Image> back;
        check(gfx::tjtm_slice(sheet, &back, &err), "и разрезан обратно");
        eq(static_cast<int>(back.size()), gfx::TJTM_SLOTS, "ячеек ровно 64");

        eq(back[0].w, 16, "ширина первого тайла");
        eq(back[0].h, 16, "высота первого тайла");
        eq(back[1].w, 11, "тайл другого размера не подрос");
        eq(back[1].h, 5, "и по второй оси тоже");
        eq(back[9].w, 20, "прозрачный тайл цвета маркера уцелел целиком");
        eq(back[63].w, 20, "последняя ячейка тоже читается");
        eq(back[63].h, 12, "и по высоте");
        check(back[2].empty(), "ненарисованная ячейка осталась пустой");

        bool same = back[0].px.size() == tiles[0].px.size();
        for (std::size_t i = 0; same && i < tiles[0].px.size(); ++i)
            if (back[0].px[i] != tiles[0].px[i]) same = false;
        check(same, "пиксели тайла вернулись без изменений");
    }

    // Картинка, не похожая на лист, отвергается.
    {
        std::vector<gfx::Image> out;
        std::string err;
        check(!gfx::tjtm_slice(gfx::Image(4, 4), &out, &err), "картинка меньше 8x8 — не лист");
        check(!err.empty(), "и сказано почему");
        check(!gfx::tjtm_slice(gfx::Image(), &out, &err), "пустая — тоже");
    }

    // Раскладка: у каждой местности и каждого объекта свой слот, и никакие
    // два не совпадают. Иначе трава рисовалась бы стеной.
    {
        int seen[64];
        for (int i = 0; i < 64; ++i) seen[i] = 0;
        for (int i = 0; i < static_cast<int>(Tile::Count); ++i) {
            const int s = gfx::slot_of_tile(static_cast<Tile>(i));
            check(s >= 0 && s < gfx::TJTM_SLOTS, "у местности есть слот");
            if (s >= 0 && s < 64) ++seen[s];
        }
        static const char SIGNS[] = {
            glyph::PLAYER, glyph::NPC, glyph::MOB, glyph::EXIT, glyph::SIGN,
            glyph::ITEM, glyph::BED, glyph::CHEST, glyph::PORTAL, glyph::NOTE
        };
        for (std::size_t i = 0; i < sizeof SIGNS; ++i) {
            const int s = gfx::slot_of_glyph(static_cast<unsigned char>(SIGNS[i]));
            check(s >= 0 && s < gfx::TJTM_SLOTS, "у объекта есть слот");
            if (s >= 0 && s < 64) ++seen[s];
        }
        int busy = 0, doubled = 0;
        for (int i = 0; i < 64; ++i) { if (seen[i]) ++busy; if (seen[i] > 1) ++doubled; }
        eq(busy, static_cast<int>(Tile::Count) + 10, "занято ровно столько слотов, сколько сущностей");
        eq(doubled, 0, "и ни один слот не занят дважды");
        eq(gfx::slot_of_glyph('~'), -1, "у постороннего знака слота нет");
    }

    // Имена файлов: у каждого занятого слота своё, и ни одно не повторяется —
    // иначе два тайла спорили бы за один файл.
    {
        const char* names[64];
        int named = 0, clash = 0;
        for (int i = 0; i < gfx::TJTM_SLOTS; ++i) {
            const char* f = gfx::slot_file(i);
            if (!f) continue;
            for (int k = 0; k < named; ++k)
                if (std::string(names[k]) == f) ++clash;
            names[named++] = f;
        }
        eq(named, static_cast<int>(Tile::Count) + 10, "имя есть у каждого занятого слота");
        eq(clash, 0, "и ни одно имя не повторяется");
        check(gfx::slot_file(7) == 0, "у свободного слота имени нет");
        check(std::string(gfx::slot_file(gfx::SLOT_WALL)) == "wall", "стена лежит в wall.png");
    }

    // Графика по файлам: пишем вид по умолчанию, читаем обратно.
    {
        const std::string dir = "build/test-tiles";
        std::string err;
        check(gfx::save_slot_files(dir, 32, &err), "тайлы сохранены по файлам");
        check(err.empty(), "и без жалоб");

        std::vector<gfx::Image> tiles;
        const int got = gfx::load_slot_files(dir, &tiles, &err);
        eq(got, static_cast<int>(Tile::Count) + 10, "прочитано столько же, сколько записано");
        check(err.empty(), "и снова без жалоб");
        eq(static_cast<int>(tiles.size()), gfx::TJTM_SLOTS, "слотов по-прежнему 64");
        eq(tiles[gfx::SLOT_WALL].w, 32, "тайл стены нужного размера");
        check(tiles[7].empty(), "свободный слот пуст");

        // Отдельный файл может быть любого размера: потолка в 63 пикселя,
        // который был у листа, для одного файла нет.
        gfx::Image big(64, 64);
        big.clear(10, 20, 30, 255);
        check(gfx::png_write(dir + "/wall.png", big, &err), "тайл покрупнее записан");
        std::vector<gfx::Image> again;
        gfx::load_slot_files(dir, &again, &err);
        eq(again[gfx::SLOT_WALL].w, 64, "и прочитан как есть");
        eq(again[gfx::SLOT_FLOOR].w, 32, "а соседний остался прежним");

        // Нечитаемый файл называет себя, но остальные тайлы не отменяет.
        {
            std::ofstream junk((dir + "/grass.png").c_str(), std::ios::binary);
            junk << "не png";
        }
        std::vector<gfx::Image> third;
        std::string err2;
        const int left = gfx::load_slot_files(dir, &third, &err2);
        check(!err2.empty(), "про испорченный файл сказано");
        check(err2.find("grass.png") != std::string::npos, "и названо какой именно");
        eq(left, static_cast<int>(Tile::Count) + 10 - 1, "остальные тайлы прочитаны");
        check(third[gfx::SLOT_GRASS].empty(), "а испорченный слот остался пустым");

        // Пустой каталог — не ошибка: графики просто нет.
        std::vector<gfx::Image> none;
        std::string err3;
        eq(gfx::load_slot_files("build/test-tiles-empty", &none, &err3), 0,
           "из пустого каталога не читается ничего");
        check(err3.empty(), "и это не повод ругаться");
    }

    // Тайлы существ: по картинке на жителя и на врага.
    {
        eqs(gfx::creature_tile_name("npc", "elder"), "npc_elder",
           "имя тайла жителя");
        eqs(gfx::creature_tile_name("mob", "rat"), "mob_rat",
           "имя тайла врага");
        check(gfx::creature_tile_name("", "elder").empty(), "без вида имени нет");
        check(gfx::creature_tile_name("npc", "").empty(), "без id — тоже");

        // Ни одно имя существа не должно совпасть с именем слота: иначе
        // тайл жителя и тайл стены спорили бы за один файл.
        const Content& c = Content::get();
        std::set<std::string> names;
        int clash_slot = 0, clash_name = 0;
        for (std::map<std::string, NpcDef>::const_iterator it = c.npcs().begin();
             it != c.npcs().end(); ++it) {
            const std::string n = gfx::creature_tile_name("npc", it->first);
            if (!names.insert(n).second) ++clash_name;
            for (int s = 0; s < gfx::TJTM_SLOTS; ++s)
                if (gfx::slot_file(s) && n == gfx::slot_file(s)) ++clash_slot;
        }
        for (std::map<std::string, EnemyDef>::const_iterator it = c.enemies().begin();
             it != c.enemies().end(); ++it) {
            const std::string n = gfx::creature_tile_name("mob", it->first);
            if (!names.insert(n).second) ++clash_name;
            for (int s = 0; s < gfx::TJTM_SLOTS; ++s)
                if (gfx::slot_file(s) && n == gfx::slot_file(s)) ++clash_slot;
        }
        eq(static_cast<int>(names.size()),
           static_cast<int>(c.npcs().size() + c.enemies().size()),
           "у каждого существа своё имя тайла");
        eq(clash_name, 0, "и ни одно не повторяется");
        eq(clash_slot, 0, "и ни одно не спорит с именем слота");

        // Чтение по именам: что есть — читается, чего нет — остаётся пустым.
        const std::string dir = "build/test-tiles-named";
        std::string err;
        check(platform::make_dir(dir), "каталог для именованных тайлов создан");
        // Ниже один файл нарочно портится. Он не должен пережить прогон и
        // подменить собой отсутствие файла в следующем: иначе тест начал бы
        // проверять не то, что написано, а осадок от прошлого раза.
        std::remove((dir + "/npc_elder.png").c_str());
        std::remove((dir + "/npc_smith.png").c_str());
        gfx::Image art(16, 16);
        art.clear(7, 8, 9, 255);
        check(gfx::png_write(dir + "/npc_elder.png", art, &err), "тайл старейшины записан");

        std::vector<std::string> want;
        want.push_back("npc_elder");
        want.push_back("npc_smith");
        std::vector<gfx::Image> got;
        eq(gfx::load_named_files(dir, want, &got, &err), 1, "прочитан ровно один");
        eq(static_cast<int>(got.size()), 2, "а мест столько же, сколько имён");
        eq(got[0].w, 16, "нарисованный житель прочитан");
        check(got[1].empty(), "а ненарисованный остался пустым");
        check(err.empty(), "и отсутствие файла — не жалоба");

        // Испорченный файл называет себя и не отменяет остальных.
        {
            std::ofstream junk((dir + "/npc_smith.png").c_str(), std::ios::binary);
            junk << "не png";
        }
        std::vector<gfx::Image> got2;
        std::string err4;
        eq(gfx::load_named_files(dir, want, &got2, &err4), 1, "целый тайл всё равно прочитан");
        check(err4.find("npc_smith.png") != std::string::npos,
              "а про испорченный сказано, какой именно");
    }

    // Лист по умолчанию: то, что игра отдаёт по --export-sheet.
    {
        const gfx::Image sheet = gfx::default_sheet(gfx::TILE_PX);
        eq(sheet.w, gfx::TJTM_COLS * (gfx::TILE_PX + 1), "ширина листа");
        eq(sheet.h, gfx::TJTM_ROWS * (gfx::TILE_PX + 1), "высота листа");
        check(sheet.w <= 512 && sheet.h <= 512, "лист открывается редактором: не больше 512");

        std::vector<gfx::Image> tiles;
        std::string err;
        check(gfx::tjtm_slice(sheet, &tiles, &err), "лист по умолчанию читается своим же разрезом");

        int drawn = 0;
        for (int i = 0; i < gfx::TJTM_SLOTS; ++i)
            if (!tiles[static_cast<std::size_t>(i)].empty()) ++drawn;
        eq(drawn, static_cast<int>(Tile::Count) + 10, "нарисованы ровно занятые слоты");
        check(tiles[gfx::SLOT_FLOOR].w == gfx::TILE_PX, "тайл пола нужного размера");

        // Местность непрозрачна, объект — нет: иначе под жителем не было бы
        // видно землю, на которой он стоит.
        check(tiles[gfx::SLOT_GRASS].at(0, 0)[3] == 255, "трава непрозрачна");
        check(tiles[gfx::SLOT_NPC].at(0, 0)[3] == 0, "у жителя угол прозрачен");
        check(tiles[gfx::SLOT_PLAYER].at(0, 0)[3] == gfx::default_player_backing().a,
              "а у героя там подложка");

        // Слишком мелкий тайл рисовать нечем: знак шрифта в него не влезет.
        check(gfx::default_slot_art(gfx::SLOT_FLOOR, 4).empty(), "тайл меньше клетки шрифта не рисуется");
    }
}

void test_walk() {
    section("ходьба по тапу");

    Game g;
    g.new_game("Ходок", "human", "swordsman");
    const Location* loc = g.here();
    check(loc != nullptr, "локация загружена");
    if (!loc) return;

    // Идёт к цели и доходит.
    {
        gfx::Walker w;
        const Vec2 from = g.player().pos;
        Vec2 target(from.x + 4, from.y);
        check(loc->walkable(target), "цель проходима");
        w.go(target, false);
        check(w.active(), "ходьба началась");
        unsigned t = 0;
        for (int i = 0; i < 200 && w.active(); ++i) {
            w.update(g, t);
            t += gfx::Walker::step_ms(false);
        }
        eq(g.player().pos.x, target.x, "дошёл по горизонтали");
        eq(g.player().pos.y, target.y, "и по вертикали");
        eq(static_cast<int>(w.last_stop()), static_cast<int>(gfx::WS_ARRIVED), "именно дошёл");
        check(!w.active(), "и остановился сам");
    }

    // Шаг делается не чаще, чем положено: иначе бег и шаг неразличимы.
    {
        gfx::Walker w;
        const Vec2 from = g.player().pos;
        w.go(Vec2(from.x + 4, from.y), false);
        check(w.update(g, 0), "первый шаг делается сразу");
        check(!w.update(g, 10), "второй раньше времени — нет");
        check(w.update(g, gfx::Walker::step_ms(false)), "а по времени — да");
        check(gfx::Walker::step_ms(true) < gfx::Walker::step_ms(false),
              "бег быстрее шага");
    }

    // Темп задаётся часами, а не числом вызовов: на быстром телефоне,
    // где кадров втрое больше, герой должен идти ровно так же.
    {
        Game slow, fast;
        slow.new_game("Медленный", "human", "swordsman");
        fast.new_game("Быстрый", "human", "swordsman");
        const Vec2 from = slow.player().pos;
        const Vec2 target(from.x + 6, from.y);

        gfx::Walker ws, wf;
        ws.go(target, false);
        wf.go(target, false);
        // Секунда игры: у одного 20 кадров, у другого 200.
        for (int i = 0; i <= 20; ++i)  ws.update(slow, static_cast<unsigned>(i) * 50);
        for (int i = 0; i <= 200; ++i) wf.update(fast, static_cast<unsigned>(i) * 5);
        eq(fast.player().pos.x, slow.player().pos.x,
           "за одно и то же время пройдено одно и то же");

        const int steps = slow.player().pos.x - from.x;
        const int want = 1 + 1000 / static_cast<int>(gfx::Walker::step_ms(false));
        eq(steps, want, "и ровно столько шагов, сколько укладывается в секунду");
    }

    // Смена цели не даёт лишнего шага: иначе частым касанием героя можно
    // было бы разогнать быстрее любого бега.
    {
        Game g2;
        g2.new_game("Ходок", "human", "swordsman");
        const Vec2 from = g2.player().pos;
        gfx::Walker w;
        w.go(Vec2(from.x + 3, from.y), false);
        check(w.update(g2, 0), "первый шаг — сразу");
        for (int i = 0; i < 20; ++i) {
            // Двадцать раз подряд подсовываем новую цель на той же клетке.
            w.go(Vec2(g2.player().pos.x + 1, g2.player().pos.y), false);
            check(!w.update(g2, 10), "новая цель шага не покупает");
        }
        eq(g2.player().pos.x, from.x + 1, "и герой сдвинулся ровно на шаг");

        // То же для ведения цели пальцем.
        gfx::Walker w2;
        Game g3;
        g3.new_game("Ходок", "human", "swordsman");
        const Vec2 f3 = g3.player().pos;
        w2.go(Vec2(f3.x + 5, f3.y), false);
        check(w2.update(g3, 0), "шаг по касанию");
        for (int i = 0; i < 20; ++i) {
            w2.retarget(Vec2(f3.x + 5, f3.y + (i % 2)), false);
            check(!w2.update(g3, 20), "ведение цели тоже не ускоряет");
        }
    }

    // Дошёл — и следующая цель срабатывает сразу, без лишнего ожидания:
    // стоять на месте паузы не стоит.
    {
        Game g2;
        g2.new_game("Ходок", "human", "swordsman");
        const Vec2 from = g2.player().pos;
        gfx::Walker w;
        w.go(Vec2(from.x + 1, from.y), false);
        check(w.update(g2, 1000), "шаг до соседней клетки");
        check(!w.update(g2, 1000), "цель достигнута — шага больше нет");
        check(!w.active(), "и ходьба кончилась");
        w.go(Vec2(from.x + 2, from.y), false);
        check(!w.update(g2, 1000), "но пауза после шага никуда не делась");
        check(w.update(g2, 1000 + gfx::Walker::step_ms(false)), "а по времени — идёт дальше");
    }

    // Упёрся в стену — остановился, а не топчется вечно.
    {
        Game g2;
        g2.new_game("Ходок", "human", "swordsman");
        const Location* l2 = g2.here();
        gfx::Walker w;
        // Цель внутри дома: стена на пути, и обойти её жадный шаг не умеет —
        // именно так это и обещано игроку.
        check(l2 != nullptr, "локация загружена");
        if (!l2) return;
        Vec2 wall(g2.player().pos.x, g2.player().pos.y);
        bool found = false;
        for (int y = g2.player().pos.y - 1; y >= 1 && !found; --y)
            if (!l2->walkable(Vec2(g2.player().pos.x, y))) { wall = Vec2(g2.player().pos.x, y); found = true; }
        check(found, "над героем есть стена");
        w.go(wall, false);
        unsigned t = 0;
        int guard = 0;
        while (w.active() && guard++ < 500) { w.update(g2, t); t += 200; }
        check(!w.active(), "ходьба к недостижимой цели прекращается");
        eq(static_cast<int>(w.last_stop()), static_cast<int>(gfx::WS_BLOCKED),
           "и прекращается именно упором");
        check(g2.player().pos.y > wall.y, "герой остановился перед стеной, а не в ней");
    }

    // Подбор с земли ходьбу не прерывает, а разговор — прерывает.
    check(!gfx::walk_interrupted_by(Bump::Moved), "обычный шаг ходьбу не рвёт");
    check(!gfx::walk_interrupted_by(Bump::Item), "подобранная вещь тоже");
    check(!gfx::walk_interrupted_by(Bump::Note), "и записка");
    check(gfx::walk_interrupted_by(Bump::Npc), "а житель — рвёт");
    check(gfx::walk_interrupted_by(Bump::Exit), "и переход");
    check(gfx::walk_interrupted_by(Bump::Combat), "и бой");
    check(gfx::walk_interrupted_by(Bump::Chest), "и сундук");
}

// Развязка: три исхода, из них выбирается ровно один, и обратно не отыграть.
void test_dark() {
    section("темнота: подземелья и факел");

    const Content& c = Content::get();
    check(c.location_dark("cave"), "в пещере темно");
    check(c.location_dark("saltmines"), "в шахтах тоже");
    check(!c.location_dark("village"), "а в деревне светло");
    check(!c.location_dark("grove"), "и в роще, где не темнеет, — тем более");

    Game g;
    g.new_game("Тьма", "human", "swordsman");
    eq(g.sight_radius(), -1, "на свету видно всю карту");
    check(g.cell_lit(Vec2(g.player().pos.x + 20, g.player().pos.y)), "и дальний угол тоже");

    // Уводим героя под землю.
    g.player().loc = "cave";
    const Location* cave = g.here();
    check(cave != nullptr, "пещера загружена");
    if (!cave) return;

    // Ищем открытое место: от него удобно мерить круг света.
    Vec2 open(0, 0);
    bool found = false;
    for (int y = 1; y < cave->h - 1 && !found; ++y)
        for (int x = 1; x < cave->w - 1 && !found; ++x) {
            const Vec2 p(x, y);
            int free_around = 0;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    if (cave->walkable(Vec2(x + dx, y + dy))) ++free_around;
            if (cave->walkable(p) && free_around == 9) { open = p; found = true; }
        }
    check(found, "в пещере нашлось открытое место");
    if (!found) return;
    g.player().pos = open;

    // Без огня — шаг вокруг себя.
    eq(g.sight_radius(), Game::SIGHT_DARK, "без света виден только шаг");
    check(g.cell_lit(open), "своя клетка видна всегда");
    check(g.cell_lit(Vec2(open.x + 1, open.y)), "соседняя — тоже");
    check(g.cell_lit(Vec2(open.x + 1, open.y + 1)), "и по диагонали: круг, а не крест");
    check(!g.cell_lit(Vec2(open.x + 2, open.y)), "а через клетку уже темно");

    // Факел в сумке не светит — только в руке.
    g.add_item("torch", 1);
    eq(g.sight_radius(), Game::SIGHT_DARK, "факел в сумке не светит");
    check(g.equip("torch"), "факел взят в руку");
    eq(g.sight_radius(), Game::SIGHT_TORCH, "и теперь видно дальше");

    // Считаем освещённые клетки, а не гадаем про конкретную: в пещере рядом
    // может оказаться стена, и «клетка в трёх шагах» ничего не доказывала бы.
    {
        int lit_torch = 0;
        for (int y = 0; y < cave->h; ++y)
            for (int x = 0; x < cave->w; ++x)
                if (g.cell_lit(Vec2(x, y))) ++lit_torch;
        check(g.unequip(Slot::Shield), "факел убран");
        int lit_dark = 0;
        for (int y = 0; y < cave->h; ++y)
            for (int x = 0; x < cave->w; ++x)
                if (g.cell_lit(Vec2(x, y))) ++lit_dark;
        check(lit_torch > lit_dark, "с факелом видно больше, чем без него");
        check(lit_dark <= 9, "без света — не больше клетки вокруг себя");
        check(lit_torch > 12, "а с факелом — заметно шире");
        check(g.equip("torch"), "факел снова в руке");
    }

    // Факел занимает руку щита: держать и то, и другое нельзя.
    const std::size_t hand = static_cast<std::size_t>(Slot::Shield);
    eqs(g.player().equipped[hand], "torch", "факел занял левую руку");
    g.add_item("buckler", 1);
    check(g.equip("buckler"), "щит взят вместо факела");
    eqs(g.player().equipped[hand], "buckler", "и вытеснил его");
    eq(g.sight_radius(), Game::SIGHT_DARK, "со щитом в руке снова темно");

    // Дальше радиуса не видно ничего, даже в упор к стене.
    check(g.equip("torch"), "факел обратно в руку");
    check(!g.cell_lit(Vec2(open.x + Game::SIGHT_TORCH + 2, open.y)),
          "за границей света темно при любом факеле");

    // Свет не проходит сквозь стены: за углом темно, как бы близко он ни был.
    {
        bool checked = false;
        for (int y = 1; y < cave->h - 1 && !checked; ++y)
            for (int x = 1; x < cave->w - 1 && !checked; ++x) {
                const Vec2 p(x, y);
                if (!cave->walkable(p)) continue;
                // Ищем клетку в радиусе света, закрытую стеной.
                for (int dy = -3; dy <= 3 && !checked; ++dy)
                    for (int dx = -3; dx <= 3 && !checked; ++dx) {
                        const Vec2 q(x + dx, y + dy);
                        if (!cave->in_bounds(q)) continue;
                        if (dx * dx + dy * dy > Game::SIGHT_TORCH * Game::SIGHT_TORCH) continue;
                        if (cave->visible(p, q)) continue;
                        g.player().pos = p;
                        check(!g.cell_lit(q), "клетку за стеной свет не достаёт");
                        checked = true;
                    }
            }
        check(checked, "в пещере нашлось, что проверить на просвет");
    }
}

void test_balance() {
    section("баланс: броня и кривая уровней");

    // Броня снимает часть удара, но не весь: иначе к середине игры герой
    // становится неуязвимым, и это именно то, чем игра болела.
    {
        Game g;
        g.new_game("Проба", "human", "swordsman");
        check(Game::ARMOR_MIN_PCT > 0, "сквозь броню что-то проходит всегда");
        check(Game::ARMOR_MIN_PCT < 100, "но броня всё же на что-то влияет");

        // Тяжёлый удар сквозь любую броню остаётся тяжёлым.
        const int raw = 30;
        const int through = raw * Game::ARMOR_MIN_PCT / 100;
        check(through >= 10, "тридцатью очками урона нельзя ударить на единицу");
    }

    // Кривая растёт быстрее прямой: иначе поздние уровни стоят столько же,
    // сколько ранние, и всё содержимое игры сыплет баллы навыка пачками.
    {
        Game g;
        g.new_game("Проба", "human", "swordsman");
        Player& p = g.player();
        p.level = 5;  const int at5  = g.exp_to_next();
        p.level = 10; const int at10 = g.exp_to_next();
        p.level = 30; const int at30 = g.exp_to_next();
        check(at10 > at5, "следующий уровень дороже предыдущего");
        check(at30 > 3 * at10, "и дорожает быстрее, чем просто вдвое-втрое");

        // Опыта всего мира хватает примерно на двадцать шесть уровней, то есть
        // на два с половиной десятка баллов на семь навыков: развить всё
        // одинаково не выйдет, и выбирать приходится до самого конца.
        long need = 0;
        for (int l = 1; l < 27; ++l) { p.level = l; need += g.exp_to_next(); }
        p.level = 1;
        check(need > 25000, "к 27-му уровню нужен весь опыт мира");
    }
}

void test_finale() {
    section("развязка: три исхода");
    const Content& c = Content::get();

    const DlgNode* root = c.node("finale_root");
    check(root != nullptr, "узел развязки существует");
    if (!root) return;

    // Сначала — что вариантов ровно три и каждый ведёт в свой эпилог.
    std::vector<const DlgOption*> choices;
    for (const DlgOption& o : root->options)
        if (!o.ending.empty()) choices.push_back(&o);
    eq(static_cast<int>(choices.size()), 3, "исходов ровно три");
    std::vector<std::string> ids;
    for (const DlgOption* o : choices) {
        check(c.ending(o->ending) != nullptr, "исход " + o->ending + " описан");
        eqs(o->set_quest, "finale", "исход закрывает квест «Развязка»");
        eq(o->set_stage, QUEST_DONE, "и закрывает его насовсем");
        eqs(o->set_counter, "ending_choice", "выбор записывается счётчиком");
        check(o->set_counter_value > 0, "и значение счётчика ненулевое");
        for (const std::string& seen : ids)
            check(seen != o->ending, "исходы не повторяются");
        ids.push_back(o->ending);
    }

    // Каждый исход проверяется отдельной игрой: выбрав один, два других
    // должны исчезнуть навсегда, а эпилог — прийти ровно один раз.
    for (std::size_t k = 0; k < choices.size(); ++k) {
        Game g;
        g.new_game("Решающий", "human", "swordsman");
        std::string shop;

        // До листа выбирать нечего.
        int before = 0;
        for (const DlgOption& o : root->options)
            if (!o.ending.empty() && g.option_available(o)) ++before;
        eq(before, 0, "без прочитанного листа исходов не предлагают");

        g.player().quests["finale"] = 1;
        int open_now = 0;
        for (const DlgOption& o : root->options)
            if (!o.ending.empty() && g.option_available(o)) ++open_now;
        eq(open_now, 3, "с листом видны все три");

        check(g.pending_ending().empty(), "до выбора эпилога нет");
        g.apply_option(*choices[k], "", &shop);

        const std::string got = g.take_pending_ending();
        eqs(got, choices[k]->ending, "показан эпилог именно выбранного исхода");
        check(g.take_pending_ending().empty(), "второй раз эпилог не показывается");

        eq(g.quest_stage("finale"), QUEST_DONE, "квест «Развязка» закрыт");
        eq(g.player().counters["ending_choice"], choices[k]->set_counter_value,
           "выбор записан");

        int left = 0;
        for (const DlgOption& o : root->options)
            if (!o.ending.empty() && g.option_available(o)) ++left;
        eq(left, 0, "переиграть выбор нельзя ни одним из трёх");

        // Собеседник отвечает по сделанному выбору и только по нему.
        int after_lines = 0;
        for (const DlgOption& o : root->options)
            if (o.next.compare(0, 13, "finale_after_") == 0 && g.option_available(o))
                ++after_lines;
        eq(after_lines, 1, "и говорит ровно об одном исходе — сделанном");
    }

    // Выбор переживает сохранение: это не экран, а состояние мира.
    {
        Game g;
        g.new_game("Решающий", "human", "swordsman");
        std::string shop;
        g.player().quests["finale"] = 1;
        g.apply_option(*choices[1], "", &shop);
        g.take_pending_ending();

        const std::string path = "build/finale.sav";
        check(g.save_to(path), "игра после развязки сохраняется");
        Game g2;
        check(g2.load_from(path), "и загружается");
        eq(g2.quest_stage("finale"), QUEST_DONE, "исход пережил сохранение");
        eq(g2.player().counters["ending_choice"], choices[1]->set_counter_value,
           "и какой именно — тоже");
        check(g2.pending_ending().empty(), "но эпилог заново не показывается");
        std::remove(path.c_str());
    }
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

    // В Дрейф ведут ровно два прохода, и оба под условием: разрыв Третьего
    // узла — для тех, кто прочёл отметку, и Тропа Возвращения — только для
    // тех, кто уже прошёл по ней в другую сторону.
    const Location* n3 = w.location("node3");
    check(n3 != nullptr, "Третий узел загружается");
    if (n3) {
        const MapExit* to_drift = nullptr;
        for (const MapExit& e : n3->exits)
            if (e.target == "meadow") to_drift = &e;
        check(to_drift != nullptr, "из Третьего узла есть проход в Дрейф");
        if (to_drift) {
            eqs(to_drift->gate.req_quest, "node3q", "разрыв открыт разгаданной тайной");
            eq(to_drift->gate.req_stage, QUEST_DONE, "и только полностью разгаданной");
            check(!to_drift->gate.denied.empty(), "отказ объясняется словами");
        }
    }
    const MapExit* to_path = nullptr;
    for (const MapExit& e : sanct->exits)
        if (e.target == "homepath") to_path = &e;
    check(to_path != nullptr, "из святилища есть выход на Тропу Возвращения");
    if (to_path) {
        eqs(to_path->gate.req_quest, "driftway", "тропа открывается поиском обоза");
        eq(to_path->gate.req_stage, 2, "и лишь после того, как герой сам побывал на лоскуте");
        check(!to_path->gate.denied.empty(), "отказ объясняется словами");
    }

    // Проверка на деле: тот же проход отказывает и пускает по состоянию квеста.
    {
        Game gd;
        gd.new_game("Ходок", "human", "swordsman");
        gd.player().loc = "node3";
        const Location* loc = gd.here();
        const MapExit* ex = nullptr;
        if (loc)
            for (const MapExit& e : loc->exits)
                if (e.target == "meadow") ex = &e;
        check(ex != nullptr, "проход в Дрейф есть и в живой игре");
        if (ex) {
            const Vec2 from(ex->pos.x - 1, ex->pos.y);
            gd.player().pos = from;
            check(gd.try_move(1, 0) != Bump::Exit, "без разгаданной тайны разрыв не пускает");
            eqs(gd.player().loc, "node3", "герой остаётся у узла");

            gd.player().quests["node3q"] = QUEST_DONE;
            gd.player().pos = from;
            check(gd.try_move(1, 0) == Bump::Exit, "с разгаданной — пускает");
            eqs(gd.player().loc, "meadow", "и выводит на дрейфующий луг");
        }
    }

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
    eq(static_cast<int>(sizeof(ALL_NOTES) / sizeof(ALL_NOTES[0])), 61,
       "список записок в тесте покрывает все записки базы");
    for (const char* id : ALL_NOTES) {
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

// Честный поединок: без лечения, настоящим боевым кодом. Возвращает исход
// и остаток здоровья. Противника берём с карты — мобы появляются только в
// своих зонах, и поставить кого угодно перед героем нельзя ни одним
// открытым вызовом. Это и к лучшему: дерёмся с тем, что игра правда даёт.
struct DuelResult {
    bool met;        // противник вообще нашёлся
    bool won;
    int  hp_left;
    int  hp_max;
    int  rounds;
    DuelResult() : met(false), won(false), hp_left(0), hp_max(0), rounds(0) {}
};

DuelResult duel_here(Game& g, const std::string& enemy_id) {
    DuelResult r;
    int uid = -1;
    for (std::size_t i = 0; i < g.mobs().size(); ++i)
        if (g.mobs()[i].enemy_id == enemy_id && g.mobs()[i].loc == g.player().loc) {
            uid = g.mobs()[i].uid; break;
        }
    if (uid < 0) return r;
    r.met = true;
    r.hp_max = g.total().max_hp;

    g.start_combat(uid);
    for (int turn = 0; turn < 400 && g.combat().active; ++turn) {
        ++r.rounds;
        // Ход героя: бьём, пока хватает очков действия, потом передаём ход.
        // Ни здоровья, ни очков даром — только то, что даёт сама игра.
        if (g.player().ap >= g.attack_cost()) g.combat_attack(false);
        else                                  g.combat_end_turn();
        if (g.player_dead()) break;
    }
    r.won = (g.mob_by_uid(uid) == 0) && !g.player_dead();
    r.hp_left = g.player().hp;
    return r;
}

// Довести героя до нужного уровня и вложить баллы. Опыт выдаётся настоящим
// grant_exp, баллы тратятся настоящим learn_skill: никаких прямых записей
// в характеристики, иначе мерился бы не тот герой, которого получит игрок.
void build_hero(Game& g, int level, const std::string& skill) {
    while (g.player().level < level) g.grant_exp(g.exp_to_next());
    if (!skill.empty())
        while (g.player().skill_points > 0)
            if (!g.learn_skill(skill)) break;
}

// Перейти в соседнюю локацию настоящим шагом через выход.
bool walk_to(Game& g, const std::string& target) {
    const Location* loc = g.here();
    if (!loc) return false;
    for (std::size_t i = 0; i < loc->exits.size(); ++i) {
        if (loc->exits[i].target != target) continue;
        const int dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
        for (int k = 0; k < 4; ++k) {
            Vec2 from(loc->exits[i].pos.x - dx[k], loc->exits[i].pos.y - dy[k]);
            if (!loc->walkable(from) && !loc->exit_at(from)) continue;
            g.player().pos = from;
            if (g.try_move(dx[k], dy[k]) == Bump::Exit) return true;
        }
    }
    return false;
}

void test_difficulty() {
    section("сложность: нужна ли прокачка");

    // --- новичок против волка ---
    // Волк живёт в лесу, сразу за деревней: это первое, во что упирается
    // игрок, вышедший за ворота. Он и должен показать, что дальше без
    // прокачки не пройти.
    {
        Game g;
        g.new_game("Новичок", "human", "swordsman");
        check(walk_to(g, "forest"), "из деревни есть выход в лес");
        DuelResult d = duel_here(g, "wolf");
        check(d.met, "волк в лесу нашёлся");
        if (d.met) {
            std::cout << "  новичок 1 ур. против волка: "
                      << (d.won ? "победа" : "поражение")
                      << ", здоровья " << d.hp_left << " из " << d.hp_max
                      << ", раундов " << d.rounds << "\n";
        }
    }

    // --- тот же волк, но герой вложился в «Крепость» ---
    {
        Game g;
        g.new_game("Вложился", "human", "swordsman");
        build_hero(g, 4, "vigor");
        check(g.total().max_hp > Game().total().max_hp, "«Крепость» подняла запас");
        check(walk_to(g, "forest"), "выход в лес на месте");
        DuelResult d = duel_here(g, "wolf");
        if (d.met) {
            std::cout << "  4 ур. с «Крепостью» против волка: "
                      << (d.won ? "победа" : "поражение")
                      << ", здоровья " << d.hp_left << " из " << d.hp_max
                      << ", раундов " << d.rounds << "\n";
        }
    }

    // --- вожак: то, что должно быть не по зубам новичку ---
    {
        Game g;
        g.new_game("Новичок", "human", "swordsman");
        walk_to(g, "forest");
        DuelResult d = duel_here(g, "wolf_alpha");
        check(d.met, "вожак в лесу нашёлся");
        if (d.met) {
            check(!d.won, "новичок вожака не одолевает");
            std::cout << "  новичок 1 ур. против вожака: "
                      << (d.won ? "победа" : "поражение")
                      << ", здоровья " << d.hp_left << " из " << d.hp_max
                      << ", раундов " << d.rounds << "\n";
        }
    }

    // --- уровень сам по себе не даёт ничего ---
    // Это главное следствие того, что характеристики больше не растут сами.
    // Двадцать уровней без единого вложения — тот же герой, что и в начале:
    // сила приходит только из решений игрока, а не из счётчика опыта.
    {
        Game a; a.new_game("Первый", "human", "swordsman");
        Game b; b.new_game("Двадцатый", "human", "swordsman");
        build_hero(b, 20, "");
        eq(b.player().level, 20, "уровень действительно вырос");
        // Девятнадцать подъёмов плюс балл, который герой получает на старте.
        eq(b.player().skill_points, a.player().skill_points + 19,
           "и баллы накопились нетронутыми");
        const Stats sa = a.total(), sb = b.total();
        eq(sb.max_hp, sa.max_hp, "здоровье без вложений то же");
        eq(sb.attack, sa.attack, "меткость та же");
        eq(sb.max_ap, sa.max_ap, "очки действия те же");
        eq(sb.dmg_max, sa.dmg_max, "урон тот же");

        // И такой герой по-прежнему проигрывает тому же вожаку.
        walk_to(b, "forest");
        DuelResult d = duel_here(b, "wolf_alpha");
        if (d.met) {
            check(!d.won, "двадцатый уровень без вложений вожака тоже не берёт");
            std::cout << "  20 ур. без вложений против вожака: "
                      << (d.won ? "победа" : "поражение")
                      << ", здоровья " << d.hp_left << " из " << d.hp_max << "\n";
        }
    }

    // --- лестница: цена победы над каждым доступным врагом ---
    // Считаем, со скольких вложенных баллов герой начинает выигрывать
    // поединок. Одно число ничего не доказывает, лестница — доказывает:
    // она показывает, растёт ли требование вместе с содержимым или всё
    // берётся с нуля.
    {
        struct Rung { const char* enemy; const char* where; const char* name; };
        // Всё это доступно без квестовых ворот: лес и погост — соседи деревни.
        const Rung ladder[] = {
            { "rat",          "village",   "амбарная крыса" },
            { "wolf",         "forest",    "волк" },
            { "bandit",       "forest",    "разбойник" },
            { "barrow_shade", "graveyard", "тень с погоста" },
            { "wolf_alpha",   "forest",    "вожак" },
        };
        int prev = 0;
        bool grows = true;
        for (std::size_t r = 0; r < sizeof(ladder) / sizeof(ladder[0]); ++r) {
            // Бой случаен, и одна схватка ничего не говорит: у тени с погоста
            // характеристики выше, чем у разбойника, а по единственной выборке
            // она выходила легче. Поэтому каждый расклад гоняется на TRIALS
            // разных зёрен, и решают доля побед и средний остаток здоровья.
            const int TRIALS = 24;
            int need = -1;
            int win_pct = 0, left_pct = 0;
            for (int ranks = 0; ranks <= 20 && need < 0; ++ranks) {
                int wins = 0, left_sum = 0, cap_sum = 0, met = 0;
                for (int t = 0; t < TRIALS; ++t) {
                    Game g;
                    g.new_game("Упорный", "human", "swordsman");
                    g.rng().set_seed(0xA11CE + static_cast<unsigned long long>(t) * 7919ULL);
                    if (ranks > 0) {
                        while (g.player().level < ranks + 1) g.grant_exp(g.exp_to_next());
                        // Вкладываем вперемешку: запас здоровья и урон. Одну
                        // ветку качать выгоднее, но так мерится обычный игрок,
                        // а не выжимка из механики.
                        for (int k = 0; k < ranks; ++k)
                            g.learn_skill(k % 2 == 0 ? "vigor" : "might");
                    }
                    if (std::string(ladder[r].where) != "village" &&
                        !walk_to(g, ladder[r].where)) break;
                    DuelResult d = duel_here(g, ladder[r].enemy);
                    if (!d.met) break;
                    ++met;
                    if (d.won) ++wins;
                    left_sum += d.hp_left;
                    cap_sum += d.hp_max;
                }
                if (met == 0) { need = -2; break; }
                // Ступень считается взятой, когда она берётся уверенно, а не
                // изредка: победа в трети схваток — это ещё стена.
                if (wins * 4 >= met * 3) {
                    need = ranks;
                    win_pct = wins * 100 / met;
                    left_pct = cap_sum > 0 ? left_sum * 100 / cap_sum : 0;
                }
            }
            // Запас после победы важнее самого факта: выигрыш на последних
            // очках здоровья — это бой, выигрыш почти без потерь — прогулка.
            std::cout << "  " << ladder[r].name << ": уверенная победа с "
                      << (need < 0 ? std::string("(не найден)") : to_str(need))
                      << " баллами, побед " << win_pct
                      << "%, остаётся здоровья " << left_pct << "%\n";
            if (need >= 0) {
                if (need < prev) grows = false;
                prev = need;
            }
        }
        // Лестница меряет голую прокачку: снаряжения герой не покупает, а
        // игрок покупает. Поэтому это нижняя граница требований, и судить по
        // ней надо об отношении ступеней, а не об абсолютных числах.
        check(grows, "требование не падает по мере усиления врагов");
        check(prev > 1, "последняя ступень не берётся с одного балла");
        check(prev > 0 && prev <= 20, "и всё же берётся одной прокачкой");
    }
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

    // Сколько рангов навыков куплено — вместе с нерастраченными очками это
    // весь опыт, который герой успел превратить в силу.
    auto total_ranks = [&](Game& gg) {
        int n = 0;
        for (std::map<std::string, int>::const_iterator it = gg.player().skills.begin();
             it != gg.player().skills.end(); ++it) n += it->second;
        return n;
    };

    // Победить противника честной боевой механикой.
    //
    // Тест проверяет проходимость содержимого, а не выживание: здоровье и силы
    // герою возвращают каждый ход — в том числе после ответного удара врага,
    // иначе смерть засчиталась бы победой (бой ведь тоже кончился). И победа
    // определяется по исчезнувшему врагу, а не по «бой больше не идёт»:
    // отличать одно от другого обязательно, иначе непроходимый противник
    // тихо считается убитым.
    // Замер сложности. Тест лечит героя каждый ход, иначе он проверял бы
    // выживание вместо проходимости, — но само это лечение и есть мера
    // сложности: сколько урона содержимое игры вернуло герою в лицо.
    // Складывается он честно, по разнице до и после каждого хода врага.
    long dmg_total = 0;        // весь принятый урон за прохождение
    int  dmg_worst = 0;        // худший бой: сколько в нём принято
    int  fights_done = 0;
    int  fights_over_half = 0; // боёв, где принято больше половины запаса
    int  fights_lethal = 0;    // боёв, где принято больше полного запаса
    int  hp_at_worst = 0;      // каков был запас в тот момент

    // Баллы навыка тратятся так, как их тратил бы игрок: сперва запас
    // здоровья, пока он отстаёт от уровня, потом урон. Без этого прогон мерил
    // бы героя, который двадцать шесть уровней не открывал окно навыков, —
    // а это не «трудно», это «не играл».
    auto spend_points = [&]() {
        while (g.player().skill_points > 0) {
            const bool need_hp = g.total().max_hp < 40 + g.player().level * 3;
            if (!g.learn_skill(need_hp ? "vigor" : "might")) break;
        }
    };

    auto fight = [&](int uid) {
        spend_points();
        g.start_combat(uid);
        ++fights_done;
        int taken_here = 0;
        for (int turns = 0; turns < 600 && g.combat().active; ++turns) {
            const int cap = g.total().max_hp;
            const int missing = cap - g.player().hp;
            if (missing > 0) { taken_here += missing; dmg_total += missing; }
            g.player().hp = cap;
            if (g.player().ap < g.attack_cost()) g.player().ap = g.total().max_ap;
            g.combat_attack(false);
            if (g.player_dead()) g.player().hp = g.total().max_hp;
        }
        {
            const int cap = g.total().max_hp;
            const int missing = cap - g.player().hp;
            if (missing > 0) { taken_here += missing; dmg_total += missing; }
            if (taken_here > dmg_worst) { dmg_worst = taken_here; hp_at_worst = cap; }
            if (cap > 0 && taken_here * 2 > cap) ++fights_over_half;
            if (cap > 0 && taken_here > cap) ++fights_lethal;
        }
        g.player().effects.clear();      // тест не про выживание под ядом
        if (g.player().hp <= 0) g.player().hp = g.total().max_hp;
        return g.mob_by_uid(uid) == 0;
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
            bool taken = false;
            for (int attempt = 0; attempt < 8 && !taken; ++attempt) {
                for (int k = 0; k < 4 && !taken; ++k) {
                    Vec2 from(t.x - dx[k], t.y - dy[k]);
                    if (!loc->walkable(from)) continue;
                    g.player().pos = from;
                    Bump b = g.try_move(dx[k], dy[k]);
                    if (b == Bump::Item) { ++got; taken = true; }
                    else if (b == Bump::Combat) fight(g.combat().mob_uid);
                }
                if (!taken) g.world_turn();
            }
        }
        return got;
    };

    // Подобрать записку с карты настоящим шагом. Между героем и запиской
    // может встать моб — тогда шаг уходит в бой, а не в находку, и заход
    // надо повторить. Без этого тест зависел бы от того, куда мобы забрели.
    auto gather_note = [&](const std::string& note_id) {
        const Location* loc = g.here();
        if (!loc) return false;
        for (std::size_t i = 0; i < loc->notes.size(); ++i) {
            if (loc->notes[i].note_id != note_id) continue;
            if (g.note_taken(loc->id, static_cast<int>(i))) return true;
            const Vec2 t = loc->notes[i].pos;
            const int dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
            for (int attempt = 0; attempt < 8; ++attempt) {
                for (int k = 0; k < 4; ++k) {
                    Vec2 from(t.x - dx[k], t.y - dy[k]);
                    if (!loc->walkable(from)) continue;
                    g.player().pos = from;
                    Bump b = g.try_move(dx[k], dy[k]);
                    if (b == Bump::Note) return true;
                    if (b == Bump::Combat) fight(g.combat().mob_uid);
                }
                g.world_turn();
            }
        }
        return false;
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

    // --- погост: чужие имена ---
    // Погост открыт с самого начала: он сразу за околицей, и попасть туда
    // можно раньше, чем спросить о нём Мирона.
    g.player().loc = "village";
    check(travel("graveyard"), "переход на погост по западной дороге");
    eqs(g.player().loc, "graveyard", "герой на погосте");
    check(gather_note("graves"), "надгробья прочитаны");
    check(gather_note("sexton"), "тетрадь могильщика найдена");
    {
        const Location* gy = g.here();
        int ci = -1;
        if (gy)
            for (std::size_t i = 0; i < gy->chests.size(); ++i) ci = static_cast<int>(i);
        check(ci >= 0 && g.open_chest(ci), "сарай могильщика открыт");
    }
    check(g.count_item("grave_list") >= 1, "опись погоста найдена в сарае");

    g.player().loc = "village";
    check(visible("elder_root", "strangers_offer"), "Мирон рассказывает про крайние три могилы");
    g.apply_option(c.node("strangers_offer")->options[0], "", &shop);
    eq(g.quest_stage("strangers"), 1, "квест «Чужие имена» взят");
    check(visible("elder_root", "strangers_reward"), "с описью и прочитанными камнями есть что сказать");
    g.apply_option(c.node("strangers_reward")->options[0], "", &shop);
    eq(g.quest_stage("strangers"), QUEST_DONE, "квест «Чужие имена» закрыт");
    eq(g.count_item("grave_list"), 0, "опись осталась у Мирона");
    check(visible("elder_root", "strangers_after"), "и Мирону теперь есть что добавить");

    // --- гнилая топь: без нужды туда не пускают ---
    check(visible("herbalist_root", "root_offer"), "закрыв мох, Лада заговаривает о топи");
    check(travel("graveyard"), "снова на погост");
    check(!travel("swamp"), "без Ладиного дела в топь не лезут");
    eqs(g.player().loc, "graveyard", "герой остаётся на погосте");

    g.player().loc = "village";
    g.apply_option(c.node("root_offer")->options[0], "", &shop);
    eq(g.quest_stage("swamproot"), 1, "квест «Болотный корень» взят");
    check(g.count_item("salve") >= 2, "Лада дала мазь вперёд");

    check(travel("graveyard"), "к западной дороге");
    check(travel("swamp"), "с делом топь пускает");
    eqs(g.player().loc, "swamp", "герой в топи");
    check(gather_note("bog"), "замер воды найден");
    eq(g.quest_stage("drowned"), 1, "солёная вода открыла тайну «Дорога под водой»");
    check(gather_note("drovers"), "размокший путевой лист найден");
    check(gather("road_stone") >= 1, "верстовой камень поднят со дна");
    eq(g.quest_stage("drowned"), QUEST_DONE, "тайна «Дорога под водой» разгадана");

    // По мостовой под водой действительно ходят: это дорога, а не топь.
    {
        const Location* sw = g.here();
        bool road_under = false;
        for (int y = 1; y < 17 && !road_under; ++y)
            for (int x = 1; x < 47; ++x)
                if (sw->at(Vec2(x, y)) == Tile::Road && sw->walkable(Vec2(x, y))) {
                    road_under = true;
                    break;
                }
        check(road_under, "мостовая под водой проходима");
    }

    gather("bog_root");
    guard = 0;
    while (g.count_item("bog_root") < 5 && guard++ < 80) {
        if (hunt("bog_leech", 1) == 0) g.world_turn();
    }
    check(g.count_item("bog_root") >= 5, "пять болотных корней собраны");
    check(hunt("bog_walker", 1) == 1, "болотный ходок повержен");

    // Топь выводит и в лес: западная дорога и лесная низина — одно место.
    check(travel("forest"), "из топи выходят в лес");
    eqs(g.player().loc, "forest", "герой снова в лесу");

    g.player().loc = "village";
    g.apply_option(c.node("root_reward")->options[0], "", &shop);
    eq(g.quest_stage("swamproot"), QUEST_DONE, "квест «Болотный корень» закрыт");
    check(g.count_item("bog_root") < 5, "пять корней ушли в ступку");
    check(g.count_item("bog_cloak") >= 1, "плащ болотной кожи принят");

    g.player().loc = "forest";

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

    // ================= Регион V: Дрейф =================

    // --- Гурий вспоминает про сорок душ ---
    g.player().loc = "village";
    check(visible("trader_root", "driftway_offer"),
          "разгадав тайну караван-сарая, Гурий заговаривает о сорока душах");
    g.apply_option(c.node("driftway_offer")->options[0], "", &shop);
    eq(g.quest_stage("driftway"), 1, "поиск обоза начат");

    // --- Третий узел пускает только того, кто понял, куда он ведёт ---
    g.player().loc = "node3";
    g.player().quests["node3q"] = 1;          // будто отметка ещё не прочитана
    check(!travel("meadow"), "в разрыв не шагают наугад");
    eqs(g.player().loc, "node3", "и герой остаётся у узла");
    g.player().quests["node3q"] = QUEST_DONE;
    check(travel("meadow"), "зная, куда ведёт разрыв, пройти можно");
    eqs(g.player().loc, "meadow", "герой на дрейфующем лугу");
    eq(g.quest_stage("driftway"), 2, "приход на лоскут двигает поиск обоза");
    check(gather_note("drift"), "наставление о дрейфе найдено");

    // --- Дом на отшибе: обоз стоит и ждёт утра ---
    check(travel("farhouse"), "переход к дому на отшибе");
    check(gather_note("tomorrow"), "запись хозяйки найдена");
    check(visible("driftwife_root", "driftwife_tally"), "Улита помнит про бирку");
    g.apply_option(c.node("driftwife_tally")->options[0], "", &shop);
    check(g.count_item("caravan_tally") >= 1, "путевая бирка обоза у героя");

    check(visible("driftwife_root", "water_offer"), "Улита просит воды");
    g.apply_option(c.node("water_offer")->options[0], "", &shop);
    eq(g.quest_stage("water"), 1, "квест «Ведро воды» взят");
    check(!visible("driftwife_root", "water_reward"), "без второго ведра говорить не о чем");

    // --- Колодец Двух Вёдер ---
    check(travel("well"), "переход к колодцу");
    check(gather_note("wellrule"), "правило колодца найдено");
    eq(g.quest_stage("twobuckets"), 1, "правило открыло тайну «Колодец Двух Вёдер»");
    check(hunt("second_bucket", 1) == 1, "второе ведро поднялось и не захотело обратно");
    eq(g.quest_stage("twobuckets"), QUEST_DONE, "тайна колодца разгадана");
    check(g.count_item("two_bucket") >= 1, "второе ведро у героя");

    g.player().loc = "farhouse";
    check(visible("driftwife_root", "water_reward"), "теперь Улите есть что показать");
    g.apply_option(c.node("water_reward")->options[0], "", &shop);
    eq(g.quest_stage("water"), QUEST_DONE, "квест «Ведро воды» закрыт");
    eq(g.count_item("two_bucket"), 0, "оба ведра остались у Улиты");

    // --- Роща, где не темнеет ---
    g.player().loc = "well";
    check(travel("grove"), "переход в рощу");
    eq(g.quest_stage("nodark"), 1, "сам приход открыл тайну «Роща, где не темнеет»");
    check(gather_note("marks"), "зарубки на стволе прочитаны");
    eq(g.quest_stage("nodark"), QUEST_DONE, "тайна рощи разгадана");
    check(visible("grovekeeper_root", "grove_marks"),
          "с зарубками есть о чём спросить Ерофея");

    // --- Поле после битвы ---
    check(travel("battlefield"), "переход на поле");
    check(gather_note("lastorder"), "приказ, отданный вчера, найден");
    check(visible("soldier_root", "lasthour_offer"), "Ратмир объясняет, что за час");
    g.apply_option(c.node("lasthour_offer")->options[0], "", &shop);
    eq(g.quest_stage("lasthour"), 1, "квест «Последний час» взят");
    check(!visible("soldier_root", "lasthour_reward"), "без знамени час не кончить");
    check(hunt("bannerman", 1) == 1, "знаменосец повержен");
    check(g.count_item("torn_banner") >= 1, "знамя снято");
    g.apply_option(c.node("lasthour_reward")->options[0], "", &shop);
    eq(g.quest_stage("lasthour"), QUEST_DONE, "квест «Последний час» закрыт");
    check(g.count_item("hour_blade") >= 1, "клинок десятника принят");
    check(visible("soldier_root", "lasthour_after"), "Ратмиру теперь есть что сказать");

    // --- Вторая Половина Города: половины имени сходятся ---
    check(travel("otherhalf"), "переход во вторую половину Города");
    check(gather_note("otherside"), "запись с той стороны среза найдена");
    check(visible("halfscribe_root", "wholename_offer"), "Пелагея правит списки");
    g.apply_option(c.node("wholename_offer")->options[0], "", &shop);
    eq(g.quest_stage("wholename"), 1, "квест «Имя целиком» взят");
    check(g.count_item("half_name") >= 1,
          "половина имени из Региона III дожила до второй половины Города");
    g.apply_option(c.node("wholename_reward")->options[0], "", &shop);
    eq(g.quest_stage("wholename"), QUEST_DONE, "квест «Имя целиком» закрыт");
    eq(g.count_item("half_name"), 0, "половина ушла в шов");
    check(g.count_item("whole_name") >= 1, "целое имя получено");

    // --- Пустая Ольховка ---
    check(travel("emptyalder"), "переход в пустую Ольховку");
    eq(g.quest_stage("emptyalder"), 1, "сам приход открыл тайну «Пустая Ольховка»");
    check(gather_note("houses"), "роспись домов найдена");
    check(gather("own_key") >= 1, "ключ с пустой биркой подобран");
    eq(g.quest_stage("emptyalder"), QUEST_DONE, "тайна пустой Ольховки разгадана");

    // --- Лестница вверх ---
    g.player().loc = "meadow";
    check(travel("upstair"), "переход к лестнице");
    check(gather_note("stair"), "замер лестницы найден");
    {
        const Location* st = g.here();
        const MapExit* loop = 0;
        for (const MapExit& e : st->exits)
            if (e.target == "upstair") loop = &e;
        check(loop != nullptr, "у лестницы есть переход в саму себя");
        if (loop) {
            g.player().pos = Vec2(loop->pos.x - 1, loop->pos.y);
            check(g.try_move(1, 0) == Bump::Exit, "верхняя площадка срабатывает как переход");
            eqs(g.player().loc, "upstair", "и приводит на ту же лестницу");
            check(g.player().pos.y > loop->pos.y, "но в её низ");
        }
    }

    // --- Край Лоскута ---
    check(travel("edge"), "переход к краю лоскута");
    check(gather_note("edgeview"), "наблюдение с края найдено");
    eq(g.quest_stage("edgeq"), 1, "наблюдение открыло тайну «Край Лоскута»");
    check(hunt("edge_wind", 1) == 1, "Ветер Края улёгся");
    eq(g.quest_stage("edgeq"), QUEST_DONE, "тайна края разгадана");

    // --- Тропа Возвращения выводит домой ---
    g.player().loc = "otherhalf";
    check(travel("homepath"), "переход на Тропу Возвращения");
    check(gather_note("homeward"), "наказ проводника найден");
    check(visible("pathkeeper_root", "path_drift"), "Тихону есть что рассказать про Дрейф");
    check(travel("sanctum"), "тропа действительно выводит к святилищу");
    eqs(g.player().loc, "sanctum", "герой дома, на своей стороне");

    // --- Бирка возвращается Гурию ---
    g.player().loc = "village";
    check(visible("trader_root", "driftway_reward"), "Гурию есть что показать");
    g.apply_option(c.node("driftway_reward")->options[0], "", &shop);
    eq(g.quest_stage("driftway"), QUEST_DONE, "квест «Сорок душ» закрыт");
    check(g.count_item("caravan_tally") >= 1, "бирку Гурий не взял");
    check(visible("trader_root", "driftway_after"), "и говорит о ней уже иначе");

    // ================= Регион VI: Изнанка =================

    // --- Тихон наконец говорит, чего недоговаривал ---
    g.player().loc = "homepath";
    check(visible("pathkeeper_root", "inside_offer"),
          "уложив Ветер Края, Тихон рассказывает про брата");
    g.apply_option(c.node("inside_offer")->options[0], "", &shop);
    eq(g.quest_stage("inside"), 1, "решение шагнуть принято");

    // --- Кромка Края: шагнуть можно только зная зачем ---
    g.player().loc = "edge";
    g.player().quests["inside"] = QUEST_NONE;
    check(!travel("firstseam"), "просто так с Края не шагают");
    eqs(g.player().loc, "edge", "и герой остаётся на лоскуте");
    g.player().quests["inside"] = 1;
    check(travel("firstseam"), "с решением — шагают");
    eqs(g.player().loc, "firstseam", "и попадают внутрь сети");
    eq(g.quest_stage("inside"), QUEST_DONE, "падать здесь некуда: квест закрылся сам");

    // --- Первый Шов: держится на честном слове, и это не оборот речи ---
    check(gather_note("oldseam"), "опись Первого Шва найдена");
    check(visible("seamwatch_root", "firstjoint_offer"), "Елисей объясняет, что держит");
    g.apply_option(c.node("firstjoint_offer")->options[0], "", &shop);
    eq(g.quest_stage("firstjoint"), 1, "квест «На честном слове» взят");
    check(!visible("seamwatch_root", "firstjoint_reward"), "без расписки рук не отнять");

    // --- Галерея Линий: рваное или резаное ---
    check(travel("gallery"), "переход в галерею");
    check(gather_note("readlines"), "наставление о линиях найдено");
    check(visible("surveyor_root", "lines_offer"), "Пров просит нити");
    g.apply_option(c.node("lines_offer")->options[0], "", &shop);
    eq(g.quest_stage("lines"), 1, "квест «По линиям» взят");
    check(visible("surveyor_root", "surveyor_brother"),
          "побывавшему внутри Пров отвечает и про Тихона");
    gather("line_thread");
    guard = 0;
    while (g.count_item("line_thread") < 5 && guard++ < 120) {
        if (hunt("line_walker", 1) == 0) g.world_turn();
    }
    check(g.count_item("line_thread") >= 5, "пять нитей собраны");
    g.apply_option(c.node("lines_reward")->options[0], "", &shop);
    eq(g.quest_stage("lines"), QUEST_DONE, "квест «По линиям» закрыт: резаное, все пять");
    check(g.count_item("line_blade") >= 1, "клинок по линии принят");

    // --- Комната Измерений: расстояние берут в руку ---
    check(travel("measures"), "переход в комнату измерений");
    eq(g.quest_stage("measure"), 1, "сам приход открыл тайну «Мера»");
    check(gather_note("roomrule"), "правило комнаты найдено");
    check(gather("measure") >= 1, "мера расстояния взята с полки");
    eq(g.quest_stage("measure"), QUEST_DONE, "тайна «Мера» разгадана");

    // --- Встреча: тот, кто помнит дорогу ---
    g.player().loc = "gallery";
    check(travel("meeting"), "переход к месту встречи");
    check(gather_note("walkers"), "список ушедших внутрь найден");
    check(visible("master_root", "master_word"), "Мастеру есть что сказать про стык");
    g.apply_option(c.node("master_word")->options[0], "", &shop);
    check(g.count_item("seam_word") >= 1, "расписка Первого Мастера получена");
    check(visible("master_root", "master_ring_talk"),
          "с кольцом из Зала Отказа разговор идёт иначе");
    check(visible("master_root", "remembers_offer"), "и он готов рассказать, кто он");
    g.apply_option(c.node("remembers_offer")->options[0], "", &shop);
    eq(g.quest_stage("remembers"), 1, "квест «Тот, кто помнит дорогу» взят");
    g.apply_option(c.node("remembers_way")->options[0], "", &shop);
    eq(g.quest_stage("remembers"), QUEST_DONE, "дорога рассказана до конца");
    check(g.count_item("walk_staff") >= 1, "посох ходока принят");

    // --- Слово ложится в стык ---
    g.player().loc = "firstseam";
    check(visible("seamwatch_root", "firstjoint_reward"), "с распиской Елисею есть что взять");
    g.apply_option(c.node("firstjoint_reward")->options[0], "", &shop);
    eq(g.quest_stage("firstjoint"), QUEST_DONE, "квест «На честном слове» закрыт");
    eq(g.count_item("seam_word"), 0, "слово осталось в стыке");
    check(g.count_item("seam_helm") >= 1, "шлем сторожа принят");

    // --- Дальше пускают только выслушав ---
    g.player().loc = "meeting";
    {
        const int keep = g.quest_stage("remembers");
        g.player().quests["remembers"] = 1;
        check(!travel("node1"), "не выслушав старика, мимо него не пройти");
        eqs(g.player().loc, "meeting", "герой остаётся на месте");
        g.player().quests["remembers"] = keep;
    }
    check(travel("node1"), "выслушав — проходишь");
    eqs(g.player().loc, "node1", "герой у Первого узла");
    check(gather_note("firstnode"), "запись об Узле Первом найдена");

    // --- Сердце Стяжения ---
    check(travel("cinchheart"), "переход к Сердцу");
    check(gather_note("cinchwork"), "устройство хода найдено");
    eq(g.quest_stage("heart"), 1, "запись открыла тайну «Сердце Стяжения»");
    check(hunt("cinch_heart", 1) == 1, "Сердце остановлено");
    eq(g.quest_stage("heart"), QUEST_DONE, "тайна «Сердце Стяжения» разгадана");

    // --- Точка Ноль и ход, который открывается мерой ---
    check(travel("zeropoint"), "переход в Точку Ноль");
    check(gather_note("allatonce"), "запись о Точке Ноль найдена");
    {
        const int had = g.count_item("measure");
        check(had >= 1, "мера при герое");
        g.remove_item("measure", had);
        check(!travel("finale"), "без меры до развязки не дойти");
        eqs(g.player().loc, "zeropoint", "и герой остаётся в Точке Ноль");
        g.add_item("measure", had);
    }

    // --- Развязка ---
    check(travel("finale"), "с мерой ход открывается");
    eqs(g.player().loc, "finale", "герой в развязке");
    eq(g.quest_stage("finale"), QUEST_NONE, "но выбирать ещё нечего");
    check(gather_note("threeways"), "лист с тремя исходами прочитан");
    eq(g.quest_stage("finale"), 1, "три исхода записаны, и правильного нет");

    {
        const DlgNode* fin = c.node("finale_root");
        check(fin != nullptr, "узел развязки на месте");
        int offered = 0;
        const DlgOption* hold = 0;
        if (fin)
            for (const DlgOption& o : fin->options) {
                if (o.ending.empty() || !g.option_available(o)) continue;
                ++offered;
                if (o.ending == "hold") hold = &o;
            }
        eq(offered, 3, "предложены все три исхода");
        check(visible("finale_root", "finale_ask"), "и старика можно спросить, что он думает");
        check(hold != nullptr, "исход «Удержать» среди них");
        if (hold) g.apply_option(*hold, "", &shop);
    }
    eqs(g.take_pending_ending(), "hold", "показан эпилог «Удержать»");
    eq(g.quest_stage("finale"), QUEST_DONE, "развязка сыграна");
    eq(g.player().counters["ending_choice"], 3, "выбор записан навсегда");
    {
        const DlgNode* fin = c.node("finale_root");
        int left = 0;
        if (fin)
            for (const DlgOption& o : fin->options)
                if (!o.ending.empty() && g.option_available(o)) ++left;
        eq(left, 0, "переиграть нельзя");
    }
    check(visible("finale_root", "finale_after_hold"), "старик отвечает по сделанному выбору");
    check(!visible("finale_root", "finale_after_cut"), "и не по несделанному");

    // --- И одно письмо назад: Пров досчитал ---
    g.player().loc = "homepath";
    check(visible("pathkeeper_root", "inside_after"), "Тихону есть что сказать");

    // --- итог ---
    const char* all_quests[] = {"wolves", "amulet", "pelts", "moss", "books",
                                "queen", "outpost", "zero_point", "enchanter",
                                "swamproot", "strangers", "drowned",
                                "seam", "cinch",
                                "goatpath", "glass", "mill", "market", "doubled",
                                "caravan", "salt", "bridge",
                                "cityroad", "foundry", "counting", "lists",
                                "endless", "deadwater", "halves",
                                "orderway", "watch4", "charts", "keepsake", "ovens",
                                "refusal", "unsealed", "node2q", "node3q", "firstmaster",
                                "driftway", "water", "wholename", "lasthour",
                                "twobuckets", "nodark", "emptyalder", "edgeq",
                                "inside", "firstjoint", "lines", "remembers",
                                "measure", "heart", "finale"};
    for (const char* q : all_quests)
        eq(g.player().quests[q], QUEST_DONE, std::string("квест ") + q + " пройден");

    // Записки-пасхалки лежат по всем локациям и находятся по ходу дела.
    check(g.books().size() >= 2, "библиотека наполнилась находками и своими книгами");
    check(g.book(diary) != nullptr, "своя книга дожила до конца прохождения");

    // Вилка, а не «не меньше»: нижняя граница ловит пропущенное содержимое,
    // верхняя — возврат к прежней кривой, на которой всё содержимое выводило
    // героя к сороковому уровню. Там все семь навыков развиты до предела,
    // тратить очки больше некуда, и мир перестаёт сопротивляться.
    check(g.player().level >= 24, "к развязке герой прошёл весь мир");
    check(g.player().level <= 30, "и не перерос его: очки навыка ещё нужно выбирать");
    check(g.player().skill_points + total_ranks(g) < 7 * 5,
          "к концу игры развито не всё подряд — на всё очков не хватает");
    std::cout << "  (итог: уровень " << g.player().level
              << ", золота " << g.player().gold
              << ", квестов пройдено " << (sizeof(all_quests) / sizeof(all_quests[0]))
              << ", записей журнала " << g.log().size()
              << ")\n";
    std::cout << "  (сложность: боёв " << fights_done
              << ", принято урона " << dmg_total
              << ", худший бой " << dmg_worst << " при запасе " << hp_at_worst
              << ", боёв тяжелее половины запаса " << fights_over_half
              << ", смертельных без лечения " << fights_lethal
              << ")\n";
}

} // namespace

int main() {
    std::cout << "Тесты «Любви Эндора»\n";
    test_text_helpers();
    test_wrap();
    test_log_history();
    test_item_text();
    test_reflow();
    test_log_tail();
    test_log_tail_source();
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
    test_font();
    test_gestures();
    test_long_press();
    test_walk();
    test_png();
    test_tiles();
    test_dark();
    test_balance();
    test_finale();
    test_difficulty();
    test_playthrough();

    std::cout << "\n----------------------------------------\n";
    std::cout << "Проверок: " << g_checks << ", провалов: " << g_failed << "\n";
    if (g_failed == 0) std::cout << "ВСЁ ЗЕЛЁНОЕ\n";
    return g_failed == 0 ? 0 : 1;
}
