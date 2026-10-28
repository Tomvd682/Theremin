#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// Aanname voor de adapter, nog controleren met het schema of de hardware
#define LCD_RS_MASK        0x01U
#define LCD_RW_MASK        0x02U
#define LCD_ENABLE_MASK    0x04U
#define LCD_BACKLIGHT_MASK 0x08U
#define LCD_D4_MASK        0x10U
#define LCD_D5_MASK        0x20U
#define LCD_D6_MASK        0x40U
#define LCD_D7_MASK        0x80U

// Gebruik 0 als de achtergrondverlichting actief LOW is
#define LCD_BACKLIGHT_ON LCD_BACKLIGHT_MASK

void lcd_init(uint8_t display7_address);
// Afstand 0 toont streepjes, frequentie is de berekende aanvraag aan de buzzer
void lcd_set_measurement(uint16_t distance_cm, uint16_t frequency_hz);
// Roep dit steeds vanuit de hoofdloop aan, nooit vanuit een ISR
void lcd_update(void);

#endif
