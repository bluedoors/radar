#include "hardware/button.h"

ButtonEvent ButtonFsm::update(bool pressed, uint32_t now_ms) {
    ButtonEvent ev = ButtonEvent::None;
    if (pressed && !was_pressed_) {
        press_start_ = now_ms; long_fired_ = false;
    }
    if (pressed && !long_fired_ && (now_ms - press_start_) >= LONG_MS) {
        long_fired_ = true; pending_clicks_ = 0;
        was_pressed_ = pressed;
        return ButtonEvent::Long;
    }
    if (!pressed && was_pressed_) {
        if (!long_fired_) { pending_clicks_++; last_release_ = now_ms; }
    }
    if (!pressed && pending_clicks_ > 0 && (now_ms - last_release_) >= DOUBLE_MS) {
        ev = (pending_clicks_ >= 2) ? ButtonEvent::Double : ButtonEvent::Short;
        pending_clicks_ = 0;
    }
    if (!pressed && pending_clicks_ >= 2) {
        ev = ButtonEvent::Double; pending_clicks_ = 0;
    }
    was_pressed_ = pressed;
    return ev;
}
