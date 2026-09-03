#include "gui.h"
#include "../types.h"

namespace gfx {

const Theme& theme() {
    static Theme t;
    static bool built = false;
    if (!built) {
        built = true;
        t.bg      = Color(14, 16, 20);
        t.panel   = Color(26, 30, 38, 244);
        t.border  = Color(92, 104, 124);
        t.dim     = Color(0, 0, 0, 150);
        t.text    = Color(222, 226, 232);
        t.faint   = Color(132, 140, 154);
        t.accent  = Color(236, 200, 120);
        t.good    = Color(126, 200, 130);
        t.warn    = Color(220, 110, 100);
        t.btn     = Color(44, 52, 66, 235);
        t.btn_hot = Color(72, 88, 112, 245);
        t.btn_off = Color(32, 36, 44, 200);
    }
    return t;
}

void dim_screen(Canvas& c, int alpha) {
    c.fill(Rect(0, 0, c.width(), c.height()), Color(0, 0, 0, alpha));
}

Rect panel_rect_px(const Canvas& c, bool has_title, int want_w, int want_h,
                   Rect* frame_out) {
    const int cw = c.cell_w(), ch = c.cell_h();
    const int pad = ch / 2 + 2;

    int w = want_w + pad * 2;
    int h = want_h + pad * 2 + (has_title ? ch * 2 : 0);
    const int max_w = c.width()  - cw * 2;
    const int max_h = c.height() - ch * 2;
    if (w > max_w) w = max_w;
    if (h > max_h) h = max_h;
    if (w < cw * 8) w = cw * 8;
    if (h < ch * 4) h = ch * 4;

    Rect f((c.width() - w) / 2, (c.height() - h) / 2, w, h);
    if (frame_out) *frame_out = f;

    const int top = f.y + pad + (has_title ? ch * 2 : 0);
    (void)cw;
    return Rect(f.x + pad, top, f.w - pad * 2, f.y + f.h - pad - top);
}

Rect panel_rect(const Canvas& c, bool has_title, int want_cols, int want_rows,
                Rect* frame_out) {
    return panel_rect_px(c, has_title, want_cols * c.cell_w(), want_rows * c.cell_h(),
                         frame_out);
}

Rect panel_px(Canvas& c, const std::string& title, int want_w, int want_h,
              Rect* frame_out) {
    const Theme& th = theme();
    const int ch = c.cell_h();
    const int pad = ch / 2 + 2;

    Rect f;
    const Rect body = panel_rect_px(c, !title.empty(), want_w, want_h, &f);
    if (frame_out) *frame_out = f;

    c.fill(f, th.panel);
    c.frame(f, th.border, 2);
    if (!title.empty()) {
        c.text(f.x + pad, f.y + pad, title, th.accent, c.scale());
        c.fill(Rect(f.x + pad, f.y + pad + ch + ch / 4, f.w - pad * 2, 1), th.border);
    }
    return body;
}

Rect panel(Canvas& c, const std::string& title, int want_cols, int want_rows,
           Rect* frame_out) {
    return panel_px(c, title, want_cols * c.cell_w(), want_rows * c.cell_h(), frame_out);
}

void button(Canvas& c, const Rect& r, const std::string& label,
            bool enabled, bool highlighted) {
    const Theme& th = theme();
    c.fill(r, enabled ? (highlighted ? th.btn_hot : th.btn) : th.btn_off);
    c.frame(r, enabled ? th.border : th.btn_off, 1);

    // Подпись ужимается под кнопку, а не вылезает из неё: сперва мельче
    // шрифтом, а если и это не помогло — обрезкой.
    const int room = r.w > 8 ? r.w - 8 : r.w;
    int sc = c.scale();
    while (sc > 1 && c.text_width(label, sc) > room) --sc;

    const int glyph_w = c.cell_w() / c.scale() * sc;
    std::string txt = label;
    if (glyph_w > 0) {
        const std::size_t fit = static_cast<std::size_t>(room / glyph_w);
        if (fit > 0 && utf8_len(txt) > fit) txt = trunc(txt, fit);
    }
    c.text_centered(r, txt, enabled ? th.text : th.faint, sc);
}

void row_of(const Rect& area, int n, int gap, std::vector<Rect>* out) {
    out->clear();
    if (n <= 0) return;
    const int w = (area.w - gap * (n - 1)) / n;
    for (int i = 0; i < n; ++i)
        out->push_back(Rect(area.x + i * (w + gap), area.y, w, area.h));
}

void column_of(const Rect& area, int n, int gap, std::vector<Rect>* out) {
    out->clear();
    if (n <= 0) return;
    const int h = (area.h - gap * (n - 1)) / n;
    for (int i = 0; i < n; ++i)
        out->push_back(Rect(area.x, area.y + i * (h + gap), area.w, h));
}

// ------------------------------------------------------------------ список

// cursor = -1: ничего не подсвечено. Тап по списку выбирает сразу, и
// заранее подсвеченная строка только вводила бы в заблуждение — палец
// метит туда, куда смотрит, а не туда, где остался курсор с клавиатуры.
ListView::ListView() : scroll(0), cursor(-1), row_h_(0) {}

int ListView::row_height(const Canvas& c) const {
    // Строка списка — это кнопка под палец, а не строка текста: ниже
    // пальцевого минимума опускаться нельзя, промахи не прощаются.
    int h = c.touch_unit();
    const int min_h = c.cell_h() * 2;
    if (h < min_h) h = min_h;
    row_h_ = h;
    return h;
}

int ListView::visible_rows(const Canvas& c, const Rect& r) const {
    const int h = row_height(c);
    return h > 0 ? r.h / h : 0;
}

int ListView::hit(const Canvas& c, const Rect& r, int x, int y, int count) const {
    if (!r.contains(x, y)) return -1;
    const int h = row_height(c);
    if (h <= 0) return -1;
    const int i = scroll + (y - r.y) / h;
    return (i >= 0 && i < count) ? i : -1;
}

void ListView::clamp(int count, int visible) {
    int max_scroll = count - visible;
    if (max_scroll < 0) max_scroll = 0;
    if (scroll > max_scroll) scroll = max_scroll;
    if (scroll < 0) scroll = 0;
    if (cursor >= count) cursor = count - 1;
}

void ListView::scroll_by(int rows_delta, int count, int visible) {
    scroll += rows_delta;
    clamp(count, visible);
}

void ListView::set_cursor(int i, int count, int visible) {
    cursor = i;
    if (count <= 0) { cursor = 0; scroll = 0; return; }
    if (cursor < 0) cursor = 0;
    if (cursor >= count) cursor = count - 1;
    if (cursor < scroll) scroll = cursor;
    if (visible > 0 && cursor >= scroll + visible) scroll = cursor - visible + 1;
    clamp(count, visible);
}

void ListView::draw(Canvas& c, const Rect& r, const std::vector<Row>& rows) const {
    const Theme& th = theme();
    const int h = row_height(c);
    const int n = static_cast<int>(rows.size());
    const int vis = h > 0 ? r.h / h : 0;

    c.clip(r);
    for (int k = 0; k < vis; ++k) {
        const int i = scroll + k;
        if (i < 0 || i >= n) break;
        Rect line(r.x, r.y + k * h, r.w, h - 2);
        const bool sel = (i == cursor);
        c.fill(line, sel ? th.btn_hot : Color(0, 0, 0, 0));
        if (sel) c.frame(line, th.accent, 1);

        const Row& row = rows[static_cast<std::size_t>(i)];
        // Ширина строки может оказаться меньше знакоместа на очень узком
        // экране; беззнаковое вычитание тут молча дало бы огромный предел.
        const int room = line.w - c.cell_w() * 2;
        const std::size_t fit = room > 0 ? static_cast<std::size_t>(room / c.cell_w()) : 1;
        const std::string txt = utf8_len(row.text) > fit ? trunc(row.text, fit) : row.text;
        c.text(line.x + c.cell_w() / 2, line.y + (line.h - c.cell_h()) / 2,
               txt, row.enabled ? row.color : th.faint, c.scale());
    }
    c.clip_off();

    // Полоса прокрутки: без неё непонятно, что список длиннее экрана.
    if (n > vis && vis > 0) {
        const int track_h = r.h;
        int bar_h = track_h * vis / n;
        if (bar_h < c.cell_h()) bar_h = c.cell_h();
        const int max_scroll = n - vis;
        const int off = max_scroll > 0 ? (track_h - bar_h) * scroll / max_scroll : 0;
        const int bw = c.cell_w() / 2 + 2;
        c.fill(Rect(r.x + r.w - bw, r.y, bw, track_h), Color(0, 0, 0, 90));
        c.fill(Rect(r.x + r.w - bw, r.y + off, bw, bar_h), th.border);
    }
}

// ------------------------------------------------------------------- текст

namespace {

std::vector<std::string> lay_out(const Canvas& c, const Rect& r, const std::string& s) {
    std::size_t cols = static_cast<std::size_t>(r.w / c.cell_w());
    if (cols < 4) cols = 4;
    return wrap(s, cols);
}

} // namespace

int text_block_rows(const Canvas& c, const Rect& r, const std::string& s) {
    return static_cast<int>(lay_out(c, r, s).size());
}

int text_block(Canvas& c, const Rect& r, const std::string& s, Color col,
               int scroll_rows) {
    const std::vector<std::string> lines = lay_out(c, r, s);
    const int ch = c.cell_h();
    const int vis = ch > 0 ? r.h / ch : 0;

    c.clip(r);
    int drawn = 0;
    for (int k = 0; k < vis; ++k) {
        const int i = scroll_rows + k;
        if (i < 0 || i >= static_cast<int>(lines.size())) break;
        c.text(r.x, r.y + k * ch, lines[static_cast<std::size_t>(i)], col, c.scale());
        ++drawn;
    }
    c.clip_off();
    return drawn;
}

} // namespace gfx
