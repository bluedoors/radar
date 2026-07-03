# ESP32 Plane Radar — Design

**Date:** 2026-07-03
**Status:** Approved for planning
**Author:** brainstormed with Claude

## 1. Overview

A round "plane radar" that centers on a fixed home location (South Turramurra, NSW) and
displays live aircraft from a public ADS-B feed on a 1.28" round LCD. Aircraft render as
colour-coded triangles oriented along their track, with magenta speed vectors and
distance-gated callsign tags. Nearby airports (YSSY, YSBK) render as static cyan markers.
WiFi and location are configured via a captive portal; a single BOOT button cycles range
and reveals a connection-info screen.

Inspired by MatixYo/ESP32-Plane-Radar, but adapted to different hardware (ESP32-WROOM-32,
not ESP32-C3) and extended with aircraft classification by type and a boot-time info screen.

## 2. Hardware

- **MCU:** ESP32-WROOM-32 (ESP32-D0WD-V3, dual-core, 4 MB flash). Flash confirmed healthy
  and flashable (GigaDevice c4/6016, 4 MB) — see note in §9.
- **Display:** Waveshare 1.28" GC9A01 round LCD, 240×240, 4-wire SPI on hardware VSPI.
- **Input:** onboard BOOT button (GPIO0).
- **Toolchain:** PlatformIO, framework = arduino, board = esp32dev.

### Pin map (confirmed against physical board)

| Function        | GPIO | Notes                          |
|-----------------|------|--------------------------------|
| Display DIN/MOSI| 23   | VSPI MOSI (hardware SPI)        |
| Display CLK/SCLK| 18   | VSPI SCLK (hardware SPI)        |
| Display CS      | 15   |                                |
| Display DC      | 2    | strapping pin, driven post-boot|
| Display RST     | 4    |                                |
| Display BL      | 22   | PWM via LEDC (dimmable)         |
| Display VCC/GND | 3V3 / GND |                           |
| BOOT button     | 0    | short / double / long press    |

Constraints honored: nothing on GPIO 6–11 (SPI flash); no new load on strapping pins
beyond DC (GPIO2), which is safe because it's only driven after boot.

## 3. Libraries

- **LovyanGFX** — GC9A01 driver / rendering (configured for the VSPI pin map above).
- **WiFiManager** — captive-portal WiFi + custom lat/lon fields.
- **ArduinoJson** — streaming/filtered parse of the ADS-B response (RAM-conscious).
- **Preferences (NVS)** — persist WiFi creds + lat/lon.

## 4. Data Source

**opendata.adsb.fi**, free, no API key.

- **Endpoint (v2, confirmed live):**
  `GET https://opendata.adsb.fi/api/v2/lat/{lat}/lon/{lon}/dist/{NM}`
  where `{NM}` is the current range preset converted from km to **nautical miles**
  (the `dist` path segment is in NM, not km).
- **Poll interval:** every 5 s.
- **Fields consumed per aircraft:** `lat`, `lon`, `track`, `alt_baro`, `flight` (callsign),
  `gs` (ground speed), `category` (ADS-B emitter category), `t` (ICAO type), and the
  API-computed `dst` (distance, NM) and `dir` (bearing from center).

## 5. Aircraft Classification & Filtering

Each aircraft is sorted into one colour bucket. Rules applied in priority order:

| Bucket        | Rule                                              | Colour      |
|---------------|---------------------------------------------------|-------------|
| Filtered      | on ground (`alt_baro == "ground"`) or category `B*`/`C*` (surface/service) | not drawn |
| Helicopter    | category `A7`, or rotor ICAO type (EC45, AS50, R44, …) | green `#3bff8f` |
| Commercial    | category `A2`–`A5`                                | red `#ff3b3b`   |
| VFR / GA      | category `A1`, or light ICAO type (P28A, C172, SR22, …) | amber `#ffcf3b` |
| Unknown       | category `A0` or missing                          | grey-blue `#7f9bb0` |

Design decision: **unknown stays grey — no altitude/speed guessing.** Honest over wrong.
(~1 in 4 aircraft report no category.) Speed vector (magenta `#ff5be0`) drawn for all
drawn buckets.

## 6. Visual Design

Palette "A" (classic sonar), **no rotating sweep** (overkill on a 32.5 mm panel, occludes
traffic). Static concentric rings.

- **Field:** dark navy `#03121f`; **rings/grid:** `#0a3a5c`; N label + center dot: blue `#7fd4ff`.
- **Rings:** 3 concentric + N/S/E/W crosshairs. Rings redrawn only on range change.
- **Aircraft:** filled triangle pointing along `track`, coloured by bucket (§5), plus a
  magenta speed vector line scaled to `gs`.
- **Aircraft tags (distance-gated):** show `callsign` (+ altitude) **when the aircraft is
  inside the 2nd ring**; **icons only** when farther out (in the outer ring). This declutters
  automatically when the sky is busy. Tag inherits bucket colour.
- **Out-of-range aircraft:** small dot / tick on the rim at correct bearing (existing
  reference behavior).

