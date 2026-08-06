// Renders one radar frame on the host and writes a PPM to stdout, using the REAL
// render_radar/render_airports from src/ui/ — so the README image matches the panel
// rather than approximating it.
//
// Build/run via tools/render_preview/render.sh. Preview tooling only.
#include "lgfx_shim.hpp"
#include "ui/radar_render.h"
#include "ui/projection.h"
#include "model/aircraft.h"
#include "model/geo.h"
#include "data/airports.h"
#include "config.h"
#include <cstdio>

// A plausible Sydney-area frame at the 10 km default range. dst_nm/dir_deg are what the
// feed supplies, so these go through exactly the same projection path as live data.
// Kept deliberately sparse — a real busy sky is legible on the panel but turns into
// overlapping tags in a scaled-down README image. Still covers every legend entry:
// commercial, VFR, helicopter, one beyond range (dot, no tag) and one on the ground
// (filtered out).
static std::vector<Aircraft> sample_frame() {
    std::vector<Aircraft> v;
    auto add = [&](const char* cs, const char* cat, const char* type,
                   float dst_nm, float dir, float track, int alt, float gs) {
        Aircraft a;
        a.callsign = cs; a.category = cat; a.type = type;
        a.dst_nm = dst_nm; a.dir_deg = dir; a.track = track;
        a.alt_ft = alt; a.gs_knots = gs; a.on_ground = false;
        v.push_back(a);
    };
    // Commercial (grey) — one inbound on the approach, one climbing out.
    add("QFA412", "A3", "B738", 1.6f, 112.0f, 351.0f,  2400, 190.0f);
    add("QFA1523","A2", "E190", 3.4f, 289.0f, 118.0f, 12500, 310.0f);
    // VFR / GA (amber) — light type west of the centre.
    add("VHDQP",  "A1", "C172", 2.4f, 224.0f,  74.0f,  1800,  95.0f);
    // Helicopter (green) — harbour scenic, close in so it carries a tag.
    add("VHHLI",  "A7", "EC45", 1.1f,  48.0f, 258.0f,  1200,  85.0f);
    // Beyond the 10 km range: renders as a dot in the outer band, no tag.
    add("ANZ108", "A3", "B789", 7.8f, 168.0f, 358.0f, 27000, 420.0f);
    // Ground traffic — filtered out entirely, proves the filter is live.
    Aircraft g; g.callsign = "QFA9"; g.category = "A3"; g.on_ground = true;
    g.dst_nm = 0.9f; g.dir_deg = 150.0f; v.push_back(g);
    return v;
}

// The wide view needs its own traffic: the close-in set above all sits within ~8 nm, so at
// 40 km it collapses into an unreadable knot at the centre. A real 40 km sky is dominated by
// en-route aircraft further out, which is what this shows.
static std::vector<Aircraft> wide_frame() {
    std::vector<Aircraft> v;
    auto add = [&](const char* cs, const char* cat, const char* type,
                   float dst_nm, float dir, float track, int alt, float gs) {
        Aircraft a;
        a.callsign = cs; a.category = cat; a.type = type;
        a.dst_nm = dst_nm; a.dir_deg = dir; a.track = track;
        a.alt_ft = alt; a.gs_knots = gs; a.on_ground = false;
        v.push_back(a);
    };
    add("JST508", "A3", "A321", 11.8f,  38.0f, 205.0f, 14200, 300.0f);
    add("QFA1523","A2", "E190",  9.4f, 292.0f, 118.0f, 12500, 310.0f);
    // Uncategorised — deliberately falls into the commercial bucket, not "unknown".
    add("UAE413", "",   "",     16.4f, 142.0f, 300.0f, 24000, 350.0f);
    add("VHZKL",  "A1", "PA28",  8.2f, 214.0f, 160.0f,  2600, 110.0f);
    add("ANZ108", "A3", "B789", 24.6f, 189.0f, 358.0f, 29000, 430.0f);  // beyond range -> dot
    return v;
}

int main(int argc, char** argv) {
    float range_km = (argc > 1) ? (float)atof(argv[1]) : RANGE_PRESETS_KM[1];  // default 10 km

    // Airports as main.cpp computes them: real haversine/bearing from a home location.
    // Chosen so the frame shows both airport marker styles: at 10 km YSSY (4.7 km) draws
    // its runway glyph inside the field while YSBK (15 km) degrades to a rim tick; at
    // 40 km both fall inside.
    const float home_lat = -33.9100f, home_lon = 151.1500f;   // Sydney, north of YSSY
    std::vector<AirportScreen> aps;
    for (auto& a : get_airports()) {
        aps.push_back({ a.icao.c_str(),
                        haversine_km(home_lat, home_lon, a.lat, a.lon),
                        bearing_deg(home_lat, home_lon, a.lat, a.lon) });
    }

    LGFX_Sprite canvas;
    auto frame = (range_km > 20.0f) ? wide_frame() : sample_frame();
    render_radar(&canvas, frame, range_km);
    render_airports(&canvas, aps, range_km);

    // Binary PPM (P6): trivially convertible, no image library needed on the C++ side.
    printf("P6\n%d %d\n255\n", LGFX_Sprite::W, LGFX_Sprite::H);
    for (int i = 0; i < LGFX_Sprite::W * LGFX_Sprite::H; ++i) {
        uint8_t idx = canvas.px[i];
        uint16_t rgb565 = RADAR_PALETTE[idx < RADAR_PALETTE_COUNT ? idx : 0];
        // RGB565 -> RGB888 with the usual bit replication so full-scale stays full-scale.
        uint8_t r = (uint8_t)((rgb565 >> 11) & 0x1F), g = (uint8_t)((rgb565 >> 5) & 0x3F),
                b = (uint8_t)(rgb565 & 0x1F);
        uint8_t out[3] = { (uint8_t)((r << 3) | (r >> 2)),
                           (uint8_t)((g << 2) | (g >> 4)),
                           (uint8_t)((b << 3) | (b >> 2)) };
        fwrite(out, 1, 3, stdout);
    }
    return 0;
}
