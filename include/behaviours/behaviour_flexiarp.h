#pragma once

#include <Arduino.h>
#include "Config.h" 

#ifdef ENABLE_FLEXIARP

#include "bpm.h"

#include "behaviours/behaviour_base.h"

#include "sequencer/Euclidian.h"
#include "outputs/output_flexiarp.h"
#include "outputs/output_processor.h"
#include "outputs/output.h"

#include "midi/midi_mapper_matrix_manager.h"

#ifdef USE_UCLOCK
    #include "uclock.h"
#endif

#ifdef ENABLE_SHUFFLE
    void flexiarp_shuffled_callback(uint32_t step, uint8_t track);
#endif

class VirtualBehaviour_FlexiArp : virtual public DeviceBehaviourUltimateBase {
  EuclidianSequencer *sequencer = nullptr;
  MIDIOutputProcessor *output_processor = nullptr;

  public:
    source_id_t source_id_2 = -1;
    source_id_t source_id_3 = -1;
    source_id_t source_id_4 = -1;

    // FlexiArp output channels (each becomes a matrix source)
    const int FLEXIARP_CHANNEL_1 = 1;
    const int FLEXIARP_CHANNEL_2 = 2;
    const int FLEXIARP_CHANNEL_3 = 3;
    const int FLEXIARP_CHANNEL_4 = 4;

    VirtualBehaviour_FlexiArp() : DeviceBehaviourUltimateBase () {
        this->output_processor = new MIDIOutputProcessor(this);
        this->sequencer = new EuclidianSequencer(output_processor->get_available_outputs(), 4);
        
        // Create FlexiArpOutput nodes (one per channel, each is independent arpeggiator instance)
        // These output nodes will receive events from sequencer patterns
        this->output_processor->addNode(new FlexiArpOutput("FlexiArp_Ch1", this, FLEXIARP_CHANNEL_1));
        this->output_processor->addNode(new FlexiArpOutput("FlexiArp_Ch2", this, FLEXIARP_CHANNEL_2));
        this->output_processor->addNode(new FlexiArpOutput("FlexiArp_Ch3", this, FLEXIARP_CHANNEL_3));
        this->output_processor->addNode(new FlexiArpOutput("FlexiArp_Ch4", this, FLEXIARP_CHANNEL_4));

        output_processor->configure_sequencer(sequencer);
        sequencer->initialise_patterns();
        sequencer->reset_patterns();
        output_processor->setup_parameters();

        #ifdef USE_UCLOCK
            #ifdef ENABLE_SHUFFLE
                uClock.setOnStep(flexiarp_shuffled_callback, NUMBER_SHUFFLE_PATTERNS);
            #endif
        #endif
    }

    virtual const char *get_label() override {
        return (const char*)"FlexiArp";
    }

    virtual int getType() override {
        return BehaviourType::virt;
    }

    virtual bool transmits_midi_notes() override { return true; }

    #ifdef ENABLE_SHUFFLE
        virtual void on_step_shuffled(uint8_t track, uint32_t step) {
            if (this->debug) Serial.printf(F("behaviour_flexiarp#on_step_shuffled(%i, %i)\n"), track, step);
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
        if (this->debug) Serial.printf(F("behaviour_flexiarp#sendNoteOn(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i\n"), channel, note, velocity, source_id);
        
        // Route to appropriate matrix source based on channel
        if (channel == FLEXIARP_CHANNEL_1) {
            midi_matrix_manager->processNoteOn(this->source_id, note, velocity, channel);
        } else if (channel == FLEXIARP_CHANNEL_2) {
            midi_matrix_manager->processNoteOn(this->source_id_2, note, velocity);
        } else if (channel == FLEXIARP_CHANNEL_3) {
            midi_matrix_manager->processNoteOn(this->source_id_3, note, velocity);
        } else if (channel == FLEXIARP_CHANNEL_4) {
            midi_matrix_manager->processNoteOn(this->source_id_4, note, velocity);
        } else {
            midi_matrix_manager->processNoteOn(this->source_id, note, velocity);
        }
    }

    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (this->debug) Serial.printf(F("behaviour_flexiarp#sendNoteOff(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i\n"), channel, note, velocity, source_id);
        
        // Route to appropriate matrix source based on channel
        if (channel == FLEXIARP_CHANNEL_1) {
            midi_matrix_manager->processNoteOff(this->source_id, note, velocity, channel);
        } else if (channel == FLEXIARP_CHANNEL_2) {
            midi_matrix_manager->processNoteOff(this->source_id_2, note, velocity);
        } else if (channel == FLEXIARP_CHANNEL_3) {
            midi_matrix_manager->processNoteOff(this->source_id_3, note, velocity);
        } else if (channel == FLEXIARP_CHANNEL_4) {
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

        // EuclidianSequencer::getParameters() returns a valid list
        ParameterList *sequencer_parameters = sequencer->getParameters();
        if (sequencer_parameters != nullptr) {
            for (auto* p : *sequencer_parameters) {
                this->parameters->add(p);
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
                Euclidian::CombinePageOption::COMBINE_LOCKS_WITH_CIRCLE | Euclidian::CombinePageOption::COMBINE_MODULATION_WITH_MUTATION,
                "FlexiArp"
            );
            this->output_processor->create_menu_items(true, "FlexiArp outputs", "FlexiArp");

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

extern VirtualBehaviour_FlexiArp *behaviour_flexiarp;

#endif // ENABLE_FLEXIARP
