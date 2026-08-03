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

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_system.h>

// ---- DIAGNOSTIC (temporary) ------------------------------------------------------
// Why did we (re)boot? On a crash-loop this is the single most decisive line: it tells
// us BROWNOUT (power) vs PANIC (exception/stack overflow) vs WDT vs a clean restart.
static const char* reset_reason_str(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON:  return "POWERON (cold boot)";
        case ESP_RST_SW:       return "SW (ESP.restart)";
        case ESP_RST_PANIC:    return "PANIC (exception/abort/stack-overflow)";
        case ESP_RST_INT_WDT:  return "INT_WDT (interrupt watchdog)";
        case ESP_RST_TASK_WDT: return "TASK_WDT (task watchdog)";
        case ESP_RST_WDT:      return "WDT (other watchdog)";
        case ESP_RST_BROWNOUT: return "BROWNOUT (power dip)";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_EXT:      return "EXT (external reset pin)";
        default:               return "UNKNOWN";
    }
}

enum class Screen { Splash, Info, Radar };
static Screen screen = Screen::Splash;
static ButtonFsm button;
static HomeConfig home;
static int range_idx = 1;               // default 10 km (RANGE_PRESETS_KM[1])
// Info screen auto-advance: `info_since` is the millis() stamp when it was shown, and
// `info_timeout` how long to hold it. Zero timeout = stay put until the button is pressed
// (used when WiFi isn't configured — there's nothing to advance to).
static uint32_t info_since = 0;
static uint32_t info_timeout = 0;
static constexpr uint32_t INFO_BOOT_MS = 15000;  // after power-on: long enough to read IP/SSID
static constexpr uint32_t INFO_PEEK_MS = 5000;   // long-press peek from the radar
static std::vector<Aircraft> planes;    // owned by the render loop (core 1)

static std::vector<AirportScreen> g_airports;

// ---- Cross-core aircraft handoff -------------------------------------------------
// The HTTPS fetch is slow and blocking, so it runs on core 0 in its own task. It writes
// results into g_shared under a mutex; the render loop (core 1) copies them out when
// fresh. This keeps the button/render loop responsive at all times — a press is never
// swallowed by an in-flight network request.
static SemaphoreHandle_t g_lock;
static std::vector<Aircraft> g_shared;
static volatile bool g_have_fresh = false;

static void fetch_task(void*) {
    for (;;) {
        // Range is display-only; the fetch always uses the widest preset so the cached
        // set covers every range the user might cycle to without needing a re-fetch.
        float fetch_km = RANGE_PRESETS_KM[RANGE_COUNT - 1];
        auto fresh = adsb_fetch(home.lat, home.lon, fetch_km);
        // DIAGNOSTIC: stack_words_left is the tightest-ever free stack for this task.
        // If it trends toward 0 we've found suspect #2 (TLS overflowing the 8 KB stack).
        Serial.printf("[FETCH] n=%d stack_words_left=%u free_heap=%u min_free_ever=%u\n",
                      (int)fresh.size(), uxTaskGetStackHighWaterMark(nullptr),
                      ESP.getFreeHeap(), ESP.getMinFreeHeap());
        if (!fresh.empty()) {
            xSemaphoreTake(g_lock, portMAX_DELAY);
            g_shared = std::move(fresh);
            g_have_fresh = true;
            xSemaphoreGive(g_lock);
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}

void setup() {
    Serial.begin(115200);
    delay(50);  // let USB-serial settle so the first line isn't truncated
    // DIAGNOSTIC: report why we just booted, plus starting heap.
    Serial.printf("\n[BOOT] reset_reason=%s  free_heap=%u  largest_block=%u\n",
                  reset_reason_str(esp_reset_reason()),
                  ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    pinMode(PIN_BOOT, INPUT_PULLUP);
    display.begin();
    Serial.printf("[BOOT] after display.begin: free_heap=%u  largest_block=%u\n",
                  ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    screen_splash(display.canvas());
    display.push();
    // Show setup instructions on the panel while the captive portal is open.
    home = wifi_begin([](const std::string& ap, const std::string& ip) {
        screen_portal(display.canvas(), ap, ip);
        display.push();
    });
    if (home.valid) {
        for (auto& a : get_airports()) {
            float d = haversine_km(home.lat, home.lon, a.lat, a.lon);
            float b = bearing_deg(home.lat, home.lon, a.lat, a.lon);
            g_airports.push_back({ a.icao.c_str(), d, b });
        }
        // Start the background fetch task on core 0 (loop()/render run on core 1).
        g_lock = xSemaphoreCreateMutex();
        xTaskCreatePinnedToCore(fetch_task, "fetch", 8192, nullptr, 1, nullptr, 0);
    }
    screen = Screen::Info;
    // Auto-advance to the radar on power-cycle so the device needs no button press. Only
    // when WiFi is configured — otherwise the info screen *is* the useful thing to show.
    info_since   = home.valid ? millis() : 0;
    info_timeout = home.valid ? INFO_BOOT_MS : 0;
    screen_info(display.canvas(), home.ip, home.ssid, home.lat, home.lon, home.valid);
    display.push();
}

// Re-render the radar frame from the aircraft/airports already in memory (no network).
// Used both after a fresh poll and after an instant range change.
static void draw_radar() {
    float range = RANGE_PRESETS_KM[range_idx];
    auto* c = display.canvas();
    render_radar(c, planes, range);
    render_airports(c, g_airports, range);
    display.push();
}

// Pull newly-fetched aircraft from the background task, if any arrived. Returns true if
// `planes` was updated (so the caller can redraw). Non-blocking.
static bool consume_fresh() {
    if (!g_have_fresh) return false;
    if (xSemaphoreTake(g_lock, 0) != pdTRUE) return false;  // task holds it; try next loop
    planes = g_shared;
    g_have_fresh = false;
    xSemaphoreGive(g_lock);
    return true;
}

void loop() {
    uint32_t now = millis();
    bool pressed = (digitalRead(PIN_BOOT) == LOW);
    ButtonEvent ev = button.update(pressed, now);

    // Very-long hold (~8 s): wipe config + reopen portal.
    if (ev == ButtonEvent::LongReset) { wifi_reset(); return; }

    if (screen == Screen::Info) {
        // Short press starts the radar (only meaningful when WiFi is configured).
        if (home.valid && ev == ButtonEvent::Short) {
            screen = Screen::Radar; info_since = 0; info_timeout = 0;
            draw_radar();
        }
    } else if (screen == Screen::Radar) {
        if (ev == ButtonEvent::Short) {
            // Instant range change — just rescale and redraw the cached aircraft.
            range_idx = (range_idx + 1) % RANGE_COUNT;
            draw_radar();
        } else if (ev == ButtonEvent::LongPeek) {
            screen = Screen::Info; info_since = now; info_timeout = INFO_PEEK_MS;
            screen_info(display.canvas(), home.ip, home.ssid, home.lat, home.lon, home.valid);
            display.push();
        }
    }

    // Leave the info screen once its timeout expires (15 s after boot, 5 s after a peek).
    if (screen == Screen::Info && info_timeout && (now - info_since) > info_timeout) {
        screen = Screen::Radar; info_since = 0; info_timeout = 0;
        draw_radar();
    }

    // Redraw when the background task delivers fresh aircraft.
    if (screen == Screen::Radar && consume_fresh()) {
        draw_radar();
    }

    delay(20);   // ~50 Hz button polling — always responsive, never blocked on network
}
#else
// Native / unit-test build: provide a no-op entry point so the
// linker is satisfied when running `pio run -e native`.
// Actual tests live in test/ and are driven by `pio test`.
int main() { return 0; }
#endif
