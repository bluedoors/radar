#include "net/wifi_setup.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>

static Preferences prefs;

HomeConfig wifi_begin() {
    HomeConfig cfg;
    prefs.begin("radar", false);
    float lat = prefs.getFloat("lat", 1000.0f);
    float lon = prefs.getFloat("lon", 1000.0f);

    WiFiManager wm;
    char latbuf[16], lonbuf[16];
    snprintf(latbuf, sizeof(latbuf), "%.5f", lat<999 ? lat : 0.0f);
    snprintf(lonbuf, sizeof(lonbuf), "%.5f", lon<999 ? lon : 0.0f);
    WiFiManagerParameter p_lat("lat","Latitude",latbuf,15);
    WiFiManagerParameter p_lon("lon","Longitude",lonbuf,15);
    wm.addParameter(&p_lat); wm.addParameter(&p_lon);

    bool ok = wm.autoConnect(AP_NAME);   // blocks in portal until connected
    if (ok) {
        lat = atof(p_lat.getValue()); lon = atof(p_lon.getValue());
        prefs.putFloat("lat", lat); prefs.putFloat("lon", lon);
        cfg.valid = true; cfg.lat = lat; cfg.lon = lon;
        cfg.ssid = std::string(WiFi.SSID().c_str());
        cfg.ip = std::string(WiFi.localIP().toString().c_str());
    }
    prefs.end();
    return cfg;
}
void wifi_reset() {
    WiFiManager wm; wm.resetSettings();
    prefs.begin("radar", false); prefs.clear(); prefs.end();
    ESP.restart();
}
