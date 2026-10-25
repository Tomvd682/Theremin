#ifndef MEDIAN_FILTER_H
#define MEDIAN_FILTER_H

#include <stdint.h>
#include <stdbool.h>

// Alleen vanuit main aanroepen, niet vanuit een ISR
// Alleen oneven groottes van 1 tot en met 15
// Bij een fout blijven de oude buffer en metingen bewaard
bool median_filter_resize(uint8_t new_size);
// Bewaar de nieuwste metingen en geef de mediaan terug
// Kies bij een even aantal metingen de onderste middelste waarde
// Zonder buffer wordt value ongewijzigd teruggegeven
uint16_t median_filter_add(uint16_t value);
uint8_t median_filter_get_size(void);
// Herhaald opruimen is veilig
// Na opruimen werkt add() zonder filter tot een geldige resize
void median_filter_free(void);

#endif
