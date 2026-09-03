#include "app.h"
#include "../content.h"

#include <SDL2/SDL.h>

namespace gfx {

namespace {

const char* kind_caption(Modal::Kind k) {
    switch (k) {
        case Modal::GameMenu:    return "Меню";
        case Modal::Pause:       return "Пауза";
        case Modal::Character:   return "Герой";
        case Modal::Inventory:   return "Сумка";
        case Modal::ItemMenu:    return "Предмет";
        case Modal::Quests:      return "Задания";
        case Modal::Skills:      return "Навыки";
        case Modal::Effects:     return "Что действует";
        case Modal::Portals:     return "Порталы";
        case Modal::Library:     return "Библиотека";
        case Modal::Book:        return "Книга";
        case Modal::Shop:        return "Торговля";
        case Modal::Enchant:     return "Зачарование";
        case Modal::EnchantPick: return "Выбор руны";
        case Modal::Help:        return "Управление";
        default:                 return "";
    }
}

// Текстовые окна — это абзац, а не список: у них своя прокрутка и одна кнопка.
bool is_text_modal(Modal::Kind k) {
    return k == Modal::Message || k == Modal::Help ||
           k == Modal::Character || k == Modal::Book || k == Modal::Ending;
}

std::string item_desc(const ItemDef* d) {
    if (!d) return "";
    const Stats& b = d->bonus;
    std::string s = d->name + " (" + kind_name(d->kind) + ")\n" + d->desc +
                    "\n\nЦена: " + to_str(d->price) + "\n";
    if (b.max_hp)  s += "+" + to_str(b.max_hp) + " к здоровью\n";
    if (b.max_ap)  s += "+" + to_str(b.max_ap) + " к очкам действия\n";
    if (b.attack)  s += to_str(b.attack) + "% к меткости\n";
    if (b.dmg_min || b.dmg_max)
        s += "урон +" + to_str(b.dmg_min) + "/" + to_str(b.dmg_max) + "\n";
    if (b.block)   s += to_str(b.block) + "% к блоку\n";
    if (b.armor)   s += to_str(b.armor) + " к броне\n";
    if (b.crit)    s += to_str(b.crit) + "% к криту\n";
    if (b.ap_atk)  s += to_str(b.ap_atk) + " AP к стоимости атаки\n";
    if (d->heal_hp) s += "восстанавливает " + to_str(d->heal_hp) + " HP\n";
    if (d->heal_ap) s += "восстанавливает " + to_str(d->heal_ap) + " AP\n";
    return s;
}

} // namespace

// ------------------------------------------------------- содержимое окон

void App::collect_rows(Modal& m, std::vector<Row>* rows, std::vector<std::string>* ids) const {
    rows->clear();
    ids->clear();
    const Content& c = Content::get();
    const Player& p = g_.player();
    const Theme& th = theme();

    switch (m.kind) {
        case Modal::GameMenu:
            rows->push_back(Row("Герой"));       ids->push_back("character");
            rows->push_back(Row("Сумка"));       ids->push_back("inventory");
            rows->push_back(Row("Задания"));     ids->push_back("quests");
            rows->push_back(Row("Навыки"));      ids->push_back("skills");
            rows->push_back(Row("Что действует")); ids->push_back("effects");
            if (p.portal_master) { rows->push_back(Row("Порталы")); ids->push_back("portals"); }
            rows->push_back(Row("Книги и записки")); ids->push_back("library");
            rows->push_back(Row("Управление"));  ids->push_back("help");
            rows->push_back(Row("Пауза"));       ids->push_back("pause");
            break;

        case Modal::Pause:
            rows->push_back(Row("Вернуться в игру")); ids->push_back("back");
            rows->push_back(Row("Сохранить игру"));   ids->push_back("save");
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
            if (d->kind == ItemKind::Consumable) { rows->push_back(Row("Применить")); ids->push_back("use"); }
            if (d->kind == ItemKind::Book)       { rows->push_back(Row("Начать книгу")); ids->push_back("startbook"); }
            if (slot_for(d->kind) != Slot::Count){ rows->push_back(Row("Надеть")); ids->push_back("equip"); }
            rows->push_back(Row("Осмотреть")); ids->push_back("look");
            rows->push_back(Row("Выбросить", th.warn)); ids->push_back("drop");
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
                const bool can = p.skill_points > 0 && rank < s.max_rank;
                rows->push_back(Row(s.name + "  " + to_str(rank) + "/" + to_str(s.max_rank) +
                                    "  — " + s.desc, can ? th.text : th.faint));
                ids->push_back(s.id);
            }
            break;
        }

