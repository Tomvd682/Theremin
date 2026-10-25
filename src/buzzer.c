#include "buzzer.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define BUZZER_PIN PD3

#define MIN_FREQUENCY 230U
#define MAX_FREQUENCY 1400U

static volatile uint8_t duty_cycle = 0;
static volatile uint8_t tone_phase = 0;

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

    // Interrupt bij Compare Match A
    TIMSK0 |= (1 << OCIE0A);
}

// Iedere halve toonperiode schakelen we PWM aan of uit
ISR(TIMER0_COMPA_vect)
{
    if (tone_phase && duty_cycle > 0)
    {
        // PWM-signaal naar de buzzer sturen
        TCCR2A |= (1 << COM2B1);
    }
    else
    {
        // PWM-output uit en pin laag maken
        TCCR2A &= ~(1 << COM2B1);
        PORTD &= ~(1 << BUZZER_PIN);
    }

    tone_phase ^= 1;
}

void buzzer_set_frequency(uint16_t frequency_hz)
{
    if (frequency_hz < MIN_FREQUENCY)
    {
        frequency_hz = MIN_FREQUENCY;
    }

    if (frequency_hz > MAX_FREQUENCY)
    {
        frequency_hz = MAX_FREQUENCY;
    }

    // Timer0 werkt op 16 MHz met prescaler 256
    // Afgerond naar een geldig aantal timer-ticks
    uint32_t denominator = 512UL * frequency_hz;

    uint32_t ticks =
        (16000000UL + denominator - 1UL) / denominator;

    // Binnen de bruikbare frequentiegrenzen houden
    if (ticks < 23)
    {
        ticks = 23;
    }

    if (ticks > 135)
    {
        ticks = 135;
    }

    OCR0A = (uint8_t)(ticks - 1);
}

void buzzer_set_volume(uint8_t volume_percent)
{
    if (volume_percent > 100)
    {
        volume_percent = 100;
    }

    // Percentage omzetten naar 8-bit PWM
    uint8_t pwm_value =
        (uint8_t)(((uint16_t)volume_percent * 255U) / 100U);

    duty_cycle = pwm_value;
    OCR2B = pwm_value;
}