#include "Config.h"

#ifdef ENABLE_SCREEN

#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <ST7789_t3.h> // Hardware-specific library for ST7789

#include <SPI.h>

#define TFT_CS        10
#define TFT_RST        6 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         9 

//Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
ST7789_t3 tft = ST7789_t3(TFT_CS, TFT_DC, TFT_RST);


void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void setup_tft(void) {
  tft.init(135, 240);           // Init ST7789 240x135

  //tft.setSPISpeed(30000000); //40000000);
                  //30000000

  tft.fillScreen(ST77XX_BLACK);
  //time = millis() - time;

  //Serial.println(time, DEC);
  delay(500);

  // large block of text
  tft.fillScreen(ST77XX_BLACK);
  testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
}

void tft_update(int ticks) {
    long t = millis();
    tft.setCursor(0,0);
    tft.setTextSize(3);
    tft.setTextColor(0xFF, 0x00);
    tft.printf("ticks: %i", ticks);
    static int f = 0;
    if (f++%100==0)
        Serial.printf("gfx took %i\n", millis()-t);
} 

#endif