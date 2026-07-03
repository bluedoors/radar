#ifndef UNIT_TEST
#include <Arduino.h>
#include "hardware/display.h"
#include "config.h"

void setup() {
    Serial.begin(115200);
    display.begin();
    auto* c = display.canvas();
    c->fillSprite(COL_FIELD);
    c->fillCircle(120, 120, 112, COL_RING);
    c->fillCircle(120, 120, 106, COL_FIELD);
    c->setTextColor(COL_N);
    c->drawString("RADAR", 96, 116);
    display.push();
}

void loop() {}
#else
// Native / unit-test build: provide a no-op entry point so the
// linker is satisfied when running `pio run -e native`.
// Actual tests live in test/ and are driven by `pio test`.
int main() { return 0; }
#endif
