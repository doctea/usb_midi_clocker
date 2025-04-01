#include "behaviours/behaviour_base.h"

#include "midi/midi_mapper_matrix_manager.h"


// called when a receive_note_on message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (this->debug) Serial_printf("%s#receive_note_on(channel=%i,\tnote=%i,\tvelocity=%i)\t(source_id=%i)\n", this->get_label(), channel, note, velocity, this->source_id);
    midi_matrix_manager->processNoteOn(this->source_id, note, velocity); //, channel);
}

// called when a note_off message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (this->debug) Serial_printf("%s#receive_note_off(channel=%i,\tnote=%i,\tvelocity=%i)\t(source_id=%i)\n", this->get_label(), channel, note, velocity, this->source_id);
    midi_matrix_manager->processNoteOff(this->source_id, note, velocity); //, channel);
}

// called when a receive_control_change message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    /*if (this->debug) {
        Serial_printf(F("%s: DeviceBehaviourUltimateBase::receive_control_change(%i, %i, %i)...\n"), this->get_label(), inChannel, inNumber, inValue);
        if (midi_matrix_manager==nullptr) {
            Serial_println(F("YO, midi_matrix_manager is null?!")); Serial_flush();
        }
    }*/
    midi_matrix_manager->processControlChange(this->source_id, inNumber, inValue); //, inChannel);
    //if (this->debug) Serial_printf(F("...DeviceBehaviourUltimateBase::receive_control_change(%i, %i, %i) done!\n"), this->get_label(), inChannel, inNumber, inValue);
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

            //Serial_printf("creating SaveableParameterOptionToggle for item %i\n", i); Serial_flush();
            saveable_parameter_recall_selector->addItem(new SaveableParameterOptionToggle(saveable_parameters->get(i)));
            last_category = category;
        }
        items->add(saveable_parameter_recall_selector);
        return items;
    }
#endif

/*void DeviceBehaviourUltimateBase::setForceOctave(int octave) {
    if (this->debug) Serial_printf("MIDIBassBehaviour#setForceOctave(%i)!", octave); Serial_flush();
    if (octave!=this->force_octave) {
        midi_matrix_manager->stop_all_notes_for_source(this->source_id);
        midi_matrix_manager->stop_all_notes_for_target(this->target_id);

        this->force_octave = octave;
    }
}*/

