#include "Config.h"

#ifdef ENABLE_SCREEN

#include "bpm.h"
#include "midi_mpk49.h"

#include "tft.h"
#include "multi_usb_handlers.h"

#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <ST7789_t3.h> // Hardware-specific library for ST7789 on Teensy

#include <SPI.h>


#define PIN_BUTTON_A  4
#define PIN_BUTTON_B  5
#define ENCODER_KNOB_L  2
#define ENCODER_KNOB_R  3

#define MAX_KNOB 1024

Encoder knob(ENCODER_KNOB_L, ENCODER_KNOB_R);
Bounce pushButtonA = Bounce(PIN_BUTTON_A, 10); // 10ms debounce
Bounce pushButtonB = Bounce(PIN_BUTTON_B, 10); // 10ms debounce

#include "menu.h"

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

    //todo: move this to menu
  pinMode(PIN_BUTTON_A, INPUT_PULLUP);
  pinMode(PIN_BUTTON_B, INPUT_PULLUP);

  setup_menu();
}

void tft_clear() {
    tft.fillScreen(ST77XX_BLACK);
}

void tft_print (const char *text) {
    tft.print(text);
}

void tft_header(ST7789_t3 *tft, const char *text) {
    tft->setTextColor(0xFFFFFF,0);
    tft->setTextSize(0);
    tft->println(text);
}

/*void button_pressed(byte button) {
    if (button==PIN_BUTTON_A) {

    } else if (button==PIN_BUTTON_B) {

    }
}*/

void tft_update(int ticks) {
    static unsigned long last_updated_tft;
    if (millis()-last_updated_tft<20) { // maximum 50 fps
        return;
    }
    last_updated_tft = millis();

    menu.display(); //&tft);

    //return;

    /*
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
    );*/

    /*
    #if defined(ENABLE_MPK49) && defined(ENABLE_RECORDING)
        mpk49_display_looper_status(&tft);
    #endif

    #if defined(ENABLE_BEATSTEP)
        beatstep_display_key_status(&tft);
    #endif
*/
    #ifdef ENABLE_USB
        display_usb_device_list(&tft);
    #endif


    //tft.printf("ticks: %i", ticks);

    static int f = 0;
    /*if (f++%100==0)
        Serial.printf("gfx took %i\n", millis()-t);*/
} 

#endif