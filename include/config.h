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
#define BL_DEFAULT_DUTY 200
// ---- Behaviour ----
static const float RANGE_PRESETS_KM[] = { 5.0f, 10.0f, 25.0f, 40.0f };
static const int   RANGE_COUNT = 4;
#define POLL_INTERVAL_MS 5000
#define API_HOST "opendata.adsb.fi"
#define AP_NAME  "PlaneRadar-setup"
#define FW_VERSION "0.1.0"
// ---- Colours ----
// The offscreen canvas is an 8-bit PALETTE sprite (1 byte/pixel = 57,600 B) rather
// than 16-bit (115,200 B), because the WROOM-32 has no PSRAM and cannot allocate a
// single contiguous 115 KB block (largest free block ~114 KB). In palette mode the
// LovyanGFX drawing primitives take the palette INDEX as their colour argument, so
// COL_* are indices here; RADAR_PALETTE maps each index to its real RGB565 colour and
// is loaded into the sprite in Display::begin().
#define COL_FIELD   0   // navy    #03121f
#define COL_RING    1   // grid    #0a3a5c
#define COL_N       2   // blue    #7fd4ff
#define COL_COMMERC 3   // grey    #7f9bb0 — commercial + uncategorised (default bucket)
#define COL_VFR     4   // amber   #ffcf3b
#define COL_HELO    5   // green   #3bff8f
#define COL_VECTOR  6   // magenta #ff5be0
#define COL_AIRPORT 7   // cyan    #00b4c8

#include <cstdint>
static const uint16_t RADAR_PALETTE[] = {
    0x0862,  // 0 FIELD
    0x0A6B,  // 1 RING
    0x7EBF,  // 2 N
    0x8536,  // 3 COMMERC (grey — preferred for readability)
    0xFE67,  // 4 VFR
    0x3FF1,  // 5 HELO
    0xFAFC,  // 6 VECTOR
    0x05B9,  // 7 AIRPORT
};
static const int RADAR_PALETTE_COUNT = 8;