        case Modal::Effects: {
            for (const ActiveEffect& e : p.effects) {
                const EffectDef* d = c.effect(e.id);
                rows->push_back(Row(std::string(d ? d->name : e.id) + "  сила " + to_str(e.power) +
                                    ", ходов " + to_str(e.turns),
                                    d && d->harmful ? th.warn : th.good));
                ids->push_back(e.id);
            }
            if (rows->empty()) { rows->push_back(Row("Ничего не действует", th.faint)); ids->push_back(""); }
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

Rect App::modal_body(const Modal& m, Rect* frame_out, std::vector<Rect>* buttons,
                     std::vector<std::string>* labels) const {
    labels->clear();
    switch (m.kind) {
        case Modal::Inventory:   labels->push_back("Закрыть"); break;
        case Modal::Shop:        labels->push_back(m.selling ? "Покупать" : "Продавать");
                                 labels->push_back("Закрыть"); break;
        case Modal::Book:        labels->push_back("Закрыть"); break;
        case Modal::Dialogue:    break;                        // варианты сами и есть кнопки
        default:                 labels->push_back("Закрыть"); break;
    }

    // Окно ужимается под содержимое: список из трёх пунктов не должен
    // занимать пол-экрана только потому, что панель одна на всех.
    const int cw = c_.cell_w(), ch = c_.cell_h();
    int want_w = 44 * cw;
    int want_h = 22 * ch;
    if (!is_text_modal(m.kind) && m.kind != Modal::Dialogue) {
        std::vector<Row> rows;
        std::vector<std::string> ids;
        collect_rows(const_cast<Modal&>(m), &rows, &ids);
        const int row_h = m.list.row_height(c_);
        int need = static_cast<int>(rows.size()) * row_h;
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
    return area;
}

// ---------------------------------------------------------------- рисование

void App::draw_modal(Modal& m) {
    const Theme& th = theme();
    Rect frame;
    std::vector<Rect> btns;
    std::vector<std::string> labels;
    const Rect area = modal_body(m, &frame, &btns, &labels);
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
        text_block(c_, Rect(area.x, area.y, area.w, text_h), n->text, th.text, m.scroll);
        for (std::size_t i = 0; i < rows.size(); ++i)
            button(c_, opt[i], rows[i].text, true, false);
        return;
    }

    if (is_text_modal(m.kind)) {
        std::string body = m.body;
        if (m.kind == Modal::Character) {
            const Player& p = g_.player();
            const Stats t = g_.total();
            const Stats nb = g_.total_no_stance();
            body  = p.name + ", уровень " + to_str(p.level) + "\n";
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
            if (b) for (const std::string& l : b->lines) body += l + "\n";
            if (body.empty()) body = "(пусто)";
        }
        text_block(c_, area, body, th.text, m.scroll);
    } else {
        std::vector<Row> rows;
        std::vector<std::string> ids;
        collect_rows(m, &rows, &ids);
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
    if (!frame.contains(x, y)) { pop(); return; }

    for (std::size_t i = 0; i < btns.size(); ++i) {
        if (!btns[i].contains(x, y)) continue;
        if (labels[i] == "Закрыть") { pop(); return; }
        if (labels[i] == "Продавать") { m.selling = true;  m.list = ListView(); return; }
        if (labels[i] == "Покупать")  { m.selling = false; m.list = ListView(); return; }
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

    if (is_text_modal(m.kind)) { pop(); return; }

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
            for (const std::string& l : e->lines) body += l + "\n";
            Modal em(Modal::Ending);
            em.title = "РАЗВЯЗКА: " + e->name;
            em.body = body;
            stack_.push_back(em);
        }
        return;
    }
    if (!shop.empty()) {
        Modal sm(Modal::Shop);
        sm.arg = shop;
        stack_.push_back(sm);
        return;
    }
    if (ench) { push(Modal::Enchant); return; }
    if (!next.empty() && npc) {
        Modal dm(Modal::Dialogue);
        dm.arg = npc->id;
        dm.node = next;
        stack_.push_back(dm);
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
            if (id == "character") push(Modal::Character);
            else if (id == "inventory") push(Modal::Inventory);
            else if (id == "quests")    push(Modal::Quests);
            else if (id == "skills")    push(Modal::Skills);
            else if (id == "effects")   push(Modal::Effects);
            else if (id == "portals")   push(Modal::Portals);
            else if (id == "library")   push(Modal::Library);
            else if (id == "help")      push(Modal::Help);
            else if (id == "pause")     push(Modal::Pause);
            return;
        }

        case Modal::Pause:
            if (id == "back") pop();
            else if (id == "save") { save_game(); pop(); }
            else if (id == "load") { load_game(); }
            else if (id == "quit") { close_all(); mode_ = MODE_MENU; menu_list_ = ListView(); }
            return;

        case Modal::Inventory: {
            if (id.empty()) return;
            if (id[0] == '!') { g_.unequip(static_cast<Slot>(to_int(id.substr(1)))); return; }
            Modal im(Modal::ItemMenu);
            im.arg = id;
            stack_.push_back(im);
            return;
        }

        case Modal::ItemMenu: {
            const std::string item = m.arg;
            if (id == "use")   { g_.use_item(item); pop(); }
            else if (id == "equip") { g_.equip(item); pop(); }
            else if (id == "drop")  { g_.drop_item(item); pop(); }
            else if (id == "startbook") {
                if (g_.start_book("Дневник")) g_.msg("Книга начата. Назвать её можно в библиотеке.");
                pop();
            } else if (id == "look") {
                const std::string body = item_desc(c.item(item));
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
            stack_.push_back(bm);
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
            if (m.selling) g_.sell(*s, id);
            else           g_.buy(*s, id);
            return;
        }

        case Modal::Enchant: {
            if (id.empty() || !g_.can_enchant(id)) return;
            Modal em(Modal::EnchantPick);
            em.arg = id;
            stack_.push_back(em);
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
