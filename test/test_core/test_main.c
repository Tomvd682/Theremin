#include <unity.h>
#include <limits.h>
#include <stdint.h>

#include "median_filter.h"
#include "theremin_math.h"
#include "button_debounce.h"
#include "filter_test_allocator.h"

void setUp(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, allocator_live_blocks());
    allocator_reset();
}

void tearDown(void)
{
    TEST_ASSERT_TRUE(allocator_guards_ok());
    median_filter_free();
    TEST_ASSERT_EQUAL_UINT32(0, allocator_live_blocks());
    TEST_ASSERT_EQUAL_UINT32(0, allocator_errors());
    TEST_ASSERT_EQUAL_UINT8(0, median_filter_get_size());
}

static void test_uninitialized_filter_passes_through(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, median_filter_get_size());
    TEST_ASSERT_EQUAL_UINT16(0, median_filter_add(0));
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, median_filter_add(UINT16_MAX));
    TEST_ASSERT_EQUAL_UINT32(0, allocator_alloc_calls());
}

static void test_all_valid_filter_sizes(void)
{
    for (uint8_t size = 1; size <= 15; size += 2)
    {
        TEST_ASSERT_TRUE(median_filter_resize(size));
        TEST_ASSERT_EQUAL_UINT8(size, median_filter_get_size());
        TEST_ASSERT_EQUAL_UINT32(1, allocator_live_blocks());
    }
    TEST_ASSERT_EQUAL_UINT32(8, allocator_alloc_calls());
    TEST_ASSERT_EQUAL_UINT32(7, allocator_free_calls());
}

static void test_all_invalid_sizes_preserve_filter(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(3));
    median_filter_add(10);
    median_filter_add(20);
    median_filter_add(30);
    for (unsigned int size = 0; size <= UINT8_MAX; ++size)
    {
        if (size >= 1U && size <= 15U && size % 2U != 0U) continue;
        TEST_ASSERT_FALSE(median_filter_resize((uint8_t)size));
        TEST_ASSERT_EQUAL_UINT8(3, median_filter_get_size());
    }
    TEST_ASSERT_EQUAL_UINT32(1, allocator_alloc_calls());
    TEST_ASSERT_EQUAL_UINT16(30, median_filter_add(40));
}

static void test_size_one_always_returns_latest(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(1));
    const uint16_t values[] = {0, UINT16_MAX, 100, 1, 30000};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        TEST_ASSERT_EQUAL_UINT16(values[i], median_filter_add(values[i]));
    }
}

static void test_unsorted_samples_and_lower_warmup_median(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(5));
    TEST_ASSERT_EQUAL_UINT16(50, median_filter_add(50));
    TEST_ASSERT_EQUAL_UINT16(10, median_filter_add(10));
    TEST_ASSERT_EQUAL_UINT16(30, median_filter_add(30));
    TEST_ASSERT_EQUAL_UINT16(20, median_filter_add(20));
    TEST_ASSERT_EQUAL_UINT16(30, median_filter_add(40));
}

static void test_duplicates_and_uint16_limits(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(3));
    median_filter_add(UINT16_MAX);
    median_filter_add(0);
    TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, median_filter_add(UINT16_MAX));
    TEST_ASSERT_EQUAL_UINT16(0, median_filter_add(0));
    TEST_ASSERT_EQUAL_UINT16(0, median_filter_add(0));
}

static void test_sliding_window_evicts_oldest_not_smallest(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(3));
    median_filter_add(100);
    median_filter_add(1);
    TEST_ASSERT_EQUAL_UINT16(90, median_filter_add(90));
    TEST_ASSERT_EQUAL_UINT16(2, median_filter_add(2));
    TEST_ASSERT_EQUAL_UINT16(80, median_filter_add(80));
    TEST_ASSERT_EQUAL_UINT16(3, median_filter_add(3));
}

