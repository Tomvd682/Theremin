#ifndef THEREMIN_MATH_H
#define THEREMIN_MATH_H

#include <stdbool.h>
#include <stdint.h>

// Timer1 loopt op 2 MHz en de afstand wordt afgerond op hele centimeters
uint16_t theremin_distance_cm(uint16_t echo_ticks);
bool theremin_distance_valid(uint16_t distance_cm);
// Buiten het bereik van 2 tot en met 65 cm gebruiken we de laagste toon
uint16_t theremin_frequency_hz(uint16_t distance_cm);
// Timer0 loopt op 16 MHz / 256 met twee compare matches per toonperiode
uint8_t theremin_timer0_top(uint16_t frequency_hz);
uint8_t theremin_volume_percent(uint8_t adc_high);
uint8_t theremin_volume_pwm(uint8_t volume_percent);

#endif
