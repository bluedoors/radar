#pragma once
#include <string>
#include <vector>
struct Airport { std::string icao; float lat; float lon; };
const std::vector<Airport>& get_airports();