static void test_shrink_keeps_newest_in_correct_order(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(5));
    const uint16_t values[] = {100, 1, 80, 2, 60};
    for (size_t i = 0; i < 5; ++i) median_filter_add(values[i]);
    TEST_ASSERT_TRUE(median_filter_resize(3));
    TEST_ASSERT_EQUAL_UINT16(3, median_filter_add(3)); // Metingen: 2, 60, 3
    TEST_ASSERT_EQUAL_UINT16(50, median_filter_add(50)); // Metingen: 60, 3, 50
}

static void test_grow_preserves_samples_and_their_order(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(3));
    median_filter_add(90);
    median_filter_add(10);
    median_filter_add(70);
    TEST_ASSERT_TRUE(median_filter_resize(5));
    TEST_ASSERT_EQUAL_UINT16(10, median_filter_add(1));
    TEST_ASSERT_EQUAL_UINT16(50, median_filter_add(50));
    TEST_ASSERT_EQUAL_UINT16(10, median_filter_add(2));
}

static void test_resize_partially_filled_filter(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(15));
    median_filter_add(30);
    median_filter_add(10);
    TEST_ASSERT_TRUE(median_filter_resize(3));
    TEST_ASSERT_EQUAL_UINT16(20, median_filter_add(20));
    TEST_ASSERT_EQUAL_UINT16(20, median_filter_add(100));
}

static void test_same_size_does_not_allocate_or_reset(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(3));
    median_filter_add(10);
    median_filter_add(20);
    TEST_ASSERT_TRUE(median_filter_resize(3));
    TEST_ASSERT_EQUAL_UINT32(1, allocator_alloc_calls());
    TEST_ASSERT_EQUAL_UINT32(0, allocator_free_calls());
    TEST_ASSERT_EQUAL_UINT16(20, median_filter_add(30));
}

static void test_initial_allocation_failure_and_recovery(void)
{
    allocator_fail_next();
    TEST_ASSERT_FALSE(median_filter_resize(5));
    TEST_ASSERT_EQUAL_UINT8(0, median_filter_get_size());
    TEST_ASSERT_EQUAL_UINT16(123, median_filter_add(123));
    TEST_ASSERT_EQUAL_UINT32(0, allocator_live_blocks());
    TEST_ASSERT_TRUE(median_filter_resize(1));
    TEST_ASSERT_EQUAL_UINT16(42, median_filter_add(42));
}

static void test_failed_grow_preserves_buffer_and_history(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(3));
    median_filter_add(100);
    median_filter_add(1);
    median_filter_add(90);
    allocator_fail_next();
    TEST_ASSERT_FALSE(median_filter_resize(15));
    TEST_ASSERT_EQUAL_UINT8(3, median_filter_get_size());
    TEST_ASSERT_EQUAL_UINT32(1, allocator_live_blocks());
    TEST_ASSERT_EQUAL_UINT32(0, allocator_free_calls());
    TEST_ASSERT_EQUAL_UINT16(2, median_filter_add(2));
}

static void test_failed_shrink_preserves_buffer_and_history(void)
{
    TEST_ASSERT_TRUE(median_filter_resize(5));
    for (uint16_t value = 10; value <= 50; value += 10) median_filter_add(value);
    allocator_fail_next();
    TEST_ASSERT_FALSE(median_filter_resize(1));
    TEST_ASSERT_EQUAL_UINT8(5, median_filter_get_size());
    TEST_ASSERT_EQUAL_UINT16(40, median_filter_add(60));
}

static void test_free_is_idempotent_and_reinitialization_is_empty(void)
{
    median_filter_free();
    TEST_ASSERT_TRUE(median_filter_resize(15));
    median_filter_add(UINT16_MAX);
    median_filter_free();
    median_filter_free();
    TEST_ASSERT_EQUAL_UINT32(1, allocator_free_calls());
    TEST_ASSERT_EQUAL_UINT16(7, median_filter_add(7));
    TEST_ASSERT_TRUE(median_filter_resize(3));
    TEST_ASSERT_EQUAL_UINT16(10, median_filter_add(10));
}

