#pragma once
#include "draw.h"
#include "gui.h"
#include "touch.h"
#include "walk.h"

#include "../game.h"

#include <string>
#include <vector>

// Графическая оболочка. Логика игры не тронута: App только рисует её
// состояние и переводит касания в те же вызовы Game, что делал терминал.

namespace gfx {

// Всплывающее окно. Их немного видов, и все они — «заголовок, содержимое,
// ряд кнопок», поэтому это одна структура с меткой, а не иерархия классов.
struct Modal {
    enum Kind {
        GameMenu,     // меню по тапу в пустоту
        Pause,        // сохранить / загрузить / выйти
        Character,
        Inventory,
        ItemMenu,     // что сделать с выбранной вещью
        Quests,
        Skills,
        Effects,
        Portals,
        Library,
        Book,
        Dialogue,
        Shop,
        Enchant,
        EnchantPick,  // выбор руны для вещи
        Message,      // табличка, записка, ошибка
        Help,
        Ending,
        TextInput     // ввод строки: название книги или её строка
    };

    Kind        kind;
    ListView    list;
    int         scroll;        // для текстовых окон
    std::string title;
    std::string body;
    std::string arg;           // npc_id / shop_id / book_id / item_id / ending_id
    std::string node;          // текущий узел диалога
    bool        selling;       // вкладка магазина: покупка или продажа
    // Для TextInput: что правим и чем ограничены. index < 0 — название книги.
    std::string buffer;
    int         index;
    std::size_t max_len;

    Modal() : kind(GameMenu), scroll(0), selling(false), index(-1), max_len(0) {}
    explicit Modal(Kind k) : kind(k), scroll(0), selling(false), index(-1), max_len(0) {}
};

class App {
public:
    App();

    int run(int argc, char** argv);

    // Прогон по сценарию без человека: команды подаются строками, кадры
    // сохраняются в BMP. Нужен, чтобы графику можно было проверять там, где
    // нет ни экрана, ни пальцев — на сборочной машине и в тестах.
    // Команды: tap X Y | hold X Y | drag X Y | release | swipe DX DY |
    //          key K | type ТЕКСТ | wait MS | where | shot ИМЯ | quit
    int run_script(const std::vector<std::string>& script, const std::string& out_dir,
                   int argc, char** argv);

private:
    // --- кадр ---
    // Открыть окно и подготовить пути. Отделено от цикла, чтобы прогон
    // по сценарию не был копией run() со своей отдельной жизнью.
    bool start(int argc, char** argv, int win_w, int win_h);
    void step(unsigned now_ms);
    void draw();
    void draw_world();
    void draw_hud();
    void draw_combat();
    void draw_main_menu();
    void draw_create_hero();
    void draw_modal(Modal& m);

    // --- ввод ---
    void pump_events();
    void on_tap(int x, int y);
    void on_key(int key);
    void on_swipe(int dx, int dy);
    // Ходьба по удерживаемому пальцу: цель — клетка под пальцем, темп —
    // шаг или бег, смотря сколько палец уже держится.
    void follow_finger();
    bool reachable_cell(const Location& loc, Vec2 cell);
    static const unsigned RUN_AFTER_MS = 700;
    // Набранный текст: и с настоящей клавиатуры, и из сценария — одним путём.
    void on_text(const char* utf8);
    void scroll_modal(Modal& m, int rows);
    void world_tap(int x, int y);
    void combat_tap(int x, int y);
    void modal_tap(Modal& m, int x, int y);
    void main_menu_tap(int x, int y);
    void create_hero_tap(int x, int y);

    // --- переходы ---
    void push(Modal::Kind k);
    void push_message(const std::string& title, const std::string& body);
    // Ввод строки поверх всего. index < 0 — название книги, иначе её строка;
    // index == BOOK_APPEND — добавить строку в конец.
    void push_text_input(const std::string& title, const std::string& book_id,
                         int index, const std::string& initial, std::size_t max_len);
    void commit_text_input(Modal& m);
    static const int BOOK_APPEND = -2;
    void pop();
    // Закрыть верхнее окно с учётом того, что за ним стоит: после экрана
    // смерти играть дальше нельзя, это выход в главное меню.
    void close_top();
    void close_all();
    Modal* top();
    bool  modal_open() const { return !stack_.empty(); }

