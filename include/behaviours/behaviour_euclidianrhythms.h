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

#ifdef USE_UCLOCK
    #include "uclock.h"
#endif


class VirtualBehaviour_EuclidianRhythms : virtual public DeviceBehaviourUltimateBase {
  EuclidianSequencer *sequencer = nullptr;
  MIDIOutputProcessor *output_processor = nullptr;

  public:
    source_id_t source_id_2 = -1;
    source_id_t source_id_3 = -1;
    source_id_t source_id_4 = -1;

    const int MISC_CHANNEL_8 = 8;
    const int MISC_CHANNEL_9 = 9;

    VirtualBehaviour_EuclidianRhythms() : DeviceBehaviourUltimateBase () {
        this->output_processor = new FullDrumKitAndBassMIDIOutputProcessor(this);
        this->sequencer = new EuclidianSequencer(output_processor->nodes);
        this->output_processor->addNode(new MIDIDrumOutput("Misc8", this, 0, MISC_CHANNEL_8));
        this->output_processor->addNode(new MIDIDrumOutput("Misc9", this, 0, MISC_CHANNEL_9));
        output_processor->configure_sequencer(sequencer);
        sequencer->initialise_patterns();
        sequencer->reset_patterns();
        output_processor->setup_parameters();
        
        // registering the shuffle callback is now done within the sequencer itself 
        // (with a TODO note that it should probably be optional so that we can decide to register
        // from behaviours etc if we want to know about shuffle events and/or do anything differently)

        // #ifdef USE_UCLOCK
        //     #ifdef ENABLE_SHUFFLE
        //         shuffle_pattern_wrapper.register_shuffle_callback(
        //             [=](uint32_t step, uint8_t track) {
        //                 this->on_step_shuffled(track, step);
        //             }
        //         );
        //     #endif
        //     /*int8_t shuff[] = { 
        //         (int8_t)0, (int8_t)0, (int8_t)3, (int8_t)0, (int8_t)0, (int8_t)-3, (int8_t)0, (int8_t)0, 
        //         (int8_t)0, (int8_t)0, (int8_t)3, (int8_t)0, (int8_t)0, (int8_t)-3, (int8_t)0, (int8_t)0
        //     };*/
        //     /*int8_t shuff[] = { 
        //         (int8_t)-2, (int8_t)2, (int8_t)3, (int8_t)3, (int8_t)3, (int8_t)-3, (int8_t)-2, (int8_t)3, 
        //         (int8_t)1, (int8_t)3, (int8_t)3, (int8_t)1, (int8_t)-2, (int8_t)-2, (int8_t)-1, (int8_t)0
        //     };
        //     uClock.setTrackShuffleTemplate(1, shuff, 16);
        //     uClock.setTrackShuffle(1, true);
        //     */
        //     //uClock.setShuffle(true);
        // #endif
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
        if (output_processor->is_enabled())
            output_processor->process();
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

    virtual void release_outputs() override {
        sequencer->release_outputs();
    }

    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // this was/should really be receive_note_on ...
        //if (this->debug) 
        if (this->debug) Serial.printf(F("behaviour_euclidianrhythms#receive_note_on(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i: \n"), channel, note, velocity, source_id);
        if (channel==GM_CHANNEL_DRUMS) {
            midi_matrix_manager->processNoteOn(this->source_id, note, MIDI_MAX_VELOCITY, channel);
        } else if (channel==MISC_CHANNEL_8) {
            midi_matrix_manager->processNoteOn(this->source_id_3, note, MIDI_MAX_VELOCITY);
        } else if (channel==MISC_CHANNEL_9) {
            midi_matrix_manager->processNoteOn(this->source_id_4, note, MIDI_MAX_VELOCITY);
        } else {
            midi_matrix_manager->processNoteOn(this->source_id_2, note, MIDI_MAX_VELOCITY);
        }
    }
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // this was/should really be receive_note_off !
        //if (this->debug) Serial.printf(F("!! behaviour_lestrum#receive_note_off(\tchannel %i,\tnote %i,\tvelocity %i)with source_id %i: \n"), channel, note, velocity, source_id_2);
        if (this->debug) Serial.printf(F("behaviour_euclidianrhythms#receive_note_off(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i: \n"), channel, note, velocity, source_id);
        if (channel==GM_CHANNEL_DRUMS) {
            midi_matrix_manager->processNoteOff(this->source_id, note, MIDI_MIN_VELOCITY, channel);
        } else if (channel==MISC_CHANNEL_8) {
            midi_matrix_manager->processNoteOff(this->source_id_3, note, MIDI_MIN_VELOCITY);
        } else if (channel==MISC_CHANNEL_9) {
            midi_matrix_manager->processNoteOff(this->source_id_4, note, MIDI_MIN_VELOCITY);
        } else {
            midi_matrix_manager->processNoteOff(this->source_id_2, note, MIDI_MIN_VELOCITY);
        }
    }


    bool already_initialised = false;
    //FLASHMEM 
    // also initialises menu items!
    virtual ParameterList *initialise_parameters() override {
        Serial.printf("%s#initialise_parameters()...", this->get_label());
        if (already_initialised && this->parameters!=nullptr)
            return this->parameters;

        DeviceBehaviourUltimateBase::initialise_parameters();

        // initialises sequencer/pattern parameters and add them to the host object's parameters list so that they will get saved and reloaded
        ParameterList *sequencer_parameters = sequencer->getParameters(); 
        for (auto* p : *sequencer_parameters) {
            this->parameters->add(p);
        }

        //Serial.printf(F("Finished initialise_parameters() in %s\n"), this->get_label());

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
                "EuclidianDrums"
            );
            this->output_processor->create_menu_items(true, "Drum outputs", "EuclidianDrums");

            return menuitems;
        }

        virtual bool show_dedicated_parameters_page() {
            return false;
        }
    #endif

    virtual void setup_saveable_settings() override {
        DeviceBehaviourUltimateBase::setup_saveable_settings();
        // Register sequencer as a child; sl_setup_all will call sequencer->setup_saveable_settings()
        // @@TODO: think there may be a problem here now where the sequencer's parameters are added twice?
        register_child(this->sequencer);
        register_child(this->output_processor);
    }

    void debug_lock() {
        Serial.println("Debugging Euclidian lock");
        this->sequencer->set_mutation_count(0);
        EuclidianPattern *pattern = (EuclidianPattern*)this->sequencer->get_pattern(10);
        pattern->set_pulses(16);
        pattern->set_steps(16);
        pattern->set_locked(true);
        pattern->set_shuffle_track(1);
        this->sequencer->set_add_phrase_enabled(false);
        this->sequencer->set_mutation_count(0);
        this->sequencer->set_fills_enabled(false);
    }

    void debug_shuffle(uint8_t index) {
        Serial.printf("Debugging Euclidian shuffle %d\n", index);
        int8_t shuffle_75[] = {0, 12, 0, 12, 0, 12, 0, 12}; //, 12, 0, 12, 0, 12, 0, 12};
        shuffle_pattern_wrapper[index]->set_steps(shuffle_75, sizeof(shuffle_75));
        shuffle_pattern_wrapper[index]->update_target();
    }

    void debug_simples() {
        Serial.println("Debugging Euclidian simples");
        for (int i = 0; i < this->sequencer->get_number_patterns(); i++) {
            EuclidianPattern *pattern = (EuclidianPattern*)this->sequencer->get_pattern(i);
            if (strcmp(pattern->get_output_label(), "Kick") == 0
                || strcmp(pattern->get_output_label(), "Clap") == 0 
                || strcmp(pattern->get_output_label(), "CHH") == 0 
                // || strcmp(pattern->get_output_label(), "OHH") == 0
            ) 
                continue;

            BaseOutput *output = pattern->get_output();
            if (output) {
                output->set_enabled(false);
            }
        }
    }

};


extern VirtualBehaviour_EuclidianRhythms *behaviour_euclidianrhythms;


#endif