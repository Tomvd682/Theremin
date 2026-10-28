#ifndef DISPLAY7_H
#define DISPLAY7_H

#include <stdbool.h>
#include <stdint.h>

// Stel eerst de gedeelde I2C-bus in
void display7_init(void);
uint8_t display7_get_address(void);
bool display7_show_hex(uint8_t value);

#endif
