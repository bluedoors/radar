#pragma once
#include <string>
#include <vector>
#include "model/aircraft.h"
// Parse an adsb.fi v2 response body into aircraft. Callsign is trimmed.
// Malformed/empty input returns an empty vector (never throws).
std::vector<Aircraft> parse_adsb(const std::string& body);

#ifndef UNIT_TEST
#include <Arduino.h>
// Stream overload: parses directly off the HTTP socket so the ~30 KB body is never
// held in RAM as a whole. Preferred on-device — see adsb_client.cpp.
std::vector<Aircraft> parse_adsb(Stream& s);
#endif
