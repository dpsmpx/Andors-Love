#pragma once
#include <vector>

// Распознавание касаний. Слой намеренно ничего не знает про SDL: на вход
// подаются «палец опустился / поехал / поднялся» с координатами и временем.
// Так это проверяется тестами без окна.
//
// Наружу выдаются две разные вещи, и разница существенная:
//
//   1. Состояние пальца — где он сейчас и сколько уже держится. По нему
//      ходит герой: держишь палец в точке — идёшь туда, ведёшь пальцем —
//      цель едет за ним. Это состояние, а не событие, и опрашивается оно
//      каждый кадр.
//   2. События — тап (подтверждение выбора) и свайп (прокрутка списков).
//      Свайп больше не двигает героя: он выдавался по разу на каждый
//      пройденный порог, и одно движение пальца превращалось в десяток
//      шагов подряд без всякой паузы между ними.

namespace gfx {

enum GestureKind {
    G_NONE = 0,
    G_TAP,          // коснулся и отпустил, не сдвинувшись
    G_SWIPE         // сдвинул дальше порога: прокрутка (dx, dy — один из четырёх)
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

    // Порог свайпа в пикселях: короче него движение считается дрожанием руки.
    void configure(int swipe_px);

    void down(int id, int x, int y, unsigned now_ms);
    void move(int id, int x, int y, unsigned now_ms);
    void up(int id, int x, int y, unsigned now_ms);

    bool poll(Gesture* out);
    void clear();

    // --- состояние ведущего пальца ---
    // Ведущий — тот, что опустился первым и ещё не отпущен. Второй палец
    // управление не перехватывает: в игре он не нужен, а случайное касание
    // ладонью не должно уводить героя.
    bool     pressed() const { return lead_ >= 0; }
    int      press_x() const { return press_x_; }
    int      press_y() const { return press_y_; }
    // Сколько миллисекунд палец уже держится. По этому времени решается,
    // идти шагом или бежать.
    unsigned held_ms(unsigned now_ms) const;

private:
    struct Touch {
        int  id;
        bool active;
        int  start_x, start_y;   // точка, от которой меряется свайп
        int  cur_x, cur_y;
        unsigned down_ms;
        bool swiped;             // касание уже ушло в свайпы
        Touch() : id(-1), active(false), start_x(0), start_y(0),
                  cur_x(0), cur_y(0), down_ms(0), swiped(false) {}
    };

    Touch* find(int id);
    void push(const Gesture& g);

    static const int MAX_TOUCHES = 10;
    Touch touches_[MAX_TOUCHES];
    std::vector<Gesture> queue_;
    int  swipe_px_;

    int      lead_;              // id ведущего пальца, -1 — ни одного
    int      press_x_, press_y_;
    unsigned press_ms_;
};

} // namespace gfx
