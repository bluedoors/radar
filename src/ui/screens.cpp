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

void screen_portal(LGFX_Sprite* c, const std::string& ap_name, const std::string& portal_ip) {
    c->fillSprite(COL_FIELD);
    c->drawCircle(120, 120, 112, COL_RING);
    c->setTextColor(COL_VFR);                       // amber = "attention / action needed"
    c->drawString("WiFi SETUP", 82, 88);
    c->setTextColor(COL_N);
    c->drawString("Join WiFi network:", 58, 108);
    c->drawString(ap_name.c_str(), 120 - (int)(ap_name.size() * 3), 124);
    c->drawString("then open", 92, 144);
    c->drawString(portal_ip.c_str(), 120 - (int)(portal_ip.size() * 3), 160);
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