#ifdef ENABLE_SCALES
//PROGMEM
int DeviceBehaviourUltimateBase::requantise_all_notes() {
    // don't requantise drum notes
    if (this->current_channel==GM_CHANNEL_DRUMS)
        return 0;

    if (note_tracker.count_held()==0) {
        //Serial_printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: no notes to requantise\n", this->get_label());
        return 0;
    }
    if (debug) Serial_printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes starting with\t%i held notes (%s)\n", this->get_label(), note_tracker.count_held(), note_tracker.get_held_notes_c());

    bool initial_global_quantise_on = midi_matrix_manager->global_quantise_on;
    bool initial_global_quantise_chord_on = midi_matrix_manager->global_quantise_chord_on;

    int requantised_notes = 0;
    
    uint32_t start_foreach = micros();
    requantised_notes = note_tracker.foreach_note([this,initial_global_quantise_chord_on,initial_global_quantise_on](int8_t note, int8_t old_transposed_note) {
        int8_t new_transposed_note = midi_matrix_manager->do_quant(note, this->current_channel);
        if (debug) Serial_printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes in foreach_requantised_note: note=%i (%s), old_transposed_note=%i (%s), new_transposed_note=%i (%s)\n", this->get_label(), note, get_note_name_c(note), old_transposed_note, get_note_name_c(old_transposed_note), new_transposed_note, get_note_name_c(new_transposed_note));
        // note is the original note, transposed_note is the note that the original note was transposed to
        // if old transposed note is the same as the new transposed note, then we don't need to do anything
        /*if (!is_valid_note(new_transposed_note)) {
            note_tracker.held_note_off(note);
        } else {
            note_tracker.held_notes[note].transposed_note = new_transposed_note;
        }*/
        // if the new transposed note is the same as the old transposed note, then we don't need to do anything
        if (old_transposed_note==new_transposed_note) {
            if (debug) Serial_printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: note %i (%s) doesn't need to be stopped as didn't change!\n", this->get_label(), note, get_note_name_c(note)); 
            return;
        }
        // if the new transposed note is invalid, then we need to stop the old note without starting a new one
        if (!is_valid_note(new_transposed_note)) {
            if (debug) Serial_printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: note %i (%s) re-quantised to invalid note; stopping old_transposed_note %i (%s) on channel %i\n", this->get_label(), note, get_note_name_c(note), old_transposed_note, get_note_name_c(old_transposed_note), this->current_channel); 
            midi_matrix_manager->global_quantise_on = false;
            midi_matrix_manager->global_quantise_chord_on = false;
            //this->sendNoteOff(old_transposed_note, MIDI_MIN_VELOCITY, this->current_channel);
            this->sendNoteOff(note, MIDI_MIN_VELOCITY, this->current_channel);
            midi_matrix_manager->global_quantise_on = initial_global_quantise_on;
            midi_matrix_manager->global_quantise_chord_on = initial_global_quantise_chord_on;
            return;
        }
        // if the new transposed note is valid, then we need to stop the old note and start the new one
        if (is_valid_note(new_transposed_note)) {
            if (debug) Serial_printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: note %i (%s) re-quantised to new_transposed_note %i (%s); stopping old_transposed_note %i (%s) on channel %i)\n", this->get_label(), note, get_note_name_c(note), new_transposed_note, get_note_name_c(new_transposed_note), old_transposed_note, get_note_name_c(old_transposed_note), this->current_channel); 
            midi_matrix_manager->global_quantise_on = false;
            midi_matrix_manager->global_quantise_chord_on = false;
            //this->sendNoteOff(old_transposed_note, MIDI_MIN_VELOCITY, this->current_channel);
            this->sendNoteOff(note, MIDI_MIN_VELOCITY, this->current_channel);
            midi_matrix_manager->global_quantise_on = initial_global_quantise_on;
            midi_matrix_manager->global_quantise_chord_on = initial_global_quantise_chord_on;
            this->sendNoteOn(note, MIDI_MAX_VELOCITY, this->current_channel);
            return;
        } else {
            if (debug) Serial_printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: note %i (%s) doesn't need to be stopped as new transposed note is invalid...?\n", this->get_label(), note, get_note_name_c(note));
            return;
        }
        if (debug) Serial_printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes: NOTE SURE WHAT TO DO!  note %i (%s) re-quantised to %i (%s) (stopping then starting on channel %i)\n", this->get_label(), note, get_note_name_c(note), new_transposed_note, get_note_name_c(new_transposed_note), this->current_channel);
    });
    if (debug) if (requantised_notes>0) Serial_printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes 'foreach' took %i us to process %i notes\n", this->get_label(), micros()-start_foreach, requantised_notes);
    if (debug) Serial_printf("%20s\t: DeviceBehaviourUltimateBase#requantise_all_notes finishing with\t%i held notes (%s)\n", this->get_label(), note_tracker.count_held(), note_tracker.get_held_notes_c());

    return requantised_notes;

}
#endif


void DeviceBehaviourUltimateBase::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    //if (debug) Serial_printf("DeviceBehaviourUltimateBase#sendNoteOn");
    // TODO: this is where ForceOctave check should go..?

    #ifdef ENABLE_SCALES
        int8_t quantised_note = midi_matrix_manager->do_quant(note, channel);
    #else
        int8_t quantised_note = note;
    #endif
    if (debug) Serial_printf("%20s:\tDeviceBehaviourUltimateBase#sendNoteOn(%i, %i, %i) -> quantised_note %i\n", this->get_label(), note, velocity, channel, quantised_note);

    quantised_note = this->recalculate_pitch(quantised_note);
    if (!is_valid_note(quantised_note)) return;
    this->current_transposed_note = quantised_note;
    this->current_channel = channel;

    quantised_note += this->TUNING_OFFSET;
    if (!is_valid_note(quantised_note)) return;

    if (debug) Serial_printf("%20s:\tDeviceBehaviourUltimateBase#sendNoteOn(%i, %i, %i) -> quantised_note %i, about to call held_note_on(%i, %i..)\n", this->get_label(), note, velocity, channel, quantised_note, note, quantised_note);
    note_tracker.held_note_on(note, quantised_note);

    this->actualSendNoteOn(quantised_note, velocity, channel);
}