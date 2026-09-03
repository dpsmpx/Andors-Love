#pragma once
#include "../game.h"

// Ходьба по тапу: герой идёт к указанной клетке, пока не упрётся или пока
// не тапнут в другое место. Поиска пути здесь нет намеренно — тот же
// жадный шаг, что у мобов: сперва по оси с большей разницей, потом по другой.
// Игрок видит ровно то, что обещано: «идёт в сторону точки, пока может».

namespace gfx {

// Что прервало ходьбу: пригодится, чтобы решить, показывать ли что-то игроку.
enum WalkStop {
    WS_RUNNING = 0,   // ещё идёт
    WS_ARRIVED,       // дошёл
    WS_BLOCKED,       // упёрся
    WS_EVENT          // наступил на что-то: переход, сундук, NPC, бой
};

class Walker {
public:
    Walker();

    // Задать цель. Бег отличается только скоростью шага.
    void go(Vec2 target, bool run);
    void stop();

    bool active() const { return active_; }
    bool running() const { return run_; }
    Vec2 target() const { return target_; }
    WalkStop last_stop() const { return stop_; }
    // Чем кончился последний шаг и куда он был сделан: по этому оболочка
    // решает, заводить ли разговор или показывать табличку — ходьба должна
    // приводить туда же, куда приводит шаг вручную.
    Bump last_bump() const { return bump_; }
    int  last_dx() const { return last_dx_; }
    int  last_dy() const { return last_dy_; }

    // Держать цель у пальца во время удержания: бег «ведёт» героя.
    void retarget(Vec2 target) { if (active_) target_ = target; }

    // Один шаг, если пришло время. now_ms — часы кадра. Возвращает true,
    // если герой действительно сдвинулся (значит, миру пора сделать ход).
    bool update(Game& g, unsigned now_ms);

    // Интервал между шагами, мс.
    static unsigned step_ms(bool run) { return run ? 90u : 170u; }

private:
    bool try_axis(Game& g, int dx, int dy, Bump* out);

    bool     active_;
    bool     run_;
    Bump     bump_;
    int      last_dx_, last_dy_;
    Vec2     target_;
    unsigned next_ms_;
    WalkStop stop_;
};

// Прерывает ли этот исход шага ходьбу. Подбор с земли — не прерывает:
// идти через траву, собирая её, естественно. Всё, что открывает экран
// или начинает бой, ходьбу останавливает.
bool walk_interrupted_by(Bump b);

} // namespace gfx
