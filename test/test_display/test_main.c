#include <unity.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lcd.h"
#include "i2c.h"
#include "display7.h"
#include "buzzer.h"

static uint32_t ticks;
static bool devices[128];
static bool fail_next;
static uint16_t writes;
static uint16_t lcd_writes;
static uint16_t display7_writes;
static uint8_t last_segments;
static uint8_t lcd_address;
static char screen[32];
static uint8_t nibbles[16];
static uint8_t nibble_count;
static uint8_t commands[32];
static uint8_t command_count;
static uint8_t clears;

uint32_t buzzer_get_time_ticks(void)
{
    return ticks;
}

bool i2c_probe(uint8_t address)
{
    return address < 128 && devices[address];
}

static uint8_t read_nibble(uint8_t pins)
{
    uint8_t value = 0;
    if (pins & LCD_D4_MASK) value |= 1;
    if (pins & LCD_D5_MASK) value |= 2;
    if (pins & LCD_D6_MASK) value |= 4;
    if (pins & LCD_D7_MASK) value |= 8;
    return value;
}

bool i2c_write(uint8_t address, const uint8_t *data, uint8_t length)
{
    ++writes;
    if (fail_next || !i2c_probe(address))
    {
        fail_next = false;
        return false;
    }

    if (address != LCD_I2C_ADDRESS)
    {
        TEST_ASSERT_EQUAL_UINT8(1, length);
        ++display7_writes;
        last_segments = data[0];
        return true;
    }

    ++lcd_writes;
    TEST_ASSERT_TRUE(length == 3 || length == 6);
    for (uint8_t i = 0; i < length; i += 3)
    {
        TEST_ASSERT_EQUAL_HEX8(0, data[i] & (LCD_ENABLE_MASK | LCD_RW_MASK));
        TEST_ASSERT_EQUAL_HEX8(data[i] | LCD_ENABLE_MASK, data[i + 1]);
        TEST_ASSERT_EQUAL_HEX8(data[i], data[i + 2]);
    }

    if (length == 3)
    {
        TEST_ASSERT_LESS_THAN_UINT8(sizeof(nibbles), nibble_count);
        nibbles[nibble_count++] = read_nibble(data[0]);
        return true;
    }

    uint8_t value = (uint8_t)((read_nibble(data[0]) << 4) | read_nibble(data[3]));
    TEST_ASSERT_EQUAL_HEX8(data[0] & LCD_RS_MASK, data[3] & LCD_RS_MASK);
    if (data[0] & LCD_RS_MASK)
    {
        // Boots alleen de twee zichtbare regels en de cursor na
        if (lcd_address < 16)
            screen[lcd_address] = (char)value;
        else if (lcd_address >= 0x40 && lcd_address < 0x50)
            screen[16 + lcd_address - 0x40] = (char)value;
        else
            TEST_FAIL_MESSAGE("LCD-cursor staat buiten de twee zichtbare regels");
        ++lcd_address;
    }
    else
    {
        if (command_count < sizeof(commands)) commands[command_count++] = value;
        if (value & 0x80) lcd_address = value & 0x7F;
        if (value == 0x01)
        {
            memset(screen, ' ', sizeof(screen));
            lcd_address = 0;
            ++clears;
        }
    }
    return true;
}

void setUp(void)
{
    ticks = 0;
    memset(devices, 0, sizeof(devices));
    devices[0x20] = true;
    devices[LCD_I2C_ADDRESS] = true;
    fail_next = false;
    writes = 0;
    lcd_writes = 0;
    display7_writes = 0;
    last_segments = 0;
    lcd_address = 0;
    memset(screen, ' ', sizeof(screen));
    nibble_count = 0;
    command_count = 0;
    clears = 0;
    display7_init();
    lcd_init(display7_get_address());
}

void tearDown(void)
{
}

static void run_steps(uint16_t count)
{
    for (uint16_t i = 0; i < count; ++i)
    {
        ticks += 135;
        uint16_t before = writes;
        lcd_update();
        TEST_ASSERT_TRUE(writes - before <= 1);
    }
}

static void assert_rows(const char *first, const char *second)
{
    TEST_ASSERT_EQUAL_UINT32(16, strlen(first));
    TEST_ASSERT_EQUAL_UINT32(16, strlen(second));
    TEST_ASSERT_EQUAL_MEMORY(first, screen, 16);
    TEST_ASSERT_EQUAL_MEMORY(second, screen + 16, 16);
}

static void test_displays_use_separate_addresses(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x20, display7_get_address());
    run_steps(300);
    TEST_ASSERT_TRUE(lcd_writes > 0);
    TEST_ASSERT_TRUE(display7_show_hex(5));
    TEST_ASSERT_EQUAL_UINT16(1, display7_writes);
    TEST_ASSERT_EQUAL_HEX8(0x92, last_segments);
}

static void test_only_shared_address_keeps_lcd_disabled(void)
{
    devices[0x20] = false;
    display7_init();
    TEST_ASSERT_EQUAL_HEX8(LCD_I2C_ADDRESS, display7_get_address());
    lcd_init(display7_get_address());
    run_steps(300);
    TEST_ASSERT_EQUAL_UINT16(0, writes);
}

static void test_direct_address_conflict_disables_lcd(void)
{
    lcd_init(LCD_I2C_ADDRESS);
    run_steps(300);
    TEST_ASSERT_EQUAL_UINT16(0, writes);
}

