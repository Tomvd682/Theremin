#include "filter_controls.h"
#include "median_filter.h"
#include "buzzer.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdint.h>

#define BUTTON_PLUS  PD4
#define BUTTON_MINUS PD5

#define BUTTON_MASK ((1 << BUTTON_PLUS) | (1 << BUTTON_MINUS))

// 20 ms bij 16 us per tijdstap
#define DEBOUNCE_TICKS 1250UL

static volatile uint8_t buttons_changed = 0;

static uint8_t stable_buttons = BUTTON_MASK;
static uint8_t debounce_active = 0;
static uint32_t debounce_deadline = 0;

void filter_controls_init(void)
{
    // Knoppen als input
    DDRD &= ~BUTTON_MASK;

    // Interne pull-upweerstanden inschakelen
    PORTD |= BUTTON_MASK;

    // Huidige toestand bewaren
    stable_buttons = PIND & BUTTON_MASK;

    // Pin Change Interrupts voor PD4 en PD5
    PCMSK2 |= (1 << PCINT20) | (1 << PCINT21);
    PCICR |= (1 << PCIE2);
}

// Interrupt bij verandering van een knop
ISR(PCINT2_vect)
{
    // Alleen doorgeven dat een knop veranderde
    buttons_changed = 1;
}

void filter_controls_update(void)
{
    uint8_t changed = 0;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        changed = buttons_changed;
        buttons_changed = 0;
    }

    uint32_t now = buzzer_get_time_ticks();

    if (changed)
    {
        // Wacht tot de knop 20 ms niet meer verandert
        debounce_deadline = now + DEBOUNCE_TICKS;
        debounce_active = 1;
    }

    if (!debounce_active)
    {
        return;
    }

    if ((int32_t)(now - debounce_deadline) < 0)
    {
        return;
    }

    debounce_active = 0;

    uint8_t current_buttons = PIND & BUTTON_MASK;

    // Pull-up: ingedrukte knop is LOW
    uint8_t pressed =
        stable_buttons & (uint8_t)~current_buttons;

    stable_buttons = current_buttons;

    uint8_t size = median_filter_get_size();

    // Plusknop: filter vergroten
    if ((pressed & (1 << BUTTON_PLUS)) &&
        !(pressed & (1 << BUTTON_MINUS)))
    {
        if (size >= 1 && size <= 13)
        {
            median_filter_resize(size + 2);
        }
    }

    // Minknop: filter verkleinen
    if ((pressed & (1 << BUTTON_MINUS)) &&
        !(pressed & (1 << BUTTON_PLUS)))
    {
        if (size >= 3 && size <= 15)
        {
            median_filter_resize(size - 2);
        }
    }
}