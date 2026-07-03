#include "ui/screens.h"
#include "config.h"
#include <cstdio>
#include <cstring>

void screen_splash(LGFX_Sprite* c) {
    c->fillSprite(COL_FIELD);
    c->drawCircle(120, 120, 112, COL_RING);
    c->setTextColor(COL_N);
    c->drawString("PLANE RADAR", 74, 110);
    c->drawString("starting...", 82, 128);
}

void screen_info(LGFX_Sprite* c, const std::string& ip, const std::string& ssid,
                 float lat, float lon, bool wifi_ok) {
    c->fillSprite(COL_FIELD);
    c->drawCircle(120, 120, 112, COL_RING);
    c->setTextColor(COL_N);
    if (!wifi_ok) {
        c->drawString("No WiFi", 92, 104);
        c->drawString("hold BOOT to setup", 58, 124);
        return;
    }
    c->setTextColor(COL_HELO);
    c->drawString(ip.c_str(), 120 - (int)(ip.size() * 3), 96);
    c->setTextColor(COL_N);
    c->drawString(ssid.c_str(), 120 - (int)(ssid.size() * 3), 116);
    char geo[32];
    snprintf(geo, sizeof(geo), "%.3f,%.3f", lat, lon);
    c->drawString(geo, 120 - (int)(strlen(geo) * 3), 132);
    c->drawString("BOOT = start", 84, 150);
}
