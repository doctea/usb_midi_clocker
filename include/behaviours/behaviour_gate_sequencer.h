// CV clock outputs

#pragma once

#include <Arduino.h>
#include "Config.h"
#include "bpm.h"

#include "interfaces/interfaces.h"

#include "behaviours/behaviour_base.h"
class VirtualBehaviour_SequencerGates : public DeviceBehaviourUltimateBase {
  GateManager *gate_manager;
  int bank = -1;

  public:
    VirtualBehaviour_SequencerGates(GateManager *gate_manager, int bank = BANK_SEQ) : DeviceBehaviourUltimateBase () {
      this->gate_manager = gate_manager;
      this->bank = bank;
    }

    virtual int getType() override {
      return BehaviourType::virt;
    }

    virtual void on_pre_clock(uint32_t ticks) override {
      this->update_cv_outs(ticks);
    };

    #define NUM_STEPS     8

    #define SEQUENCER_MAX_VALUE 6

    int duration = PPQN/8;

    void sequencer_clear_pattern();
    void sequencer_clear_row(byte row);

    void init_sequence();
    byte read_sequence(byte row, byte col);
    void write_sequence(byte row, byte col, byte value);
    void sequencer_press(byte row, byte col, bool shift = false);
    bool should_trigger_sequence(unsigned long ticks, byte sequence, int offset = 0);

    void raw_write_pin(int,int);

    void update_cv_outs(unsigned long ticks);

    void cv_out_sequence_pin_off(byte i) {
      gate_manager->send_gate(this->bank, i, LOW);
    }
    void cv_out_sequence_pin_on(byte i) {
      gate_manager->send_gate(this->bank, i, HIGH);
    }

};

extern VirtualBehaviour_SequencerGates *behaviour_sequencer_gates;