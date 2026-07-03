#include "services/adsb_client.h"
#include "services/adsb_parse.h"
#include "model/units.h"
#include "config.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

std::vector<Aircraft> adsb_fetch(float lat, float lon, float range_km) {
    float nm = km_to_nm(range_km);
    char url[160];
    snprintf(url, sizeof(url), "https://%s/api/v2/lat/%.5f/lon/%.5f/dist/%d",
             API_HOST, lat, lon, (int)(nm + 0.5f));
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, url)) return {};
    http.setTimeout(8000);
    int code = http.GET();
    if (code != 200) { http.end(); return {}; }
    std::string body = std::string(http.getString().c_str());
    http.end();
    return parse_adsb(body);
}
