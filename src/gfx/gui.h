#pragma once
#include "draw.h"

#include <string>
#include <vector>

// Небольшой набор виджетов поверх Canvas: всплывающая панель, кнопка,
// список с прокруткой, абзац текста.
//
// Готовая библиотека (ImGui и подобные) сюда не легла: у неё свой атлас
// шрифта без кириллицы, а её «касание» — это эмуляция мыши, тогда как здесь
// тап, удержание и свайп — разные вещи с разным смыслом. Виджетов нужно
// немного, и они целиком помещаются в один файл, зато размеры считаются
// от размера пальца, а не от размера курсора.

namespace gfx {

struct Theme {
    Color bg;            // фон мира
    Color panel;         // полупрозрачная подложка окна
    Color border;
    Color dim;           // затемнение под окном
    Color text;
    Color faint;         // пояснения, недоступное
    Color accent;        // заголовки, выбранное
    Color good;
    Color warn;
    Color btn;
    Color btn_hot;
    Color btn_off;
};

const Theme& theme();

// Затемнить весь экран: всё, что открывается поверх мира, начинается с этого.
void dim_screen(Canvas& c, int alpha);

// Геометрия всплывающего окна и его отрисовка — разные вызовы, и это
// важно: попадание пальца считается до того, как окно нарисовано, и если
// расчёт продублировать, он рано или поздно разъедется с рисунком.
// panel_rect ничего не рисует, panel рисует и возвращает ровно то же.
Rect panel_rect(const Canvas& c, bool has_title, int want_cols, int want_rows,
                Rect* frame_out);
Rect panel(Canvas& c, const std::string& title, int want_cols, int want_rows,
           Rect* frame_out);
// То же, но размер содержимого задаётся в пикселях: окно на три пункта не
// должно занимать пол-экрана только потому, что панель одна на всех.
Rect panel_rect_px(const Canvas& c, bool has_title, int want_w, int want_h,
                   Rect* frame_out);
Rect panel_px(Canvas& c, const std::string& title, int want_w, int want_h,
              Rect* frame_out);

// Кнопка с подписью. Рисование и попадание разделены нарочно: попадание
// проверяется по тапу кадром раньше, чем кнопка нарисована.
void button(Canvas& c, const Rect& r, const std::string& label,
            bool enabled, bool highlighted);

// Разложить n кнопок в ряд по ширине области.
void row_of(const Rect& area, int n, int gap, std::vector<Rect>* out);
// То же столбцом.
void column_of(const Rect& area, int n, int gap, std::vector<Rect>* out);

// Строка списка: текст, цвет и доступность.
struct Row {
    std::string text;
    Color       color;
    bool        enabled;
    Row() : color(theme().text), enabled(true) {}
    Row(const std::string& t) : text(t), color(theme().text), enabled(true) {}
    Row(const std::string& t, Color c) : text(t), color(c), enabled(true) {}
};

// Список с прокруткой. Состояние живёт у вызывающего экрана — так проще
// проверять и незачем держать общий контекст.
class ListView {
public:
    ListView();

    int  row_height(const Canvas& c) const;
    // Индекс строки под точкой или -1.
    int  hit(const Canvas& c, const Rect& r, int x, int y, int count) const;
    void draw(Canvas& c, const Rect& r, const std::vector<Row>& rows) const;

    void scroll_by(int rows_delta, int count, int visible);
    void set_cursor(int i, int count, int visible);
    int  visible_rows(const Canvas& c, const Rect& r) const;
    void clamp(int count, int visible);

    int scroll;
    int cursor;

private:
    mutable int row_h_;
};

// Абзац с переносом по ширине области. Возвращает число нарисованных строк.
int text_block(Canvas& c, const Rect& r, const std::string& s, Color col,
               int scroll_rows);
// Сколько строк займёт текст в этой области.
int text_block_rows(const Canvas& c, const Rect& r, const std::string& s);

// То же, что text_block, но по уже разложенным строкам. Нужно там, где
// раскладка стоит дорого и её держат готовой: журнал за партию набирает
// под тысячу записей, и переносить их заново на каждом кадре незачем.
int text_lines(Canvas& c, const Rect& r, const std::vector<std::string>& lines,
               Color col, int scroll_rows);

// Полоса прокрутки у правого края области. Без неё непонятно, что содержимое
// длиннее окна и в каком месте ты сейчас находишься.
void scrollbar(Canvas& c, const Rect& r, int total, int visible, int scroll);

} // namespace gfx
