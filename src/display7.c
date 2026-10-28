#include "display7.h"
#include "i2c.h"
#include "buzzer.h"

static uint8_t display_address = 0xFF;
static bool waiting = false;
static uint32_t retry_started = 0;

void display7_init(void)
{
    display_address = 0xFF;
    waiting = false;

    if (DISPLAY7_I2C_ADDRESS != 0xFF)
    {
        if (i2c_probe(DISPLAY7_I2C_ADDRESS))
            display_address = DISPLAY7_I2C_ADDRESS;
        return;
    }

    // Zoek eerst buiten het ingestelde LCD-adres
    for (uint8_t address = 0x20; address <= 0x27; ++address)
    {
        if (address != LCD_I2C_ADDRESS && i2c_probe(address))
        {
            display_address = address;
            return;
        }
    }

    // Alleen het gereserveerde adres gevonden: behoud het bestaande display
    if (LCD_I2C_ADDRESS >= 0x20 && LCD_I2C_ADDRESS <= 0x27 &&
        i2c_probe(LCD_I2C_ADDRESS))
    {
        display_address = LCD_I2C_ADDRESS;
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

    if (value > 15 || display_address == 0xFF)
    {
        return false;
    }

    if (waiting && (uint32_t)(buzzer_get_time_ticks() - retry_started) < 6250UL)
    {
        return false;
    }

    // De segmenten zijn actief LOW
    uint8_t pattern = (uint8_t)~hex_segments[value];

    bool success = i2c_write(display_address, &pattern, 1);

    waiting = !success;
    if (waiting) retry_started = buzzer_get_time_ticks();
    return success;
}
