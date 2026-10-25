#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

void buzzer_init(void);
void buzzer_set_frequency(uint16_t frequency_hz);
void buzzer_set_volume(uint8_t volume_percent);

#endif