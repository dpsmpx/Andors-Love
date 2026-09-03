#include "touch.h"

#include <cstdlib>

namespace gfx {

Pointer::Pointer()
    : swipe_px_(24), lead_(-1), press_x_(0), press_y_(0), press_ms_(0) {}

void Pointer::configure(int swipe_px) {
    if (swipe_px > 0) swipe_px_ = swipe_px;
}

Pointer::Touch* Pointer::find(int id) {
    for (int i = 0; i < MAX_TOUCHES; ++i)
        if (touches_[i].active && touches_[i].id == id) return &touches_[i];
    return 0;
}

void Pointer::push(const Gesture& g) { queue_.push_back(g); }

unsigned Pointer::held_ms(unsigned now_ms) const {
    if (lead_ < 0) return 0;
    return now_ms > press_ms_ ? now_ms - press_ms_ : 0;
}

void Pointer::down(int id, int x, int y, unsigned now_ms) {
    if (find(id)) return;
    for (int i = 0; i < MAX_TOUCHES; ++i) {
        if (touches_[i].active) continue;
        Touch& t = touches_[i];
        t.id = id; t.active = true;
        t.start_x = t.cur_x = x;
        t.start_y = t.cur_y = y;
        t.down_ms = now_ms;
        t.swiped = false;

        if (lead_ < 0) {
            lead_ = id;
            press_x_ = x; press_y_ = y;
            press_ms_ = now_ms;
        }
        return;
    }
}

void Pointer::move(int id, int x, int y, unsigned now_ms) {
    Touch* t = find(id);
    if (!t) return;
    t->cur_x = x; t->cur_y = y;

    // Ведущий палец тянет за собой цель: где он сейчас, туда герой и идёт.
    if (id == lead_) { press_x_ = x; press_y_ = y; }

    // Свайп выдаётся столько раз, сколько порогов прошёл палец: одно долгое
    // движение — несколько щелчков прокрутки, как и ждёшь от «веду пальцем».
    // Героя это больше не двигает — за него отвечает состояние пальца.
    for (;;) {
        const int dx = x - t->start_x;
        const int dy = y - t->start_y;
        if (std::abs(dx) < swipe_px_ && std::abs(dy) < swipe_px_) break;

        Gesture g;
        g.kind = G_SWIPE;
        g.x = x; g.y = y;
        if (std::abs(dx) > std::abs(dy)) {
            g.dx = dx > 0 ? 1 : -1;
            t->start_x += g.dx * swipe_px_;
        } else {
            g.dy = dy > 0 ? 1 : -1;
            t->start_y += g.dy * swipe_px_;
        }
        t->swiped = true;
        push(g);
    }
    (void)now_ms;
}

void Pointer::up(int id, int x, int y, unsigned now_ms) {
    Touch* t = find(id);
    if (!t) return;

    // Тап — только если палец не уезжал: иначе это была прокрутка или ведение
    // цели, и подтверждать им ничего не надо.
    if (!t->swiped) {
        Gesture g;
        g.kind = G_TAP;
        g.x = x; g.y = y;
        push(g);
    }
    (void)now_ms;
    t->active = false;
    t->id = -1;

    if (id == lead_) {
        // Ведущим становится любой другой палец, который ещё на экране.
        lead_ = -1;
        for (int i = 0; i < MAX_TOUCHES; ++i) {
            if (!touches_[i].active) continue;
            lead_ = touches_[i].id;
            press_x_ = touches_[i].cur_x;
            press_y_ = touches_[i].cur_y;
            press_ms_ = touches_[i].down_ms;
            break;
        }
    }
}

bool Pointer::poll(Gesture* out) {
    if (queue_.empty()) return false;
    if (out) *out = queue_.front();
    queue_.erase(queue_.begin());
    return true;
}

void Pointer::clear() {
    queue_.clear();
    for (int i = 0; i < MAX_TOUCHES; ++i) touches_[i] = Touch();
    lead_ = -1;
}

} // namespace gfx
