#include <unity.h>
#include "data/airports.h"

void test_has_yssy_and_ysbk() {
    auto v = get_airports();
    bool yssy=false, ysbk=false;
    for (auto& a : v) { if (a.icao=="YSSY") yssy=true; if (a.icao=="YSBK") ysbk=true; }
    TEST_ASSERT_TRUE(yssy); TEST_ASSERT_TRUE(ysbk);
}
void test_yssy_coords_plausible() {
    for (auto& a : get_airports()) if (a.icao=="YSSY") {
        TEST_ASSERT_FLOAT_WITHIN(0.05f, -33.9461f, a.lat);
        TEST_ASSERT_FLOAT_WITHIN(0.05f, 151.1772f, a.lon);
    }
}
int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_has_yssy_and_ysbk); RUN_TEST(test_yssy_coords_plausible);
    return UNITY_END();
}
