#include <unity.h>
#include "model/geo.h"
void test_distance_home_to_yssy() {
    float d = haversine_km(-33.745f,151.115f, -33.9461f,151.1772f);
    TEST_ASSERT_FLOAT_WITHIN(1.5f, 23.1f, d);
}
void test_bearing_home_to_yssy() {
    float b = bearing_deg(-33.745f,151.115f, -33.9461f,151.1772f);
    TEST_ASSERT_FLOAT_WITHIN(8.0f, 166.0f, b);
}
int main(int,char**){ UNITY_BEGIN(); RUN_TEST(test_distance_home_to_yssy); RUN_TEST(test_bearing_home_to_yssy); return UNITY_END(); }
