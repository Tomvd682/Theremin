#include "volume.h"
#include "theremin_math.h"
#include <avr/io.h>
#include <avr/interrupt.h>

static volatile uint8_t volume_raw = 0;

void volume_init(void)
{
    // AVCC als referentie, links uitgelijnd, ADC0
    ADMUX = (1 << REFS0) | (1 << ADLAR);

    // Laat de ADC steeds opnieuw meten
    ADCSRB = 0;

    // Digitale inputbuffer op PC0 uitschakelen
    DIDR0 |= (1 << ADC0D);

    // ADC aan, auto trigger, interrupt en prescaler 128
    ADCSRA = (1 << ADEN)
           | (1 << ADATE)
           | (1 << ADIE)
           | (1 << ADPS2)
           | (1 << ADPS1)
           | (1 << ADPS0);

    // Eerste conversie starten
    ADCSRA |= (1 << ADSC);
}

// Bewaar het resultaat zodra de ADC-meting klaar is
ISR(ADC_vect)
{
    volume_raw = ADCH;
}

uint8_t volume_get_percent(void)
{
    return theremin_volume_percent(volume_raw);
}