static void test_initialization_sequence_and_first_wait(void)
{
    ticks = 3259;
    lcd_update();
    TEST_ASSERT_EQUAL_UINT16(0, writes);
    ticks = 3260;
    lcd_update();
    TEST_ASSERT_EQUAL_UINT16(1, writes);

    ticks += 447;
    lcd_update();
    TEST_ASSERT_EQUAL_UINT16(1, writes);
    ++ticks;
    lcd_update();
    TEST_ASSERT_EQUAL_UINT16(2, writes);

    run_steps(300);
    const uint8_t expected_nibbles[] = {3, 3, 3, 2};
    const uint8_t expected_commands[] = {0x28, 0x08, 0x01, 0x06, 0x0C};
    TEST_ASSERT_EQUAL_UINT8(4, nibble_count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_nibbles, nibbles, 4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected_commands, commands, 5);
    TEST_ASSERT_EQUAL_UINT8(1, clears);
}

static void test_shows_raw_distance_and_filtered_frequency(void)
{
    lcd_set_measurement(12, 1184);
    run_steps(300);
    assert_rows("Afstand:   12 cm", "Freq:    1184 Hz");
}

static void test_shorter_values_erase_old_digits_without_clearing(void)
{
    lcd_set_measurement(565, 230);
    run_steps(300);
    assert_rows("Afstand:  565 cm", "Freq:     230 Hz");
    lcd_set_measurement(65, 1364);
    run_steps(300);
    assert_rows("Afstand:   65 cm", "Freq:    1364 Hz");
    lcd_set_measurement(2, 230);
    run_steps(300);
    assert_rows("Afstand:    2 cm", "Freq:     230 Hz");
    TEST_ASSERT_EQUAL_UINT8(1, clears);
}

static void test_unchanged_values_do_not_write_again(void)
{
    lcd_set_measurement(25, 950);
    run_steps(300);
    uint16_t before = writes;
    run_steps(300);
    TEST_ASSERT_EQUAL_UINT16(before, writes);
}

static void test_distance_outside_playing_range_remains_visible(void)
{
    lcd_set_measurement(565, 230);
    run_steps(300);
    assert_rows("Afstand:  565 cm", "Freq:     230 Hz");
}

static void test_timeout_replaces_previous_distance_with_dashes(void)
{
    lcd_set_measurement(10, 1220);
    run_steps(300);
    lcd_set_measurement(0, 230);
    run_steps(300);
    assert_rows("Afstand:  --- cm", "Freq:     230 Hz");
}

static void test_missing_lcd_waits_before_retry_and_can_recover(void)
{
    devices[LCD_I2C_ADDRESS] = false;
    ticks = 3260;
    lcd_update();
    TEST_ASSERT_EQUAL_UINT16(1, writes);
    ticks += 62634;
    lcd_update();
    TEST_ASSERT_EQUAL_UINT16(1, writes);
    ++ticks;
    lcd_update();
    TEST_ASSERT_EQUAL_UINT16(2, writes);

    devices[LCD_I2C_ADDRESS] = true;
    lcd_set_measurement(20, 1040);
    run_steps(800);
    assert_rows("Afstand:   20 cm", "Freq:    1040 Hz");
}

static void test_failed_update_reinitializes_and_restores_the_screen(void)
{
    lcd_set_measurement(10, 1220);
    run_steps(300);
    lcd_set_measurement(40, 680);
    fail_next = true;
    run_steps(900);
    assert_rows("Afstand:   40 cm", "Freq:     680 Hz");
    TEST_ASSERT_EQUAL_UINT8(2, clears);
}

static void test_clock_wrap_does_not_stop_initialization(void)
{
    ticks = UINT32_MAX - 1000UL;
    lcd_init(0x20);
    lcd_set_measurement(15, 1130);
    run_steps(300);
    assert_rows("Afstand:   15 cm", "Freq:    1130 Hz");
}

static void test_segment_failure_uses_a_retry_pause(void)
{
    fail_next = true;
    TEST_ASSERT_FALSE(display7_show_hex(5));
    uint16_t before = writes;
    ticks = 6249;
    TEST_ASSERT_FALSE(display7_show_hex(5));
    TEST_ASSERT_EQUAL_UINT16(before, writes);
    ticks = 6250;
    TEST_ASSERT_TRUE(display7_show_hex(5));
    TEST_ASSERT_EQUAL_UINT16(before + 1, writes);
}

static void test_invalid_segment_value_does_not_touch_the_bus(void)
{
    TEST_ASSERT_FALSE(display7_show_hex(16));
    TEST_ASSERT_EQUAL_UINT16(0, writes);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_displays_use_separate_addresses);
    RUN_TEST(test_only_shared_address_keeps_lcd_disabled);
    RUN_TEST(test_direct_address_conflict_disables_lcd);
    RUN_TEST(test_initialization_sequence_and_first_wait);
    RUN_TEST(test_shows_raw_distance_and_filtered_frequency);
    RUN_TEST(test_shorter_values_erase_old_digits_without_clearing);
    RUN_TEST(test_unchanged_values_do_not_write_again);
    RUN_TEST(test_distance_outside_playing_range_remains_visible);
    RUN_TEST(test_timeout_replaces_previous_distance_with_dashes);
    RUN_TEST(test_missing_lcd_waits_before_retry_and_can_recover);
    RUN_TEST(test_failed_update_reinitializes_and_restores_the_screen);
    RUN_TEST(test_clock_wrap_does_not_stop_initialization);
    RUN_TEST(test_segment_failure_uses_a_retry_pause);
    RUN_TEST(test_invalid_segment_value_does_not_touch_the_bus);
    return UNITY_END();
}
