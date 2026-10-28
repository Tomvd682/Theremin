#include "lcd.h"
#include "i2c.h"
#include "buzzer.h"

#include <stdbool.h>
#include <string.h>

#define LCD_COLUMNS 16U
#define LCD_CHARACTERS 32U
#define LCD_REFRESH_TICKS 15625UL
#define TIMER0_MAX_TICKS 135U

static bool enabled;
static uint8_t init_step;
static uint8_t cursor;
static uint16_t distance;
static uint16_t frequency;
static uint32_t wait_started;
static uint16_t wait_ticks;
static uint32_t frame_started;
static char wanted[LCD_CHARACTERS];
static char shown[LCD_CHARACTERS];

static uint8_t lcd_pins(uint8_t nibble, bool data)
{
    uint8_t pins = LCD_BACKLIGHT_ON;
    if (data) pins |= LCD_RS_MASK;
    if (nibble & 1U) pins |= LCD_D4_MASK;
    if (nibble & 2U) pins |= LCD_D5_MASK;
    if (nibble & 4U) pins |= LCD_D6_MASK;
    if (nibble & 8U) pins |= LCD_D7_MASK;
    return pins & (uint8_t)~LCD_RW_MASK;
}

static bool lcd_write_nibble(uint8_t nibble)
{
    uint8_t pins = lcd_pins(nibble, false);
    uint8_t bytes[3] = {pins, pins | LCD_ENABLE_MASK, pins};
    return i2c_write(LCD_I2C_ADDRESS, bytes, sizeof(bytes));
}

static bool lcd_write_byte(uint8_t value, bool data)
{
    uint8_t high = lcd_pins(value >> 4, data);
    uint8_t low = lcd_pins(value & 0x0F, data);
    uint8_t bytes[6] = {
        high, high | LCD_ENABLE_MASK, high,
        low, low | LCD_ENABLE_MASK, low
    };

    // Bij 100 kHz geven de I2C-bytes genoeg tijd voor gewone LCD-opdrachten
    return i2c_write(LCD_I2C_ADDRESS, bytes, sizeof(bytes));
}

static void lcd_wait_ms(uint16_t milliseconds)
{
    wait_started = buzzer_get_time_ticks();
    // De klok springt per tooninterrupt, tel daarom één maximale periode extra
    wait_ticks = (uint16_t)(((uint32_t)milliseconds * 1000UL + 15UL) / 16UL
                           + TIMER0_MAX_TICKS);
}

static void lcd_number(uint8_t start, uint8_t width, uint16_t value)
{
    for (uint8_t i = 0; i < width; ++i) wanted[start + i] = ' ';
    uint8_t position = start + width;
    do
    {
        wanted[--position] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0 && position > start);
}

static void lcd_format(void)
{
    memcpy(wanted, "Afstand:  --- cm", LCD_COLUMNS);
    memcpy(wanted + LCD_COLUMNS, "Freq:    ---- Hz", LCD_COLUMNS);
    if (distance != 0) lcd_number(10, 3, distance);
    lcd_number(LCD_COLUMNS + 9, 4, frequency);
}

static void lcd_retry(void)
{
    init_step = 0;
    cursor = 0xFF;
    memset(shown, 0xFF, sizeof(shown));
    // Een ontbrekend LCD niet bij elke hoofdloop opnieuw aanspreken
    lcd_wait_ms(1000);
}

void lcd_init(uint8_t display7_address)
{
    // Een ACK kan geen onderscheid maken tussen beide soorten displays
    enabled = display7_address != LCD_I2C_ADDRESS;
    init_step = 0;
    cursor = 0xFF;
    distance = 0;
    frequency = 230;
    memset(shown, 0xFF, sizeof(shown));
    lcd_format();
    frame_started = buzzer_get_time_ticks();
    lcd_wait_ms(50);
}

void lcd_set_measurement(uint16_t distance_cm, uint16_t frequency_hz)
{
    distance = distance_cm;
    frequency = frequency_hz;
}

void lcd_update(void)
{
    // Eerst drie keer 8-bit kiezen, daarna overschakelen op 4-bit
    static const uint8_t init_commands[] = {
        0x03, 0x03, 0x03, 0x02,
        0x28, // 4-bit, twee regels
        0x08, // Display uit tijdens instellen
        0x01, // Display wissen
        0x06, // Cursor naar rechts, tekst niet schuiven
        0x0C  // Display aan, cursor uit
    };

    if (!enabled) return;
    uint32_t now = buzzer_get_time_ticks();
    if ((uint32_t)(now - wait_started) < wait_ticks) return;

    if (init_step < sizeof(init_commands))
    {
        uint8_t command = init_commands[init_step];
        bool success;
        if (init_step < 4)
            success = lcd_write_nibble(command);
        else
            success = lcd_write_byte(command, false);

        if (!success)
        {
            lcd_retry();
            return;
        }

        uint16_t delay_ms = 1;
        if (init_step == 0) delay_ms = 5;
        if (init_step >= 4 && command == 0x01) delay_ms = 3;
        lcd_wait_ms(delay_ms);
        ++init_step;

        if (init_step == sizeof(init_commands))
        {
            lcd_format();
            frame_started = buzzer_get_time_ticks();
        }
        return;
    }

    for (uint8_t position = 0; position < LCD_CHARACTERS; ++position)
    {
        if (shown[position] == wanted[position]) continue;

        bool success;
        if (cursor != position)
        {
            uint8_t address = position < LCD_COLUMNS
                            ? position : 0x40 + position - LCD_COLUMNS;
            success = lcd_write_byte(0x80 | address, false);
            if (success) cursor = position;
        }
        else
        {
            success = lcd_write_byte((uint8_t)wanted[position], true);
            if (success)
            {
                shown[position] = wanted[position];
                // Na kolom 16 ligt de volgende regel op een ander LCD-adres
                cursor = position == 15 || position == 31 ? 0xFF : position + 1;
            }
        }

        if (!success) lcd_retry();
        return; // Hoogstens één LCD-byte per ronde van de hoofdloop
    }

    // Maak eerst het huidige beeld af en neem dan de nieuwste meting over
    if ((uint32_t)(now - frame_started) >= LCD_REFRESH_TICKS)
    {
        lcd_format();
        frame_started = now;
    }
}
