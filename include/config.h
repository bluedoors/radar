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
// ---- Colours (RGB565) ----
#define COL_FIELD   0x0862
#define COL_RING    0x0A6B
#define COL_N       0x7EBF
#define COL_COMMERC 0xF9E7
#define COL_VFR     0xFE67
#define COL_HELO    0x3FF1
#define COL_UNKNOWN 0x8536
#define COL_VECTOR  0xFAFC
#define COL_AIRPORT 0x05B9
