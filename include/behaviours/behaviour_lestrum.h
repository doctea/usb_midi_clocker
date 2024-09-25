#ifndef BEHAVIOUR_LESTRUM__INCLUDED
#define BEHAVIOUR_LESTRUM__INCLUDED

#include <Arduino.h>

#include "midi/midi_mapper_matrix_manager.h"

#include "Config.h"
#include "behaviours/behaviour_base_serial.h"
#include "bpm.h"


void lestrum_note_on(byte pitch, byte velocity, byte channel);
void lestrum_note_off(byte pitch, byte velocity, byte channel);

class DeviceBehaviour_LeStrum : virtual public DeviceBehaviourSerialBase {
    public:
        DeviceBehaviour_LeStrum () : DeviceBehaviourSerialBase () {}

        virtual const char *get_label() override {
            return "LeStrum";
        }

        source_id_t source_id_2 = -1;

        //FLASHMEM // crashes on setup if in FLASHMEM ?
        virtual void setup_callbacks() override {
            Serial.println(F("DeviceBehaviour_LeStrum#setup_callbacks..")); Serial_flush();
            this->input_device->setHandleNoteOn(lestrum_note_on);
            this->input_device->setHandleNoteOff(lestrum_note_off);
            Serial.println(F("DeviceBehaviour_LeStrum#setup_callback finished.")); Serial_flush();
        }

        virtual void receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            if (this->debug) Serial.printf(F("behaviour_lestrum#receive_note_on(\tchannel %i,\tnote %i,\tvelocity %i) with source_id %i: \n"), channel, note, velocity, source_id);
            if (channel==1) {
                midi_matrix_manager->processNoteOn(this->source_id, note, 127);
                //lestrum_arp_output->sendNoteOn(note, 127);
            } else {
                midi_matrix_manager->processNoteOn(this->source_id_2, note, 127);
                //lestrum_pads_output->sendNoteOn(note, 127);
            }
        }
        virtual void receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            if (this->debug) Serial.printf(F("!! behaviour_lestrum#receive_note_off(\tchannel %i,\tnote %i,\tvelocity %i)with source_id %i: \n"), channel, note, velocity, source_id_2);
            if (channel==1) {
                midi_matrix_manager->processNoteOff(this->source_id, note, 0);
                //lestrum_arp_output->sendNoteOff(note, 0);
            } else {
                midi_matrix_manager->processNoteOff(this->source_id_2, note, 0);
                //lestrum_pads_output->sendNoteOff(note, 0);
            }
        }
};

extern DeviceBehaviour_LeStrum *behaviour_lestrum;

#endif