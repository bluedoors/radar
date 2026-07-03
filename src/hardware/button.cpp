#include "hardware/button.h"

ButtonEvent ButtonFsm::update(bool pressed, uint32_t now_ms) {
    ButtonEvent ev = ButtonEvent::None;

    // Falling edge: press started.
    if (pressed && !was_pressed_) {
        press_start_ = now_ms;
        peek_fired_  = false;
        reset_fired_ = false;
    }

    if (pressed) {
        // Long-hold thresholds fire once each while still held.
        if (!reset_fired_ && (now_ms - press_start_) >= RESET_MS) {
            reset_fired_ = true;
            ev = ButtonEvent::LongReset;
        } else if (!peek_fired_ && (now_ms - press_start_) >= PEEK_MS) {
            peek_fired_ = true;
            ev = ButtonEvent::LongPeek;
        }
    } else if (was_pressed_) {
        // Rising edge: released. A Short only counts if no long gesture fired.
        if (!peek_fired_ && !reset_fired_) {
            ev = ButtonEvent::Short;
        }
    }

    was_pressed_ = pressed;
    return ev;
}