// Gebruik een andere sorteermethode om de filteruitkomst te controleren
static uint16_t reference_median(const uint16_t *values, uint8_t count)
{
    uint16_t sorted[15];
    for (uint8_t i = 0; i < count; ++i)
    {
        uint8_t j = i;
        while (j > 0 && sorted[j - 1] > values[i])
        {
            sorted[j] = sorted[j - 1];
            --j;
        }
        sorted[j] = values[i];
    }
    return sorted[(count - 1U) / 2U];
}

static void test_long_sequence_with_resizes_matches_reference(void)
{
    uint32_t random = 0x12345678UL;
    uint16_t chronological[15];
    uint8_t count = 0;
    uint8_t size = 15;
    TEST_ASSERT_TRUE(median_filter_resize(size));
    for (uint32_t step = 0; step < 30000UL; ++step)
    {
        // Elke testrun gebruikt dezelfde meetreeks
        random = random * 1664525UL + 1013904223UL;
        if (step % 37UL == 0)
        {
            size = (uint8_t)(1U + 2U * ((random >> 16) % 8U));
            TEST_ASSERT_TRUE(median_filter_resize(size));
            if (count > size)
            {
                uint8_t discard = count - size;
                for (uint8_t i = 0; i < size; ++i)
                {
                    chronological[i] = chronological[i + discard];
                }
                count = size;
            }
        }
        if (count == size)
        {
            for (uint8_t i = 1; i < count; ++i)
            {
                chronological[i - 1] = chronological[i];
            }
            --count;
        }
        uint16_t value = (uint16_t)(random >> 8);
        chronological[count++] = value;
        TEST_ASSERT_EQUAL_UINT16(reference_median(chronological, count), median_filter_add(value));
        TEST_ASSERT_EQUAL_UINT32(1, allocator_live_blocks());
        TEST_ASSERT_TRUE(allocator_guards_ok());
    }
}

static void test_repeated_resize_and_free_has_no_leaks(void)
{
    for (unsigned int cycle = 0; cycle < 100; ++cycle)
    {
        for (uint8_t size = 1; size <= 15; size += 2)
        {
            TEST_ASSERT_TRUE(median_filter_resize(size));
            median_filter_add((uint16_t)cycle);
        }
        median_filter_free();
        TEST_ASSERT_EQUAL_UINT32(0, allocator_live_blocks());
    }
    TEST_ASSERT_EQUAL_UINT32(800, allocator_alloc_calls());
    TEST_ASSERT_EQUAL_UINT32(800, allocator_free_calls());
}

static void test_distance_rounding_boundaries(void)
{
    TEST_ASSERT_EQUAL_UINT16(0, theremin_distance_cm(0));
    TEST_ASSERT_EQUAL_UINT16(0, theremin_distance_cm(57));
    TEST_ASSERT_EQUAL_UINT16(1, theremin_distance_cm(58));
    TEST_ASSERT_EQUAL_UINT16(1, theremin_distance_cm(173));
    TEST_ASSERT_EQUAL_UINT16(2, theremin_distance_cm(174));
    TEST_ASSERT_EQUAL_UINT16(65, theremin_distance_cm(7540));
    TEST_ASSERT_EQUAL_UINT16(65, theremin_distance_cm(7597));
    TEST_ASSERT_EQUAL_UINT16(66, theremin_distance_cm(7598));
}

static void test_distance_avr_overflow_regression(void)
{
    TEST_ASSERT_EQUAL_UINT16(564, theremin_distance_cm(65477));
    TEST_ASSERT_EQUAL_UINT16(564, theremin_distance_cm(65478));
    TEST_ASSERT_EQUAL_UINT16(564, theremin_distance_cm(65481));
    TEST_ASSERT_EQUAL_UINT16(565, theremin_distance_cm(65482));
    TEST_ASSERT_EQUAL_UINT16(565, theremin_distance_cm(UINT16_MAX));
    TEST_ASSERT_FALSE(theremin_distance_valid(theremin_distance_cm(UINT16_MAX)));
}

