#pragma once
#include <string>
#include <functional>
struct HomeConfig { bool valid=false; float lat=0, lon=0; std::string ssid; std::string ip; };
// Called when the captive portal opens, with (ap_name, portal_ip), so the caller can
// show setup instructions on the display. Optional.
using PortalCallback = std::function<void(const std::string&, const std::string&)>;
// Loads config from NVS; if none/failed, opens the captive portal (blocking) until set.
HomeConfig wifi_begin(PortalCallback on_portal = nullptr);
// Wipes stored WiFi + lat/lon and reboots into the portal.
void wifi_reset();
