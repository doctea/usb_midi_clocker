#ifndef BEHAVIOUR_DRUMKIT__INCLUDED
#define BEHAVIOUR_DRUMKIT__INCLUDED

#include <Arduino.h>

#include "midi_mapper_matrix_manager.h"

#include "Config.h"
#include "behaviour_base_serial.h"

#include "Drums.h"

void drumkit_note_on(byte pitch, byte velocity, byte channel);
void drumkit_note_off(byte pitch, byte velocity, byte channel);

class DeviceBehaviour_DrumKit : public DeviceBehaviourSerialBase {
    public:
        DeviceBehaviour_DrumKit () : DeviceBehaviourSerialBase () {}

        source_id_t source_id_2 = -1;

        char *get_label() override {
            return "Drumkit";
        }

        void setup_callbacks() override {
            Serial.println("DeviceBehaviour_DrumKit#setup_callbacks..");
            this->input_device->setHandleNoteOn(drumkit_note_on);
            this->input_device->setHandleNoteOff(drumkit_note_off);
        }

        void receive_note_on(byte note, byte velocity, byte channel) override {
            if (note==GM_NOTE_ACOUSTIC_SNARE)   note = GM_NOTE_ELECTRIC_SNARE;
            if (note==GM_NOTE_LOW_FLOOR_TOM)    note = GM_NOTE_HI_MID_TOM;

            DeviceBehaviourSerialBase::receive_note_on(note, velocity, channel);
        }

        void receive_note_off(byte note, byte velocity, byte channel) override {
            if (note==GM_NOTE_ACOUSTIC_SNARE)   note = GM_NOTE_ELECTRIC_SNARE;
            if (note==GM_NOTE_LOW_FLOOR_TOM)    note = GM_NOTE_HI_MID_TOM;

            DeviceBehaviourSerialBase::receive_note_off(note, velocity, channel);
        }
};

extern DeviceBehaviour_DrumKit behaviour_drumkit;

#endif