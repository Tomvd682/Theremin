#include <avr/io.h>

int main(void)
{
    // Ingebouwde LED op Arduino pin 13 als output
    DDRB |= (1 << DDB5);

    // Zet de LED aan
    PORTB |= (1 << PORTB5);

    while (1)
    {
        // Hoofdprogramma
    }

    return 0;
}