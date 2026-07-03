#pragma once
#include <cstdint>
// Button gestures:
//   Short     — press < PEEK_MS, fires immediately on release (no double-click wait).
//   LongPeek  — held >= PEEK_MS (1.5 s); fires once while still held.
//   LongReset — held >= RESET_MS (8 s); fires once while still held.
// (A hold long enough for Reset will emit LongPeek first at 1.5 s, then LongReset at 8 s.)
enum class ButtonEvent { None, Short, LongPeek, LongReset };
class ButtonFsm {
public:
    ButtonEvent update(bool pressed, uint32_t now_ms);
private:
    static constexpr uint32_t PEEK_MS  = 1500;
    static constexpr uint32_t RESET_MS = 8000;
    bool     was_pressed_     = false;
    uint32_t press_start_     = 0;
    bool     peek_fired_      = false;
    bool     reset_fired_     = false;
};
