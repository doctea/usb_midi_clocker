#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef ENABLE_EUCLIDIAN

#include "bpm.h"

#include "behaviours/behaviour_base.h"

#include "sequencer/Euclidian.h"
#include "outputs/output_processor.h"
#include "outputs/output.h"

#include "midi/midi_mapper_matrix_manager.h"


class VirtualBehaviour_EuclidianRhythms : virtual public DeviceBehaviourUltimateBase {
  EuclidianSequencer *sequencer = nullptr;
  MIDIOutputProcessor *output_processor = nullptr;

  public:
    source_id_t source_id_2 = -1;

    VirtualBehaviour_EuclidianRhythms() : DeviceBehaviourUltimateBase () {
        this->output_processor = new MIDIOutputProcessor(this);
        this->sequencer = new EuclidianSequencer(output_processor->nodes);
        sequencer->initialise_patterns();
        sequencer->reset_patterns();
        output_processor->configure_sequencer(sequencer);
        output_processor->setup_parameters();
    }

    virtual const char *get_label() override {
        return (const char*)"Euclidian Rhythms";
    }

    virtual int getType() override {
        return BehaviourType::virt;
    }

    virtual void on_tick(uint32_t ticks) override {
        if (sequencer->is_running()) 
            sequencer->on_tick(ticks);
        if (is_bpm_on_sixteenth(ticks) && output_processor->is_enabled()) {
            output_processor->process();
        }
        /*if (is_bpm_on_sixteenth(ticks)) 
            this->sequencer->on_step(BPM_CURRENT_STEP_OF_PHRASE);
        if (is_bpm_on_sixteenth(ticks),PPQN-1) 
            this->sequencer->on_step_end(BPM_CURRENT_STEP_OF_PHRASE);
        if (is_bpm_on_beat(ticks))
            this->sequencer->on_beat(BPM_CURRENT_BEAT_OF_PHRASE);*/
    };

    virtual void loop(uint32_t ticks) override {
        output_processor->loop();
        sequencer->on_loop(ticks);
    }



    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // this was/should really be receive_note_on ...
        //if (this->debug) 
        if (this->debug) Serial.printf(F("behaviour_euclidianrhythms#receive_note_on(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i: \n"), channel, note, velocity, source_id);
        if (channel==GM_CHANNEL_DRUMS) {
            midi_matrix_manager->processNoteOn(this->source_id, note, 127, channel);
        } else {
            midi_matrix_manager->processNoteOn(this->source_id_2, note, 127);
        }
    }
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // this was/should really be receive_note_off !
        //if (this->debug) Serial.printf(F("!! behaviour_lestrum#receive_note_off(\tchannel %i,\tnote %i,\tvelocity %i)with source_id %i: \n"), channel, note, velocity, source_id_2);
        if (this->debug) Serial.printf(F("behaviour_euclidianrhythms#receive_note_off(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i: \n"), channel, note, velocity, source_id);
        if (channel==GM_CHANNEL_DRUMS) {
            midi_matrix_manager->processNoteOff(this->source_id, note, 0, channel);
            //lestrum_arp_output->sendNoteOff(note, 0);
        } else {
            midi_matrix_manager->processNoteOff(this->source_id_2, note, 0);
            //lestrum_pads_output->sendNoteOff(note, 0);
        }
    }


    bool already_initialised = false;
    //FLASHMEM 
    // also initialises menu items!
    virtual LinkedList<FloatParameter*> *initialise_parameters() override {
        Serial.printf("%s#initialise_parameters()...", this->get_label());
        if (already_initialised && this->parameters!=nullptr)
            return this->parameters;

        DeviceBehaviourUltimateBase::initialise_parameters();
        sequencer->getParameters(); // initialises the sequencer and pattern parameters?

        //Serial.printf(F("Finished initialise_parameters() in %s\n"), this->get_label());

        already_initialised = true;

        return parameters;
    }

    virtual LinkedList<MenuItem*> *make_menu_items() override {
        LinkedList<MenuItem *> *menuitems = DeviceBehaviourUltimateBase::make_menu_items();

        this->sequencer->make_menu_items(menu, true);
        this->output_processor->create_menu_items(true);

        return menuitems;
    }

    virtual void setup_saveable_parameters() override {
        DeviceBehaviourUltimateBase::setup_saveable_parameters();
        this->sequencer->setup_saveable_parameters();

        // todo: better way of 'nesting' a sequencer/child object's saveableparameters within a host object's
        for(unsigned int i = 0 ; i < sequencer->saveable_parameters->size() ; i++) {
            this->saveable_parameters->add(sequencer->saveable_parameters->get(i));
        }
    }

};


extern VirtualBehaviour_EuclidianRhythms *behaviour_euclidianrhythms;

#endif