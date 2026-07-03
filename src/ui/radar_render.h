#pragma once
#include <vector>
#include <LovyanGFX.hpp>
#include "model/aircraft.h"
// Renders one full frame: field, rings, then classified aircraft.
void render_radar(LGFX_Sprite* c, const std::vector<Aircraft>& aircraft, float range_km);
// Airport screen-projection input (dst/bearing precomputed from home).
struct AirportScreen { const char* icao; float dst_km; float dir_deg; };
void render_airports(LGFX_Sprite* c, const std::vector<AirportScreen>& aps, float range_km);
