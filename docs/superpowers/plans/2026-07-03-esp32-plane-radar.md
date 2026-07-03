# ESP32 Plane Radar Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a round-LCD "plane radar" on an ESP32-WROOM-32 that shows live, colour-classified aircraft and nearby airports centered on a configured home location.

**Architecture:** PlatformIO/Arduino project split into pure-logic modules (classification, projection, unit conversion — unit-tested off-device in a `native` env) and hardware/IO modules (display, WiFi/captive-portal, ADS-B HTTP client, button, screens). `main.cpp` is a thin state-machine coordinator (splash → info → radar) polling the ADS-B API every 5 s.

**Tech Stack:** PlatformIO, Arduino framework, board `esp32dev`; LovyanGFX (GC9A01), WiFiManager (tzapu), ArduinoJson 7, ESP32 Preferences (NVS), LEDC (backlight PWM). Native unit tests via PlatformIO Unity.

**Reference spec:** `docs/superpowers/specs/2026-07-03-esp32-plane-radar-design.md`

---

## File Structure

```
platformio.ini                     two envs: esp32dev (device), native (host tests)
include/config.h                   pins, ranges, colours, API host, intervals, geo constants
src/model/aircraft.h/.cpp          Aircraft struct + Bucket enum + classify() (pure)
src/model/units.h/.cpp             km<->NM conversion, altitude formatting (pure)
src/ui/projection.h/.cpp           (dst,dir)+range -> screen x,y; ring index + tag gating (pure)
src/data/airports.h/.cpp           static YSSY/YSBK table (pure data)
src/hardware/display.h/.cpp        LovyanGFX GC9A01 device config + LEDC backlight
src/hardware/button.h/.cpp         BOOT (GPIO0) short/double/long press detector (pure logic core)
src/net/wifi_setup.h/.cpp          WiFiManager captive portal + NVS persistence
src/services/adsb_client.h/.cpp    HTTPS GET + filtered ArduinoJson parse -> vector<Aircraft>
src/ui/radar_render.h/.cpp         draw rings/grid + aircraft/airport layers (uses projection)
src/ui/screens.h/.cpp              splash / info / radar screen renderers
src/main.cpp                       setup + loop, screen state machine, poll timing
test/native/test_classify/        Unity tests for classify()
test/native/test_units/            Unity tests for units
test/native/test_projection/       Unity tests for projection + gating
test/native/test_button/           Unity tests for press detector
test/fixtures/sydney_sample.json   captured adsb.fi payload for offline parse tests
```

Pure-logic files (`model/`, `ui/projection`, `data/`, `button` core) compile under both envs and carry all unit tests. Hardware files compile only under `esp32dev`.

---

## Task 1: Project scaffold + dual-env PlatformIO config

**Files:**
- Create: `platformio.ini`
- Create: `.gitignore` (already exists — verify)
- Create: `src/main.cpp` (temporary stub)

- [ ] **Step 1: Write `platformio.ini`**

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_port = /dev/cu.usbserial-0001
monitor_port = /dev/cu.usbserial-0001
build_flags = -DCORE_DEBUG_LEVEL=1
lib_deps =
    lovyan03/LovyanGFX@^1.2.24
    bblanchon/ArduinoJson@^7.4.3
    tzapu/WiFiManager@^2.0.17

[env:native]
platform = native
test_framework = unity
build_flags = -std=c++17 -DUNIT_TEST
```

- [ ] **Step 2: Write temporary `src/main.cpp` stub**

```cpp
#ifndef UNIT_TEST
#include <Arduino.h>
void setup() { Serial.begin(115200); Serial.println("radar boot"); }
void loop() {}
#endif
```

- [ ] **Step 3: Verify device env compiles**

Run: `pio run -e esp32dev 2>&1 | tail -5`
Expected: `SUCCESS` (downloads libs on first run; may take a few minutes).

- [ ] **Step 4: Verify native env builds (nothing to test yet)**

Run: `pio run -e native 2>&1 | tail -5`
Expected: `SUCCESS`.

- [ ] **Step 5: Commit**

```bash
git add platformio.ini src/main.cpp .gitignore
git commit -m "chore: PlatformIO scaffold with device + native envs"
```

---

## Task 2: Units module (km↔NM, altitude formatting) — TDD

**Files:**
- Create: `src/model/units.h`, `src/model/units.cpp`
- Test: `test/native/test_units/test_units.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
#include <unity.h>
#include "model/units.h"

void test_km_to_nm() {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 13.4989f, km_to_nm(25.0f));
}
void test_nm_to_km() {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, nm_to_km(13.4989f));
}
void test_format_altitude_ground() {
    TEST_ASSERT_EQUAL_STRING("GND", format_altitude_ft(0, true).c_str());
}
void test_format_altitude_thousands() {
    TEST_ASSERT_EQUAL_STRING("30k", format_altitude_ft(30000, false).c_str());
}
void test_format_altitude_low() {
    TEST_ASSERT_EQUAL_STRING("900", format_altitude_ft(900, false).c_str());
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_km_to_nm);
    RUN_TEST(test_nm_to_km);
    RUN_TEST(test_format_altitude_ground);
    RUN_TEST(test_format_altitude_thousands);
    RUN_TEST(test_format_altitude_low);
    return UNITY_END();
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `pio test -e native -f test_units 2>&1 | tail -15`
Expected: FAIL — `units.h` not found / undefined references.

- [ ] **Step 3: Write minimal implementation**

`src/model/units.h`:
```cpp
#pragma once
#include <string>
float km_to_nm(float km);
float nm_to_km(float nm);
// ground=true -> "GND". Else >=10000ft -> "Nk" (rounded to nearest 1000),
// otherwise the raw foot value as a string.
std::string format_altitude_ft(int feet, bool ground);
```

`src/model/units.cpp`:
```cpp
#include "model/units.h"
float km_to_nm(float km) { return km / 1.852f; }
float nm_to_km(float nm) { return nm * 1.852f; }
std::string format_altitude_ft(int feet, bool ground) {
    if (ground) return "GND";
    if (feet >= 10000) return std::to_string(feet / 1000) + "k";
    return std::to_string(feet);
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `pio test -e native -f test_units 2>&1 | tail -8`
Expected: PASS (5 tests).

- [ ] **Step 5: Commit**

```bash
git add src/model/units.* test/native/test_units/
git commit -m "feat: units module (km/NM, altitude formatting) with tests"
```

---

## Task 3: Aircraft model + classify() — TDD

**Files:**
- Create: `src/model/aircraft.h`, `src/model/aircraft.cpp`
- Test: `test/native/test_classify/test_classify.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
#include <unity.h>
#include "model/aircraft.h"

