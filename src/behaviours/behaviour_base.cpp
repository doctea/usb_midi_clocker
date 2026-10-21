#include "behaviours/behaviour_base.h"

#include "midi/midi_mapper_matrix_manager.h"

void DeviceBehaviourUltimateBase::ensure_note_limit_parameters() {
    if (note_limit_parameters_setup)
        return;
    note_limit_parameters_setup = true;

    parameters->add(new BehaviourNoteLimitParameter(
        "Lowest note", this, &this->lowest_note, &this->effective_lowest_note
    ));
    parameters->add(new BehaviourNoteLimitParameter(
        "Highest note", this, &this->highest_note, &this->effective_highest_note
    ));
}


// called when a receive_note_on message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (this->debug) Serial_printf("%s#receive_note_on(channel=%i,\tnote=%i,\tvelocity=%i)\t(source_id=%i)\n", this->get_label(), channel, note, velocity, this->source_id);
    midi_matrix_manager->processNoteOn(this->source_id, note, velocity, channel);
}

// called when a note_off message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (this->debug) Serial_printf("%s#receive_note_off(channel=%i,\tnote=%i,\tvelocity=%i)\t(source_id=%i)\n", this->get_label(), channel, note, velocity, this->source_id);
    midi_matrix_manager->processNoteOff(this->source_id, note, velocity, channel);
}

// called when a receive_control_change message is received from the device; default behaviour is to pass it on to the midi_matrix_manager to route it
void DeviceBehaviourUltimateBase::receive_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
    /*if (this->debug) {
        Serial_printf(F("%s: DeviceBehaviourUltimateBase::receive_control_change(%i, %i, %i)...\n"), this->get_label(), inChannel, inNumber, inValue);
        if (midi_matrix_manager==nullptr) {
            Serial_println(F("YO, midi_matrix_manager is null?!")); Serial_flush();
        }
    }*/
    midi_matrix_manager->processControlChange(this->source_id, inNumber, inValue, inChannel);
    //if (this->debug) Serial_printf(F("...DeviceBehaviourUltimateBase::receive_control_change(%i, %i, %i) done!\n"), this->get_label(), inChannel, inNumber, inValue);
}

void DeviceBehaviourUltimateBase::receive_pitch_bend(uint8_t inChannel, int16_t bend) {
    #if defined(ENABLE_ADVANCED_PITCHBEND) && defined(ENABLE_PARAMETERS)
        if (this->supports_advanced_pitch_bend() && this->handle_modulated_pitch_bend(inChannel, bend)) {
            return;
        }
    #endif

    // Preserve existing passthrough behavior by default, and keep it as a safe
    // fallback for MODULATED behaviours until their advanced path is wired.
    if (this->supports_passthru_pitch_bend() || this->supports_advanced_pitch_bend()) {
        midi_matrix_manager->processPitchBend(this->source_id, bend, inChannel);
    }
}

#ifdef ENABLE_SCREEN
    MenuItemList *DeviceBehaviourUltimateBase::create_saveable_parameters_recall_selector() {
        // TODO: rebuild recall-selector UI using saveloadlib settings tree if needed
        return nullptr;
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

// Note limits can change while this behaviour holds notes it generated itself (eg a drone note);
// routed notes are migrated by the matrix, so only the local NoteTracker entries are handled here.
int DeviceBehaviourUltimateBase::reapply_local_note_limits() {
    // don't touch drum notes
    if (this->current_channel==GM_CHANNEL_DRUMS)
        return 0;

    if (note_tracker_count_held()==0)
        return 0;

    if (debug) { Serial_printf("%20s\t: DeviceBehaviourUltimateBase#reapply_local_note_limits starting with\t%i held notes (%s)\n", this->get_label(), note_tracker_count_held(), note_tracker_get_held_notes_c()); }

    return note_tracker_foreach_note([this](int8_t note, int8_t old_transposed_note) {
        int8_t new_transposed_note = apply_note_limits(
            note, this->getLowestNoteMode(), this->getHighestNoteMode(),
            get_effective_lowest_note(), get_effective_highest_note()
        );
        if (old_transposed_note==new_transposed_note)
            return;
        if (debug) Serial_printf("%20s\t: DeviceBehaviourUltimateBase#reapply_local_note_limits: note %i (%s) moves from %i (%s) to %i (%s) on channel %i\n", this->get_label(), note, get_note_name_c(note), old_transposed_note, get_note_name_c(old_transposed_note), new_transposed_note, get_note_name_c(new_transposed_note), this->current_channel);
        this->sendNoteOff(note, MIDI_MIN_VELOCITY, this->current_channel);
        if (is_valid_note(new_transposed_note))
            this->sendNoteOn(note, MIDI_MAX_VELOCITY, this->current_channel);
    });
}

void DeviceBehaviourUltimateBase::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    //if (debug) Serial_printf("DeviceBehaviourUltimateBase#sendNoteOn");

    // Note quantisation is applied per-connection by the MIDI matrix, not here.
    // Routed deliveries are tracked by the matrix ledger; local notes use the NoteTracker.

    int8_t quantised_note = note;
    if (debug) Serial_printf("%20s:\tDeviceBehaviourUltimateBase#sendNoteOn(%i, %i, %i)\n", this->get_label(), note, velocity, channel);

    // quantised_note = this->recalculate_pitch(quantised_note);
    quantised_note = apply_note_limits(quantised_note, this->getLowestNoteMode(), this->getHighestNoteMode(), get_effective_lowest_note(), get_effective_highest_note());
    if (!is_valid_note(quantised_note)) return;
    this->current_transposed_note = quantised_note;
    this->current_channel = channel;

    if (debug) Serial_printf("%20s:\tDeviceBehaviourUltimateBase#sendNoteOn(%i, %i, %i) -> quantised_note %i, about to call held_note_on(%i, %i..)\n", this->get_label(), note, velocity, channel, quantised_note, note, quantised_note);
    if (!is_processing_routed_note())
        note_tracker_held_note_on(note, quantised_note, channel);
    
    quantised_note += this->TUNING_OFFSET;
    if (!is_valid_note(quantised_note)) return;

    this->actualSendNoteOn(quantised_note, velocity, channel);
}