#pragma once

#include <Arduino.h>
#include "Config.h" 

#ifdef ENABLE_TURINGMACHINE

#include "bpm.h"

#include "behaviours/behaviour_base.h"

#include "sequencer/TuringMachine/TuringMachinePattern.h"
#include "sequencer/Base/Sequencer.h"
#include "outputs/output_processor.h"
#include "outputs/output.h"

#include "midi/midi_mapper_matrix_manager.h"

#ifdef USE_UCLOCK
    #include "uclock.h"
#endif

#ifdef ENABLE_SHUFFLE
    void turingmachine_shuffled_callback(uint32_t step, uint8_t track);
#endif

class VirtualBehaviour_TuringMachine : virtual public DeviceBehaviourUltimateBase {
  SimpleSequencer *sequencer = nullptr;
  MIDIOutputProcessor *output_processor = nullptr;

  public:
    source_id_t source_id_2 = -1;
    source_id_t source_id_3 = -1;
    source_id_t source_id_4 = -1;

    const int TURINGMACHINE_CHANNEL_1 = 1;
    const int TURINGMACHINE_CHANNEL_2 = 2;
    const int TURINGMACHINE_CHANNEL_3 = 3;
    const int TURINGMACHINE_CHANNEL_4 = 4;

    VirtualBehaviour_TuringMachine() : DeviceBehaviourUltimateBase () {
        this->output_processor = new MIDIOutputProcessor(this);
        this->sequencer = new SimpleSequencer(output_processor->get_available_outputs());
        
        // Create TuringMachine pattern(s)
        TuringMachinePattern *tm_pattern = new TuringMachinePattern(output_processor->get_available_outputs());
        tm_pattern->set_path_segment("pattern_0");
        tm_pattern->set_steps(16);
        this->sequencer->add_pattern(tm_pattern);

        // Add output nodes for multi-source routing via MIDI channels
        this->output_processor->addNode(new MIDINoteOutput("TuringMachine_Ch1", this, TURINGMACHINE_CHANNEL_1));
        this->output_processor->addNode(new MIDINoteOutput("TuringMachine_Ch2", this, TURINGMACHINE_CHANNEL_2));
        this->output_processor->addNode(new MIDINoteOutput("TuringMachine_Ch3", this, TURINGMACHINE_CHANNEL_3));
        this->output_processor->addNode(new MIDINoteOutput("TuringMachine_Ch4", this, TURINGMACHINE_CHANNEL_4));

        output_processor->configure_sequencer(sequencer);
        // Note: SimpleSequencer doesn't have initialise_patterns() or reset_patterns()
        // Those methods are specific to EuclidianSequencer
        output_processor->setup_parameters();

        #ifdef USE_UCLOCK
            #ifdef ENABLE_SHUFFLE
                uClock.setOnStep(turingmachine_shuffled_callback, 1);  // 1 shuffle track for TuringMachine
            #endif
        #endif
    }

    virtual const char *get_label() override {
        return (const char*)"Turing Machine";
    }

    virtual int getType() override {
        return BehaviourType::virt;
    }

    virtual bool transmits_midi_notes() override { return true; }

    #ifdef ENABLE_SHUFFLE
        virtual void on_step_shuffled(uint8_t track, uint32_t step) {
            if (this->debug) Serial.printf(F("behaviour_turingmachine#on_step_shuffled(%i, %i)\n"), track, step);
            sequencer->on_step_shuffled(track, step);
        }
    #endif

    virtual void on_tick(uint32_t ticks) override {
        if (sequencer->is_running()) 
            sequencer->on_tick(ticks);
        if (output_processor->is_enabled())
            output_processor->process();
    };

    virtual void loop(uint32_t ticks) override {
        output_processor->loop();
        sequencer->on_loop(ticks);
    }

    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (this->debug) Serial.printf(F("behaviour_turingmachine#sendNoteOn(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i\n"), channel, note, velocity, source_id);
        
        // Route to appropriate matrix source based on channel
        if (channel == TURINGMACHINE_CHANNEL_1) {
            midi_matrix_manager->processNoteOn(this->source_id, note, velocity, channel);
        } else if (channel == TURINGMACHINE_CHANNEL_2) {
            midi_matrix_manager->processNoteOn(this->source_id_2, note, velocity);
        } else if (channel == TURINGMACHINE_CHANNEL_3) {
            midi_matrix_manager->processNoteOn(this->source_id_3, note, velocity);
        } else if (channel == TURINGMACHINE_CHANNEL_4) {
            midi_matrix_manager->processNoteOn(this->source_id_4, note, velocity);
        } else {
            midi_matrix_manager->processNoteOn(this->source_id, note, velocity);
        }
    }

    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (this->debug) Serial.printf(F("behaviour_turingmachine#sendNoteOff(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i\n"), channel, note, velocity, source_id);
        
        // Route to appropriate matrix source based on channel
        if (channel == TURINGMACHINE_CHANNEL_1) {
            midi_matrix_manager->processNoteOff(this->source_id, note, velocity, channel);
        } else if (channel == TURINGMACHINE_CHANNEL_2) {
            midi_matrix_manager->processNoteOff(this->source_id_2, note, velocity);
        } else if (channel == TURINGMACHINE_CHANNEL_3) {
            midi_matrix_manager->processNoteOff(this->source_id_3, note, velocity);
        } else if (channel == TURINGMACHINE_CHANNEL_4) {
            midi_matrix_manager->processNoteOff(this->source_id_4, note, velocity);
        } else {
            midi_matrix_manager->processNoteOff(this->source_id, note, velocity);
        }
    }


    bool already_initialised = false;
    virtual ParameterList *initialise_parameters() override {
        Serial.printf("%s#initialise_parameters()...", this->get_label());
        if (already_initialised && this->parameters!=nullptr)
            return this->parameters;

        DeviceBehaviourUltimateBase::initialise_parameters();

        // Fetch parameters from each pattern and add to behaviour's parameter list
        for (unsigned int i = 0 ; i < sequencer->get_number_patterns() ; i++) {
            ParameterList *pattern_parameters = sequencer->get_pattern(i)->getParameters(i);
            if (pattern_parameters != nullptr) {
                for (auto* p : *pattern_parameters) {
                    this->parameters->add(p);
                }
            }
        }

        output_processor->setup_parameters();

        already_initialised = true;

        return parameters;
    }

    #ifdef ENABLE_SCREEN
        virtual MenuItemList *make_menu_items() override {
            MenuItemList *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

            this->sequencer->make_menu_items(
                menu,
                0xFF,   // no combine pages for TuringMachine
                "TuringMachine"                  
            );
            this->output_processor->create_menu_items(true, "TuringMachine outputs", "TuringMachine");

            return menuitems;
        }

        virtual bool show_dedicated_parameters_page() {
            return false;
        }
    #endif

    virtual void setup_saveable_settings() override {
        DeviceBehaviourUltimateBase::setup_saveable_settings();
        // Register sequencer as a child; sl_setup_all will call sequencer->setup_saveable_settings()
        register_child(this->sequencer);
        register_child(this->output_processor);
    }

};


extern VirtualBehaviour_TuringMachine *behaviour_turingmachine;

#endif