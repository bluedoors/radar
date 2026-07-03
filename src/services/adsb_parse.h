#pragma once
#include <string>
#include <vector>
#include "model/aircraft.h"
// Parse an adsb.fi v2 response body into aircraft. Callsign is trimmed.
// Malformed/empty input returns an empty vector (never throws).
std::vector<Aircraft> parse_adsb(const std::string& body);
