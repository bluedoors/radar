#pragma once
#include <string>
struct HomeConfig { bool valid=false; float lat=0, lon=0; std::string ssid; std::string ip; };
// Loads config from NVS; if none/failed, opens the captive portal (blocking) until set.
HomeConfig wifi_begin();
// Wipes stored WiFi + lat/lon and reboots into the portal.
void wifi_reset();
bool wifi_connected();
