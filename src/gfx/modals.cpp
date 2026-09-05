#include "app.h"
#include "../content.h"

#include <SDL2/SDL.h>

namespace gfx {

// Текстовые окна — это абзац, а не список: у них своя прокрутка и одна
// кнопка. Определено снаружи анонимного пространства имён: тот же список
// нужен прокрутке в app.cpp, а вторая его копия однажды разошлась бы
// с первой.
bool is_text_modal(Modal::Kind k) {
    return k == Modal::Message || k == Modal::Help || k == Modal::Character ||
           k == Modal::Book || k == Modal::Ending || k == Modal::Log;
}

namespace {

const char* kind_caption(Modal::Kind k) {
    switch (k) {
        case Modal::GameMenu:    return "Меню";
        case Modal::Log:         return "Журнал";
        case Modal::Character:   return "Герой";
        case Modal::Inventory:   return "Сумка";
        case Modal::ItemMenu:    return "Предмет";
        case Modal::ShopItem:    return "Товар";
        case Modal::Amount:      return "Сколько продать";
        case Modal::Quests:      return "Задания";
        case Modal::Skills:      return "Навыки";
        case Modal::Portals:     return "Порталы";
        case Modal::Library:     return "Библиотека";
        case Modal::Book:        return "Книга";
        case Modal::Shop:        return "Торговля";
        case Modal::Enchant:     return "Зачарование";
        case Modal::EnchantPick: return "Выбор руны";
        case Modal::Help:        return "Управление";
        case Modal::TextInput:   return "";
        default:                 return "";
    }
}

} // namespace

// ------------------------------------------------------- содержимое окон

void App::collect_rows(const Modal& m, std::vector<Row>* rows,
                       std::vector<std::string>* ids) const {
    rows->clear();
    ids->clear();
    const Content& c = Content::get();
    const Player& p = g_.player();
    const Theme& th = theme();

    switch (m.kind) {
        case Modal::GameMenu:
            // Героя, сумки, заданий и навыков здесь нет: они кнопками внизу
            // окна, и второй путь к ним только удлинял дорогу.
            // Сохранение и выход стоят прямо тут, а не за отдельной паузой:
            // лишнее нажатие ради списка из трёх пунктов ничего не давало.
            if (p.portal_master) { rows->push_back(Row("Порталы")); ids->push_back("portals"); }
            rows->push_back(Row("Книги и записки")); ids->push_back("library");
            rows->push_back(Row("Журнал"));      ids->push_back("log");
            rows->push_back(Row("Управление"));  ids->push_back("help");
            rows->push_back(Row("Сохранить игру"));       ids->push_back("save");
            rows->push_back(Row("Загрузить сохранение")); ids->push_back("load");
            rows->push_back(Row("Выйти в главное меню")); ids->push_back("quit");
            break;

        case Modal::Inventory: {
            for (int i = 0; i < static_cast<int>(Slot::Count); ++i) {
                const std::string& id = p.equipped[static_cast<std::size_t>(i)];
                if (id.empty()) continue;
                const ItemDef* d = c.item(id);
                rows->push_back(Row(std::string("[надето] ") + (d ? d->name : id), th.good));
                ids->push_back("!" + to_str(i));
            }
            for (const ItemStack& st : p.inv) {
                const ItemDef* d = c.item(st.id);
                std::string line = d ? d->name : st.id;
                if (st.count > 1) line += "  x" + to_str(st.count);
                rows->push_back(Row(line));
                ids->push_back(st.id);
            }
            if (rows->empty()) { rows->push_back(Row("Пусто", th.faint)); ids->push_back(""); }
            break;
        }

        case Modal::ItemMenu: {
            const ItemDef* d = c.item(m.arg);
            if (!d) break;
            // Надетая вещь: её только снимают. Выбросить прямо с себя нельзя —
            // сперва снять, и это не лишний шаг, а понятный порядок.
            // Осмотра в списке нет ни там, ни там: характеристики стоят
            // прямо над кнопками, а отдельный пункт «Осмотреть» открывал бы
            // окно поверх окна ради того, что уже видно.
            if (m.slot >= 0) {
                rows->push_back(Row("Снять")); ids->push_back("unequip");
                break;
            }
            if (d->kind == ItemKind::Consumable) { rows->push_back(Row("Применить")); ids->push_back("use"); }
            if (d->kind == ItemKind::Book)       { rows->push_back(Row("Начать книгу")); ids->push_back("startbook"); }
            if (slot_for(d->kind) != Slot::Count){ rows->push_back(Row("Надеть")); ids->push_back("equip"); }
            rows->push_back(Row("Выбросить", th.warn)); ids->push_back("drop");
            break;
        }

        case Modal::ShopItem: {
            const ShopDef* sh = c.shop(m.arg);
            const ItemDef* d = c.item(m.item);
            if (!sh || !d) break;
            const int price = g_.buy_price(*sh, *d);
            const bool can = p.gold >= price;
            rows->push_back(Row("Купить за " + to_str(price) + " зол.",
                                can ? th.good : th.faint));
            ids->push_back(can ? "buy" : "");
            if (!can) {
                rows->push_back(Row("Не хватает " + to_str(price - p.gold) + " зол.", th.faint));
                ids->push_back("");
            }
            break;
        }

        case Modal::Quests: {
            for (const QuestDef& q : c.quests()) {
                const int st = g_.quest_stage(q.id);
                if (st == QUEST_NONE) continue;
                const bool done = (st == QUEST_DONE);
                std::string line = (q.secret ? "Тайна: " : "") + q.name +
                                   (done ? "  — пройдено" : "");
                rows->push_back(Row(line, done ? th.faint : th.text));
                ids->push_back(q.id);
            }
            if (rows->empty()) { rows->push_back(Row("Пока ничего", th.faint)); ids->push_back(""); }
            break;
        }

        case Modal::Skills: {
            for (const SkillDef& s : c.skills()) {
                int rank = 0;
                std::map<std::string, int>::const_iterator it = p.skills.find(s.id);
                if (it != p.skills.end()) rank = it->second;
                // Вложить можно в любой навык, был бы свободный балл: предела
                // у ранга нет, и гасить строку по «уже некуда» больше нечем.
                const bool can = p.skill_points > 0;
                rows->push_back(Row(s.name + "  ранг " + to_str(rank) +
                                    "  — " + s.desc, can ? th.text : th.faint));
                ids->push_back(s.id);
            }
            break;
        }

        case Modal::Portals: {
            rows->push_back(Row("Поставить портал здесь")); ids->push_back("place");
            rows->push_back(Row("Снять портал под ногами")); ids->push_back("remove");
            for (const Portal& pt : p.portals) {
                const Location* l = g_.world().location(pt.loc);
                rows->push_back(Row(std::string("· ") + (l ? l->name : pt.loc) +
                                    " (" + to_str(pt.pos.x) + "," + to_str(pt.pos.y) + ")",
                                    th.faint));
                ids->push_back("");
            }
            break;
        }

        case Modal::Library: {
            for (const Book& b : g_.books()) {
                rows->push_back(Row(b.title + (b.readonly ? "  (найдено)" : ""),
                                    b.readonly ? th.faint : th.text));
                ids->push_back(b.id);
            }
            if (rows->empty()) { rows->push_back(Row("Библиотека пуста", th.faint)); ids->push_back(""); }
            break;
        }

        case Modal::Dialogue: {
            const DlgNode* n = c.node(m.node);
            if (!n) break;
            for (std::size_t i = 0; i < n->options.size(); ++i) {
                if (!g_.option_available(n->options[i])) continue;
                rows->push_back(Row(n->options[i].text));
                ids->push_back(to_str(static_cast<int>(i)));
            }
            break;
        }

        case Modal::Shop: {
            const ShopDef* s = c.shop(m.arg);
            if (!s) break;
            if (!m.selling) {
                for (const std::string& gid : s->goods) {
                    const ItemDef* d = c.item(gid);
                    if (!d) continue;
                    const int price = g_.buy_price(*s, *d);
                    rows->push_back(Row(d->name + "   " + to_str(price) + " зол.",
                                        p.gold >= price ? th.text : th.faint));
                    ids->push_back(gid);
                }
            } else {
                for (const ItemStack& st : p.inv) {
                    const ItemDef* d = c.item(st.id);
                    if (!d) continue;
                    rows->push_back(Row(d->name + " x" + to_str(st.count) + "   " +
                                        to_str(g_.sell_price(*s, *d)) + " зол."));
                    ids->push_back(st.id);
                }
            }
            if (rows->empty()) { rows->push_back(Row("Ничего нет", th.faint)); ids->push_back(""); }
            break;
        }

        case Modal::Enchant: {
            for (int i = 0; i < static_cast<int>(Slot::Count); ++i) {
                const std::string& id = p.equipped[static_cast<std::size_t>(i)];
                if (id.empty()) continue;
                const ItemDef* d = c.item(id);
                const bool can = g_.can_enchant(id);
                std::map<std::string, std::string>::const_iterator it = p.enchants.find(id);
                std::string line = d ? d->name : id;
                if (it != p.enchants.end()) {
                    const EnchantDef* ed = c.enchant(it->second);
                    line += "  — " + std::string(ed ? ed->name : it->second);
                }
                rows->push_back(Row(line, can ? th.text : th.faint));
                ids->push_back(id);
            }
            if (rows->empty()) { rows->push_back(Row("Зачаровывать нечего", th.faint)); ids->push_back(""); }
            break;
        }

        case Modal::EnchantPick: {
            for (const EnchantDef& e : c.enchants()) {
                std::string line = e.name + "   " + to_str(e.price) + " зол.";
                if (!e.reagent.empty()) {
                    const ItemDef* rd = c.item(e.reagent);
                    line += " + " + std::string(rd ? rd->name : e.reagent) +
                            " x" + to_str(e.reagent_count);
                }
                const bool afford = p.gold >= e.price &&
                                    (e.reagent.empty() ||
                                     g_.count_item(e.reagent) >= e.reagent_count);
                rows->push_back(Row(line, afford ? th.text : th.faint));
                ids->push_back(e.id);
            }
            break;
        }

        default:
            break;
    }
}

// ---------------------------------------------------------------- раскладка

std::string App::modal_title(const Modal& m) const {
    std::string title = kind_caption(m.kind);
    if (m.kind == Modal::Message || m.kind == Modal::Ending || m.kind == Modal::Book)
        return m.title;
    if (m.kind == Modal::Dialogue) {
        const NpcDef* n = Content::get().npc(m.arg);
        return n ? n->name : std::string();
    }
    if (m.kind == Modal::Inventory)
        return "Сумка · золото: " + to_str(g_.player().gold);
    if (m.kind == Modal::Shop) {
        const ShopDef* s = Content::get().shop(m.arg);
        return std::string(s ? s->name : "Торговля") + " · " +
               (m.selling ? "продажа" : "покупка") + " · " + to_str(g_.player().gold) + " зол.";
    }
    return title;
}

void App::dialogue_layout(const Modal& m, const Rect& area, int n,
                          std::vector<Rect>* out) const {
    (void)m;
    const int bh = c_.touch_unit();
    const int need = n * (bh + 6);
    column_of(Rect(area.x, area.y + area.h - need, area.w, need), n, 6, out);
}

// Окна, у которых над списком действий стоит текст характеристик. Список
// один: по нему считается высота, делится область и рисуется шапка, и
// разойтись эти три места не могут.
static bool has_header(Modal::Kind k) {
    return k == Modal::ItemMenu || k == Modal::ShopItem;
}

Rect App::modal_body(const Modal& m, Rect* frame_out, std::vector<Rect>* buttons,
                     std::vector<std::string>* labels, Rect* head_out) const {
    labels->clear();
    switch (m.kind) {
        case Modal::Inventory:   labels->push_back("Закрыть"); break;
        case Modal::Shop:        labels->push_back(m.selling ? "Покупать" : "Продавать");
                                 labels->push_back("Закрыть"); break;
        case Modal::Book: {
            // Найденные записки не правятся — у них только «Закрыть».
            const Book* b = g_.book(m.arg);
            if (b && !b->readonly) {
                labels->push_back("Дописать");
                labels->push_back("Название");
                labels->push_back("Стереть");
            }
            labels->push_back("Закрыть");
            break;
        }
        case Modal::TextInput:
            labels->push_back("Готово");
            labels->push_back("Отмена");
            break;
        case Modal::Amount:
            labels->push_back("Продать");
            labels->push_back("Отмена");
            break;
        case Modal::Dialogue:    break;                        // варианты сами и есть кнопки
        default:                 labels->push_back("Закрыть"); break;
    }

    // Окно ужимается под содержимое: список из трёх пунктов не должен
    // занимать пол-экрана только потому, что панель одна на всех.
    const int cw = c_.cell_w(), ch = c_.cell_h();
    int want_w = 44 * cw;
    int want_h = 22 * ch;
    if (m.kind == Modal::TextInput) {
        want_w = 40 * cw;
        want_h = c_.touch_unit() + ch * 3;
    } else if (m.kind == Modal::Log) {
        // Журнал ужимается под себя, как всякое другое окно: в начале партии
        // записей десяток, и растягивать окно на весь экран ради них незачем.
        // Ширина панели от высоты не зависит (см. panel_rect_px), поэтому её
        // можно узнать заранее и разложить журнал ровно по той ширине, по
        // какой он потом и рисуется, — высота тогда считается по настоящему
        // числу строк, а не на глаз.
        const Rect probe = panel_rect_px(c_, !modal_title(m).empty(), want_w, ch, 0);
        const int rows = static_cast<int>(log_lines(probe.w / cw).size());
        int need = (rows + 1) * ch;
        const int cap = c_.height() - ch * 8;
        if (need > cap) need = cap;
        if (need < ch * 4) need = ch * 4;
        want_h = need;
    } else if (is_text_modal(m.kind) && !m.body.empty()) {
        // Текст, известный заранее, задаёт окну высоту сам — так же, как
        // список задаёт её числом пунктов. Иначе окно фиксированной высоты
        // молча обрезает хвост, и до последних строк надо докручивать,
        // не зная, что они там есть. Экран ограничивает panel_rect_px.
        int lines = 1;
        for (std::size_t i = 0; i < m.body.size(); ++i)
            if (m.body[i] == '\n') ++lines;
        int need = (lines + 1) * ch;
        const int cap = c_.height() - ch * 8;
        if (need > cap) need = cap;
        if (need < ch * 4) need = ch * 4;
        want_h = need;
    } else if (m.kind == Modal::Amount) {
        // Строка «сколько и почём», ползунок ростом с палец и поле ввода.
        want_w = 34 * cw;
        want_h = ch * 3 + c_.touch_unit() * 2 + 12;
    } else if (!is_text_modal(m.kind) && m.kind != Modal::Dialogue) {
        std::vector<Row> rows;
        std::vector<std::string> ids;
        collect_rows(m, &rows, &ids);
        const int row_h = m.list.row_height(c_);
        int need = static_cast<int>(rows.size()) * row_h;
        // Характеристики над списком — часть содержимого, и место им нужно
        // такое же настоящее, как строкам: иначе окно вырастет ровно на
        // список, а текст его молча обрежет.
        if (has_header(m.kind) && !m.body.empty()) {
            int lines = 1;
            for (std::size_t i = 0; i < m.body.size(); ++i)
                if (m.body[i] == '\n') ++lines;
            need += (lines + 1) * ch;
        }
        const int cap = c_.height() - ch * 8;
        if (need > cap) need = cap;
        if (need < row_h) need = row_h;
        want_h = need;
    }
    if (!labels->empty()) want_h += c_.touch_unit() + 6;

    Rect area = panel_rect_px(c_, !modal_title(m).empty(), want_w, want_h, frame_out);

    const int bh = c_.touch_unit();
    if (!labels->empty()) {
        Rect brow(area.x, area.y + area.h - bh, area.w, bh);
        row_of(brow, static_cast<int>(labels->size()), 6, buttons);
        area.h -= bh + 6;
    } else {
        buttons->clear();
    }

    // Шапка отрезается от содержимого последней — после кнопок, чтобы список
    // получил ровно то, что осталось. Отрезается один раз, здесь: рисование
    // и попадание пальцем берут результат отсюда и потому не спорят о том,
    // где кончается текст и начинается первая строка списка.
    if (head_out) *head_out = Rect(area.x, area.y, area.w, 0);
    if (has_header(m.kind) && !m.body.empty()) {
        // Строки считаются с переносом по ширине окна — тем же, каким текст
        // потом рисуется. Считать по '\n' было бы короче и неверно: длинная
        // строка описания займёт две, и список наехал бы на её хвост.
        int hh = text_block_rows(c_, Rect(area.x, area.y, area.w, area.h), m.body) * ch;
        if (hh > area.h - m.list.row_height(c_)) hh = area.h - m.list.row_height(c_);
        if (hh < 0) hh = 0;
        if (head_out) *head_out = Rect(area.x, area.y, area.w, hh);
        area.y += hh + ch / 2;
        area.h -= hh + ch / 2;
        if (area.h < 0) area.h = 0;
    }
    return area;
}

// ---------------------------------------------------------------- рисование

void App::draw_modal(Modal& m) {
    const Theme& th = theme();
    Rect frame;
    std::vector<Rect> btns;
    std::vector<std::string> labels;
    Rect head;
    const Rect area = modal_body(m, &frame, &btns, &labels, &head);
    // Панель рисуется по уже посчитанной геометрии — ровно той же.
    {
        const Theme& tt = theme();
        const int ch2 = c_.cell_h();
        const int pad = ch2 / 2 + 2;
        c_.fill(frame, tt.panel);
        c_.frame(frame, tt.border, 2);
        const std::string title = modal_title(m);
        if (!title.empty()) {
            c_.text(frame.x + pad, frame.y + pad, title, tt.accent, c_.scale());
            c_.fill(Rect(frame.x + pad, frame.y + pad + ch2 + ch2 / 4,
                         frame.w - pad * 2, 1), tt.border);
        }
    }

    if (m.kind == Modal::Dialogue) {
        // Реплика сверху, ответы — кнопками во всю ширину: попасть пальцем
        // в строку диалога должно быть так же легко, как в кнопку.
        const DlgNode* n = Content::get().node(m.node);
        if (!n) { pop(); return; }
        std::vector<Row> rows;
        std::vector<std::string> ids;
        collect_rows(m, &rows, &ids);

        std::vector<Rect> opt;
        dialogue_layout(m, area, static_cast<int>(rows.size()), &opt);
        const int need = static_cast<int>(rows.size()) * (c_.touch_unit() + 6);
        const int text_h = area.h - need > c_.cell_h() * 3 ? area.h - need : c_.cell_h() * 3;
        // Проза переливается под ширину окна: в исходнике она разбита на
        // строки под какую-то одну ширину, и без этого фраза обрывалась бы
        // посреди, не дотянув до края.
        text_block(c_, Rect(area.x, area.y, area.w, text_h), reflow(n->text),
                   th.text, m.scroll);
        for (std::size_t i = 0; i < rows.size(); ++i)
            button(c_, opt[i], rows[i].text, true, false);
        return;
    }

    if (m.kind == Modal::Amount) {
        const Theme& tt = theme();
        const Content& cc = Content::get();
        const ItemDef* d = cc.item(m.item);
        const ShopDef* sh = cc.shop(m.arg);
        const int have = g_.count_item(m.item);
        const int one = (d && sh) ? g_.sell_price(*sh, *d) : 0;
        clamp_amount(m);

        // Строка сверху: что продаём, сколько есть и на сколько выйдет.
        // Итог считается тут же: главный вопрос у прилавка — сколько дадут.
        const int ch4 = c_.cell_h();
        c_.text(area.x, area.y, (d ? d->name : m.item) + "  (есть " + to_str(have) + ")",
                tt.text, c_.scale());
        c_.text(area.x, area.y + ch4,
                to_str(m.amount) + " x " + to_str(one) + " = " +
                to_str(m.amount * one) + " зол.", tt.good, c_.scale());

        // Ползунок. Заполнен ровно на долю выбранного от имеющегося, поэтому
        // «сколько от всего» видно, не читая чисел.
        const Rect tr = amount_track(area);
        c_.fill(tr, Color(0, 0, 0, 90));
        const int span = have > 1 ? have - 1 : 1;
        const int fill = tr.w * (m.amount - 1) / span;
        c_.fill(Rect(tr.x, tr.y, fill, tr.h), tt.border);
        c_.frame(tr, tt.border, 1);
        // Бегунок: по нему видно, куда попадать пальцем.
        const int kw = c_.cell_w();
        int kx = tr.x + fill - kw / 2;
        if (kx < tr.x) kx = tr.x;
        if (kx > tr.x + tr.w - kw) kx = tr.x + tr.w - kw;
        c_.fill(Rect(kx, tr.y, kw, tr.h), tt.accent);

        // Поле ввода и кнопки шага. Набранное показывается как есть — обрезание
        // случается при подтверждении, иначе цифру нельзя было бы стереть,
        // чтобы набрать другую.
        const Rect row(area.x, tr.y + tr.h + 6, area.w, c_.touch_unit());
        std::vector<Rect> cells;
        row_of(row, 3, 6, &cells);
        button(c_, cells[0], "-", true, false);
        c_.fill(cells[1], tt.panel);
        c_.frame(cells[1], tt.border, 1);
        const std::string shown = m.buffer.empty() ? std::string("_") : m.buffer;
        c_.text(cells[1].x + (cells[1].w - c_.cell_w() * static_cast<int>(shown.size())) / 2,
                cells[1].y + (cells[1].h - ch4) / 2, shown, tt.text, c_.scale());
        button(c_, cells[2], "+", true, false);

        for (std::size_t i = 0; i < btns.size(); ++i)
            button(c_, btns[i], labels[i], true, false);
        return;
    }

    if (m.kind == Modal::TextInput) {
        const int ch3 = c_.cell_h();
        c_.text(area.x, area.y, m.title, th.faint, c_.scale());
        Rect field(area.x, area.y + ch3 + ch3 / 2, area.w, c_.touch_unit());
        c_.fill(field, Color(30, 34, 42, 250));
        c_.frame(field, th.accent, 2);
        // Виден хвост строки: набирающий смотрит туда, где курсор.
        std::size_t fit = static_cast<std::size_t>(field.w / c_.cell_w());
        fit = fit > 2 ? fit - 2 : 1;
        std::string shown = m.buffer;
        while (utf8_len(shown) > fit) {
            std::size_t k = 1;
            while (k < shown.size() && (static_cast<unsigned char>(shown[k]) & 0xC0) == 0x80) ++k;
            shown.erase(0, k);
        }
        c_.text(field.x + c_.cell_w() / 2, field.y + (field.h - ch3) / 2, shown + "_",
                th.text, c_.scale());
        for (std::size_t i = 0; i < btns.size(); ++i)
            button(c_, btns[i], labels[i], true, false);
        return;
    }

    if (m.kind == Modal::Log) {
        // Журнал разложен заранее и лежит готовым, поэтому строки рисуются
        // как есть. Полоса справа показывает, где ты в истории партии:
        // без неё непонятно, сколько ещё осталось до начала.
        const std::vector<std::string>& ls = log_lines(area.w / c_.cell_w());
        const int lch = c_.cell_h();
        const int vis = lch > 0 ? area.h / lch : 0;
        int maxs = static_cast<int>(ls.size()) - vis;
        if (maxs < 0) maxs = 0;
        if (m.scroll > maxs) m.scroll = maxs;
        // Цвета — те же, что в ленте под картой: одно событие не должно
        // выглядеть в двух местах по-разному. Обычные строки здесь читают,
        // а не проглядывают, поэтому они не приглушены, как в ленте.
        const std::vector<unsigned char>& lt = log_line_tones();
        std::vector<Color> cols(ls.size(), th.text);
        for (std::size_t i = 0; i < ls.size() && i < lt.size(); ++i)
            if (lt[i] != 0) cols[i] = tone_color(lt[i]);
        text_lines(c_, area, ls, cols, m.scroll);
        scrollbar(c_, area, static_cast<int>(ls.size()), vis, m.scroll);
    } else if (is_text_modal(m.kind)) {
        std::string body = m.body;
        if (m.kind == Modal::Character) {
            const Player& p = g_.player();
            const Stats t = g_.total();
            const Stats nb = g_.total_no_stance();
            // Раса и ремесло выбираются один раз при создании героя и дальше
            // нигде не показывались — а бонусы они дают те самые, что стоят
            // ниже в столбце, и без них непонятно, откуда взялась разница.
            const RaceDef* race_def = Content::get().race(p.race);
            const SpecDef* spec_def = Content::get().spec(p.spec);
            body  = p.name + ", уровень " + to_str(p.level) + "\n";
            body += std::string(race_def ? race_def->name : p.race) + ", " +
                    std::string(spec_def ? spec_def->name : p.spec) + "\n";
            body += "Опыт: " + to_str(p.exp) + " / " + to_str(g_.exp_to_next()) + "\n";
            body += "Золото: " + to_str(p.gold) + "\n\n";
            body += "Здоровье      " + to_str(p.hp) + " / " + to_str(t.max_hp) + "\n";
            body += "Очки действия " + to_str(p.ap) + " / " + to_str(t.max_ap) + "\n";
            body += "Урон          " + to_str(t.dmg_min) + " - " + to_str(t.dmg_max) + "\n";
            body += "Меткость      " + to_str(t.attack) + "%\n";
            body += "Блок          " + to_str(t.block) + "%\n";
            body += "Броня         " + to_str(t.armor) + "\n";
            body += "Крит          " + to_str(t.crit) + "%\n";
            body += "Атака стоит   " + to_str(g_.attack_cost()) + " AP\n\n";
            body += "Стойка: " + std::string(stance_name(p.stance)) + "\n";
            body += std::string(stance_hint(p.stance)) + "\n";
            body += "Без стойки: мет " + to_str(nb.attack) + "%, блок " +
                    to_str(nb.block) + "%, броня " + to_str(nb.armor) + "\n\n";
            // Действующие эффекты стоят здесь же, а не отдельным окном:
            // они меняют те самые числа, что написаны выше, и смотреть их
            // порознь значило бы гадать, откуда взялась разница.
            body += "Что действует:\n";
            if (p.effects.empty()) body += "  ничего\n";
            for (const ActiveEffect& e : p.effects) {
                const EffectDef* ed = Content::get().effect(e.id);
                body += "  " + std::string(ed ? ed->name : e.id) +
                        " — сила " + to_str(e.power) +
                        ", ходов " + to_str(e.turns) + "\n";
            }
            body += "\n";
            body += "Навыки:\n";
            bool any = false;
            for (const auto& kv : p.skills) {
                if (kv.second <= 0) continue;
                any = true;
                const SkillDef* sd = Content::get().skill(kv.first);
                body += "  " + std::string(sd ? sd->name : kv.first) +
                        " — ранг " + to_str(kv.second) + "\n";
            }
            if (!any) body += "  пока никаких\n";
        } else if (m.kind == Modal::Book) {
            const Book* b = g_.book(m.arg);
            body.clear();
            if (b && b->readonly) {
                // Найденная записка — это проза, и номера строк ей ни к чему:
                // править её нельзя, а читается она сплошным текстом.
                body = reflow(b->lines);
            } else if (b) {
                // Своя книга правится построчно, поэтому строки нумерованы
                // и остаются такими, как их набрали.
                for (std::size_t i = 0; i < b->lines.size(); ++i) {
                    std::string num = to_str(static_cast<int>(i) + 1);
                    while (num.size() < 2) num = " " + num;
                    body += num + " " +
                            (b->lines[i].empty() ? std::string("·") : b->lines[i]) + "\n";
                }
            }
            if (body.empty()) body = "(пусто)";
        }
        // Прокрутка ограничена и сверху: без этого свайп уводил текст выше
        // окна, и «Герой» показывал пустую панель, будто в нём ничего нет.
        // Полоса справа показывает, что содержимое не кончилось, — окно
        // упирается в потолок высоты и обрывает хвост молча.
        const int tch = c_.cell_h();
        const int trows = text_block_rows(c_, area, body);
        const int tvis = tch > 0 ? area.h / tch : 0;
        int tmax = trows - tvis;
        if (tmax < 0) tmax = 0;
        if (m.scroll > tmax) m.scroll = tmax;
        text_block(c_, area, body, th.text, m.scroll);
        scrollbar(c_, area, trows, tvis, m.scroll);
    } else {
        std::vector<Row> rows;
        std::vector<std::string> ids;
        collect_rows(m, &rows, &ids);
        // Характеристики над списком действий. Место под них уже отрезано
        // в modal_body, поэтому здесь остаётся только нарисовать.
        if (head.h > 0) text_block(c_, head, m.body, th.text, 0);
        m.list.clamp(static_cast<int>(rows.size()), m.list.visible_rows(c_, area));
        m.list.draw(c_, area, rows);
    }

    for (std::size_t i = 0; i < btns.size(); ++i)
        button(c_, btns[i], labels[i], true, false);
}

// -------------------------------------------------------------------- ввод

void App::modal_tap(Modal& m, int x, int y) {
    Rect frame;
    std::vector<Rect> btns;
    std::vector<std::string> labels;
    const Rect area = modal_body(m, &frame, &btns, &labels);

    // Тап мимо окна закрывает его: так на телефоне закрывается всё.
    if (!frame.contains(x, y)) { close_top(); return; }

    for (std::size_t i = 0; i < btns.size(); ++i) {
        if (!btns[i].contains(x, y)) continue;
        if (labels[i] == "Закрыть") { close_top(); return; }
        if (labels[i] == "Продавать") { m.selling = true;  m.list = ListView(); return; }
        if (labels[i] == "Покупать")  { m.selling = false; m.list = ListView(); return; }
        if (labels[i] == "Готово")    { commit_text_input(m); return; }
        if (labels[i] == "Продать")   { commit_amount(m); return; }
        if (labels[i] == "Отмена")    { pop(); return; }
        if (labels[i] == "Дописать") {
            push_text_input("Новая строка", m.arg, BOOK_APPEND, "",
                            static_cast<std::size_t>(BOOK_MAX_CHARS));
            return;
        }
        if (labels[i] == "Название") {
            const Book* b = g_.book(m.arg);
            push_text_input("Название книги", m.arg, -1, b ? b->title : std::string(),
                            static_cast<std::size_t>(BOOK_TITLE_MAX));
            return;
        }
        if (labels[i] == "Стереть") {
            const Book* b = g_.book(m.arg);
            if (b && !b->lines.empty())
                g_.book_remove_line(m.arg, static_cast<int>(b->lines.size()) - 1);
            return;
        }
        return;
    }

    if (m.kind == Modal::Dialogue) {
        std::vector<Row> rows;
        std::vector<std::string> ids;
        collect_rows(m, &rows, &ids);
        std::vector<Rect> opt;
        dialogue_layout(m, area, static_cast<int>(rows.size()), &opt);
        for (std::size_t i = 0; i < opt.size(); ++i)
            if (opt[i].contains(x, y)) { activate_row(m, static_cast<int>(i)); return; }
        return;
    }

    if (m.kind == Modal::Amount) {
        const Rect tr = amount_track(area);
        if (tr.contains(x, y)) {
            // Тап по дорожке — это и есть выбор количества: попал в середину,
            // выбрал половину. Ведение пальцем по ней делает то же самое,
            // просто много раз подряд (см. follow_finger).
            const int have = g_.count_item(m.item);
            const int span = have > 1 ? have - 1 : 1;
            int v = 1;
            if (tr.w > 0) v = 1 + (x - tr.x) * span / tr.w;
            m.amount = v;
            clamp_amount(m);
            m.buffer = to_str(m.amount);
            return;
        }
        const Rect row(area.x, tr.y + tr.h + 6, area.w, c_.touch_unit());
        std::vector<Rect> cells;
        row_of(row, 3, 6, &cells);
        if (cells[0].contains(x, y)) {
            --m.amount; clamp_amount(m); m.buffer = to_str(m.amount); return;
        }
        if (cells[2].contains(x, y)) {
            ++m.amount; clamp_amount(m); m.buffer = to_str(m.amount); return;
        }
        return;
    }

    if (m.kind == Modal::TextInput) return;      // само поле — не действие
    if (is_text_modal(m.kind)) { close_top(); return; }

    std::vector<Row> rows;
    std::vector<std::string> ids;
    collect_rows(m, &rows, &ids);
    const int hit = m.list.hit(c_, area, x, y, static_cast<int>(rows.size()));
    if (hit < 0) return;
    // Тап по пункту — это и есть выбор. Промахи ловятся ростом строки под
    // палец, а не вторым подтверждающим касанием.
    m.list.cursor = hit;
    activate_row(m, hit);
}

// Сколько штук продаём. Границы одни на все пути: ползунок, «−»/«+» и набор
// с клавиатуры зовут именно её, и потому не могут разойтись в том, что
// считают допустимым. Больше, чем есть, обрезается до имеющегося; меньше
// одной — до одной.
void App::clamp_amount(Modal& m) {
    const int have = g_.count_item(m.item);
    if (m.amount > have) m.amount = have;
    if (m.amount < 1) m.amount = 1;
}

// Набранное превращается в число, обрезается и пишется обратно в поле. Поле
// поэтому никогда не показывает невозможного: набрал двадцать, имея три, —
// увидел три сразу, а не после нажатия «Продать». Разбор нарочно свой, а не
// atoi: длинный набор не должен переполнить int по дороге к обрезанию.
void App::amount_from_text(Modal& m) {
    if (m.buffer.empty()) { m.amount = 1; return; }   // пустое поле стереть можно
    long v = 0;
    for (std::size_t i = 0; i < m.buffer.size(); ++i) {
        const char ch = m.buffer[i];
        if (ch < '0' || ch > '9') continue;
        v = v * 10 + (ch - '0');
        if (v > 1000000) { v = 1000000; break; }      // дальше считать нечего
    }
    m.amount = static_cast<int>(v);
    clamp_amount(m);
    m.buffer = to_str(m.amount);
}

// Дорожка ползунка. Одна на рисование и на попадание пальцем — разъехавшись,
// они показывали бы одно, а выставляли другое.
Rect App::amount_track(const Rect& area) const {
    const int h = c_.touch_unit();
    return Rect(area.x, area.y + c_.cell_h() * 2, area.w, h);
}

// Ведение пальцем по дорожке. Значение берётся из того же расчёта, что и у
// тапа по ней: одна дорожка — одно правило, куда какой пиксель попадает.
void App::drag_amount(Modal& m) {
    // Касание, которым окно открыли, ползунок не двигает. Палец в этот миг
    // лежит ровно там, где всплыла дорожка, и без этой оговорки окно
    // открывалось бы сразу с количеством, выставленным по случайной точке
    // нажатия, а не по тому, что в нём написано.
    if (m.born_press == ptr_.press_id()) return;

    Rect frame;
    std::vector<Rect> btns;
    std::vector<std::string> labels;
    const Rect area = modal_body(m, &frame, &btns, &labels);
    const Rect tr = amount_track(area);
    if (!tr.contains(ptr_.press_x(), ptr_.press_y())) return;

    const int have = g_.count_item(m.item);
    const int span = have > 1 ? have - 1 : 1;
    if (tr.w > 0) m.amount = 1 + (ptr_.press_x() - tr.x) * span / tr.w;
    clamp_amount(m);
    m.buffer = to_str(m.amount);
}

void App::modal_long(Modal& m, int x, int y) {
    // Пока удержание значит одно: продать пачкой. В остальных окнах второго
    // действия над строкой нет, и выдумывать его на пустом месте незачем.
    if (m.kind != Modal::Shop || !m.selling) return;

    Rect frame;
    std::vector<Rect> btns;
    std::vector<std::string> labels;
    const Rect area = modal_body(m, &frame, &btns, &labels);
    if (!frame.contains(x, y)) return;

    std::vector<Row> rows;
    std::vector<std::string> ids;
    collect_rows(m, &rows, &ids);
    const int hit = m.list.hit(c_, area, x, y, static_cast<int>(rows.size()));
    if (hit < 0 || hit >= static_cast<int>(ids.size())) return;
    const std::string id = ids[static_cast<std::size_t>(hit)];
    if (id.empty()) return;
    if (g_.count_item(id) <= 1) return;   // одну штуку и так продают тапом

    Modal am(Modal::Amount);
    am.arg    = m.arg;      // лавка
    am.item   = id;
    am.amount = g_.count_item(id);   // чаще всего сбывают всё разом
    am.buffer = to_str(am.amount);
    push_modal(am);
}

void App::apply_dialogue_option(const DlgOption& o) {
    Modal& m = stack_.back();
    const NpcDef* npc = Content::get().npc(m.arg);
    std::string shop;
    bool ench = false;
    g_.apply_option(o, npc ? npc->shop : std::string(), &shop, &ench);

    const std::string ending = g_.take_pending_ending();
    const std::string next = o.next;
    pop();                                   // разговор всегда уступает место

    if (!ending.empty()) {
        const EndingDef* e = Content::get().ending(ending);
        if (e) {
            std::string body;
            body = reflow(e->lines);
            Modal em(Modal::Ending);
            em.title = "РАЗВЯЗКА: " + e->name;
            em.body = body;
            push_modal(em);
        }
        return;
    }
    if (!shop.empty()) {
        Modal sm(Modal::Shop);
        sm.arg = shop;
        push_modal(sm);
        return;
    }
    if (ench) { push(Modal::Enchant); return; }
    if (!next.empty() && npc) {
        Modal dm(Modal::Dialogue);
        dm.arg = npc->id;
        dm.node = next;
        push_modal(dm);
    }
}

void App::activate_row(Modal& m, int index) {
    std::vector<Row> rows;
    std::vector<std::string> ids;
    collect_rows(m, &rows, &ids);
    if (index < 0 || index >= static_cast<int>(ids.size())) return;
    const std::string id = ids[static_cast<std::size_t>(index)];
    const Content& c = Content::get();

    switch (m.kind) {
        case Modal::GameMenu: {
            pop();
            if (id == "portals")        push(Modal::Portals);
            else if (id == "library")   push(Modal::Library);
            else if (id == "log")       push(Modal::Log);
            else if (id == "help")      push(Modal::Help);
            else if (id == "save")      { save_game(); pop(); }
            else if (id == "load")      { load_game(); }
            else if (id == "quit")      { close_all(); mode_ = MODE_MENU; menu_list_ = ListView(); }
            return;
        }

        case Modal::Inventory: {
            if (id.empty()) return;
            // И надетая вещь, и лежащая в сумке открывают одно и то же окно с
            // характеристиками. Раньше клик по надетому снимал её сразу, и
            // промах по строке разоблачал героя посреди боя, ничего не спросив.
            Modal im(Modal::ItemMenu);
            if (id[0] == '!') {
                const int sl = to_int(id.substr(1));
                im.slot = sl;
                im.arg = g_.player().equipped[static_cast<std::size_t>(sl)];
            } else {
                im.arg = id;
            }
            if (const ItemDef* dd = c.item(im.arg)) im.body = item_desc(*dd);
            push_modal(im);
            return;
        }

        case Modal::ItemMenu: {
            const std::string item = m.arg;
            // В бою предмет стоит очков действия. Обычный use_item эту цену
            // не берёт, и через сумку можно было бы лечиться даром.
            if (id == "use") {
                if (g_.combat().active) g_.combat_use_item(item);
                else                    g_.use_item(item);
                pop();
            }
            else if (id == "equip")   { g_.equip(item); pop(); }
            else if (id == "unequip") { g_.unequip(static_cast<Slot>(m.slot)); pop(); }
            else if (id == "drop")    { g_.drop_item(item); pop(); }
            else if (id == "startbook") {
                pop();
                if (g_.start_book("Дневник") && !g_.books().empty())
                    push_text_input("Как назвать книгу?", g_.books().back().id, -1, "Дневник",
                                    static_cast<std::size_t>(BOOK_TITLE_MAX));
            } else if (id == "look") {
                const ItemDef* dd = c.item(item);
                const std::string body = dd ? item_desc(*dd) : std::string();
                pop();
                push_message("Предмет", body);
            }
            return;
        }

        case Modal::Quests: {
            if (id.empty()) return;
            const QuestDef* q = c.quest(id);
            if (!q) return;
            const int st = g_.quest_stage(id);
            push_message(q->name, c.quest_stage_text(id, st));
            return;
        }

        case Modal::Skills:
            if (!id.empty() && g_.learn_skill(id)) g_.msg("Навык улучшен.");
            return;

        case Modal::Portals:
            if (id == "place") { g_.place_portal(); pop(); }
            else if (id == "remove") { g_.remove_portal_here(); pop(); }
            return;

        case Modal::Library: {
            if (id.empty()) return;
            const Book* b = g_.book(id);
            Modal bm(Modal::Book);
            bm.arg = id;
            bm.title = b ? b->title : std::string("Книга");
            push_modal(bm);
            return;
        }

        case Modal::Dialogue: {
            const DlgNode* n = c.node(m.node);
            if (!n) { pop(); return; }
            const int want = to_int(id);
            int seen = 0;
            for (std::size_t i = 0; i < n->options.size(); ++i) {
                if (!g_.option_available(n->options[i])) continue;
                if (seen == index || static_cast<int>(i) == want) {
                    const DlgOption opt = n->options[i];
                    apply_dialogue_option(opt);
                    return;
                }
                ++seen;
            }
            pop();
            return;
        }

        case Modal::Shop: {
            if (id.empty()) return;
            const ShopDef* s = c.shop(m.arg);
            if (!s) return;
            if (m.selling) { g_.sell(*s, id); return; }
            // Покупка идёт через витрину: характеристики и сравнение с надетым
            // видны до того, как деньги ушли. Вслепую покупать нечего.
            const ItemDef* d = c.item(id);
            if (!d) return;
            Modal si(Modal::ShopItem);
            si.arg  = m.arg;
            si.item = id;
            // Гнездо и надетое в нём разрешает вызывающий: описание вещи —
            // знание о содержимом, а не о том, кто во что одет.
            const Slot sl = slot_for(d->kind);
            const ItemDef* worn = 0;
            if (sl != Slot::Count) {
                const std::string& wid = g_.player().equipped[static_cast<std::size_t>(sl)];
                if (!wid.empty()) worn = c.item(wid);
            }
            si.body = item_desc(*d) + compare_worn(*d, worn);
            push_modal(si);
            return;
        }

        case Modal::ShopItem: {
            if (id != "buy") return;
            const ShopDef* s = c.shop(m.arg);
            const ItemDef* d = c.item(m.item);
            if (!s || !d) return;
            if (g_.buy(*s, m.item)) pop();
            return;
        }

        case Modal::Enchant: {
            if (id.empty() || !g_.can_enchant(id)) return;
            Modal em(Modal::EnchantPick);
            em.arg = id;
            push_modal(em);
            return;
        }

        case Modal::EnchantPick: {
            const std::string item = m.arg;
            if (g_.enchant_item(item, id)) { pop(); pop(); }
            return;
        }

        default:
            pop();
            return;
    }
}

} // namespace gfx
