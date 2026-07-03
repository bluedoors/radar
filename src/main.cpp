#ifndef UNIT_TEST
#include <Arduino.h>
void setup() { Serial.begin(115200); Serial.println("radar boot"); }
void loop() {}
#else
// Native / unit-test build: provide a no-op entry point so the
// linker is satisfied when running `pio run -e native`.
// Actual tests live in test/ and are driven by `pio test`.
int main() { return 0; }
#endif
