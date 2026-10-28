#include "display7.h"
#include "i2c.h"
#include "buzzer.h"

#define DISPLAY7_NOT_FOUND 0xFFU
#define DISPLAY7_RETRY_INTERVAL_TICKS (100000UL / BUZZER_TIME_TICK_US) // 100 ms

static uint8_t display_address = DISPLAY7_NOT_FOUND;
static bool waiting = false;
static uint32_t retry_started = 0;

void display7_init(void)
{
    display_address = DISPLAY7_NOT_FOUND;
    waiting = false;

    if (i2c_probe(DISPLAY7_I2C_ADDRESS))
    {
        display_address = DISPLAY7_I2C_ADDRESS;
    }
}

uint8_t display7_get_address(void)
{
    return display_address;
}

bool display7_show_hex(uint8_t value)
{
    // Segmenten A t/m G zitten op P0 t/m P6
    // Bit 7 is de decimale punt
    static const uint8_t hex_segments[16] =
    {
        0x3F, // 0
        0x06, // 1
        0x5B, // 2
        0x4F, // 3
        0x66, // 4
        0x6D, // 5
        0x7D, // 6
        0x07, // 7
        0x7F, // 8
        0x6F, // 9
        0x77, // A
        0x7C, // b
        0x39, // C
        0x5E, // d
        0x79, // E
        0x71  // F
    };

    if (value > 15 || display_address == DISPLAY7_NOT_FOUND)
    {
        return false;
    }

    if (waiting && (uint32_t)(buzzer_get_time_16us_ticks() - retry_started) <
                   DISPLAY7_RETRY_INTERVAL_TICKS)
    {
        return false;
    }

    // De segmenten zijn actief LOW
    uint8_t pattern = (uint8_t)~hex_segments[value];

    bool success = i2c_write(display_address, &pattern, 1);

    waiting = !success;
    if (waiting) retry_started = buzzer_get_time_16us_ticks();
    return success;
}
