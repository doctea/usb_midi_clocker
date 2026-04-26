#pragma once

#include <Arduino.h>
#include "Config.h" 

#ifdef ENABLE_TURINGMACHINE

#include "bpm.h"

#include "behaviours/behaviour_base.h"

#include "sequencer/TuringMachine/TuringMachinePattern.h"
#include "outputs/output_processor.h"
#include "outputs/output.h"

#include "midi/midi_mapper_matrix_manager.h"

#ifdef USE_UCLOCK
    #include "uclock.h"
#endif

#ifdef ENABLE_SHUFFLE
    void shuffled_callback(uint32_t step, uint8_t track);
#endif

class VirtualBehaviour_TuringMachine : virtual public DeviceBehaviourUltimateBase {
  BaseSequencer *sequencer = nullptr;
  MIDIOutputProcessor *output_processor = nullptr;

  public:
    source_id_t source_id_2 = -1;
    source_id_t source_id_3 = -1;
    source_id_t source_id_4 = -1;

    VirtualBehaviour_TuringMachine() : DeviceBehaviourUltimateBase () {
        this->output_processor = new BassAndChordsAndMelodyMIDIOutputProcessor(this);
        this->sequencer = new SimpleSequencer(output_processor->get_available_outputs());
        TuringMachinePattern *tm_pattern = new TuringMachinePattern(output_processor->get_available_outputs());
        tm_pattern->set_output_by_name("Melody");
        tm_pattern->set_path_segment("pattern_0");
        tm_pattern->set_steps(16);
        this->sequencer->add_pattern(tm_pattern);

        #if defined(ENABLE_PARAMETERS)
            parameter_manager->addInput(tm_pattern);

            //Serial.println("..calling sequencer.getParameters()..");
            LinkedList<FloatParameter*> *params = sequencer->getParameters();
            Debug_printf("after setting up sequencer parameters, free RAM is %u\n", freeRam());
        #endif
    }

    virtual const char *get_label() override {
        return (const char*)"Turing Machine";
    }

    virtual int getType() override {
        return BehaviourType::virt;
    }

    virtual void on_tick(uint32_t ticks) override {
        if (sequencer->is_running()) 
            sequencer->on_tick(ticks);
        if (output_processor->is_enabled())
            output_processor->process();
    };

    virtual void loop(uint32_t ticks) override {
        //output_processor->loop();
        sequencer->on_loop(ticks);
    }

    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // this was/should really be receive_note_on ...
        //if (this->debug) 
        if (this->debug) Serial.printf(F("behaviour_turingmachine#receive_note_on(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i: \n"), channel, note, velocity, source_id);
        midi_matrix_manager->processNoteOn(this->source_id, note, velocity, channel);
    }
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // this was/should really be receive_note_off !
        //if (this->debug) Serial.printf(F("!! behaviour_lestrum#receive_note_off(\tchannel %i,\tnote %i,\tvelocity %i)with source_id %i: \n"), channel, note, velocity, source_id_2);
        if (this->debug) Serial.printf(F("behaviour_turingmachine#receive_note_off(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i: \n"), channel, note, velocity, source_id);
        midi_matrix_manager->processNoteOff(this->source_id, note, MIDI_MIN_VELOCITY, channel);
    }


    bool already_initialised = false;
    //FLASHMEM 
    // also initialises menu items!
    virtual LinkedList<FloatParameter*> *initialise_parameters() override {
        Serial.printf("%s#initialise_parameters()...", this->get_label());
        if (already_initialised && this->parameters!=nullptr)
            return this->parameters;

        DeviceBehaviourUltimateBase::initialise_parameters();

        // initialises sequencer/pattern parameters and add them to the host object's parameters list so that they will get saved and reloaded
        // LinkedList<FloatParameter*> *sequencer_parameters = sequencer->getParameters(); 
        // for (unsigned int i = 0 ; i < sequencer_parameters->size() ; i++) {
        //     this->parameters->add(sequencer_parameters->get(i));
        // }

        output_processor->setup_parameters();

        //Serial.printf(F("Finished initialise_parameters() in %s\n"), this->get_label());

        already_initialised = true;

        return parameters;
    }

    #ifdef ENABLE_SCREEN
        virtual LinkedList<MenuItem*> *make_menu_items() override {
            LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

            this->sequencer->make_menu_items(menu, true);
            this->output_processor->create_menu_items(true);

            return menuitems;
        }
    #endif

    virtual void setup_saveable_settings() override {
        DeviceBehaviourUltimateBase::setup_saveable_settings();
        // Register sequencer as a child; sl_setup_all will call sequencer->setup_saveable_settings()
        // @@TODO: think there may be a problem here now where the sequencer's parameters are added twice?
        register_child(this->sequencer);
        register_child(this->output_processor);
    }

};


extern VirtualBehaviour_TuringMachine *behaviour_turingmachine;

#endif