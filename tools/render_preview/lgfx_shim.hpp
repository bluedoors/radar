// Host-side stand-in for the subset of LovyanGFX that radar_render.cpp uses.
//
// The point is that the REAL renderer compiles against this unchanged, so the preview
// image is pixel-accurate rather than an artist's impression. Like the device, the buffer
// is 8-bit palette-indexed (1 byte/pixel) and the colour arguments are palette indices.
//
// Preview tooling only — not compiled into the firmware.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>
#include "font5x7.hpp"

class LGFX_Sprite {
public:
    static constexpr int W = 240, H = 240;
    std::vector<uint8_t> px = std::vector<uint8_t>(W * H, 0);

    void fillSprite(uint8_t c) { std::fill(px.begin(), px.end(), c); }
    void setTextColor(uint8_t c) { text_col_ = c; }

    void drawPixel(int x, int y, uint8_t c) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        px[(size_t)y * W + x] = c;
    }

    void drawFastVLine(int x, int y, int h, uint8_t c) {
        for (int i = 0; i < h; ++i) drawPixel(x, y + i, c);
    }
    void drawFastHLine(int x, int y, int w, uint8_t c) {
        for (int i = 0; i < w; ++i) drawPixel(x + i, y, c);
    }

    // Midpoint circle, matching LovyanGFX's 1px outline.
    void drawCircle(int cx, int cy, int r, uint8_t c) {
        int x = r, y = 0, err = 1 - r;
        while (x >= y) {
            drawPixel(cx + x, cy + y, c); drawPixel(cx + y, cy + x, c);
            drawPixel(cx - y, cy + x, c); drawPixel(cx - x, cy + y, c);
            drawPixel(cx - x, cy - y, c); drawPixel(cx - y, cy - x, c);
            drawPixel(cx + y, cy - x, c); drawPixel(cx + x, cy - y, c);
            ++y;
            if (err < 0) err += 2 * y + 1;
            else { --x; err += 2 * (y - x) + 1; }
        }
    }

    void fillCircle(int cx, int cy, int r, uint8_t c) {
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                if (dx * dx + dy * dy <= r * r) drawPixel(cx + dx, cy + dy, c);
    }

    void drawLine(int x0, int y0, int x1, int y1, uint8_t c) {
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            drawPixel(x0, y0, c);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    void fillTriangle(int x0,int y0,int x1,int y1,int x2,int y2, uint8_t c) {
        int minx = std::min({x0,x1,x2}), maxx = std::max({x0,x1,x2});
        int miny = std::min({y0,y1,y2}), maxy = std::max({y0,y1,y2});
        auto edge = [](int ax,int ay,int bx,int by,int cx,int cy){
            return (bx-ax)*(cy-ay) - (by-ay)*(cx-ax);
        };
        int area = edge(x0,y0,x1,y1,x2,y2);
        if (area == 0) return;
        for (int y = miny; y <= maxy; ++y) {
            for (int x = minx; x <= maxx; ++x) {
                int w0 = edge(x1,y1,x2,y2,x,y);
                int w1 = edge(x2,y2,x0,y0,x,y);
                int w2 = edge(x0,y0,x1,y1,x,y);
                bool inside = area > 0 ? (w0 >= 0 && w1 >= 0 && w2 >= 0)
                                       : (w0 <= 0 && w1 <= 0 && w2 <= 0);
                if (inside) drawPixel(x, y, c);
            }
        }
    }

    // LovyanGFX's default GLCD font is 6x8 on a 5x7 glyph grid, drawn with (x,y) as the
    // top-left corner — matching that keeps label placement faithful to the panel.
    void drawString(const char* s, int x, int y) {
        int cx = x;
        for (const char* p = s; *p; ++p) {
            const uint8_t* g = font5x7_glyph(*p);
            for (int col = 0; col < 5; ++col)
                for (int row = 0; row < 7; ++row)
                    if (g[col] & (1u << row)) drawPixel(cx + col, y + row, text_col_);
            cx += 6;
        }
    }
    void drawString(const std::string& s, int x, int y) { drawString(s.c_str(), x, y); }

private:
    uint8_t text_col_ = 1;
};
