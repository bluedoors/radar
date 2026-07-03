#ifndef UNIT_TEST
#include <Arduino.h>
#include "config.h"
#include "hardware/display.h"
#include "hardware/button.h"
#include "net/wifi_setup.h"
#include "services/adsb_client.h"
#include "ui/screens.h"
#include "ui/radar_render.h"
#include "model/geo.h"
#include "data/airports.h"

enum class Screen { Splash, Info, Radar };
static Screen screen = Screen::Splash;
static ButtonFsm button;
static HomeConfig home;
static int range_idx = 2;               // default 25 km (RANGE_PRESETS_KM[2])
static uint32_t last_poll = 0;
static uint32_t info_since = 0;
static std::vector<Aircraft> planes;

static std::vector<AirportScreen> g_airports;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BOOT, INPUT_PULLUP);
    display.begin();
    screen_splash(display.canvas());
    display.push();
    home = wifi_begin();
    if (home.valid) {
        for (auto& a : get_airports()) {
            float d = haversine_km(home.lat, home.lon, a.lat, a.lon);
            float b = bearing_deg(home.lat, home.lon, a.lat, a.lon);
            g_airports.push_back({ a.icao.c_str(), d, b });
        }
    }
    screen = Screen::Info;
    screen_info(display.canvas(), home.ip, home.ssid, home.lat, home.lon, home.valid);
    display.push();
}

void loop() {
    uint32_t now = millis();
    bool pressed = (digitalRead(PIN_BOOT) == LOW);
    ButtonEvent ev = button.update(pressed, now);

    if (ev == ButtonEvent::Long) { wifi_reset(); return; }

    if (screen == Screen::Info) {
        if (home.valid && (ev == ButtonEvent::Short || ev == ButtonEvent::Double)) {
            screen = Screen::Radar; last_poll = 0; info_since = 0;
        }
    } else if (screen == Screen::Radar) {
        if (ev == ButtonEvent::Short) {
            range_idx = (range_idx + 1) % RANGE_COUNT;
            last_poll = 0;
        } else if (ev == ButtonEvent::Double) {
            screen = Screen::Info; info_since = now;
            screen_info(display.canvas(), home.ip, home.ssid, home.lat, home.lon, home.valid);
            display.push();
        }
    }

    // Auto-return from peeked info screen after 5 000 ms
    if (screen == Screen::Info && info_since && (now - info_since) > 5000) {
        screen = Screen::Radar; info_since = 0; last_poll = 0;
    }

    if (screen == Screen::Radar && (last_poll == 0 || now - last_poll >= POLL_INTERVAL_MS)) {
        last_poll = now;
        float range = RANGE_PRESETS_KM[range_idx];
        auto fresh = adsb_fetch(home.lat, home.lon, range);
        if (!fresh.empty()) planes = fresh;
        auto* c = display.canvas();
        render_radar(c, planes, range);
        render_airports(c, g_airports, range);
        display.push();
    }

    delay(20);
}
#else
// Native / unit-test build: provide a no-op entry point so the
// linker is satisfied when running `pio run -e native`.
// Actual tests live in test/ and are driven by `pio test`.
int main() { return 0; }
#endif
