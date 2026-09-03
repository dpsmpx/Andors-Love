#pragma once
#include <string>
#include <vector>

// Распознавание жестов. Слой намеренно ничего не знает про SDL: на вход
// подаются «палец опустился / поехал / поднялся» с координатами и временем,
// на выходе — очередь жестов. Так это можно проверять тестами без окна.
//
// Правила, по которым живёт управление:
//   тап            — коснулся и отпустил, не сдвинувшись и не задержавшись;
//   удержание      — держит дольше порога, не сдвигаясь: включает бег;
//   свайп          — сдвинул дальше порога: шаг в сторону сдвига, и так
//                    столько раз, сколько порогов уместилось в движении.
// Свайп «съедает» касание: отпускание после него тапом уже не считается.

namespace gfx {

enum GestureKind {
    G_NONE = 0,
    G_TAP,          // подтверждение выбора, цель для ходьбы
    G_HOLD_BEGIN,   // палец задержался: с этого мига герой бежит
    G_HOLD_END,     // палец отпущен после удержания
    G_SWIPE         // шаг в сторону (dx, dy — один из четырёх)
};

struct Gesture {
    GestureKind kind;
    int x, y;       // где это случилось, в пикселях окна
    int dx, dy;     // только для свайпа: -1, 0 или 1
    Gesture() : kind(G_NONE), x(0), y(0), dx(0), dy(0) {}
};

class Pointer {
public:
    Pointer();

    // Порог свайпа в пикселях и время удержания в миллисекундах.
    void configure(int swipe_px, unsigned hold_ms);

    void down(int id, int x, int y, unsigned now_ms);
    void move(int id, int x, int y, unsigned now_ms);
    void up(int id, int x, int y, unsigned now_ms);
    // Удержание срабатывает и без движения — этот вызов даёт ему шанс.
    void tick(unsigned now_ms);

    bool poll(Gesture* out);
    void clear();

    // Держит ли палец прямо сейчас (для бега) и где именно.
    bool holding() const { return hold_active_; }
    int  hold_x() const { return hold_x_; }
    int  hold_y() const { return hold_y_; }

private:
    struct Touch {
        int  id;
        bool active;
        int  start_x, start_y;   // точка, от которой меряется свайп
        int  origin_x, origin_y; // где палец опустился
        int  cur_x, cur_y;
        unsigned down_ms;
        bool swiped;             // касание уже ушло в свайпы
        bool held;               // по нему уже сработало удержание
        Touch() : id(-1), active(false), start_x(0), start_y(0),
                  origin_x(0), origin_y(0), cur_x(0), cur_y(0),
                  down_ms(0), swiped(false), held(false) {}
    };

    Touch* find(int id);
    void push(const Gesture& g);

    static const int MAX_TOUCHES = 10;
    Touch touches_[MAX_TOUCHES];
    std::vector<Gesture> queue_;
    int  swipe_px_;
    unsigned hold_ms_;
    bool hold_active_;
    int  hold_x_, hold_y_;
    int  hold_id_;
};

} // namespace gfx
