#include "ui/radar_render.h"
#include "ui/projection.h"
#include "model/units.h"
#include "config.h"
#include <cmath>

static uint16_t bucket_colour(Bucket b) {
    switch (b) {
        case Bucket::VFR:        return COL_VFR;
        case Bucket::Helicopter: return COL_HELO;
        default:                 return COL_COMMERC;  // Commercial (grey) is the default
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

// Format a ring distance compactly: whole km (e.g. "8", "17", "25"); one decimal below 10
// only when it isn't already whole (e.g. "3.3" for a 10 km range's inner ring).
static void ring_label(char* buf, size_t n, float km) {
    if (km >= 10.0f || km == (float)(int)km) snprintf(buf, n, "%d", (int)(km + 0.5f));
    else snprintf(buf, n, "%.1f", km);
}

void render_radar(LGFX_Sprite* c, const std::vector<Aircraft>& aircraft, float range_km) {
    c->fillSprite(COL_FIELD);
    static const int ring_r[3] = { 38, 75, 112 };
    for (int r : ring_r) c->drawCircle(120,120,r, COL_RING);
    c->drawFastVLine(120, 8, 224, COL_RING);
    c->drawFastHLine(8, 120, 224, COL_RING);
    c->setTextColor(COL_N); c->drawString("N", 116, 4);
    c->fillCircle(120,120,2, COL_N);

    // Range label (km) on each ring along the NORTH axis: aircraft/airports near this
    // location are mostly to the south, so the top of the radar stays clear. Labels sit
    // just inside each ring, nudged a few px right of the vertical crosshair.
    c->setTextColor(COL_N);
    for (int i = 0; i < 3; ++i) {
        char lbl[8];
        ring_label(lbl, sizeof(lbl), range_km * (float)(i + 1) / 3.0f);
        c->drawString(lbl, 124, 120 - ring_r[i] + 3);   // just below/inside the ring, north side
    }

    for (const auto& a : aircraft) {
        Bucket b = classify(a);
        if (b == Bucket::Filtered) continue;
        float dst_km = nm_to_km(a.dst_nm);
        int ring = ring_index(dst_km, range_km);
        Point p = project(dst_km, a.dir_deg, range_km);
        uint16_t col = bucket_colour(b);
        if (ring == 3) { c->fillCircle(p.x, p.y, 2, col); continue; }
        draw_triangle(c, p, a.track, col);
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

void render_airports(LGFX_Sprite* c, const std::vector<AirportScreen>& aps, float range_km) {
    for (const auto& ap : aps) {
        int ring = ring_index(ap.dst_km, range_km);
        Point p = project(ap.dst_km, ap.dir_deg, range_km);
        if (ring == 3) {
            c->fillCircle(p.x, p.y, 2, COL_AIRPORT);
        } else {
            c->drawCircle(p.x, p.y, 5, COL_AIRPORT);
            c->drawLine(p.x-3, p.y+3, p.x+3, p.y-3, COL_AIRPORT);
        }
        c->setTextColor(COL_AIRPORT);
        c->drawString(ap.icao, p.x-10, p.y+7);
    }
}
