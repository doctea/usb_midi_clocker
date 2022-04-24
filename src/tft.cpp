#include "Config.h"
#include "bpm.h"
#include "midi_mpk49.h"

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

// by ktownsend from https://forums.adafruit.com/viewtopic.php?t=21536
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}
uint16_t rgb(uint32_t rgb) {
  uint8_t r = rgb>>16;
  uint8_t g = rgb>>8;
  uint8_t b = rgb & 0b11111111;
  return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

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
  //testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
}

void tft_clear() {
    tft.fillScreen(ST77XX_BLACK);
}

void tft_print (char *text) {
    tft.print(text);
}

void tft_update(int ticks) {
    long t = millis();
    tft.setCursor(0,0);
    tft.setTextSize(2);
    if (playing) {
        tft.setTextColor(rgb(0, 0xFF, 0), 0x00);
    } else {
        tft.setTextColor(rgb(0, 0, 0xFF), 0x00);
    }
    tft.printf("%04i:%02i:%02i @ %03.2f\n", 
        (ticks / (PPQN*4*4)) + 1, 
        (ticks % (PPQN*4*4) / (PPQN*4)) + 1,
        (ticks % (PPQN*4) / PPQN) + 1,
        bpm_current
    );

    #if defined(ENABLE_MPK49) && defined(ENABLE_RECORDING)
        if (mpk49_recording) {
            tft.setTextColor(rgb(0xFF,0,0),0);
            tft.print("[Rec]");
        } else {
            tft.setTextColor(0xFF,0);
            tft.print("     ");
        }
        if (mpk49_playing) {
            tft.setTextColor(rgb(0x00,0xFF,0x00),0);
            tft.print("[>>]");
        } else {
            tft.setTextColor(rgb(0x00,0x00,0xFF),0);
            tft.print("[##]");
        }
        tft.print("\n");
    #endif

    //tft.printf("ticks: %i", ticks);

    static int f = 0;
    /*if (f++%100==0)
        Serial.printf("gfx took %i\n", millis()-t);*/
} 

#endif