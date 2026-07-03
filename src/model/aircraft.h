#pragma once
#include <string>
enum class Bucket { Commercial, VFR, Helicopter, Unknown, Filtered };
struct Aircraft {
    float lat = 0, lon = 0;
    float track = 0;
    int alt_ft = 0;
    bool on_ground = false;
    float gs_knots = 0;
    float dst_nm = 0;
    float dir_deg = 0;
    std::string callsign;
    std::string category;
    std::string type;
};
Bucket classify(const Aircraft& a);
