#include "ram_stuff.h"

#include <Arduino.h>

int psram_clocked_at = 88; // in MHz

// thanks to beermat again (and originally KurtE) for this code
// at 198mhz this runs and gives an approx 25% FPS speed up - from ~14fps up to ~19fps!
const float flexspi2_clock_speeds[4] = {396.0f, 720.0f, 664.62f, 528.0f};
FLASHMEM void setPSRamSpeed(int mhz) {
  //See what the closest setting might be:
  uint8_t clk_save = 0, divider_save = 0;
  int min_delta = mhz;
  for (uint8_t clk = 0; clk < 4; clk++) {
      uint8_t divider = (flexspi2_clock_speeds[clk] + (mhz / 2)) / mhz;
      int delta = abs(mhz - flexspi2_clock_speeds[clk] / divider);
      if ((delta < min_delta) && (divider < 8)) {
          min_delta = delta;
          clk_save = clk;
          divider_save = divider;
      }
  }
  //First turn off FLEXSPI2
  CCM_CCGR7 &= ~CCM_CCGR7_FLEXSPI2(CCM_CCGR_ON);
  divider_save--; // 0 biased
  //Set the clock settings.
  CCM_CBCMR = (CCM_CBCMR & ~(CCM_CBCMR_FLEXSPI2_PODF_MASK | CCM_CBCMR_FLEXSPI2_CLK_SEL_MASK))
              | CCM_CBCMR_FLEXSPI2_PODF(divider_save) | CCM_CBCMR_FLEXSPI2_CLK_SEL(clk_save);
  //Turn FlexSPI2 clock back on
  CCM_CCGR7 |= CCM_CCGR7_FLEXSPI2(CCM_CCGR_ON);

  Serial.printf("Update FLEXSPI2 speed: %u clk:%u div:%u Actual:%u\n", mhz, clk_save, divider_save,
      flexspi2_clock_speeds[clk_save] / (divider_save + 1));

  psram_clocked_at = flexspi2_clock_speeds[clk_save] / (divider_save + 1); // save the clock speed for later use
}


void setup_psram_overclock() {
  setPSRamSpeed(PSRAM_SPEED);
}
