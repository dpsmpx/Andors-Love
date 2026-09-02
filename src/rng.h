#pragma once
#include <cstdint>

// Детерминированный генератор (xorshift64*). Состояние входит в сохранение,
// поэтому загруженная игра продолжается тем же потоком случайностей.
class Rng {
public:
    explicit Rng(std::uint64_t seed = 0x9E3779B97F4A7C15ULL) { set_seed(seed); }

    void set_seed(std::uint64_t seed) { state_ = seed ? seed : 0x9E3779B97F4A7C15ULL; }
    std::uint64_t state() const { return state_; }

    std::uint64_t next() {
        state_ ^= state_ >> 12;
        state_ ^= state_ << 25;
        state_ ^= state_ >> 27;
        return state_ * 0x2545F4914F6CDD1DULL;
    }

    // Целое в диапазоне [lo, hi] включительно.
    int range(int lo, int hi) {
        if (hi <= lo) return lo;
        return lo + static_cast<int>(next() % static_cast<std::uint64_t>(hi - lo + 1));
    }

    // Проверка шанса в процентах.
    bool chance(int percent) {
        if (percent <= 0)  return false;
        if (percent >= 100) return true;
        return range(1, 100) <= percent;
    }

private:
    std::uint64_t state_;
};
