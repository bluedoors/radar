#include <unity.h>
#include "hardware/button.h"

void test_short_press() {
    ButtonFsm b;
    TEST_ASSERT_EQUAL(ButtonEvent::None, b.update(true, 0));
    TEST_ASSERT_EQUAL(ButtonEvent::None, b.update(false, 100));
    TEST_ASSERT_EQUAL(ButtonEvent::Short, b.update(false, 600));
}
void test_long_press() {
    ButtonFsm b;
    b.update(true, 0);
    TEST_ASSERT_EQUAL(ButtonEvent::Long, b.update(true, 1500));
}
void test_double_press() {
    ButtonFsm b;
    b.update(true, 0);  b.update(false, 80);
    b.update(true, 150);
    TEST_ASSERT_EQUAL(ButtonEvent::Double, b.update(false, 220));
}
int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_short_press); RUN_TEST(test_long_press); RUN_TEST(test_double_press);
    return UNITY_END();
}
