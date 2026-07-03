#include <unity.h>
#include "hardware/button.h"
#include <vector>

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

// Helper: drive FSM with ~10ms polling steps, collect non-None events.
static std::vector<ButtonEvent> poll(ButtonFsm &b,
                                     bool pressed_state,
                                     uint32_t t_start,
                                     uint32_t t_end,
                                     uint32_t step = 10) {
    std::vector<ButtonEvent> events;
    for (uint32_t t = t_start; t <= t_end; t += step) {
        ButtonEvent ev = b.update(pressed_state, t);
        if (ev != ButtonEvent::None) events.push_back(ev);
    }
    return events;
}

// Two slow clicks: the second press arrives BEFORE the first click's 400 ms
// window has timed out, but the second RELEASE is 401 ms after the first
// RELEASE — i.e., the two clicks are NOT within 400 ms of each other.
//
// Timeline:
//   down@0, up@80  (first click)
//   down@381, up@481  (second click; second release is 481-80=401 ms after first release)
//
// The first click window (from t=80) expires at t=480. The second press
// arrives at t=381 (before the window expires), so with coarse polling the
// FSM sees pending_clicks_==2 before the first window is resolved.
//
// Correct behaviour: these are NOT a double-click. Both should resolve to Short.
// Buggy behaviour: the eager-double block (line 20 in original) fires Double
// immediately on the second release regardless of timing.
void test_two_slow_clicks_not_double() {
    ButtonFsm b;
    std::vector<ButtonEvent> all_events;

    // First click
    auto ev = b.update(true, 0);
    if (ev != ButtonEvent::None) all_events.push_back(ev);
    ev = b.update(false, 80);           // first release at t=80
    if (ev != ButtonEvent::None) all_events.push_back(ev);

    // Poll idle t=90..370 (no second press yet, first window not yet elapsed)
    {
        auto seg = poll(b, false, 90, 370, 10);
        all_events.insert(all_events.end(), seg.begin(), seg.end());
    }

    // Second press arrives at t=381 — still inside the first window (80+400=480)
    ev = b.update(true, 381);
    if (ev != ButtonEvent::None) all_events.push_back(ev);
    // Second release at t=481 — that is 401 ms after first release (NOT a double)
    ev = b.update(false, 481);
    if (ev != ButtonEvent::None) all_events.push_back(ev);

    // Poll idle t=490..1100 — let both windows resolve
    {
        auto seg = poll(b, false, 490, 1100, 10);
        all_events.insert(all_events.end(), seg.begin(), seg.end());
    }

    // Correct: exactly two Short events, no Double.
    // (The order may be: Short for first click, Short for second click.)
    TEST_ASSERT_EQUAL(2, (int)all_events.size());
    TEST_ASSERT_EQUAL(ButtonEvent::Short, all_events[0]);
    TEST_ASSERT_EQUAL(ButtonEvent::Short, all_events[1]);
}

// Fast double-click: two releases within 400 ms of the first release.
// Must emit exactly one Double and no stray Short.
void test_fast_double_still_works() {
    ButtonFsm b;
    std::vector<ButtonEvent> all_events;

    // First click: down@0, up@50
    auto ev = b.update(true, 0);
    if (ev != ButtonEvent::None) all_events.push_back(ev);
    ev = b.update(false, 50);
    if (ev != ButtonEvent::None) all_events.push_back(ev);

    // Second click: down@120, up@170 (170 ms from first release — well within 400 ms)
    ev = b.update(true, 120);
    if (ev != ButtonEvent::None) all_events.push_back(ev);
    ev = b.update(false, 170);
    if (ev != ButtonEvent::None) all_events.push_back(ev);

    // Poll idle from t=180 to t=700 — Double should fire somewhere in here.
    auto idle = poll(b, false, 180, 700, 10);
    all_events.insert(all_events.end(), idle.begin(), idle.end());

    // Must be exactly one Double event and nothing else.
    TEST_ASSERT_EQUAL(1, (int)all_events.size());
    TEST_ASSERT_EQUAL(ButtonEvent::Double, all_events[0]);
}

int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_short_press);
    RUN_TEST(test_long_press);
    RUN_TEST(test_double_press);
    RUN_TEST(test_two_slow_clicks_not_double);
    RUN_TEST(test_fast_double_still_works);
    return UNITY_END();
}
