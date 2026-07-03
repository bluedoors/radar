#pragma once
#include <string>
#include <LovyanGFX.hpp>
void screen_splash(LGFX_Sprite* c);
// Shown while the WiFi captive portal is open, so the panel isn't a misleading "starting...".
void screen_portal(LGFX_Sprite* c, const std::string& ap_name, const std::string& portal_ip);
void screen_info(LGFX_Sprite* c, const std::string& ip, const std::string& ssid,
                 float lat, float lon, bool wifi_ok);
