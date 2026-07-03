#include "hardware/display.h"
#include "config.h"

// LovyanGFX GC9A01 config for the VSPI pin map.
class LGFX_GC9A01 : public lgfx::LGFX_Device {
    lgfx::Panel_GC9A01 _panel;
    lgfx::Bus_SPI      _bus;
public:
    LGFX_GC9A01() {
        { auto c = _bus.config();
          c.spi_host   = VSPI_HOST;
          c.spi_mode   = 0;
          c.freq_write = 40000000;
          c.freq_read  = 16000000;
          c.pin_sclk   = PIN_SCLK;
          c.pin_mosi   = PIN_MOSI;
          c.pin_miso   = -1;
          c.pin_dc     = PIN_DC;
          _bus.config(c);
          _panel.setBus(&_bus); }
        { auto c = _panel.config();
          c.pin_cs     = PIN_CS;
          c.pin_rst    = PIN_RST;
          c.pin_busy   = -1;
          c.panel_width  = 240;
          c.panel_height = 240;
          c.offset_x   = 0;
          c.offset_y   = 0;
          c.readable   = false;
          c.invert     = true;
          c.rgb_order  = false;
          c.dlen_16bit = false;
          c.bus_shared = false;
          _panel.config(c); }
        setPanel(&_panel);
    }
};

static LGFX_GC9A01  g_lgfx;
static LGFX_Sprite  g_canvas(&g_lgfx);

Display display;

void Display::begin() {
    g_lgfx.init();
    g_lgfx.setRotation(0);
    ledcSetup(BL_LEDC_CHANNEL, BL_LEDC_FREQ, BL_LEDC_RES);
    ledcAttachPin(PIN_BL, BL_LEDC_CHANNEL);
    set_backlight(BL_DEFAULT_DUTY);
    // 8-bit palette canvas: 240x240x1 = 57,600 B (a 16-bit canvas needs 115,200 B,
    // which will not fit in the WROOM-32's largest contiguous free block ~114 KB).
    g_canvas.setColorDepth(lgfx::color_depth_t::palette_8bit);
    g_canvas.createSprite(240, 240);
    for (int i = 0; i < RADAR_PALETTE_COUNT; ++i) {
        // Wrap in rgb565_t so LovyanGFX converts from 565 (a bare uint16_t would be
        // misread as a 24-bit value and come out near-black).
        g_canvas.setPaletteColor(i, lgfx::rgb565_t((uint16_t)RADAR_PALETTE[i]));
    }
}

void Display::set_backlight(uint8_t duty) {
    ledcWrite(BL_LEDC_CHANNEL, duty);
}

LGFX_Sprite* Display::canvas() {
    return &g_canvas;
}

void Display::push() {
    g_canvas.pushSprite(&g_lgfx, 0, 0);
}
