// Shadows the real LovyanGFX header when building the host preview, so
// src/ui/radar_render.h can be included unmodified. Points at the shim instead.
//
// Preview tooling only — never on the include path for the firmware build.
#pragma once
#include "lgfx_shim.hpp"