static void test_every_echo_tick_matches_wide_reference(void)
{
    uint16_t previous = 0;
    for (uint32_t ticks = 0; ticks <= UINT16_MAX; ++ticks)
    {
        uint16_t expected = (uint16_t)(((uint64_t)ticks + 58ULL) / 116ULL);
        uint16_t actual = theremin_distance_cm((uint16_t)ticks);
        TEST_ASSERT_EQUAL_UINT16(expected, actual);
        TEST_ASSERT_TRUE(actual >= previous);
        previous = actual;
    }
}

static void test_distance_validity_limits(void)
{
    TEST_ASSERT_FALSE(theremin_distance_valid(0));
    TEST_ASSERT_FALSE(theremin_distance_valid(1));
    TEST_ASSERT_TRUE(theremin_distance_valid(2));
    TEST_ASSERT_TRUE(theremin_distance_valid(65));
    TEST_ASSERT_FALSE(theremin_distance_valid(66));
    TEST_ASSERT_FALSE(theremin_distance_valid(UINT16_MAX));
}

static void test_frequency_mapping_and_invalid_input(void)
{
    TEST_ASSERT_EQUAL_UINT16(1364, theremin_frequency_hz(2));
    TEST_ASSERT_EQUAL_UINT16(1220, theremin_frequency_hz(10));
    TEST_ASSERT_EQUAL_UINT16(500, theremin_frequency_hz(50));
    TEST_ASSERT_EQUAL_UINT16(230, theremin_frequency_hz(65));
    TEST_ASSERT_EQUAL_UINT16(230, theremin_frequency_hz(0));
    TEST_ASSERT_EQUAL_UINT16(230, theremin_frequency_hz(1));
    TEST_ASSERT_EQUAL_UINT16(230, theremin_frequency_hz(66));
    TEST_ASSERT_EQUAL_UINT16(230, theremin_frequency_hz(UINT16_MAX));
}

static void test_frequency_is_monotonic_over_valid_distances(void)
{
    for (uint16_t cm = 3; cm <= 65; ++cm)
        TEST_ASSERT_EQUAL_UINT16(18, theremin_frequency_hz(cm - 1) - theremin_frequency_hz(cm));
}

static void test_timer_top_clamping_and_known_values(void)
{
    TEST_ASSERT_EQUAL_UINT8(134, theremin_timer0_top(0));
    TEST_ASSERT_EQUAL_UINT8(134, theremin_timer0_top(229));
    TEST_ASSERT_EQUAL_UINT8(134, theremin_timer0_top(230));
    TEST_ASSERT_EQUAL_UINT8(62, theremin_timer0_top(500));
    TEST_ASSERT_EQUAL_UINT8(31, theremin_timer0_top(1000));
    TEST_ASSERT_EQUAL_UINT8(22, theremin_timer0_top(1400));
    TEST_ASSERT_EQUAL_UINT8(22, theremin_timer0_top(UINT16_MAX));
}

static void test_every_frequency_has_safe_monotonic_timer_top(void)
{
    uint8_t previous = 134;
    for (uint32_t hz = 0; hz <= UINT16_MAX; ++hz)
    {
        uint8_t top = theremin_timer0_top((uint16_t)hz);
        TEST_ASSERT_TRUE(top >= 22U && top <= 134U);
        TEST_ASSERT_TRUE(top <= previous);
        previous = top;
    }
}

static void test_adc_volume_known_rounding_values(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, theremin_volume_percent(0));
    TEST_ASSERT_EQUAL_UINT8(0, theremin_volume_percent(1));
    TEST_ASSERT_EQUAL_UINT8(1, theremin_volume_percent(2));
    TEST_ASSERT_EQUAL_UINT8(49, theremin_volume_percent(126));
    TEST_ASSERT_EQUAL_UINT8(50, theremin_volume_percent(127));
    TEST_ASSERT_EQUAL_UINT8(50, theremin_volume_percent(128));
    TEST_ASSERT_EQUAL_UINT8(100, theremin_volume_percent(254));
    TEST_ASSERT_EQUAL_UINT8(100, theremin_volume_percent(255));
}

