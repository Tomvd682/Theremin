#ifndef BUTTON_DEBOUNCE_H
#define BUTTON_DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>

// 20 ms bij een klok van 16 us per tick
#define BUTTON_DEBOUNCE_TICKS 1250UL

typedef struct
{
    uint32_t last_change;
    uint8_t stable_buttons;
    bool active;
} ButtonDebounce;

// state moet naar een geldig object wijzen, niet naar NULL
void button_debounce_init(ButtonDebounce *state, uint8_t buttons);
// Geef de bits terug van knoppen die stabiel zijn ingedrukt
// Een ingedrukte knop is LOW
// Lees changed, now en buttons samen in een atomic block
uint8_t button_debounce_update(ButtonDebounce *state, bool changed,
                              uint32_t now, uint8_t buttons);

#endif
