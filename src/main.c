
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdint.h>

#include "volume.h"
#include "buzzer.h"
#include "median_filter.h"
#include "filter_controls.h"
#include "display7.h"
#include "theremin_math.h"

// HC-SR04: TRIG = D9 (PB1), ECHO = D8 (PB0)
// Ingebouwde Arduino LED = D13 (PB5)

#define TRIG_PIN PB1
#define LED_PIN  PB5

#define SENSOR_IDLE       0
#define SENSOR_TRIGGER    1
#define SENSOR_WAIT_RISE  2
#define SENSOR_WAIT_FALL  3
#define SENSOR_COOLDOWN   4

static volatile uint8_t sensor_state = SENSOR_IDLE;
static uint8_t cooldown_count = 0; // Alleen in ISR's die elkaar niet onderbreken
static volatile uint8_t measurement_ready = 0;
static volatile uint16_t measured_ticks = 0;
static uint16_t echo_start_ticks = 0; // Alleen in de Input Capture-ISR gebruikt

void sensor_start(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if (sensor_state == SENSOR_IDLE)
        {
            sensor_state = SENSOR_TRIGGER;

            TCNT1 = 0;

            // Wacht tot ECHO van LOW naar HIGH gaat
            TCCR1B |= (1 << ICES1);

            // Oude interruptflags wissen
            TIFR1 = (1 << ICF1) |
                     (1 << OCF1A) |
                     (1 << TOV1);

            // Bij prescaler 8: 30 ticks = 15 us
            OCR1A = 30;

            PORTB |= (1 << TRIG_PIN);

            // Interrupt om trigger weer laag te maken
            TIMSK1 |= (1 << OCIE1A);
        }
    }
}

// Maak TRIG laag zodra Timer1 de ingestelde triggerduur bereikt
ISR(TIMER1_COMPA_vect)
{
    PORTB &= ~(1 << TRIG_PIN);
    TIMSK1 &= ~(1 << OCIE1A);

    sensor_state = SENSOR_WAIT_RISE;
}

// Timer1 Input Capture: meet de echopuls
ISR(TIMER1_CAPT_vect)
{
    if (sensor_state == SENSOR_WAIT_RISE)
    {
        // Gebruik het bepaalde  tijdstip, niet het moment waarop de ISR begint
        // Zo telt de wachttijd op de ISR niet mee in de afstand
        echo_start_ticks = ICR1;

        // Wacht tot ECHO van HIGH naar LOW gaat
        TCCR1B &= ~(1 << ICES1);
        TIFR1 = (1 << ICF1);

        sensor_state = SENSOR_WAIT_FALL;
    }
    else if (sensor_state == SENSOR_WAIT_FALL)
    {
        measured_ticks = (uint16_t)(ICR1 - echo_start_ticks);
        measurement_ready = 1;

        // Klaarzetten voor de volgende meting
        TCCR1B |= (1 << ICES1);
        TIFR1 = (1 << ICF1);

        TCNT1 = 0;
        TIFR1 = (1 << TOV1);
        cooldown_count = 2;
        sensor_state = SENSOR_COOLDOWN;
    }
}

// Timer1 overflow: timeout en pauze tussen metingen
ISR(TIMER1_OVF_vect)
{
    if (sensor_state == SENSOR_WAIT_RISE ||
        sensor_state == SENSOR_WAIT_FALL)
    {
        // Geen volledige echo ontvangen
        measured_ticks = 0;
        measurement_ready = 1;

        cooldown_count = 2;
        sensor_state = SENSOR_COOLDOWN;
    }
    else if (sensor_state == SENSOR_COOLDOWN)
    {
        if (cooldown_count > 0)
        {
            cooldown_count--;
        }

        if (cooldown_count == 0)
        {
            sensor_state = SENSOR_IDLE;
        }
    }
}

int main(void)
{
    // D9 en D13 als output
    DDRB |= (1 << DDB1) | (1 << DDB5);

    // D8 als input
    DDRB &= ~(1 << DDB0);

    // Trigger en LED uit
    PORTB &= ~((1 << TRIG_PIN) | (1 << LED_PIN));

    // Timer1 in normale modus
    TCCR1A = 0;

    // Prescaler 8 en Input Capture op de stijgende flank
    TCCR1B = (1 << CS11) | (1 << ICES1);

    // Interrupts voor Input Capture en overflow
    TIMSK1 = (1 << ICIE1) | (1 << TOIE1);

    volume_init();
    buzzer_init();
    display7_init();

    // Zonder buffer geeft het filter de meting ongewijzigd terug
    // Het display toont dan 0 en plus kan opnieuw geheugen aanvragen
    (void)median_filter_resize(5);

    filter_controls_init();

    sei();

    // 255 zorgt dat volume en display ook bij de eerste lus worden ingesteld
    // Dat geldt ook als de gewenste waarde 0 is
    uint8_t last_volume = 255;
    uint8_t last_displayed_size = 255;

    while (1)
    {
        filter_controls_update();
        uint8_t current_filter_size = median_filter_get_size();

        if (current_filter_size != last_displayed_size)
        {
            if (display7_show_hex(current_filter_size))
            {
                last_displayed_size = current_filter_size;
            }
        }
        uint8_t current_volume = volume_get_percent();

        if (current_volume != last_volume)
        {
            buzzer_set_volume(current_volume);
            last_volume = current_volume;
        }

        if (sensor_state == SENSOR_IDLE)
        {
            sensor_start();
        }

        uint16_t ticks = 0;
        uint8_t new_measurement = 0;

        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            if (measurement_ready)
            {
                ticks = measured_ticks;
                measurement_ready = 0;
                new_measurement = 1;
            }
        }

        if (new_measurement)
        {
            // Timer1: 1 tick = 0,5 us
            // Afstand = tijd in us / 58
            uint16_t distance_cm = theremin_distance_cm(ticks);

            if (theremin_distance_valid(distance_cm))
            {
                PORTB |= (1 << LED_PIN);

                uint16_t filtered_distance = median_filter_add(distance_cm);

                uint16_t frequency = theremin_frequency_hz(filtered_distance);

                buzzer_set_frequency(frequency);
            }
            else
            {
                // Ongeldige afstand: LED uit
                PORTB &= ~(1 << LED_PIN);

                // Laagste toonhoogte
                buzzer_set_frequency(230);
            }
        }
    }
}
