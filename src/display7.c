#include "display7.h"

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#define TWI_TIMEOUT 30000U

static uint8_t display_address = 0xFF;

static bool twi_wait(void)
{
    uint16_t timeout = TWI_TIMEOUT;

    while (!(TWCR & (1 << TWINT)))
    {
        if (--timeout == 0)
        {
            return false;
        }
    }

    return true;
}

static void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

static bool twi_start(uint8_t address)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    if (!twi_wait())
    {
        twi_stop();
        return false;
    }

    uint8_t status = TWSR & 0xF8;

    if (status != 0x08 && status != 0x10)
    {
        twi_stop();
        return false;
    }

    TWDR = address << 1;
    TWCR = (1 << TWINT) | (1 << TWEN);

    if (!twi_wait())
    {
        twi_stop();
        return false;
    }

    if ((TWSR & 0xF8) != 0x18)
    {
        twi_stop();
        return false;
    }

    return true;
}

static bool twi_write(uint8_t value)
{
    TWDR = value;
    TWCR = (1 << TWINT) | (1 << TWEN);

    if (!twi_wait())
    {
        return false;
    }

    return (TWSR & 0xF8) == 0x28;
}

void display7_init(void)
{
    // SDA = PC4, SCL = PC5
    DDRC &= ~((1 << DDC4) | (1 << DDC5));

    // Interne pull-ups
    PORTC |= (1 << PORTC4) | (1 << PORTC5);

    // I2C op ongeveer 100 kHz
    TWSR = 0;
    TWBR = 72;
    TWCR = (1 << TWEN);

    // Zoek het adres van de PCF8574
    display_address = 0xFF;

    for (uint8_t address = 0x20; address <= 0x27; address++)
    {
        if (twi_start(address))
        {
            display_address = address;
            twi_stop();
            break;
        }
    }
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

    if (!twi_start(display_address))
    {
        return false;
    }

    // De segmenten zijn actief LOW
    uint8_t pattern = (uint8_t)~hex_segments[value];

    bool success = twi_write(pattern);
    twi_stop();

    return success;
}