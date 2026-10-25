#include "buzzer.h"
#include "theremin_math.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/atomic.h>

#define BUZZER_PIN PD3

static volatile uint8_t duty_cycle = 0;
static uint8_t tone_phase = 0;
static volatile uint8_t requested_top = 134;
static volatile uint32_t time_16us_ticks = 0; // Tijd in stappen van 16 us

void buzzer_init(void)
{
    // D3 (OC2B) als output
    DDRD |= (1 << DDD3);
    PORTD &= ~(1 << BUZZER_PIN);

    // Timer2: Fast PWM, prescaler 1 (62,5 kHz)
    // PWM-output begint uitgeschakeld
    TCCR2A = (1 << WGM21) | (1 << WGM20);
    TCCR2B = (1 << CS20);
    OCR2B = 0;

    // Timer0: CTC, prescaler 256
    // Begint met ongeveer 230 Hz
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS02);
    OCR0A = 134;

    // Timer0-interrupt bij compare match A
    TIMSK0 |= (1 << OCIE0A);

}


// Schakel PWM elke halve toonperiode aan of uit
ISR(TIMER0_COMPA_vect)
{
    // Tijd bijhouden voor de debounce van knoppen
    time_16us_ticks += (uint16_t)OCR0A + 1U;

    // Pas TOP na de compare match aan om geen match over te slaan
    // Tel eerst de oude periode bij de klok op
    OCR0A = requested_top;

    if (tone_phase && duty_cycle > 0)
    {
        // PWM signaal naar de buzzee
        TCCR2A |= (1 << COM2B1);
    }
    else
    {
        // PWM output uit en pin laag maken
        TCCR2A &= ~(1 << COM2B1);
        PORTD &= ~(1 << BUZZER_PIN);
    }

    tone_phase ^= 1;
}

void buzzer_set_frequency(uint16_t frequency_hz)
{
    requested_top = theremin_timer0_top(frequency_hz);
}

void buzzer_set_volume(uint8_t volume_percent)
{
    uint8_t pwm_value = theremin_volume_pwm(volume_percent);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        duty_cycle = pwm_value;
        OCR2B = pwm_value;
    }
}

uint32_t buzzer_get_time_ticks(void)
{
    uint32_t ticks;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ticks = time_16us_ticks;
    }

    return ticks;
}
