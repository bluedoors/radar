#pragma once
#include <cstdint>
enum class ButtonEvent { None, Short, Double, Long };
class ButtonFsm {
public:
    ButtonEvent update(bool pressed, uint32_t now_ms);
private:
    static constexpr uint32_t LONG_MS = 1500;
    static constexpr uint32_t DOUBLE_MS = 400;
    bool was_pressed_ = false;
    uint32_t press_start_ = 0;
    uint32_t last_release_ = 0;
    uint32_t first_release_ = 0;
    int pending_clicks_ = 0;
    bool long_fired_ = false;
};
