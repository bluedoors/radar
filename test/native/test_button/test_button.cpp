#include <unity.h>
#include "hardware/button.h"
#include <vector>

// Helper: drive FSM with polling steps, collect non-None events.
static std::vector<ButtonEvent> poll(ButtonFsm &b, bool pressed_state,
                                     uint32_t t_start, uint32_t t_end, uint32_t step = 10) {
    std::vector<ButtonEvent> events;
    for (uint32_t t = t_start; t <= t_end; t += step) {
        ButtonEvent ev = b.update(pressed_state, t);
        if (ev != ButtonEvent::None) events.push_back(ev);
    }
    return events;
}

// Short press fires IMMEDIATELY on release — no double-click wait.
void test_short_fires_on_release() {
    ButtonFsm b;
    TEST_ASSERT_EQUAL(ButtonEvent::None,  b.update(true, 0));
    TEST_ASSERT_EQUAL(ButtonEvent::None,  b.update(true, 100));   // still held, short
    TEST_ASSERT_EQUAL(ButtonEvent::Short, b.update(false, 200));  // release -> Short now
}

// Two quick taps = two independent Short events (no double-click gesture anymore).
void test_two_taps_two_shorts() {
    ButtonFsm b;
    std::vector<ButtonEvent> ev;
    auto push=[&](ButtonEvent e){ if(e!=ButtonEvent::None) ev.push_back(e); };
    push(b.update(true, 0));
    push(b.update(false, 50));    // Short #1
    push(b.update(true, 120));
    push(b.update(false, 170));   // Short #2
    auto idle = poll(b, false, 180, 400);
    ev.insert(ev.end(), idle.begin(), idle.end());
    TEST_ASSERT_EQUAL(2, (int)ev.size());
    TEST_ASSERT_EQUAL(ButtonEvent::Short, ev[0]);
    TEST_ASSERT_EQUAL(ButtonEvent::Short, ev[1]);
}

// Hold to 1.5 s -> LongPeek fires exactly once, and NOT a Short on release.
void test_long_peek() {
    ButtonFsm b;
    b.update(true, 0);
    auto held = poll(b, true, 10, 1600);   // hold through 1.5 s
    int peeks=0, resets=0;
    for (auto e: held){ if(e==ButtonEvent::LongPeek)peeks++; if(e==ButtonEvent::LongReset)resets++; }
    TEST_ASSERT_EQUAL(1, peeks);
    TEST_ASSERT_EQUAL(0, resets);
    // release after peek -> no Short
    TEST_ASSERT_EQUAL(ButtonEvent::None, b.update(false, 1700));
}

// Hold to 4 s -> LongPeek at 1.5 s, then LongReset at 4 s (each once), no Short on release.
void test_long_reset() {
    ButtonFsm b;
    b.update(true, 0);
    auto held = poll(b, true, 10, 4100);
    int peeks=0, resets=0;
    for (auto e: held){ if(e==ButtonEvent::LongPeek)peeks++; if(e==ButtonEvent::LongReset)resets++; }
    TEST_ASSERT_EQUAL(1, peeks);
    TEST_ASSERT_EQUAL(1, resets);
    TEST_ASSERT_EQUAL(ButtonEvent::None, b.update(false, 4200));
}

// A press just under the peek threshold is a Short, not a LongPeek.
void test_just_under_peek_is_short() {
    ButtonFsm b;
    b.update(true, 0);
    auto held = poll(b, true, 10, 1400);
    for (auto e: held) TEST_ASSERT_NOT_EQUAL(ButtonEvent::LongPeek, e);
    TEST_ASSERT_EQUAL(ButtonEvent::Short, b.update(false, 1490));
}

int main(int,char**){
    UNITY_BEGIN();
    RUN_TEST(test_short_fires_on_release);
    RUN_TEST(test_two_taps_two_shorts);
    RUN_TEST(test_long_peek);
    RUN_TEST(test_long_reset);
    RUN_TEST(test_just_under_peek_is_short);
    return UNITY_END();
}
