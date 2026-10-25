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

/*
Sooo, we have 4 flexiarp output nodes.
These can be triggered by any of the 4 patterns in the flexiarp sequencer.
When triggered, they will send noteOn/noteOff events to the MIDI matrix manager, with a source_id corresponding to the MIDI channel set for the flexiarp output node (1-4). 
This way, the flexiarp output nodes can be used as independent sources in the MIDI matrix, and can be routed to any destination(s) in the matrix.
And we can combine multiple flexiarp output nodes to the same MIDI channel, in order to have multiple patterns triggering the same output channel, if desired.
TODO: perhaps we want to update the FlexiArp Output UI so that it instead shows the source_id (1-4) instead of the MIDI channel, since the MIDI channel is just a proxy for the source_id, and the source_id is what actually matters in the matrix.
Orrrr, maybe we just need to limit the MIDI channel to the actual source_id (1-4) and not allow any other values.
*/

const int NUM_FLEXIARP_PATTERNS = 4;

class VirtualBehaviour_FlexiArp : virtual public DeviceBehaviourUltimateBase {
  EuclidianSequencer *sequencer = nullptr;
  MIDIOutputProcessor *output_processor = nullptr;

  public:
    // for storing the next 3 source_ids (first is regular source_id variable in DeviceBehaviourUltimateBase) for the 4 FlexiArp patterns
    source_id_t source_ids[NUM_FLEXIARP_PATTERNS-1] = { -1, -1, -1 };

    VirtualBehaviour_FlexiArp() : DeviceBehaviourUltimateBase () {

        this->output_processor = new MIDIOutputProcessor(this);
        
        this->sequencer = new EuclidianSequencer(
            output_processor->get_available_outputs(), 
            "FlexiPat ", 
            NUM_FLEXIARP_PATTERNS
        );

        // Patterns are created automatically by the EuclidianSequencer based on the number specified (NUM_FLEXIARP_PATTERNS)

        // Create FlexiArpOutput nodes (one per source, each is independent arpeggiator instance)
        // These output nodes will receive events from sequencer patterns
        // Add output nodes for multi-source routing via MIDI channels
        for (int i = 0 ; i < NUM_FLEXIARP_PATTERNS ; i++) {
            char label[32];
            snprintf(label, 32, "FlexiArp Ch%i", i+1);
            this->output_processor->addNode(new FlexiArpOutput(label, this, i+1));
        }

        output_processor->configure_sequencer(sequencer);
        sequencer->initialise_patterns();
        sequencer->reset_patterns();
        output_processor->setup_parameters();
    }

    virtual const char *get_label() override {
        return (const char*)"FlexiArp";
    }

    virtual int getType() override {
        return BehaviourType::virt;
    }

    virtual bool transmits_midi_notes() override { return true; }

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

    virtual void release_outputs() override {
        sequencer->release_outputs();
    }

    virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // Route to appropriate matrix source based on channel
        if (channel>1 && channel <= NUM_FLEXIARP_PATTERNS) {
            midi_matrix_manager->processNoteOn(this->source_ids[channel - 2], note, velocity);
        } else {
            midi_matrix_manager->processNoteOn(this->source_id, note, velocity);
        }
    }

    // virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
    virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        if (this->debug) Serial.printf(F("behaviour_flexiarp#sendNoteOff(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i\n"), channel, note, velocity, source_id);
        
        // Route to appropriate matrix source based on channel
        if (channel>1 && channel <= NUM_FLEXIARP_PATTERNS) {
            midi_matrix_manager->processNoteOff(this->source_ids[channel - 2], note, velocity);
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

        // TODO: this differs from the behaviour_turingmachine implementation; investigate if this causes 
        // any issues with parameter initialisation or saving, and unify behaviour if necessary

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
                Euclidian::CombinePageOption::COMBINE_LOCKS_WITH_CIRCLE 
                | Euclidian::CombinePageOption::COMBINE_MODULATION_WITH_MUTATION 
                | Euclidian::CombinePageOption::COMBINE_PATTERN_MODULATION_WITH_PATTERN,
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
