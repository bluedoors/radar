#include "hardware/button.h"

ButtonEvent ButtonFsm::update(bool pressed, uint32_t now_ms) {
    ButtonEvent ev = ButtonEvent::None;

    // Falling edge: press started
    if (pressed && !was_pressed_) {
        press_start_ = now_ms;
        long_fired_ = false;
    }

    // Long-press detection (fires while still held)
    if (pressed && !long_fired_ && (now_ms - press_start_) >= LONG_MS) {
        long_fired_ = true;
        pending_clicks_ = 0;
        was_pressed_ = pressed;
        return ButtonEvent::Long;
    }

    // Rising edge: button released
    if (!pressed && was_pressed_ && !long_fired_) {
        if (pending_clicks_ == 0) {
            // First release of a new gesture — record when the window opens
            first_release_ = now_ms;
            pending_clicks_ = 1;
            last_release_ = now_ms;
        } else {
            // Second (or later) release — check if still inside the double-click window
            if ((now_ms - first_release_) > DOUBLE_MS) {
                // Outside the window: flush the previous pending click as Short,
                // then start a fresh gesture with this release.
                was_pressed_ = pressed;
                pending_clicks_ = 1;
                first_release_ = now_ms;
                last_release_ = now_ms;
                return ButtonEvent::Short;
            }
            // Inside the window: accumulate
            pending_clicks_++;
            last_release_ = now_ms;
        }
    }

    // Eager-double: second (or more) pending clicks, still inside window
    if (!pressed && pending_clicks_ >= 2) {
        ev = ButtonEvent::Double;
        pending_clicks_ = 0;
        was_pressed_ = pressed;
        return ev;
    }

    // Timeout path: double-click window expired with only one pending click → Short
    if (!pressed && pending_clicks_ == 1 && (now_ms - first_release_) >= DOUBLE_MS) {
        ev = ButtonEvent::Short;
        pending_clicks_ = 0;
    }

    was_pressed_ = pressed;
    return ev;
}
