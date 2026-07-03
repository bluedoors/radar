#pragma once
#include <LovyanGFX.hpp>

class Display {
public:
    void begin();
    void set_backlight(uint8_t duty);   // 0..255 via LEDC
    LGFX_Sprite* canvas();              // offscreen 240x240 buffer
    void push();                        // blit canvas to panel
};

extern Display display;
