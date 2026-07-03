#pragma once
#include <string>
float km_to_nm(float km);
float nm_to_km(float nm);
// ground=true -> "GND". Else >=10000ft -> "Nk" (integer thousands),
// otherwise the raw foot value as a string.
std::string format_altitude_ft(int feet, bool ground);
