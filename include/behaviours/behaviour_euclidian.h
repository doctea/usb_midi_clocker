#pragma once

#ifdef ENABLE_EUCLIDIAN

#include <Arduino.h>
#include "Config.h"
#include "bpm.h"

#include "interfaces/interfaces.h"

#include "behaviours/behaviour_base.h"

#include "sequencer/Euclidian.h"

#include "outputs/output_processor.h"
#include "outputs/output.h"

#include "midi_helpers.h"

extern EuclidianSequencer *sequencer;

class VirtualBehaviour_Euclidian : public DeviceBehaviourUltimateBase {

  public:
    VirtualBehaviour_Euclidian() : DeviceBehaviourUltimateBase () {        
            Serial.println("setting up sequencer.."); Serial_flush();
            setup_output(this);
            Serial.println("instantiating EuclidianSequencer.."); Serial_flush();
            sequencer = new EuclidianSequencer(output_processor->nodes);
            Serial.println("initialise_patterns.."); Serial_flush();
            sequencer->initialise_patterns();
            sequencer->reset_patterns();
            Serial.println("configure sequencer.."); Serial_flush();
            output_processor->configure_sequencer(sequencer);
    }

    const char *get_label() override {
        return "Euclidian Sequencer";
    }

    virtual int getType() override {
        return BehaviourType::virt;
    }

    virtual bool transmits_midi_notes() override { return true; }

    virtual void on_tick(uint32_t ticks) override {
        if (sequencer->is_running()) sequencer->on_tick(ticks);
        if (is_bpm_on_sixteenth(ticks) && output_processor->is_enabled()) {
            output_processor->process();
        }
    };
    virtual void loop(uint32_t ticks) override {
        output_processor->loop();
    }

    #ifdef ENABLE_SCREEN
        virtual LinkedList<MenuItem*> *make_menu_items() override {
            LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();
            
            // todo: make this more nicer
            setup_menu_euclidian(sequencer);
            //setup_sequencer_menu();
            sequencer->make_menu_items(menu);
            Debug_printf("after setup_sequencer_menu, free RAM is %u\n", freeRam());
        
            return menuitems;
        }
    #endif
};

extern VirtualBehaviour_Euclidian *behaviour_euclidian;

#endif