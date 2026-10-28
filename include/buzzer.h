#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

#define BUZZER_TIME_TICK_US 16UL

void buzzer_init(void);
void buzzer_set_frequency(uint16_t frequency_hz);
void buzzer_set_volume(uint8_t volume_percent);

// De klok loopt per tooninterrupt vooruit, in ticks van 16 us
uint32_t buzzer_get_time_16us_ticks(void);

#endif
