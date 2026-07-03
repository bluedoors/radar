#include <unity.h>
#include "ui/projection.h"

void test_center_when_zero_dist() {
    Point p = project(0.0f, 0.0f, 25.0f);
    TEST_ASSERT_INT_WITHIN(1, 120, p.x);
    TEST_ASSERT_INT_WITHIN(1, 120, p.y);
}
void test_north_edge_at_range() {
    Point p = project(25.0f, 0.0f, 25.0f);
    TEST_ASSERT_INT_WITHIN(1, 120, p.x);
    TEST_ASSERT_INT_WITHIN(2, 8, p.y);
}
void test_east_half_range() {
    Point p = project(12.5f, 90.0f, 25.0f);
    TEST_ASSERT_INT_WITHIN(2, 176, p.x);
    TEST_ASSERT_INT_WITHIN(2, 120, p.y);
}
void test_ring_index_inner()  { TEST_ASSERT_EQUAL(0, ring_index(3.0f, 25.0f)); }
void test_ring_index_middle() { TEST_ASSERT_EQUAL(1, ring_index(12.0f, 25.0f)); }
void test_ring_index_outer()  { TEST_ASSERT_EQUAL(2, ring_index(22.0f, 25.0f)); }
void test_ring_index_beyond() { TEST_ASSERT_EQUAL(3, ring_index(40.0f, 25.0f)); }
void test_tag_visible_inside_2nd() { TEST_ASSERT_TRUE(tag_visible(ring_index(12.0f, 25.0f))); }
void test_tag_hidden_outer()       { TEST_ASSERT_FALSE(tag_visible(ring_index(22.0f, 25.0f))); }
int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_center_when_zero_dist); RUN_TEST(test_north_edge_at_range);
    RUN_TEST(test_east_half_range); RUN_TEST(test_ring_index_inner);
    RUN_TEST(test_ring_index_middle); RUN_TEST(test_ring_index_outer);
    RUN_TEST(test_ring_index_beyond); RUN_TEST(test_tag_visible_inside_2nd);
    RUN_TEST(test_tag_hidden_outer);
    return UNITY_END();
}
