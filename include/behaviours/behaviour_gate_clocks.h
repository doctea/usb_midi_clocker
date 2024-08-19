// CV clock outputs

#pragma once

#include <Arduino.h>
#include "Config.h"
#include "bpm.h"

#include "interfaces/interfaces.h"

#include "behaviours/behaviour_base.h"
class VirtualBehaviour_ClockGates : public DeviceBehaviourUltimateBase {
  GateManager *gate_manager;
  int bank = -1;

  public:
    VirtualBehaviour_ClockGates(GateManager *gate_manager, int bank = BANK_CLOCK) : DeviceBehaviourUltimateBase () {
      this->gate_manager = gate_manager;
      this->bank = bank;
    }

    virtual int getType() override {
      return BehaviourType::virt;
    }

    virtual void on_tick(uint32_t ticks) override {
      this->update_cv_outs(ticks);
    };

    #if defined(ENABLE_CLOCKS) && defined(PROTOTYPE)
    const byte cv_out_clock_pin[NUM_CLOCKS] = {
        PIN_CLOCK_1, PIN_CLOCK_2, PIN_CLOCK_3, PIN_CLOCK_4,
        #if NUM_CLOCKS > 4
          PIN_CLOCK_5, 
        #endif
        #if NUM_CLOCKS > 5
          PIN_CLOCK_6, 
        #endif
        #if NUM_CLOCKS > 6
          PIN_CLOCK_7,
        #endif
        #if NUM_CLOCKS > 7
          PIN_CLOCK_8
        #endif
    };
    #endif

    #define CLOCK_MULTIPLIER_MIN  0.25
    #define CLOCK_MULTIPLIER_MAX  16.0
    #define CLOCK_DELAY_MAX 15

    int duration = PPQN/8;

    #define NUM_CLOCK_MULTIPLIER_VALUES 9
    float clock_multiplier_values[NUM_CLOCK_MULTIPLIER_VALUES] = {
      0.25,     // 0 - 16th notes
      0.5,      // 1 - 8th notes
      1,        // 2 - quarter notes
      2,        // 3 - every two beats
      4,        // 4 - every four beats    (1 bar)
      8,        // 5 - every eight beats   (2 bars)
      16,       // 6 - every sixteen beats (4 bars, 1 phrase)
      32,       // 7 - every thirty two beats (8 bars, 2 phrases)
      64,       // 8 - clock is completely off (would be every 64 beats, except we don't have enough colours to represent this on the apcmini display)
    };
    #define CLOCK_MULTIPLIER_OFF        64.0  // if clock multipler is set to this value, then actually turn it off completely
    
    void setup_cv_output();
    float get_clock_multiplier(byte i);
    void increase_clock_multiplier(byte i);
    void decrease_clock_multiplier(byte i); 
    bool is_clock_off(byte i);
    byte get_clock_delay(byte i);
    void decrease_clock_delay(byte clock_selected, byte amount = 1);
    void increase_clock_delay(byte clock_selected, byte amount = 1);
    bool should_trigger_clock(unsigned long ticks, byte i, byte offset = 0);
    void update_cv_outs(unsigned long ticks);

    void cv_out_clock_pin_off(byte i) {
      gate_manager->send_gate(this->bank, i, LOW);
    }
    void cv_out_clock_pin_on(byte i) {
      gate_manager->send_gate(this->bank, i, HIGH);
    }

};


extern VirtualBehaviour_ClockGates *behaviour_clock_gates;