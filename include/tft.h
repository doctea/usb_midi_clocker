#include <Adafruit_GFX.h>    // Core graphics library
#include <ST7789_t3.h> // Hardware-specific library for ST7789 on Teensy

void setup_tft(void);
void tft_update(int ticks);
void tft_print (const char *text);
void tft_clear();
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b);
uint16_t rgb(uint32_t rgb);

void tft_header(ST7789_t3 *tft, const char *text);

extern ST7789_t3 tft;