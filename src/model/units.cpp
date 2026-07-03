#include "model/units.h"
float km_to_nm(float km) { return km / 1.852f; }
float nm_to_km(float nm) { return nm * 1.852f; }
std::string format_altitude_ft(int feet, bool ground) {
    if (ground) return "GND";
    if (feet >= 10000) return std::to_string(feet / 1000) + "k";
    return std::to_string(feet);
}
