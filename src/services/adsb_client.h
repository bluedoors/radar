#pragma once
#include <vector>
#include "model/aircraft.h"
// GET aircraft within range_km of (lat,lon). Returns empty on any network/HTTP failure.
std::vector<Aircraft> adsb_fetch(float lat, float lon, float range_km);
