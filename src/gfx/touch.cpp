#include "touch.h"

#include <cstdlib>

namespace gfx {

Pointer::Pointer()
    : swipe_px_(24), hold_ms_(320), hold_active_(false),
      hold_x_(0), hold_y_(0), hold_id_(-1) {}

void Pointer::configure(int swipe_px, unsigned hold_ms) {
    if (swipe_px > 0) swipe_px_ = swipe_px;
    if (hold_ms > 0)  hold_ms_ = hold_ms;
}

Pointer::Touch* Pointer::find(int id) {
    for (int i = 0; i < MAX_TOUCHES; ++i)
        if (touches_[i].active && touches_[i].id == id) return &touches_[i];
    return 0;
}

void Pointer::push(const Gesture& g) { queue_.push_back(g); }

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
        t.held = false;
        return;
    }
}

void Pointer::move(int id, int x, int y, unsigned now_ms) {
    Touch* t = find(id);
    if (!t) return;
    t->cur_x = x; t->cur_y = y;

    if (t->held) {                 // бег: цель едет за пальцем
        hold_x_ = x; hold_y_ = y;
        return;
    }

    // Свайп выдаётся столько раз, сколько порогов прошёл палец: одно долгое
    // движение — несколько шагов, как и ждёшь от «веду пальцем».
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

void Pointer::tick(unsigned now_ms) {
    for (int i = 0; i < MAX_TOUCHES; ++i) {
        Touch& t = touches_[i];
        if (!t.active || t.held || t.swiped) continue;
        if (now_ms - t.down_ms < hold_ms_) continue;
        // Задержался на месте — это бег, а не тап.
        t.held = true;
        hold_active_ = true;
        hold_id_ = t.id;
        hold_x_ = t.cur_x; hold_y_ = t.cur_y;

        Gesture g;
        g.kind = G_HOLD_BEGIN;
        g.x = t.cur_x; g.y = t.cur_y;
        push(g);
    }
}

void Pointer::up(int id, int x, int y, unsigned now_ms) {
    Touch* t = find(id);
    if (!t) return;

    if (t->held) {
        Gesture g;
        g.kind = G_HOLD_END;
        g.x = x; g.y = y;
        push(g);
        if (hold_id_ == id) { hold_active_ = false; hold_id_ = -1; }
    } else if (!t->swiped) {
        // Ни сдвига, ни задержки — тап. Отпускание может прийти позже порога
        // удержания только если tick() не звали: считаем тапом и это.
        Gesture g;
        g.kind = G_TAP;
        g.x = x; g.y = y;
        push(g);
    }
    (void)now_ms;
    t->active = false;
    t->id = -1;
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
    hold_active_ = false;
    hold_id_ = -1;
}

} // namespace gfx
