#include "i2c.h"

#include <avr/io.h>
#include <stddef.h>
#include <util/delay.h>

#define I2C_WAIT_LIMIT 1000U

static bool i2c_wait(uint16_t *remaining)
{
    while (!(TWCR & (1 << TWINT)))
    {
        if (*remaining == 0) return false;
        --(*remaining);
        _delay_us(1);
    }
    return true;
}

static bool i2c_stop(uint16_t *remaining)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    while (TWCR & (1 << TWSTO))
    {
        if (*remaining == 0)
        {
            // Laat de bus los als een apparaat de overdracht tegenhoudt
            TWCR = 0;
            TWCR = (1 << TWEN);
            return false;
        }
        --(*remaining);
        _delay_us(1);
    }
    return true;
}

static bool i2c_start(uint8_t address, uint16_t *remaining)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    if (!i2c_wait(remaining)) return false;

    uint8_t status = TWSR & 0xF8;
    if (status != 0x08 && status != 0x10) return false;

    TWDR = (uint8_t)(address << 1);
    TWCR = (1 << TWINT) | (1 << TWEN);
    if (!i2c_wait(remaining)) return false;

    return (TWSR & 0xF8) == 0x18;
}

static bool i2c_transfer(uint8_t address, const uint8_t *data, uint8_t length)
{
    if (address < 0x08 || address > 0x77) return false;
    if (length > 0 && data == NULL) return false;

    // Eén wachtbudget voor de hele overdracht, ook voor STOP
    uint16_t remaining = I2C_WAIT_LIMIT;
    bool success = i2c_start(address, &remaining);

    for (uint8_t i = 0; success && i < length; ++i)
    {
        TWDR = data[i];
        TWCR = (1 << TWINT) | (1 << TWEN);
        success = i2c_wait(&remaining);
        if (success) success = (TWSR & 0xF8) == 0x28;
    }

    bool stopped = i2c_stop(&remaining);
    return success && stopped;
}

void i2c_init(void)
{
    // Beide displays delen SDA op PC4 en SCL op PC5
    DDRC &= ~((1 << DDC4) | (1 << DDC5));
    PORTC |= (1 << PORTC4) | (1 << PORTC5);
    TWSR = 0;
    TWBR = 72; // 100 kHz bij 16 MHz
    TWCR = (1 << TWEN);
}

bool i2c_probe(uint8_t address)
{
    return i2c_transfer(address, NULL, 0);
}

bool i2c_write(uint8_t address, const uint8_t *data, uint8_t length)
{
    return i2c_transfer(address, data, length);
}
