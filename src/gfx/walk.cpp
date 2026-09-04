#include "walk.h"

#include <cstdlib>

namespace gfx {

bool walk_interrupted_by(Bump b) {
    switch (b) {
        case Bump::Moved:
        case Bump::Item:
        case Bump::Note:
            return false;
        default:
            return true;    // переход, NPC, табличка, лежанка, сундук, портал, бой
    }
}

Walker::Walker()
    : active_(false), run_(false), bump_(Bump::Blocked), last_dx_(0), last_dy_(0),
      target_(0, 0), next_ms_(0), stop_(WS_ARRIVED) {}

void Walker::go(Vec2 target, bool run) {
    // Часы шагов не сбрасываются: они идут поверх всех целей. Иначе можно
    // было бы разогнать героя, подсовывая ему новую цель каждый раз, как он
    // дошёл до прежней — по клетке за касание, быстрее любого шага. А если
    // ходьба стояла дольше паузы, next_ms_ уже позади, и первый шаг всё
    // равно делается сразу: тап отзывается без задержки.
    active_ = true;
    run_ = run;
    target_ = target;
    stop_ = WS_RUNNING;
}

void Walker::retarget(Vec2 target, bool run) {
    if (!active_) { go(target, run); return; }
    target_ = target;
    run_ = run;
}

void Walker::stop() {
    active_ = false;
    run_ = false;
}

bool Walker::try_axis(Game& g, int dx, int dy, Bump* out) {
    const Bump b = g.try_move(dx, dy);
    if (out) *out = b;
    // Всё, кроме упора, считается состоявшимся шагом — в том числе разговор
    // и сундук, где герой остаётся на месте: шаг сделан, ходьбе конец.
    return b != Bump::Blocked;
}

bool Walker::update(Game& g, unsigned now_ms) {
    if (!active_) return false;
    if (now_ms < next_ms_) return false;

    const Vec2 p = g.player().pos;
    const int ddx = target_.x - p.x;
    const int ddy = target_.y - p.y;
    // Стоять на цели — не шаг, и паузы он не стоит: палец уже на герое,
    // а следующая цель должна сработать сразу.
    if (ddx == 0 && ddy == 0) { stop_ = WS_ARRIVED; active_ = false; return false; }
    next_ms_ = now_ms + step_ms(run_);

    // Сперва по оси, где разница больше: этого хватает, чтобы обойти угол.
    int ax = 0, ay = 0, bx = 0, by = 0;
    if (std::abs(ddx) >= std::abs(ddy)) {
        ax = ddx > 0 ? 1 : (ddx < 0 ? -1 : 0);
        by = ddy > 0 ? 1 : (ddy < 0 ? -1 : 0);
    } else {
        ay = ddy > 0 ? 1 : (ddy < 0 ? -1 : 0);
        bx = ddx > 0 ? 1 : (ddx < 0 ? -1 : 0);
    }

    Bump b = Bump::Blocked;
    bool moved = false;
    if ((ax || ay) && try_axis(g, ax, ay, &b)) { moved = true; last_dx_ = ax; last_dy_ = ay; }
    else if ((bx || by) && try_axis(g, bx, by, &b)) { moved = true; last_dx_ = bx; last_dy_ = by; }
    bump_ = b;

    if (!moved) { stop_ = WS_BLOCKED; active_ = false; return false; }

    if (walk_interrupted_by(b)) { stop_ = WS_EVENT; active_ = false; return true; }
    if (g.player().pos.x == target_.x && g.player().pos.y == target_.y) {
        stop_ = WS_ARRIVED;
        active_ = false;
    }
    return true;
}

} // namespace gfx
