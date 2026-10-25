#ifndef MEDIAN_FILTER_H
#define MEDIAN_FILTER_H

#include <stdint.h>
#include <stdbool.h>

bool median_filter_resize(uint8_t new_size);
uint16_t median_filter_add(uint16_t value);
uint8_t median_filter_get_size(void);
void median_filter_free(void);

#endif