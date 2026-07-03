#include <unity.h>
#include "model/units.h"

void test_km_to_nm() {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 13.4989f, km_to_nm(25.0f));
}
void test_nm_to_km() {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 25.0f, nm_to_km(13.4989f));
}
void test_format_altitude_ground() {
    TEST_ASSERT_EQUAL_STRING("GND", format_altitude_ft(0, true).c_str());
}
void test_format_altitude_thousands() {
    TEST_ASSERT_EQUAL_STRING("30k", format_altitude_ft(30000, false).c_str());
}
void test_format_altitude_low() {
    TEST_ASSERT_EQUAL_STRING("900", format_altitude_ft(900, false).c_str());
}
int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_km_to_nm);
    RUN_TEST(test_nm_to_km);
    RUN_TEST(test_format_altitude_ground);
    RUN_TEST(test_format_altitude_thousands);
    RUN_TEST(test_format_altitude_low);
    return UNITY_END();
}