    // --- мир ---
    void step_player(int dx, int dy);
    void handle_bump(Bump b, int dx, int dy);
    // Разговор или табличка после того, как ходьба упёрлась в них: ход мира
    // при этом уже сделан, второй раз его крутить нельзя.
    void interact_after_walk();
    void begin_dialogue(const std::string& npc_id);
    void apply_dialogue_option(const DlgOption& o);
    // Строки окна и их «имена»: по имени и решается, что делать при выборе.
    void collect_rows(const Modal& m, std::vector<Row>* rows,
                      std::vector<std::string>* ids) const;

    // Раскладки считаются в одном месте и на неё смотрят и рисование, и
    // попадание пальца. Дублировать расчёт нельзя: разъедется — и кнопка
    // окажется не там, где её видно.
    Rect modal_body(const Modal& m, Rect* frame_out, std::vector<Rect>* buttons,
                    std::vector<std::string>* labels) const;
    std::string modal_title(const Modal& m) const;
    void dialogue_layout(const Modal& m, const Rect& area, int n,
                         std::vector<Rect>* out) const;
    void menu_layout(std::vector<std::string>* items, std::vector<Rect>* out) const;
    void create_layout(Rect* list_area, Rect* next_btn) const;
    void combat_layout(std::vector<Rect>* top_row, std::vector<Rect>* bottom_row,
                       Rect* body) const;
    void hud_buttons(std::vector<Rect>* out) const;
    void activate_row(Modal& m, int index);
    void save_game();
    void load_game();

    Rect map_area() const;
    // Карта живёт своим масштабом, а не текстовым: клетка квадратная и
    // такая, чтобы поперёк экрана их помещалось около полутора десятков —
    // столько, сколько имеет смысл разглядывать с телефона в руке.
    int  map_scale() const;
    int  map_cell() const;
    // Нижняя граница нарисованной карты: под ней остаётся место, и оно
    // отдано журналу — иначе на телефоне полэкрана пустует.
    int  map_bottom() const;
    Rect hud_area() const;
    // Клетка карты под точкой экрана; возвращает false, если мимо карты.
    bool cell_at(int x, int y, Vec2* out) const;
    void camera(int* cx, int* cy, int* cols, int* rows) const;

    Canvas   c_;
    Pointer  ptr_;
    Walker   walk_;
    Game     g_;

    enum Mode { MODE_MENU, MODE_CREATE, MODE_PLAY };
    Mode mode_;
    bool quit_;
    bool has_save_;

    std::vector<Modal> stack_;

    // Главное меню и создание героя
    ListView menu_list_;
    // Сводка по сохранению для главного меню. Считается один раз: разбирать
    // файл сохранения каждый кадр — это чтение с диска шестьдесят раз
    // в секунду ради пяти строчек, которые не меняются.
    void refresh_save_summary();
    struct SaveSummary {
        bool        ok;
        std::string name, kind, place;
        int         level, done, open_q, gold, turn;
        SaveSummary() : ok(false), level(0), done(0), open_q(0), gold(0), turn(0) {}
    };
    SaveSummary summary_;
    int      create_step_;      // 0 — имя, 1 — раса, 2 — специализация
    ListView create_list_;
    std::string new_name_, new_race_, new_spec_;

    // Мир
    unsigned now_ms_;
    bool     died_;              // экран смерти показывается один раз
    std::string data_root_;      // тот же, что у игры: сводка грузится оттуда же
    std::string save_dir_;
    std::string save_path_;
    std::string status_;        // последняя строка журнала, показанная в HUD

    App(const App&);
    App& operator=(const App&);
};

} // namespace gfx
