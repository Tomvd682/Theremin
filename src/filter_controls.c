#include "filter_controls.h"
#include "median_filter.h"
#include "buzzer.h"
#include "button_debounce.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdint.h>

#define BUTTON_PLUS  PD4
#define BUTTON_MINUS PD5

#define BUTTON_MASK ((1 << BUTTON_PLUS) | (1 << BUTTON_MINUS))

static volatile uint8_t buttons_changed = 0;

static ButtonDebounce debounce;

void filter_controls_init(void)
{
    // Knoppen als input
    DDRD &= ~BUTTON_MASK;

    // Interne pull-upweerstanden inschakelen
    PORTD |= BUTTON_MASK;

    // Een ingedrukte knop bij opstart telt niet als nieuwe druk
    button_debounce_init(&debounce, PIND & BUTTON_MASK);

    // Pin Change Interrupts voor PD4 en PD5
    PCMSK2 |= (1 << PCINT20) | (1 << PCINT21);
    PCICR |= (1 << PCIE2);
}

// Laat de hoofdloop de knopdruk verwerken
ISR(PCINT2_vect)
{
    buttons_changed = 1;
}

void filter_controls_update(void)
{
    uint8_t changed = 0;
    uint8_t current_buttons;
    uint32_t now;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        changed = buttons_changed;
        buttons_changed = 0;
        current_buttons = PIND & BUTTON_MASK;
        now = buzzer_get_time_16us_ticks();

        // Een knop kan ook veranderen terwijl interrupts uitstaan
        // Controleer drm de hardwareflag en laat de ISR die afhandelen
        if (PCIFR & (1 << PCIF2)) changed = 1;
    }

    uint8_t pressed = button_debounce_update(&debounce, changed != 0,
                                            now, current_buttons);

    uint8_t size = median_filter_get_size();

    // Beide knoppen tegelijk geven geen stap
    if (pressed == (1 << BUTTON_PLUS))
    {
        if (size == 0)
        {
            // Probeer opnieuw geheugen aan te vragen als dat bij opstart mislukte
            (void)median_filter_resize(1);
        }
        else if (size <= 13)
        {
            (void)median_filter_resize(size + 2);
        }
    }

    else if (pressed == (1 << BUTTON_MINUS))
    {
        if (size >= 3 && size <= 15)
        {
            (void)median_filter_resize(size - 2);
        }
    }
}
