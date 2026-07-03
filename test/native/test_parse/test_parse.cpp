#include <unity.h>
#include <fstream>
#include <sstream>
#include "services/adsb_parse.h"

static std::string load() {
    std::ifstream f("test/fixtures/sydney_sample.json");
    std::stringstream ss; ss << f.rdbuf(); return ss.str();
}
void test_parses_all_aircraft() {
    auto v = parse_adsb(load());
    TEST_ASSERT_EQUAL(6, (int)v.size());
}
void test_first_has_position_or_ground() {
    auto v = parse_adsb(load());
    TEST_ASSERT_TRUE(v[0].callsign.size() >= 0);
}
void test_ground_flag_detected() {
    auto v = parse_adsb(R"({"aircraft":[{"flight":"X","alt_baro":"ground","lat":1,"lon":2}]})");
    TEST_ASSERT_EQUAL(1, (int)v.size());
    TEST_ASSERT_TRUE(v[0].on_ground);
}
void test_numeric_alt_parsed() {
    auto v = parse_adsb(R"({"aircraft":[{"flight":"Y","alt_baro":30650,"track":124.1,"gs":300.9,"category":"A3","t":"B738","lat":1,"lon":2,"dst":21.8,"dir":312.7}]})");
    TEST_ASSERT_EQUAL(30650, v[0].alt_ft);
    TEST_ASSERT_FALSE(v[0].on_ground);
    TEST_ASSERT_EQUAL_STRING("A3", v[0].category.c_str());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 21.8f, v[0].dst_nm);
}
void test_empty_or_garbage_safe() {
    TEST_ASSERT_EQUAL(0, (int)parse_adsb("").size());
    TEST_ASSERT_EQUAL(0, (int)parse_adsb("not json").size());
    TEST_ASSERT_EQUAL(0, (int)parse_adsb("{}").size());
}
int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_parses_all_aircraft); RUN_TEST(test_first_has_position_or_ground);
    RUN_TEST(test_ground_flag_detected); RUN_TEST(test_numeric_alt_parsed);
    RUN_TEST(test_empty_or_garbage_safe);
    return UNITY_END();
}