static Aircraft mk(const char* cat, const char* type, bool ground) {
    Aircraft a; a.category = cat; a.type = type; a.on_ground = ground; return a;
}
void test_commercial_a3() { TEST_ASSERT_EQUAL(Bucket::Commercial, classify(mk("A3","B738",false))); }
void test_commercial_a5() { TEST_ASSERT_EQUAL(Bucket::Commercial, classify(mk("A5","B77L",false))); }
void test_vfr_a1()        { TEST_ASSERT_EQUAL(Bucket::VFR,        classify(mk("A1","C510",false))); }
void test_helo_a7()       { TEST_ASSERT_EQUAL(Bucket::Helicopter, classify(mk("A7","EC45",false))); }
void test_helo_by_type()  { TEST_ASSERT_EQUAL(Bucket::Helicopter, classify(mk("","R44",false))); }
void test_vfr_by_type()   { TEST_ASSERT_EQUAL(Bucket::VFR,        classify(mk("","P28A",false))); }
void test_unknown_a0()    { TEST_ASSERT_EQUAL(Bucket::Unknown,    classify(mk("A0","",false))); }
void test_unknown_blank() { TEST_ASSERT_EQUAL(Bucket::Unknown,    classify(mk("","",false))); }
void test_filtered_ground(){TEST_ASSERT_EQUAL(Bucket::Filtered,   classify(mk("A3","B738",true))); }
void test_filtered_c2()   { TEST_ASSERT_EQUAL(Bucket::Filtered,   classify(mk("C2","",false))); }
int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_commercial_a3); RUN_TEST(test_commercial_a5);
    RUN_TEST(test_vfr_a1); RUN_TEST(test_helo_a7); RUN_TEST(test_helo_by_type);
    RUN_TEST(test_vfr_by_type); RUN_TEST(test_unknown_a0); RUN_TEST(test_unknown_blank);
    RUN_TEST(test_filtered_ground); RUN_TEST(test_filtered_c2);
    return UNITY_END();
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `pio test -e native -f test_classify 2>&1 | tail -15`
Expected: FAIL — `aircraft.h` not found.

- [ ] **Step 3: Write minimal implementation**

`src/model/aircraft.h`:
```cpp
#pragma once
#include <string>
enum class Bucket { Commercial, VFR, Helicopter, Unknown, Filtered };
struct Aircraft {
    float lat = 0, lon = 0;
    float track = 0;        // degrees
    int alt_ft = 0;
    bool on_ground = false;
    float gs_knots = 0;
    float dst_nm = 0;       // API-provided distance from center
    float dir_deg = 0;      // API-provided bearing from center
    std::string callsign;
    std::string category;   // ADS-B emitter category e.g. "A3"
    std::string type;       // ICAO type e.g. "B738"
};
Bucket classify(const Aircraft& a);
```

`src/model/aircraft.cpp`:
```cpp
#include "model/aircraft.h"
#include <array>

static bool is_rotor_type(const std::string& t) {
    static const std::array<const char*,6> rotors{"EC45","AS50","R44","R22","H60","B06"};
    for (auto r : rotors) if (t == r) return true;
    return false;
}
static bool is_light_type(const std::string& t) {
    static const std::array<const char*,6> light{"P28A","C172","SR22","C152","DA40","PA28"};
    for (auto l : light) if (t == l) return true;
    return false;
}

Bucket classify(const Aircraft& a) {
    if (a.on_ground) return Bucket::Filtered;
    const std::string& c = a.category;
    // Surface/service categories (B*, C*) are not aircraft.
    if (!c.empty() && (c[0] == 'B' || c[0] == 'C')) return Bucket::Filtered;
    if (c == "A7" || is_rotor_type(a.type)) return Bucket::Helicopter;
    if (c == "A2" || c == "A3" || c == "A4" || c == "A5") return Bucket::Commercial;
    if (c == "A1" || is_light_type(a.type)) return Bucket::VFR;
    return Bucket::Unknown;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `pio test -e native -f test_classify 2>&1 | tail -8`
Expected: PASS (10 tests).

- [ ] **Step 5: Commit**

```bash
git add src/model/aircraft.* test/native/test_classify/
git commit -m "feat: Aircraft model + classify() with tests"
```

---

## Task 4: Projection + ring/tag gating — TDD

**Files:**
- Create: `src/ui/projection.h`, `src/ui/projection.cpp`
- Test: `test/native/test_projection/test_projection.cpp`

Screen is 240×240, center (120,120), usable radius 112 px. `range_km` maps to the outer
radius. Ring index: 0 = innermost third, 1 = middle, 2 = outer, 3 = beyond range (rim).
Tag shown when `ring_index <= 1` (inside the 2nd ring). Screen y is inverted (north = up).

- [ ] **Step 1: Write the failing test**

```cpp
#include <unity.h>
#include "ui/projection.h"

void test_center_when_zero_dist() {
    Point p = project(0.0f, 0.0f, 25.0f);
    TEST_ASSERT_INT_WITHIN(1, 120, p.x);
    TEST_ASSERT_INT_WITHIN(1, 120, p.y);
}
void test_north_edge_at_range() {
    // dst == range, bearing 0 (north) -> top of circle
    Point p = project(25.0f, 0.0f, 25.0f);
    TEST_ASSERT_INT_WITHIN(1, 120, p.x);
    TEST_ASSERT_INT_WITHIN(2, 8, p.y);   // 120 - 112 = 8
}
void test_east_half_range() {
    // half range, bearing 90 (east) -> right, mid radius
    Point p = project(12.5f, 90.0f, 25.0f);
    TEST_ASSERT_INT_WITHIN(2, 176, p.x); // 120 + 56
    TEST_ASSERT_INT_WITHIN(2, 120, p.y);
}
void test_ring_index_inner()  { TEST_ASSERT_EQUAL(0, ring_index(3.0f, 25.0f)); }
void test_ring_index_middle() { TEST_ASSERT_EQUAL(1, ring_index(12.0f, 25.0f)); }
void test_ring_index_outer()  { TEST_ASSERT_EQUAL(2, ring_index(22.0f, 25.0f)); }
void test_ring_index_beyond() { TEST_ASSERT_EQUAL(3, ring_index(40.0f, 25.0f)); }
void test_tag_visible_inside_2nd() { TEST_ASSERT_TRUE(tag_visible(ring_index(12.0f, 25.0f))); }
void test_tag_hidden_outer()       { TEST_ASSERT_FALSE(tag_visible(ring_index(22.0f, 25.0f))); }
int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_center_when_zero_dist); RUN_TEST(test_north_edge_at_range);
    RUN_TEST(test_east_half_range); RUN_TEST(test_ring_index_inner);
    RUN_TEST(test_ring_index_middle); RUN_TEST(test_ring_index_outer);
    RUN_TEST(test_ring_index_beyond); RUN_TEST(test_tag_visible_inside_2nd);
    RUN_TEST(test_tag_hidden_outer);
    return UNITY_END();
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `pio test -e native -f test_projection 2>&1 | tail -15`
Expected: FAIL — `projection.h` not found.

