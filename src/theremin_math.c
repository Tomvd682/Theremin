#include "theremin_math.h"

uint16_t theremin_distance_cm(uint16_t echo_ticks)
{
    // Reken in 32 bits om overflow op de AVR te voorkomen
    return (uint16_t)(((uint32_t)echo_ticks + 58UL) / 116UL);
}

bool theremin_distance_valid(uint16_t distance_cm)
{
    return distance_cm >= 2U && distance_cm <= 65U;
}

uint16_t theremin_frequency_hz(uint16_t distance_cm)
{
    if (!theremin_distance_valid(distance_cm))
    {
        return 230U;
    }

    return (uint16_t)(1400U - 18U * distance_cm);
}

uint8_t theremin_timer0_top(uint16_t frequency_hz)
{
    if (frequency_hz < 230U)
    {
        frequency_hz = 230U;
    }
    if (frequency_hz > 1400U)
    {
        frequency_hz = 1400U;
    }

    // Twee interrupts per toonperiode met prescaler 256 geven de factor 512
    uint32_t denominator = 512UL * frequency_hz;
    // Rond het aantal timerticks naar boven af
    uint32_t ticks = (16000000UL + denominator - 1UL) / denominator;

    // Deze timergrenzen geven ongeveer 231 tot 1359 Hz
    if (ticks < 23UL)
    {
        ticks = 23UL;
    }
    if (ticks > 135UL)
    {
        ticks = 135UL;
    }

    return (uint8_t)(ticks - 1UL);
}

uint8_t theremin_volume_percent(uint8_t adc_high)
{
    // Tel 127 op om af te ronden op het dichtstbijzijnde percentage
    return (uint8_t)(((uint16_t)adc_high * 100U + 127U) / 255U);
}

uint8_t theremin_volume_pwm(uint8_t volume_percent)
{
    if (volume_percent > 100U)
    {
        volume_percent = 100U;
    }
    return (uint8_t)(((uint16_t)volume_percent * 255U) / 100U);
}