### Range presets

BOOT short-press cycles: **5 → 10 → 25 → 40 km → 5**. The 40 km ring gives YSSY/YSBK
(both ~23 km from home) comfortable margin instead of clinging to the rim.

## 7. Airports

Airports are **not** in the ADS-B feed — they come from a **static embedded table**
generated at build time from the OurAirports public-domain dataset, filtered to
**large + medium airports with an ICAO code** within ~100 km of home. For South Turramurra
that is effectively **YSSY** (Sydney, ~23 km, bearing ~166°) and **YSBK** (Bankstown,
~23 km, bearing ~210°).

**Representation — adaptive (Option C):**
- Inside current range → cyan runway glyph (⊘) + ICAO code.
- Outside current range → cyan tick on the rim at correct bearing.

Same in/out-of-range logic as aircraft. Cyan (`#00b4c8`) keeps airports visually distinct
from red traffic.

## 8. Screens & Button Behavior

### Boot flow
1. Splash "starting…", backlight on (LEDC).
2. Load config from NVS. If unconfigured → **captive portal** (`PlaneRadar-setup` AP):
   enter WiFi creds + lat/lon; saved to NVS.
3. On WiFi connect → **Info screen**, held until BOOT is pressed:
   - IP address (large, centered — primary purpose)
   - SSID joined
   - configured lat/lon
   - firmware version + "Press BOOT to start radar"
   - If WiFi never connects: show "No WiFi — hold BOOT to reconfigure" (never a frozen splash).
4. **BOOT press → enter radar.** Range cycling begins here.

### Button (GPIO0) — triple duty
- **Short press (in radar):** cycle range 5→10→25→40.
- **Double press (in radar):** re-show Info screen for ~5 s, then return to radar
  (lets you check if the DHCP-assigned IP changed without rebooting).
- **Long press (~3 s):** wipe saved config (WiFi + lat/lon) and reopen captive portal.
- **Any press on Info screen:** dismiss → radar.

## 9. Data Flow & Timing

- Static rings drawn once; only redrawn on range change.
- Every 5 s: build URL with current range (km→NM) → HTTPS GET → filtered ArduinoJson parse
  → per aircraft: filter ground/surface, classify, capture track/alt/callsign/gs/dst/dir →
  project to screen for current range → redraw aircraft + airport layer.
- Brownout guard: init display after a short settle; keep backlight PWM moderate to limit
  inrush (learned from a flash-comms/power scare during bring-up — flash reads failed with
  the display's 3V3 connected under a marginal USB supply; resolved by cleaner power).

## 10. Error Handling

- **WiFi drop:** corner "⚠ WiFi" glyph, keep last frame dimmed, auto-reconnect; portal
  reopens after repeated auth failure.
- **API failure / timeout / non-200:** keep last good frame, subtle stale indicator, retry
  next cycle. Never blank on a single failed fetch.
- **Bad/empty JSON:** guarded parse; treat as "no update," never crash.
- **No aircraft:** clean radar (rings + airports). Normal, not an error.

## 11. Component Structure

```
src/main.cpp                   setup + loop coordinator, button, timing, screen state machine
include/config.h               pins, ranges {5,10,25,40}, colours, API host, intervals
include/hardware/display.h     LovyanGFX GC9A01 init for the VSPI pin map + LEDC backlight
include/hardware/button.h      BOOT: short / double / long press detection
include/net/wifi_setup.h       WiFiManager captive portal (WiFi + lat/lon), NVS persistence
include/services/adsb_client.h HTTPS GET v2 endpoint + filtered ArduinoJson parse
include/model/aircraft.h       Aircraft struct + classify() -> bucket enum (§5 logic)
include/data/airports.h        static YSSY/YSBK table (generated from OurAirports)
include/ui/screens.h           splash / info / radar screen renderers
include/ui/radar_display.h     draw rings/grid (once) + aircraft/airport layer (per refresh)
include/ui/projection.h        (lat,lon)|(dst,dir) + range -> screen x,y; ring/tag gating
```

Each unit has one responsibility and a narrow interface, so pure logic (classify,
projection, km→NM, tag/ring gating) can be unit-tested off-device.

## 12. Testing

- **Host-side unit tests** (PlatformIO `native` env, no hardware):
  - `classify()` against real category/type samples captured near Sydney.
  - km→NM conversion.
  - lat/lon (and dst/dir) → screen projection for each range preset.
  - in/out-of-ring gating + tag-visibility rule (inside 2nd ring ⇒ tag).
  - Fixtures: sample adsb.fi payloads captured 2026-07-03 near Sydney, saved for
    deterministic offline runs.
- **On-device smoke checks:** display init (rings render), captive portal reachable,
  one live fetch parses > 0 aircraft, button cycles range, info screen shows IP.

## 13. Out of Scope (YAGNI)

- No historical tracks / trails.
- No IP-geolocation auto-centering (explicit lat/lon only).
- No multi-source failover (adsb.fi only; swappable later via config host).
- No aircraft photos / external metadata lookups.
