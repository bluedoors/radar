#include "ui/projection.h"
#include <cmath>

static constexpr int CENTER = 120;
static constexpr float MAX_R = 112.0f;

Point project(float dst_km, float dir_deg, float range_km) {
    float frac = (range_km <= 0) ? 0 : dst_km / range_km;
    if (frac > 1.0f) frac = 1.0f;
    float r = frac * MAX_R;
    float a = dir_deg * (float)M_PI / 180.0f;
    int x = (int)lroundf(CENTER + r * sinf(a));
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
