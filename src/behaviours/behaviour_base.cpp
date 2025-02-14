#include "behaviours/behaviour_base.h"

#include "midi/midi_mapper_matrix_manager.h"


// called when a receive_note_on message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (this->debug)
        Serial.printf("%s#receive_note_on(channel=%i,\tnote=%i,\tvelocity=%i)\t(source_id=%i)\n", this->get_label(), channel, note, velocity, this->source_id);
    midi_matrix_manager->processNoteOn(this->source_id, note, velocity); //, channel);
}

// called when a note_off message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (this->debug)
        Serial.printf("%s#receive_note_off(channel=%i,\tnote=%i,\tvelocity=%i)\t(source_id=%i)\n", this->get_label(), channel, note, velocity, this->source_id);
    midi_matrix_manager->processNoteOff(this->source_id, note, velocity); //, channel);
}

// called when a receive_control_change message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    /*if (this->debug) {
        Serial.printf(F("%s: DeviceBehaviourUltimateBase::receive_control_change(%i, %i, %i)...\n"), this->get_label(), inChannel, inNumber, inValue);
        if (midi_matrix_manager==nullptr) {
            Serial.println(F("YO, midi_matrix_manager is null?!")); Serial_flush();
        }
    }*/
    midi_matrix_manager->processControlChange(this->source_id, inNumber, inValue); //, inChannel);
    //if (this->debug) Serial.printf(F("...DeviceBehaviourUltimateBase::receive_control_change(%i, %i, %i) done!\n"), this->get_label(), inChannel, inNumber, inValue);
}

void DeviceBehaviourUltimateBase::receive_pitch_bend(uint8_t inChannel, int bend) {
    midi_matrix_manager->processPitchBend(this->source_id, bend);
}

#ifdef ENABLE_SCREEN
    //FLASHMEM
    LinkedList<MenuItem *> *DeviceBehaviourUltimateBase::create_saveable_parameters_recall_selector() {
        if (!this->has_saveable_parameters()) //==nullptr || this->saveable_parameters->size()==0)
            return nullptr;
        //ObjectMultiToggleControl *saveable_parameter_recall_selector = new ObjectMultiToggleControl("Recall parameters", true);
        const char *category = nullptr;
        const char *last_category = nullptr;
        ObjectMultiToggleControl *saveable_parameter_recall_selector = nullptr;
        LinkedList<MenuItem *> *items = new LinkedList<MenuItem*>();

        for (unsigned int i = 0 ; i < this->saveable_parameters->size() ; i++) {
            SaveableParameterBase *p = this->saveable_parameters->get(i);
            category = p->category_name;

            if (last_category==nullptr || strcmp(category, last_category)!=0) {
                if (saveable_parameter_recall_selector!=nullptr)
                    items->add(saveable_parameter_recall_selector);
                saveable_parameter_recall_selector = new ObjectMultiToggleControl((String("Recall parameters: ") + String(p->category_name)).c_str(), true);
            }

            //Serial.printf("creating SaveableParameterOptionToggle for item %i\n", i); Serial_flush();
            saveable_parameter_recall_selector->addItem(new SaveableParameterOptionToggle(saveable_parameters->get(i)));
            last_category = category;
        }
        items->add(saveable_parameter_recall_selector);
        return items;
    }
#endif

/*void DeviceBehaviourUltimateBase::setForceOctave(int octave) {
    if (this->debug) Serial.printf("MIDIBassBehaviour#setForceOctave(%i)!", octave); Serial_flush();
    if (octave!=this->force_octave) {
        midi_matrix_manager->stop_all_notes_for_source(this->source_id);
        midi_matrix_manager->stop_all_notes_for_target(this->target_id);

        this->force_octave = octave;
    }
}*/

void DeviceBehaviourUltimateBase::requantise_all_notes() {
    if (this->current_channel==GM_CHANNEL_DRUMS)
        return;

    if (note_tracker.count_held()==0) {
        Serial.printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: no notes to requantise\n", this->get_label());
        return;
    }
    Serial.printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes starting with\t%i held notes (%s)\n", this->get_label(), note_tracker.count_held(), note_tracker.get_held_notes_c());

    bool initial_global_quantise_on = midi_matrix_manager->global_quantise_on;
    bool initial_global_quantise_chord_on = midi_matrix_manager->global_quantise_chord_on;

    /*midi_matrix_manager->global_quantise_on = false;
    midi_matrix_manager->global_quantise_chord_on = false;
    this->killCurrentNote();
    //return;
    midi_matrix_manager->global_quantise_on = initial_global_quantise_on;
    midi_matrix_manager->global_quantise_chord_on = initial_global_quantise_chord_on;*/

    note_tracker.foreach_note([=](int8_t note) {
        int8_t new_note = midi_matrix_manager->do_quant(note, this->current_channel);
        //this->sendNoteOff(note, MIDI_MIN_VELOCITY, this->current_channel);
        // possible states:-
        //      old note is still valid -- don't do anything
        if (new_note==note) {
            Serial.printf("\t%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: note %i (%s) doesn't need to be stopped\n", this->get_label(), note, get_note_name_c(note)); 
            return;
        }
        // old note is invalid, and no new note to quantise to - kill old note
        if (!is_valid_note(new_note)) {
            Serial.printf("\t%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: note %i (%s) re-quantised to %i (%s) (invalid note, stopping)\n", this->get_label(), note, get_note_name_c(note), new_note, get_note_name_c(new_note)); 
            //note_tracker.held_note_off(note);
            midi_matrix_manager->global_quantise_on = false;
            midi_matrix_manager->global_quantise_chord_on = false;
            this->sendNoteOff(note, MIDI_MIN_VELOCITY, this->current_channel);
            midi_matrix_manager->global_quantise_on = initial_global_quantise_on;
            midi_matrix_manager->global_quantise_chord_on = initial_global_quantise_chord_on;
            return;
        } /*else {
            Serial.printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: note %i (%s) doesn't need to be stopped\n", this->get_label(), note, get_note_name_c(note)); 
        }*/

        // old note is invalid, and new note is valid - kill old note, start new one
        if (is_valid_note(new_note)) {
            Serial.printf("\t%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: note %i (%s) re-quantised to %i (%s) (stopping then starting)\n", this->get_label(), note, get_note_name_c(note), new_note, get_note_name_c(new_note)); 
            //note_tracker.held_note_off(note);
            //this->actualSendNoteOff(note, MIDI_MIN_VELOCITY, this->current_channel);
            midi_matrix_manager->global_quantise_on = false;
            midi_matrix_manager->global_quantise_chord_on = false;
            this->sendNoteOff(note, MIDI_MIN_VELOCITY, this->current_channel);
            midi_matrix_manager->global_quantise_on = initial_global_quantise_on;
            midi_matrix_manager->global_quantise_chord_on = initial_global_quantise_chord_on;
            this->sendNoteOn(new_note, MIDI_MAX_VELOCITY, this->current_channel);
        } else {
            Serial.printf("\t%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: note %i (%s) doesn't need to be stopped\n", this->get_label(), note, get_note_name_c(note)); 
        }
    });

    Serial.printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes finishing with\t%i held notes (%s)\n", this->get_label(), note_tracker.count_held(), note_tracker.get_held_notes_c());

}