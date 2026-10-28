#ifndef I2C_H
#define I2C_H

#include <stdbool.h>
#include <stdint.h>

// Gemeten op 28 oktober 2026
// Gebruik 0xFF om automatisch te zoeken binnen 0x20 t/m 0x27
#ifndef DISPLAY7_I2C_ADDRESS
#define DISPLAY7_I2C_ADDRESS 0x21
#endif

// Gemeten op 28 oktober 2026
#ifndef LCD_I2C_ADDRESS
#define LCD_I2C_ADDRESS 0x27
#endif

#if DISPLAY7_I2C_ADDRESS != 0xFF && \
    (DISPLAY7_I2C_ADDRESS < 0x20 || DISPLAY7_I2C_ADDRESS > 0x27)
#error "Het 7-segmentadres moet tussen 0x20 en 0x27 liggen, of 0xFF zijn"
#endif

#if LCD_I2C_ADDRESS < 0x08 || LCD_I2C_ADDRESS > 0x77
#error "Gebruik een geldig 7-bit LCD-adres"
#endif

#if DISPLAY7_I2C_ADDRESS == LCD_I2C_ADDRESS
#error "Het LCD en het 7-segmentdisplay moeten verschillende adressen hebben"
#endif

// Gebruik de bus vanuit main, niet vanuit een ISR
void i2c_init(void);
bool i2c_probe(uint8_t address);
bool i2c_write(uint8_t address, const uint8_t *data, uint8_t length);

#endif
