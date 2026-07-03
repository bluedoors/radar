# ESP32 Plane Radar

A round-LCD "plane radar" for the ESP32-WROOM-32 + Waveshare 1.28" GC9A01 display. It
centers on your configured location and shows live aircraft (from the free
[adsb.fi](https://opendata.adsb.fi) ADS-B feed) as colour-coded triangles with heading and
speed vectors, plus nearby airports as static markers.

Inspired by [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar),
adapted to the WROOM-32 (dual-core ESP32) and extended with aircraft classification by type
and a boot-time connection-info screen.

## Features

- Live aircraft within a selectable range, polled every 5 s.
- **Colour by aircraft class:** commercial (red), VFR/GA (amber), helicopter (green),
  unknown (grey — never guessed).
- Heading triangle + magenta ground-speed vector. Distance-gated callsign+altitude tags
  (shown only inside the 2nd ring, so a busy sky stays readable).
- Adaptive airport markers (runway glyph inside range, rim tick outside). Ships with YSSY
  (Sydney) and YSBK (Bankstown).
- Range presets cycled by the BOOT button: **5 / 10 / 25 / 40 km**.
- WiFi + location via a captive portal — no recompiling to move it.
- Boot-time info screen showing the DHCP-assigned IP (glanceable, re-checkable any time).

## Hardware & Wiring

ESP32-WROOM-32 + Waveshare 1.28" GC9A01 (240×240 round SPI LCD).

| Display pin | ESP32 GPIO | Notes                    |
|-------------|-----------:|--------------------------|
| VCC         | 3V3        |                          |
| GND         | GND        |                          |
| DIN (MOSI)  | 23         | hardware VSPI MOSI       |
| CLK (SCLK)  | 18         | hardware VSPI SCLK       |
| CS          | 15         |                          |
| DC          | 2          |                          |
| RST         | 4          |                          |
| BL          | 22         | PWM backlight (LEDC)     |
| —           | BOOT (0)   | onboard button           |

Pins are defined in [`include/config.h`](include/config.h).

> **Power note:** During bring-up, reading the SPI flash failed while the display's 3V3 was
> connected under a marginal USB supply (the flash browned out during init). Flash from a
> solid USB port / powered hub / good data cable. If `esptool flash-id` ever returns
> manufacturer `00`/`ff`, suspect power before the chip.

## Build & Flash

Requires [PlatformIO](https://platformio.org/).

```bash
# Build firmware
pio run -e esp32dev

# Flash (board on /dev/cu.usbserial-0001 — edit upload_port in platformio.ini if different)
pio run -e esp32dev -t upload

# Serial monitor
pio device monitor -b 115200
```

## Tests

Pure logic (classification, projection, unit conversion, geo math, button FSM, JSON parsing)
is unit-tested off-device on the host:

```bash
pio test -e native
```

38 test cases across 7 suites. No hardware required.

## First-Run Setup

1. Flash and power the board. A **"PLANE RADAR — starting…"** splash appears.
2. On first boot the ESP32 opens a WiFi hotspot **`PlaneRadar-setup`**. Connect with your
   phone; a captive portal opens.
3. Enter your WiFi credentials and your **Latitude / Longitude** (the radar's center point).
   Saved to NVS — you won't be asked again.
4. The **Info screen** shows the assigned IP, SSID, and your coordinates.
5. Press **BOOT** to enter the radar.

## Button Gestures (BOOT / GPIO0)

| Gesture       | In Radar                          | On Info screen        |
|---------------|-----------------------------------|-----------------------|
| Short press   | Cycle range 5 → 10 → 25 → 40 km   | Start radar           |
| Double press  | Peek Info screen (auto-returns 5 s) | Start radar         |
| Long press (~1.5 s) | Wipe config + reopen captive portal | (same)          |

## Display Legend

| Symbol | Meaning |
|--------|---------|
| 🔺 red | Commercial aircraft (ADS-B category A2–A5) |
| 🔺 amber | VFR / general aviation (A1 or light type) |
| 🔺 green | Helicopter (A7 or rotor type) |
| 🔺 grey | Unknown class (no category reported) |
| magenta line | Ground-speed vector |
| cyan ⊘ / tick | Airport (glyph inside range, rim tick outside) |
| blue dot | You (center) |

Ground vehicles and surface objects are filtered out.

## Project Layout

```
include/config.h            pins, ranges, colours, API host, timings
src/model/                  aircraft+classify, units, geo (pure, host-tested)
src/ui/projection.*         coordinate projection + ring/tag gating (pure, host-tested)
src/data/airports.*         static airport table
src/services/adsb_parse.*   JSON parser (pure, host-tested)
src/services/adsb_client.*  HTTPS fetch (device)
src/hardware/               display driver, button FSM
src/net/wifi_setup.*        captive portal + NVS
src/ui/radar_render.*       rings / aircraft / airport rendering
src/ui/screens.*            splash / info screens
src/main.cpp                splash → info → radar state machine
test/native/                Unity unit tests
docs/superpowers/           design spec + implementation plan
```

## Changing the Airport List

Airports are a static table in [`src/data/airports.cpp`](src/data/airports.cpp)
(large/medium ICAO airports near home). To cover a different area, regenerate it from the
[OurAirports](https://ourairports.com/data/) dataset — filter to `large_airport`/
`medium_airport` with an `ident` within range of your location.
