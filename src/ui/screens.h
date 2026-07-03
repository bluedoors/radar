#pragma once
#include <string>
#include <LovyanGFX.hpp>
void screen_splash(LGFX_Sprite* c);
void screen_info(LGFX_Sprite* c, const std::string& ip, const std::string& ssid,
                 float lat, float lon, bool wifi_ok);