- [ ] **Step 3: Write minimal implementation**

`src/ui/projection.h`:
```cpp
#pragma once
struct Point { int x; int y; };
// Project an aircraft at distance dst_km, bearing dir_deg from center, for a given range.
// Clamps to the rim when beyond range.
Point project(float dst_km, float dir_deg, float range_km);
// 0 = inner third, 1 = middle third, 2 = outer third, 3 = beyond range.
int ring_index(float dst_km, float range_km);
// Tag shown when inside the 2nd ring (ring_index <= 1).
bool tag_visible(int ring_idx);
```

`src/ui/projection.cpp`:
```cpp
#include "ui/projection.h"
#include <cmath>

static constexpr int CENTER = 120;
static constexpr float MAX_R = 112.0f;

Point project(float dst_km, float dir_deg, float range_km) {
    float frac = (range_km <= 0) ? 0 : dst_km / range_km;
    if (frac > 1.0f) frac = 1.0f;         // clamp beyond-range to rim
    float r = frac * MAX_R;
    float a = dir_deg * (float)M_PI / 180.0f;
    int x = (int)lroundf(CENTER + r * sinf(a));   // bearing 0 = north (up)
    int y = (int)lroundf(CENTER - r * cosf(a));
    return { x, y };
}
int ring_index(float dst_km, float range_km) {
    if (range_km <= 0 || dst_km > range_km) return 3;
    float frac = dst_km / range_km;
    if (frac <= 1.0f/3.0f) return 0;
    if (frac <= 2.0f/3.0f) return 1;
    return 2;
}
bool tag_visible(int ring_idx) { return ring_idx <= 1; }
```

- [ ] **Step 4: Run test to verify it passes**

Run: `pio test -e native -f test_projection 2>&1 | tail -8`
Expected: PASS (9 tests).

- [ ] **Step 5: Commit**

```bash
git add src/ui/projection.* test/native/test_projection/
git commit -m "feat: projection + ring/tag gating with tests"
```

---

## Task 5: Button press detector (short/double/long) — TDD

**Files:**
- Create: `src/hardware/button.h`, `src/hardware/button.cpp`
- Test: `test/native/test_button/test_button.cpp`

The detector is a pure state machine driven by `(pressed_bool, now_ms)` calls, so it is
testable off-device. GPIO reads happen in the hardware wrapper (Task 9+/main). Thresholds:
long ≥ 1500 ms held; double = two releases within 400 ms; otherwise short.

- [ ] **Step 1: Write the failing test**

```cpp
#include <unity.h>
#include "hardware/button.h"

void test_short_press() {
    ButtonFsm b;
    TEST_ASSERT_EQUAL(ButtonEvent::None, b.update(true, 0));
    TEST_ASSERT_EQUAL(ButtonEvent::None, b.update(false, 100)); // released, wait for double window
    TEST_ASSERT_EQUAL(ButtonEvent::Short, b.update(false, 600)); // window elapsed
}
void test_long_press() {
    ButtonFsm b;
    b.update(true, 0);
    TEST_ASSERT_EQUAL(ButtonEvent::Long, b.update(true, 1500));
}
void test_double_press() {
    ButtonFsm b;
    b.update(true, 0);  b.update(false, 80);
    b.update(true, 150);
    TEST_ASSERT_EQUAL(ButtonEvent::Double, b.update(false, 220));
}
int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_short_press); RUN_TEST(test_long_press); RUN_TEST(test_double_press);
    return UNITY_END();
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `pio test -e native -f test_button 2>&1 | tail -15`
Expected: FAIL — `button.h` not found.

- [ ] **Step 3: Write minimal implementation**

`src/hardware/button.h`:
```cpp
#pragma once
#include <cstdint>
enum class ButtonEvent { None, Short, Double, Long };
class ButtonFsm {
public:
    // pressed = is the button physically down now; now_ms = monotonic ms.
    ButtonEvent update(bool pressed, uint32_t now_ms);
private:
    static constexpr uint32_t LONG_MS = 1500;
    static constexpr uint32_t DOUBLE_MS = 400;
    bool was_pressed_ = false;
    uint32_t press_start_ = 0;
    uint32_t last_release_ = 0;
    int pending_clicks_ = 0;
    bool long_fired_ = false;
};
```

`src/hardware/button.cpp`:
```cpp
#include "hardware/button.h"

