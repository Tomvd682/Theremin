#include "button_debounce.h"

void button_debounce_init(ButtonDebounce *state, uint8_t buttons)
{
    state->last_change = 0;
    state->stable_buttons = buttons;
    state->active = false;
}

uint8_t button_debounce_update(ButtonDebounce *state, bool changed,
                              uint32_t now, uint8_t buttons)
{
    if (changed)
    {
        state->last_change = now;
        state->active = true;
    }

    // Unsigned aftrekken werkt ook als de klok weer bij 0 begint
    // Er mag dan minder dan 1 volledige klokomloop verstreken zijn
    if (!state->active ||
        (uint32_t)(now - state->last_change) < BUTTON_DEBOUNCE_TICKS)
    {
        return 0;
    }

    state->active = false;
    // Een bit van 1 naar 0 betekent dat de knop is ingedrujt
    uint8_t pressed = state->stable_buttons & (uint8_t)~buttons;
    state->stable_buttons = buttons;
    return pressed;
}
