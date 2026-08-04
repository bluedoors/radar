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
    // Stream-parse off the socket. Buffering the body first (getString() + a std::string
    // copy) needed ~60 KB of transient heap for a ~30 KB response, which overflowed the
    // largest free block (~41 KB after the TLS buffers fragment the heap) and aborted
    // with bad_alloc every poll. parse_adsb also filters to the ~10 keys we render.
    auto out = parse_adsb(http.getStream());
    http.end();
    return out;
}