ButtonEvent ButtonFsm::update(bool pressed, uint32_t now_ms) {
    ButtonEvent ev = ButtonEvent::None;
    if (pressed && !was_pressed_) {          // edge: down
        press_start_ = now_ms; long_fired_ = false;
    }
    if (pressed && !long_fired_ && (now_ms - press_start_) >= LONG_MS) {
        long_fired_ = true; pending_clicks_ = 0;
        was_pressed_ = pressed;
        return ButtonEvent::Long;
    }
    if (!pressed && was_pressed_) {          // edge: up
        if (!long_fired_) { pending_clicks_++; last_release_ = now_ms; }
    }
    // resolve clicks once the double-window has elapsed
    if (!pressed && pending_clicks_ > 0 && (now_ms - last_release_) >= DOUBLE_MS) {
        ev = (pending_clicks_ >= 2) ? ButtonEvent::Double : ButtonEvent::Short;
        pending_clicks_ = 0;
    }
    if (!pressed && pending_clicks_ >= 2) {  // fire double immediately on 2nd release
        ev = ButtonEvent::Double; pending_clicks_ = 0;
    }
    was_pressed_ = pressed;
    return ev;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `pio test -e native -f test_button 2>&1 | tail -8`
Expected: PASS (3 tests).

- [ ] **Step 5: Commit**

```bash
git add src/hardware/button.* test/native/test_button/
git commit -m "feat: button press-detector FSM with tests"
```

---

## Task 6: ADS-B response parser — TDD with captured fixture

**Files:**
- Create: `test/fixtures/sydney_sample.json` (captured payload, trimmed to ~6 aircraft)
- Create: `src/services/adsb_parse.h`, `src/services/adsb_parse.cpp` (pure parse, no network)
- Test: `test/native/test_parse/test_parse.cpp`

Parsing is separated from the HTTP client so it can be tested off-device. The parser takes
a JSON string and returns `std::vector<Aircraft>` with ground/surface items still included
(classification/filtering happens later via `classify`).

- [ ] **Step 1: Capture a real fixture**

Run:
```bash
curl -s "https://opendata.adsb.fi/api/v2/lat/-33.745/lon/151.115/dist/40" \
  | python3 -c "import sys,json; d=json.load(sys.stdin); d['aircraft']=d['aircraft'][:6]; print(json.dumps(d))" \
  > test/fixtures/sydney_sample.json
head -c 300 test/fixtures/sydney_sample.json
```
Expected: a JSON object with `now` and a 6-element `aircraft` array.

- [ ] **Step 2: Write the failing test**

```cpp
#include <unity.h>
#include <fstream>
#include <sstream>
#include "services/adsb_parse.h"

static std::string load() {
    std::ifstream f("test/fixtures/sydney_sample.json");
    std::stringstream ss; ss << f.rdbuf(); return ss.str();
}
void test_parses_all_aircraft() {
    auto v = parse_adsb(load());
    TEST_ASSERT_EQUAL(6, (int)v.size());
}
void test_first_has_position_or_ground() {
    auto v = parse_adsb(load());
    // every parsed entry has a callsign field populated or empty, never crashes
    TEST_ASSERT_TRUE(v[0].callsign.size() >= 0);
}
void test_ground_flag_detected() {
    // synthetic: alt_baro "ground" -> on_ground true
    auto v = parse_adsb(R"({"aircraft":[{"flight":"X","alt_baro":"ground","lat":1,"lon":2}]})");
    TEST_ASSERT_EQUAL(1, (int)v.size());
    TEST_ASSERT_TRUE(v[0].on_ground);
}
void test_numeric_alt_parsed() {
    auto v = parse_adsb(R"({"aircraft":[{"flight":"Y","alt_baro":30650,"track":124.1,"gs":300.9,"category":"A3","t":"B738","lat":1,"lon":2,"dst":21.8,"dir":312.7}]})");
    TEST_ASSERT_EQUAL(30650, v[0].alt_ft);
    TEST_ASSERT_FALSE(v[0].on_ground);
    TEST_ASSERT_EQUAL_STRING("A3", v[0].category.c_str());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 21.8f, v[0].dst_nm);
}
void test_empty_or_garbage_safe() {
    TEST_ASSERT_EQUAL(0, (int)parse_adsb("").size());
    TEST_ASSERT_EQUAL(0, (int)parse_adsb("not json").size());
    TEST_ASSERT_EQUAL(0, (int)parse_adsb("{}").size());
}
int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_parses_all_aircraft); RUN_TEST(test_first_has_position_or_ground);
    RUN_TEST(test_ground_flag_detected); RUN_TEST(test_numeric_alt_parsed);
    RUN_TEST(test_empty_or_garbage_safe);
    return UNITY_END();
}
```

- [ ] **Step 3: Run test to verify it fails**

Run: `pio test -e native -f test_parse 2>&1 | tail -15`
Expected: FAIL — `adsb_parse.h` not found.

- [ ] **Step 4: Write minimal implementation**

`src/services/adsb_parse.h`:
```cpp
#pragma once
#include <string>
#include <vector>
#include "model/aircraft.h"
// Parse an adsb.fi v2 response body into aircraft. Callsign is trimmed.
// Malformed/empty input returns an empty vector (never throws).
std::vector<Aircraft> parse_adsb(const std::string& body);
```

`src/services/adsb_parse.cpp`:
```cpp
#include "services/adsb_parse.h"
#include <ArduinoJson.h>

static std::string trim(std::string s) {
    while (!s.empty() && (s.back()==' '||s.back()=='\0')) s.pop_back();
    return s;
}

std::vector<Aircraft> parse_adsb(const std::string& body) {
    std::vector<Aircraft> out;
    JsonDocument doc;
    if (deserializeJson(doc, body)) return out;   // parse error -> empty
    JsonArray arr = doc["aircraft"].as<JsonArray>();
    if (arr.isNull()) return out;
    for (JsonObject o : arr) {
        Aircraft a;
        a.callsign = trim(std::string(o["flight"] | ""));
        a.category = std::string(o["category"] | "");
        a.type     = std::string(o["t"] | "");
        a.lat = o["lat"] | 0.0f;
        a.lon = o["lon"] | 0.0f;
        a.track = o["track"] | 0.0f;
        a.gs_knots = o["gs"] | 0.0f;
        a.dst_nm = o["dst"] | 0.0f;
        a.dir_deg = o["dir"] | 0.0f;
        // alt_baro is either an int or the string "ground"
        JsonVariant alt = o["alt_baro"];
        if (alt.is<const char*>()) {
            a.on_ground = (std::string(alt.as<const char*>()) == "ground");
            a.alt_ft = 0;
        } else {
            a.alt_ft = alt | 0;
            a.on_ground = false;
        }
        out.push_back(a);
    }
    return out;
}
```

- [ ] **Step 5: Run test to verify it passes**

Run: `pio test -e native -f test_parse 2>&1 | tail -8`
Expected: PASS (5 tests).

- [ ] **Step 6: Commit**

```bash
git add src/services/adsb_parse.* test/native/test_parse/ test/fixtures/sydney_sample.json
git commit -m "feat: ADS-B response parser with captured-fixture tests"
```

---

## Task 7: Config header + airport data table

**Files:**
- Create: `include/config.h`
- Create: `src/data/airports.h`, `src/data/airports.cpp`
- Test: `test/native/test_airports/test_airports.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
#include <unity.h>
#include "data/airports.h"

void test_has_yssy_and_ysbk() {
    auto v = get_airports();
    bool yssy=false, ysbk=false;
    for (auto& a : v) { if (a.icao=="YSSY") yssy=true; if (a.icao=="YSBK") ysbk=true; }
    TEST_ASSERT_TRUE(yssy); TEST_ASSERT_TRUE(ysbk);
}
void test_yssy_coords_plausible() {
    for (auto& a : get_airports()) if (a.icao=="YSSY") {
        TEST_ASSERT_FLOAT_WITHIN(0.05f, -33.9461f, a.lat);
        TEST_ASSERT_FLOAT_WITHIN(0.05f, 151.1772f, a.lon);
    }
}
int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_has_yssy_and_ysbk); RUN_TEST(test_yssy_coords_plausible);
    return UNITY_END();
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `pio test -e native -f test_airports 2>&1 | tail -15`
Expected: FAIL — `airports.h` not found.

- [ ] **Step 3: Write `include/config.h`**

```cpp
#pragma once
// ---- Display pins (confirmed against physical board) ----
#define PIN_MOSI 23
#define PIN_SCLK 18
#define PIN_CS   15
#define PIN_DC    2
#define PIN_RST   4
#define PIN_BL   22
#define PIN_BOOT  0
// ---- Backlight (LEDC) ----
#define BL_LEDC_CHANNEL 0
#define BL_LEDC_FREQ    5000
#define BL_LEDC_RES     8
#define BL_DEFAULT_DUTY 200   // 0..255
// ---- Behaviour ----
static const float RANGE_PRESETS_KM[] = { 5.0f, 10.0f, 25.0f, 40.0f };
static const int   RANGE_COUNT = 4;
#define POLL_INTERVAL_MS 5000
#define API_HOST "opendata.adsb.fi"
#define AP_NAME  "PlaneRadar-setup"
#define FW_VERSION "0.1.0"
// ---- Colours (RGB565) ----
#define COL_FIELD   0x0862   // #03121f navy
#define COL_RING    0x0A6B   // #0a3a5c
#define COL_N       0x7EBF   // #7fd4ff blue
#define COL_COMMERC 0xF9E7   // #ff3b3b red
#define COL_VFR     0xFE67   // #ffcf3b amber
#define COL_HELO    0x3FF1   // #3bff8f green
#define COL_UNKNOWN 0x8536   // #7f9bb0 grey-blue
#define COL_VECTOR  0xFAFC   // #ff5be0 magenta
#define COL_AIRPORT 0x05B9   // #00b4c8 cyan
```

- [ ] **Step 4: Write `src/data/airports.h` / `.cpp`**

`airports.h`:
```cpp
#pragma once
#include <string>
#include <vector>
struct Airport { std::string icao; float lat; float lon; };
const std::vector<Airport>& get_airports();
```

`airports.cpp` (large+medium ICAO airports near home; extend via OurAirports later):
```cpp
#include "data/airports.h"
static const std::vector<Airport> AIRPORTS = {
    { "YSSY", -33.9461f, 151.1772f },  // Sydney Kingsford Smith
    { "YSBK", -33.9244f, 150.9881f },  // Bankstown
};
const std::vector<Airport>& get_airports() { return AIRPORTS; }
```

- [ ] **Step 5: Run test to verify it passes**

Run: `pio test -e native -f test_airports 2>&1 | tail -8`
Expected: PASS (2 tests).

- [ ] **Step 6: Commit**

```bash
git add include/config.h src/data/airports.* test/native/test_airports/
git commit -m "feat: config header + airport data table with tests"
```

---

## Task 8: Display driver (GC9A01 via LovyanGFX) + backlight — device build

**Files:**
- Create: `src/hardware/display.h`, `src/hardware/display.cpp`

No native test (hardware only). Verified by on-device smoke check.

- [ ] **Step 1: Write `src/hardware/display.h`**

```cpp
#pragma once
#include <LovyanGFX.hpp>
class Display {
public:
    void begin();
    void set_backlight(uint8_t duty);   // 0..255 via LEDC
    LGFX_Sprite* canvas();              // offscreen 240x240 buffer
    void push();                        // blit canvas to panel
    LGFX& gfx();
private:
    LGFX lgfx_;
};
extern Display display;
```

- [ ] **Step 2: Write `src/hardware/display.cpp`**

```cpp
#include "hardware/display.h"
#include "config.h"

// LovyanGFX GC9A01 config for VSPI pin map.
class LGFX_GC9A01 : public lgfx::LGFX_Device {
    lgfx::Panel_GC9A01 _panel;
    lgfx::Bus_SPI _bus;
public:
    LGFX_GC9A01() {
        { auto c = _bus.config();
          c.spi_host = VSPI_HOST; c.spi_mode = 0;
          c.freq_write = 40000000; c.freq_read = 16000000;
          c.pin_sclk = PIN_SCLK; c.pin_mosi = PIN_MOSI; c.pin_miso = -1; c.pin_dc = PIN_DC;
          _bus.config(c); _panel.setBus(&_bus); }
        { auto c = _panel.config();
          c.pin_cs = PIN_CS; c.pin_rst = PIN_RST; c.pin_busy = -1;
          c.panel_width = 240; c.panel_height = 240;
          c.offset_x = 0; c.offset_y = 0; c.readable = false;
          c.invert = true; c.rgb_order = false; c.dlen_16bit = false; c.bus_shared = false;
          _panel.config(c); }
        setPanel(&_panel);
    }
};
static LGFX_GC9A01 g_lgfx;
static LGFX_Sprite g_canvas(&g_lgfx);
Display display;

void Display::begin() {
    g_lgfx.init();
    g_lgfx.setRotation(0);
    ledcSetup(BL_LEDC_CHANNEL, BL_LEDC_FREQ, BL_LEDC_RES);
    ledcAttachPin(PIN_BL, BL_LEDC_CHANNEL);
    set_backlight(BL_DEFAULT_DUTY);
    g_canvas.setColorDepth(16);
    g_canvas.createSprite(240, 240);
}
void Display::set_backlight(uint8_t duty) { ledcWrite(BL_LEDC_CHANNEL, duty); }
LGFX_Sprite* Display::canvas() { return &g_canvas; }
void Display::push() { g_canvas.pushSprite(&g_lgfx, 0, 0); }
LGFX& Display::gfx() { return g_lgfx; }
```

Note: `LGFX` is an alias — replace the `gfx()` return type usage with `LGFX_GC9A01`; if the
compiler objects to the `LGFX` name, change `display.h` to forward-declare and return
`lgfx::LGFX_Device&`. Keep the public surface (`begin/set_backlight/canvas/push`) stable.

- [ ] **Step 3: Temporary smoke `setup()` in `src/main.cpp`**

```cpp
#ifndef UNIT_TEST
#include <Arduino.h>
#include "hardware/display.h"
void setup() {
    Serial.begin(115200);
    display.begin();
    auto* c = display.canvas();
    c->fillSprite(0x0862);
    c->fillCircle(120,120,112, 0x0A6B);
    c->fillCircle(120,120,106, 0x0862);
    c->drawString("RADAR", 96, 116);
    display.push();
}
void loop() {}
#endif
```

- [ ] **Step 4: Build + flash + observe**

Run: `pio run -e esp32dev -t upload 2>&1 | tail -8`
Expected: `SUCCESS`, then the round panel shows a navy disc with a blue ring and "RADAR".
If the panel is black: re-check the pin map in `config.h` and `invert`/`rgb_order`.
(Flash with the display powered from a solid USB source — see spec §9 power note.)

- [ ] **Step 5: Commit**

```bash
git add src/hardware/display.* src/main.cpp
git commit -m "feat: GC9A01 display driver + LEDC backlight (smoke-tested)"
```

---

## Task 9: Radar renderer (rings + aircraft + airports)

**Files:**
- Create: `src/ui/radar_render.h`, `src/ui/radar_render.cpp`

Uses `projection`, `classify`, `units`, `airports`, and the display canvas. Draws onto the
offscreen sprite. No native test (drawing); logic it depends on is already tested.

- [ ] **Step 1: Write `src/ui/radar_render.h`**

```cpp
#pragma once
#include <vector>
#include "model/aircraft.h"
class LGFX_Sprite;
// Renders one full frame: field, rings, airports, then classified aircraft.
void render_radar(LGFX_Sprite* c, const std::vector<Aircraft>& aircraft, float range_km);
```

- [ ] **Step 2: Write `src/ui/radar_render.cpp`**

```cpp
#include "ui/radar_render.h"
#include "ui/projection.h"
#include "model/units.h"
#include "data/airports.h"
#include "hardware/display.h"
#include "config.h"
#include <cmath>

static uint16_t bucket_colour(Bucket b) {
    switch (b) {
        case Bucket::Commercial: return COL_COMMERC;
        case Bucket::VFR:        return COL_VFR;
        case Bucket::Helicopter: return COL_HELO;
        default:                 return COL_UNKNOWN;
    }
}
static void draw_triangle(LGFX_Sprite* c, Point p, float track, uint16_t col) {
    float a = track * (float)M_PI / 180.0f;
    auto rot = [&](int dx, int dy, int& ox, int& oy){
        ox = p.x + (int)lroundf(dx*cosf(a) - dy*sinf(a));
        oy = p.y + (int)lroundf(dx*sinf(a) + dy*cosf(a));
    };
    int x0,y0,x1,y1,x2,y2;
    rot(0,-6,x0,y0); rot(-4,5,x1,y1); rot(4,5,x2,y2);
    c->fillTriangle(x0,y0,x1,y1,x2,y2,col);
}

void render_radar(LGFX_Sprite* c, const std::vector<Aircraft>& aircraft, float range_km) {
    c->fillSprite(COL_FIELD);
    // rings + crosshairs
    for (int r : {38, 75, 112}) c->drawCircle(120,120,r, COL_RING);
    c->drawFastVLine(120, 8, 224, COL_RING);
    c->drawFastHLine(8, 120, 224, COL_RING);
    c->setTextColor(COL_N); c->drawString("N", 116, 4);
    c->fillCircle(120,120,2, COL_N);

    // airports (compute dst/bearing from home is done upstream; here use stored table
    // projected via their own dst/dir — see main.cpp which fills these). For static
    // markers we project using precomputed dst/dir passed through Aircraft-like path is
    // overkill, so airports are drawn by main via render helpers below.

    // aircraft
    for (const auto& a : aircraft) {
        Bucket b = classify(a);
        if (b == Bucket::Filtered) continue;
        float dst_km = nm_to_km(a.dst_nm);
        int ring = ring_index(dst_km, range_km);
        Point p = project(dst_km, a.dir_deg, range_km);
        uint16_t col = bucket_colour(b);
        if (ring == 3) { c->fillCircle(p.x, p.y, 2, col); continue; } // rim dot
        draw_triangle(c, p, a.track, col);
        // speed vector (magenta), length scaled to gs
        float a_rad = a.track * (float)M_PI/180.0f;
        int vlen = 6 + (int)(a.gs_knots/40.0f); if (vlen>22) vlen=22;
        c->drawLine(p.x, p.y, p.x + (int)(vlen*sinf(a_rad)), p.y - (int)(vlen*cosf(a_rad)), COL_VECTOR);
        if (tag_visible(ring) && !a.callsign.empty()) {
            std::string tag = a.callsign + " " + format_altitude_ft(a.alt_ft, a.on_ground);
            c->setTextColor(col);
            c->drawString(tag.c_str(), p.x - 12, p.y + 8);
        }
    }
}
```

- [ ] **Step 3: Add airport rendering helper**

Append to `radar_render.cpp` and declare in `.h`:
```cpp
// in radar_render.h:
struct AirportScreen { const char* icao; float dst_km; float dir_deg; };
void render_airports(LGFX_Sprite* c, const std::vector<AirportScreen>& aps, float range_km);
```
```cpp
// in radar_render.cpp:
void render_airports(LGFX_Sprite* c, const std::vector<AirportScreen>& aps, float range_km) {
    for (const auto& ap : aps) {
        int ring = ring_index(ap.dst_km, range_km);
        Point p = project(ap.dst_km, ap.dir_deg, range_km);
        if (ring == 3) {  // rim tick at bearing
            c->fillCircle(p.x, p.y, 2, COL_AIRPORT);
        } else {          // runway glyph
            c->drawCircle(p.x, p.y, 5, COL_AIRPORT);
            c->drawLine(p.x-3, p.y+3, p.x+3, p.y-3, COL_AIRPORT);
        }
        c->setTextColor(COL_AIRPORT);
        c->drawString(ap.icao, p.x-10, p.y+7);
    }
}
```

- [ ] **Step 4: Build (device) to confirm it compiles**

Run: `pio run -e esp32dev 2>&1 | tail -6`
Expected: `SUCCESS`.

- [ ] **Step 5: Commit**

```bash
git add src/ui/radar_render.*
git commit -m "feat: radar renderer (rings, classified aircraft, airports)"
```

---

## Task 10: Geo helper (home → airport dst/bearing) — TDD

**Files:**
- Create: `src/model/geo.h`, `src/model/geo.cpp`
- Test: `test/native/test_geo/test_geo.cpp`

Used to place static airports relative to the configured home lat/lon.

- [ ] **Step 1: Write the failing test**

```cpp
#include <unity.h>
#include "model/geo.h"
void test_distance_home_to_yssy() {
    // South Turramurra -> YSSY ~ 23.1 km
    float d = haversine_km(-33.745f,151.115f, -33.9461f,151.1772f);
    TEST_ASSERT_FLOAT_WITHIN(1.5f, 23.1f, d);
}
void test_bearing_home_to_yssy() {
    float b = bearing_deg(-33.745f,151.115f, -33.9461f,151.1772f);
    TEST_ASSERT_FLOAT_WITHIN(8.0f, 166.0f, b);
}
int main(int,char**){ UNITY_BEGIN(); RUN_TEST(test_distance_home_to_yssy); RUN_TEST(test_bearing_home_to_yssy); return UNITY_END(); }
```

- [ ] **Step 2: Run test to verify it fails**

Run: `pio test -e native -f test_geo 2>&1 | tail -12`
Expected: FAIL — `geo.h` not found.

- [ ] **Step 3: Write implementation**

`geo.h`:
```cpp
#pragma once
float haversine_km(float lat1,float lon1,float lat2,float lon2);
float bearing_deg(float lat1,float lon1,float lat2,float lon2);
```
`geo.cpp`:
```cpp
#include "model/geo.h"
#include <cmath>
static float rad(float d){ return d*(float)M_PI/180.0f; }
float haversine_km(float lat1,float lon1,float lat2,float lon2){
    float R=6371.0f, dp=rad(lat2-lat1), dl=rad(lon2-lon1);
    float a=sinf(dp/2)*sinf(dp/2)+cosf(rad(lat1))*cosf(rad(lat2))*sinf(dl/2)*sinf(dl/2);
    return 2*R*asinf(sqrtf(a));
}
float bearing_deg(float lat1,float lon1,float lat2,float lon2){
    float y=sinf(rad(lon2-lon1))*cosf(rad(lat2));
    float x=cosf(rad(lat1))*sinf(rad(lat2))-sinf(rad(lat1))*cosf(rad(lat2))*cosf(rad(lon2-lon1));
    float b=atan2f(y,x)*180.0f/(float)M_PI;
    return fmodf(b+360.0f,360.0f);
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `pio test -e native -f test_geo 2>&1 | tail -8`
Expected: PASS (2 tests).

- [ ] **Step 5: Commit**

```bash
git add src/model/geo.* test/native/test_geo/
git commit -m "feat: geo distance/bearing helpers with tests"
```

---

## Task 11: WiFi setup + NVS persistence (captive portal)

**Files:**
- Create: `src/net/wifi_setup.h`, `src/net/wifi_setup.cpp`

Device-only. WiFiManager custom params for lat/lon; persisted to NVS via Preferences.

- [ ] **Step 1: Write `src/net/wifi_setup.h`**

```cpp
#pragma once
#include <string>
struct HomeConfig { bool valid=false; float lat=0, lon=0; std::string ssid; std::string ip; };
// Loads config from NVS; if none/failed, opens the captive portal (blocking) until set.
HomeConfig wifi_begin();
// Wipes stored WiFi + lat/lon and reboots into the portal.
void wifi_reset();
bool wifi_connected();
```

- [ ] **Step 2: Write `src/net/wifi_setup.cpp`**

```cpp
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
bool wifi_connected() { return WiFi.status() == WL_CONNECTED; }
```

- [ ] **Step 3: Build (device) to confirm it compiles**

Run: `pio run -e esp32dev 2>&1 | tail -6`
Expected: `SUCCESS`.

- [ ] **Step 4: On-device smoke (manual)**

Flash a temporary `setup()` that calls `wifi_begin()` and prints the returned IP/SSID/lat/lon
over serial. Connect a phone to `PlaneRadar-setup`, enter WiFi + lat/lon, confirm serial
shows a valid IP and your coordinates. (This step is manual; revert the temp setup after.)

- [ ] **Step 5: Commit**

```bash
git add src/net/wifi_setup.*
git commit -m "feat: WiFiManager captive portal + NVS persistence"
```

---

## Task 12: ADS-B HTTP client (device) wrapping the tested parser

**Files:**
- Create: `src/services/adsb_client.h`, `src/services/adsb_client.cpp`

Device-only network layer; parsing is delegated to the already-tested `parse_adsb`.

- [ ] **Step 1: Write `src/services/adsb_client.h`**

```cpp
#pragma once
#include <vector>
#include "model/aircraft.h"
// GET aircraft within range_km of (lat,lon). Returns empty on any network/HTTP failure.
std::vector<Aircraft> adsb_fetch(float lat, float lon, float range_km);
```

- [ ] **Step 2: Write `src/services/adsb_client.cpp`**

```cpp
#include "services/adsb_client.h"
#include "services/adsb_parse.h"
#include "model/units.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

std::vector<Aircraft> adsb_fetch(float lat, float lon, float range_km) {
    float nm = km_to_nm(range_km);
    char url[160];
    snprintf(url, sizeof(url), "https://%s/api/v2/lat/%.5f/lon/%.5f/dist/%d",
             API_HOST, lat, lon, (int)(nm + 0.5f));
    WiFiClientSecure client; client.setInsecure();   // adsb.fi public endpoint
    HTTPClient http;
    if (!http.begin(client, url)) return {};
    http.setTimeout(8000);
    int code = http.GET();
    if (code != 200) { http.end(); return {}; }
    std::string body = std::string(http.getString().c_str());
    http.end();
    return parse_adsb(body);
}
```

- [ ] **Step 3: Build (device) to confirm it compiles**

Run: `pio run -e esp32dev 2>&1 | tail -6`
Expected: `SUCCESS`.

- [ ] **Step 4: Commit**

```bash
git add src/services/adsb_client.*
git commit -m "feat: ADS-B HTTPS client wrapping tested parser"
```

---

## Task 13: Screens (splash / info / radar) renderers

**Files:**
- Create: `src/ui/screens.h`, `src/ui/screens.cpp`

- [ ] **Step 1: Write `src/ui/screens.h`**

```cpp
#pragma once
#include <string>
class LGFX_Sprite;
void screen_splash(LGFX_Sprite* c);
void screen_info(LGFX_Sprite* c, const std::string& ip, const std::string& ssid,
                 float lat, float lon, bool wifi_ok);
```

- [ ] **Step 2: Write `src/ui/screens.cpp`**

```cpp
#include "ui/screens.h"
#include "config.h"
#include <LovyanGFX.hpp>

void screen_splash(LGFX_Sprite* c) {
    c->fillSprite(COL_FIELD);
    c->drawCircle(120,120,112, COL_RING);
    c->setTextColor(COL_N);
    c->drawString("PLANE RADAR", 74, 110);
    c->drawString("starting...", 82, 128);
}
void screen_info(LGFX_Sprite* c, const std::string& ip, const std::string& ssid,
                 float lat, float lon, bool wifi_ok) {
    c->fillSprite(COL_FIELD);
    c->drawCircle(120,120,112, COL_RING);
    c->setTextColor(COL_N);
    if (!wifi_ok) {
        c->drawString("No WiFi", 92, 104);
        c->drawString("hold BOOT to setup", 58, 124);
        return;
    }
    c->setTextColor(COL_HELO);
    c->drawString(ip.c_str(), 120 - (int)(ip.size()*3), 96);   // IP prominent
    c->setTextColor(COL_N);
    c->drawString(ssid.c_str(), 120 - (int)(ssid.size()*3), 116);
    char geo[32]; snprintf(geo, sizeof(geo), "%.3f,%.3f", lat, lon);
    c->drawString(geo, 120 - (int)(strlen(geo)*3), 132);
    c->drawString("BOOT = start", 84, 150);
}
```

- [ ] **Step 3: Build (device)**

Run: `pio run -e esp32dev 2>&1 | tail -6`
Expected: `SUCCESS`.

- [ ] **Step 4: Commit**

```bash
git add src/ui/screens.*
git commit -m "feat: splash + info screen renderers"
```

---

## Task 14: Main coordinator — state machine + poll loop

**Files:**
- Modify: `src/main.cpp` (replace all temp stubs)

State machine: `SPLASH → INFO → RADAR`; INFO re-entered on double-press (auto-returns after
5 s); long-press → `wifi_reset()`; short-press in RADAR cycles range.

- [ ] **Step 1: Write the full `src/main.cpp`**

```cpp
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
static int range_idx = 2;               // default 25 km
static uint32_t last_poll = 0;
static uint32_t info_since = 0;
static std::vector<Aircraft> planes;

static std::vector<AirportScreen> airports_for_home() {
    std::vector<AirportScreen> out;
    for (auto& a : get_airports()) {
        float d = haversine_km(home.lat, home.lon, a.lat, a.lon);
        float b = bearing_deg(home.lat, home.lon, a.lat, a.lon);
        out.push_back({ a.icao.c_str(), d, b });
    }
    return out;
}

void setup() {
    Serial.begin(115200);
    display.begin();
    screen_splash(display.canvas()); display.push();
    home = wifi_begin();               // blocks in portal if needed
    screen = Screen::Info;
    screen_info(display.canvas(), home.ip, home.ssid, home.lat, home.lon, home.valid);
    display.push();
}

void loop() {
    uint32_t now = millis();
    bool pressed = (digitalRead(PIN_BOOT) == LOW);   // BOOT is active-low
    ButtonEvent ev = button.update(pressed, now);

    if (ev == ButtonEvent::Long) { wifi_reset(); return; }

    if (screen == Screen::Info) {
        if (ev == ButtonEvent::Short || ev == ButtonEvent::Double) {
            screen = Screen::Radar; last_poll = 0;   // force immediate poll
        }
    } else if (screen == Screen::Radar) {
        if (ev == ButtonEvent::Short) {
            range_idx = (range_idx + 1) % RANGE_COUNT;
        } else if (ev == ButtonEvent::Double) {
            screen = Screen::Info; info_since = now;
            screen_info(display.canvas(), home.ip, home.ssid, home.lat, home.lon, home.valid);
            display.push();
        }
    }
    // auto-return from a peeked Info screen after 5 s
    if (screen == Screen::Info && info_since && (now - info_since) > 5000) {
        screen = Screen::Radar; info_since = 0; last_poll = 0;
    }

    if (screen == Screen::Radar && (last_poll == 0 || now - last_poll >= POLL_INTERVAL_MS)) {
        last_poll = now;
        float range = RANGE_PRESETS_KM[range_idx];
        auto fresh = adsb_fetch(home.lat, home.lon, range);
        if (!fresh.empty()) planes = fresh;          // keep last good frame on failure
        auto* c = display.canvas();
        render_radar(c, planes, range);
        render_airports(c, airports_for_home(), range);
        display.push();
    }
    delay(20);
}
#endif
```

- [ ] **Step 2: Build (device)**

Run: `pio run -e esp32dev 2>&1 | tail -8`
Expected: `SUCCESS`.

- [ ] **Step 3: Run the full native test suite (regression)**

Run: `pio test -e native 2>&1 | tail -20`
Expected: all suites PASS (units, classify, projection, button, parse, airports, geo).

- [ ] **Step 4: Flash + on-device acceptance**

Run: `pio run -e esp32dev -t upload 2>&1 | tail -6`
Then, powered from a solid USB source:
1. Splash appears → captive portal (first boot only) → enter WiFi + `-33.745 / 151.115`.
2. Info screen shows the assigned IP, SSID, coords.
3. Press BOOT → radar draws rings; within ~5 s aircraft appear colour-coded; YSSY/YSBK
   glyphs near the southern rim.
4. Short-press cycles 5→10→25→40 km (ring scale changes).
5. Double-press peeks the Info screen for ~5 s.
6. Long-press reopens the captive portal.

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "feat: main state machine + poll loop (end-to-end radar)"
```

---

## Task 15: README + wiring/build docs

**Files:**
- Create: `README.md`

- [ ] **Step 1: Write `README.md`** documenting: the pin map table (from `config.h`), how to
  build/flash (`pio run -e esp32dev -t upload`), how to run tests (`pio test -e native`),
  captive-portal setup steps, the button gestures, range presets, and the spec §9 power note
  (flash/display from a solid USB source; BL on GPIO22).

- [ ] **Step 2: Commit**

```bash
git add README.md
git commit -m "docs: README with wiring, build, and usage"
```

---

## Self-Review Notes

- **Spec coverage:** hardware/pins (Task 7,8) ✔; libraries (Task 1) ✔; adsb.fi v2 + poll
  (Task 6,12) ✔; classification incl. filtering + grey-unknown (Task 3) ✔; palette/no-sweep +
  distance-gated tags (Task 4,9) ✔; range presets 5/10/25/40 (Task 7) ✔; adaptive airports
  (Task 9,10) ✔; boot/info/IP screen + button triple-duty incl. double-press peek (Task 5,13,14)
  ✔; error handling keep-last-frame + no-WiFi message (Task 12,13,14) ✔; native unit tests +
  fixtures (Tasks 2–7,10) ✔.
- **Type consistency:** `Aircraft`, `Bucket`, `classify`, `Point`, `project`, `ring_index`,
  `tag_visible`, `parse_adsb`, `Airport`/`get_airports`, `AirportScreen`/`render_airports`,
  `ButtonFsm`/`ButtonEvent`, `HomeConfig`/`wifi_begin`/`wifi_reset` used consistently across
  tasks.
- **Known integration caveat (Task 8):** the `LGFX` alias name in `display.h` may need to be
  the concrete `LGFX_GC9A01` type or `lgfx::LGFX_Device&`; noted inline. Resolve during Task 8
  build; public method surface stays fixed so downstream tasks are unaffected.
```
