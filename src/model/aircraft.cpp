#include "model/aircraft.h"
#include <array>

static bool is_rotor_type(const std::string& t) {
    static const std::array<const char*,6> rotors{"EC45","AS50","R44","R22","H60","B06"};
    for (auto r : rotors) if (t == r) return true;
    return false;
}
static bool is_light_type(const std::string& t) {
    static const std::array<const char*,6> light{"P28A","C172","SR22","C152","DA40","PA28"};
    for (auto l : light) if (t == l) return true;
    return false;
}

Bucket classify(const Aircraft& a) {
    if (a.on_ground) return Bucket::Filtered;
    const std::string& c = a.category;
    if (!c.empty() && (c[0] == 'B' || c[0] == 'C')) return Bucket::Filtered;
    if (c == "A7" || is_rotor_type(a.type)) return Bucket::Helicopter;
    if (c == "A1" || is_light_type(a.type)) return Bucket::VFR;
    // Everything else — A2-A5 AND aircraft with no/unknown category — is Commercial.
    // Near a major hub, uncategorised traffic is almost always commercial.
    return Bucket::Commercial;
}