static void test_all_adc_values_round_to_nearest_percent(void)
{
    uint8_t previous = 0;
    for (unsigned int raw = 0; raw <= UINT8_MAX; ++raw)
    {
        uint8_t percent = theremin_volume_percent((uint8_t)raw);
        TEST_ASSERT_EQUAL_UINT8((raw * 100U + 127U) / 255U, percent);
        TEST_ASSERT_TRUE(percent <= 100U && percent >= previous);
        int error = (int)percent * 255 - (int)raw * 100;
        TEST_ASSERT_TRUE(error >= -127 && error <= 127);
        previous = percent;
    }
}

static void test_pwm_volume_clamping_and_rounding(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, theremin_volume_pwm(0));
    TEST_ASSERT_EQUAL_UINT8(2, theremin_volume_pwm(1));
    TEST_ASSERT_EQUAL_UINT8(127, theremin_volume_pwm(50));
    TEST_ASSERT_EQUAL_UINT8(252, theremin_volume_pwm(99));
    TEST_ASSERT_EQUAL_UINT8(255, theremin_volume_pwm(100));
    TEST_ASSERT_EQUAL_UINT8(255, theremin_volume_pwm(101));
    TEST_ASSERT_EQUAL_UINT8(255, theremin_volume_pwm(255));
}

static void test_all_pwm_inputs_are_bounded_and_monotonic(void)
{
    uint8_t previous = 0;
    for (unsigned int input = 0; input <= UINT8_MAX; ++input)
    {
        uint8_t pwm = theremin_volume_pwm((uint8_t)input);
        unsigned int clamped = input > 100U ? 100U : input;
        TEST_ASSERT_EQUAL_UINT8(clamped * 255U / 100U, pwm);
        TEST_ASSERT_TRUE(pwm >= previous);
        previous = pwm;
    }
}

#define BUTTONS 0x30U
#define PLUS_DOWN 0x20U

static void test_debounce_waits_twenty_ms_and_emits_once(void)
{
    ButtonDebounce state;
    button_debounce_init(&state, BUTTONS);
    TEST_ASSERT_EQUAL_HEX8(0, button_debounce_update(&state, true, 100, PLUS_DOWN));
    TEST_ASSERT_EQUAL_HEX8(0, button_debounce_update(&state, false, 1349, PLUS_DOWN));
    TEST_ASSERT_EQUAL_HEX8(0x10, button_debounce_update(&state, false, 1350, PLUS_DOWN));
    TEST_ASSERT_EQUAL_HEX8(0, button_debounce_update(&state, false, 9999, PLUS_DOWN));
}

static void test_bounce_restarts_stability_interval(void)
{
    ButtonDebounce state;
    button_debounce_init(&state, BUTTONS);
    button_debounce_update(&state, true, 0, PLUS_DOWN);
    button_debounce_update(&state, true, 500, BUTTONS);
    button_debounce_update(&state, true, 1000, PLUS_DOWN);
    TEST_ASSERT_EQUAL_HEX8(0, button_debounce_update(&state, false, 2250 - 1, PLUS_DOWN));
    TEST_ASSERT_EQUAL_HEX8(0x10, button_debounce_update(&state, false, 2250, PLUS_DOWN));
}

static void test_release_then_second_press(void)
{
    ButtonDebounce state;
    button_debounce_init(&state, BUTTONS);
    button_debounce_update(&state, true, 0, PLUS_DOWN);
    TEST_ASSERT_EQUAL_HEX8(0x10, button_debounce_update(&state, false, 1250, PLUS_DOWN));
    button_debounce_update(&state, true, 2000, BUTTONS);
    TEST_ASSERT_EQUAL_HEX8(0, button_debounce_update(&state, false, 3250, BUTTONS));
    button_debounce_update(&state, true, 4000, PLUS_DOWN);
    TEST_ASSERT_EQUAL_HEX8(0x10, button_debounce_update(&state, false, 5250, PLUS_DOWN));
}

