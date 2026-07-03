#include "services/adsb_parse.h"
#include <ArduinoJson.h>

static std::string trim(std::string s) {
    while (!s.empty() && (s.back()==' '||s.back()=='\0')) s.pop_back();
    return s;
}

std::vector<Aircraft> parse_adsb(const std::string& body) {
    std::vector<Aircraft> out;
    JsonDocument doc;
    if (deserializeJson(doc, body)) return out;   // parse error -> empty
    JsonArray arr = doc["aircraft"].as<JsonArray>();
    if (arr.isNull()) return out;
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
