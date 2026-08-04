#include "services/adsb_parse.h"
#include <ArduinoJson.h>

static std::string trim(std::string s) {
    while (!s.empty() && (s.back()==' '||s.back()=='\0')) s.pop_back();
    return s;
}

// The response carries ~28 keys per aircraft; we render 10. A filter makes ArduinoJson
// skip the rest without allocating for them, which on a WROOM-32 is the difference
// between fitting in the largest free block and throwing bad_alloc.
static JsonDocument adsb_filter() {
    JsonDocument f;
    // For arrays the filter's element 0 is the template applied to every element.
    JsonObject keep = f["aircraft"][0].to<JsonObject>();
    keep["flight"]    = true;
    keep["category"]  = true;
    keep["t"]         = true;
    keep["lat"]       = true;
    keep["lon"]       = true;
    keep["track"]     = true;
    keep["gs"]        = true;
    keep["dst"]       = true;
    keep["dir"]       = true;
    keep["alt_baro"]  = true;
    return f;
}

// Shared field extraction, so the string and stream entry points can't drift apart.
static std::vector<Aircraft> extract(JsonDocument& doc) {
    std::vector<Aircraft> out;
    JsonArray arr = doc["aircraft"].as<JsonArray>();
    if (arr.isNull()) return out;
    out.reserve(arr.size());   // one allocation instead of log2(n) reallocs
    for (JsonObject o : arr) {
        Aircraft a;
        a.callsign = trim(std::string(o["flight"] | ""));
        a.category = std::string(o["category"] | "");
        a.type     = std::string(o["t"] | "");
        a.lat = o["lat"] | 0.0f;
        a.lon = o["lon"] | 0.0f;
        a.track = o["track"] | 0.0f;
        a.gs_knots = o["gs"] | 0.0f;
        a.dst_nm = o["dst"] | 0.0f;
        a.dir_deg = o["dir"] | 0.0f;
        JsonVariant alt = o["alt_baro"];
        if (alt.is<const char*>()) {
            a.on_ground = (std::string(alt.as<const char*>()) == "ground");
            a.alt_ft = 0;
        } else {
            a.alt_ft = alt | 0;
            a.on_ground = false;
        }
        out.push_back(a);
    }
    return out;
}

std::vector<Aircraft> parse_adsb(const std::string& body) {
    JsonDocument doc;
    if (deserializeJson(doc, body, DeserializationOption::Filter(adsb_filter()))) return {};
    return extract(doc);
}

#ifndef UNIT_TEST
std::vector<Aircraft> parse_adsb(Stream& s) {
    JsonDocument doc;
    // Parsing straight off the socket avoids materialising the ~30 KB body at all.
    if (deserializeJson(doc, s, DeserializationOption::Filter(adsb_filter()))) return {};
    return extract(doc);
}
#endif