static void test_debounce_handles_clock_wrap(void)
{
    ButtonDebounce state;
    button_debounce_init(&state, BUTTONS);
    uint32_t start = UINT32_MAX - 500UL;
    button_debounce_update(&state, true, start, PLUS_DOWN);
    TEST_ASSERT_EQUAL_HEX8(0, button_debounce_update(&state, false, 748, PLUS_DOWN));
    TEST_ASSERT_EQUAL_HEX8(0x10, button_debounce_update(&state, false, 749, PLUS_DOWN));
}

static void test_edge_at_deadline_defers_acceptance(void)
{
    ButtonDebounce state;
    button_debounce_init(&state, BUTTONS);
    button_debounce_update(&state, true, 0, PLUS_DOWN);
    TEST_ASSERT_EQUAL_HEX8(0, button_debounce_update(&state, true, 1250, BUTTONS));
    TEST_ASSERT_EQUAL_HEX8(0, button_debounce_update(&state, false, 2500, BUTTONS));
}

static void test_simultaneous_presses_and_initially_held_button(void)
{
    ButtonDebounce state;
    button_debounce_init(&state, BUTTONS);
    button_debounce_update(&state, true, 0, 0);
    TEST_ASSERT_EQUAL_HEX8(BUTTONS, button_debounce_update(&state, false, 1250, 0));
    button_debounce_init(&state, PLUS_DOWN);
    TEST_ASSERT_EQUAL_HEX8(0, button_debounce_update(&state, false, 3000, PLUS_DOWN));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_uninitialized_filter_passes_through);
    RUN_TEST(test_all_valid_filter_sizes);
    RUN_TEST(test_all_invalid_sizes_preserve_filter);
    RUN_TEST(test_size_one_always_returns_latest);
    RUN_TEST(test_unsorted_samples_and_lower_warmup_median);
    RUN_TEST(test_duplicates_and_uint16_limits);
    RUN_TEST(test_sliding_window_evicts_oldest_not_smallest);
    RUN_TEST(test_shrink_keeps_newest_in_correct_order);
    RUN_TEST(test_grow_preserves_samples_and_their_order);
    RUN_TEST(test_resize_partially_filled_filter);
    RUN_TEST(test_same_size_does_not_allocate_or_reset);
    RUN_TEST(test_initial_allocation_failure_and_recovery);
    RUN_TEST(test_failed_grow_preserves_buffer_and_history);
    RUN_TEST(test_failed_shrink_preserves_buffer_and_history);
    RUN_TEST(test_free_is_idempotent_and_reinitialization_is_empty);
    RUN_TEST(test_long_sequence_with_resizes_matches_reference);
    RUN_TEST(test_repeated_resize_and_free_has_no_leaks);
    RUN_TEST(test_distance_rounding_boundaries);
    RUN_TEST(test_distance_avr_overflow_regression);
    RUN_TEST(test_every_echo_tick_matches_wide_reference);
    RUN_TEST(test_distance_validity_limits);
    RUN_TEST(test_frequency_mapping_and_invalid_input);
    RUN_TEST(test_frequency_is_monotonic_over_valid_distances);
    RUN_TEST(test_timer_top_clamping_and_known_values);
    RUN_TEST(test_every_frequency_has_safe_monotonic_timer_top);
    RUN_TEST(test_adc_volume_known_rounding_values);
    RUN_TEST(test_all_adc_values_round_to_nearest_percent);
    RUN_TEST(test_pwm_volume_clamping_and_rounding);
    RUN_TEST(test_all_pwm_inputs_are_bounded_and_monotonic);
    RUN_TEST(test_debounce_waits_twenty_ms_and_emits_once);
    RUN_TEST(test_bounce_restarts_stability_interval);
    RUN_TEST(test_release_then_second_press);
    RUN_TEST(test_debounce_handles_clock_wrap);
    RUN_TEST(test_edge_at_deadline_defers_acceptance);
    RUN_TEST(test_simultaneous_presses_and_initially_held_button);
    return UNITY_END();
}
