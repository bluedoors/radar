#include <unity.h>
#include "model/aircraft.h"

static Aircraft mk(const char* cat, const char* type, bool ground) {
    Aircraft a; a.category = cat; a.type = type; a.on_ground = ground; return a;
}
void test_commercial_a3() { TEST_ASSERT_EQUAL(Bucket::Commercial, classify(mk("A3","B738",false))); }
void test_commercial_a5() { TEST_ASSERT_EQUAL(Bucket::Commercial, classify(mk("A5","B77L",false))); }
void test_vfr_a1()        { TEST_ASSERT_EQUAL(Bucket::VFR,        classify(mk("A1","C510",false))); }
void test_helo_a7()       { TEST_ASSERT_EQUAL(Bucket::Helicopter, classify(mk("A7","EC45",false))); }
void test_helo_by_type()  { TEST_ASSERT_EQUAL(Bucket::Helicopter, classify(mk("","R44",false))); }
void test_vfr_by_type()   { TEST_ASSERT_EQUAL(Bucket::VFR,        classify(mk("","P28A",false))); }
void test_unknown_a0()    { TEST_ASSERT_EQUAL(Bucket::Unknown,    classify(mk("A0","",false))); }
void test_unknown_blank() { TEST_ASSERT_EQUAL(Bucket::Unknown,    classify(mk("","",false))); }
void test_filtered_ground(){TEST_ASSERT_EQUAL(Bucket::Filtered,   classify(mk("A3","B738",true))); }
void test_filtered_c2()   { TEST_ASSERT_EQUAL(Bucket::Filtered,   classify(mk("C2","",false))); }
int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_commercial_a3); RUN_TEST(test_commercial_a5);
    RUN_TEST(test_vfr_a1); RUN_TEST(test_helo_a7); RUN_TEST(test_helo_by_type);
    RUN_TEST(test_vfr_by_type); RUN_TEST(test_unknown_a0); RUN_TEST(test_unknown_blank);
    RUN_TEST(test_filtered_ground); RUN_TEST(test_filtered_c2);
    return UNITY_END();
}
