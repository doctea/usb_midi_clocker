#ifndef BEHAVIOUR_CHOCOLATE__INCLUDED
#define BEHAVIOUR_CHOCOLATE__INCLUDED

#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_CHOCOLATEFEET_USB

#include "behaviours/behaviour_base.h"

#include "multi_usb_handlers.h"

#include "project.h"

//extern MIDIOutputWrapper *beatstep_output;
//void beatstep_setOutputWrapper(MIDIOutputWrapper *);

//void beatstep_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void chocolate_handle_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void chocolate_handle_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
//void chocolate_handle_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);

#define CHOC_NOTE_TOGGLEPLAY    0
#define CHOC_NOTE_TOGGLEREC     1
#define CHOC_NOTE_OVERWRITE_MOM 2
#define CHOC_NOTE_RECORD_MOM    3

class DeviceBehaviour_Chocolate : public DeviceBehaviourUSBBase {
    public:
        uint16_t vid = 0x4353, pid = 0x4B4D;
        //uint16_t vid = 0x1c75, pid = 0x0288;

        #define NUM_TRACK_NOTES 4
        signed long note_pressed_at[NUM_TRACK_NOTES];
        signed long note_released_at[NUM_TRACK_NOTES];
        bool already_processed[NUM_TRACK_NOTES];

        virtual const char *get_label() override {
            return (char*)"Chocolate";
        }

        DeviceBehaviour_Chocolate () {
            for (int i = 0 ; i < NUM_TRACK_NOTES ; i++) {
                note_pressed_at[i] = -1;
                note_released_at[i] = -1;
                already_processed[i] = false;
            }
        };

        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        void setup_callbacks() override {
            Serial.println("DeviceBehaviour_Chocolate#setup_callbacks()");
            this->device->setHandleNoteOn(chocolate_handle_note_on);
            this->device->setHandleNoteOff(chocolate_handle_note_off);
            //this->device->setHandleNoteOff(chocolate_handle_control_change);
        }

        void receive_note_on(uint8_t channel, uint8_t note, uint8_t velocity) override {
            //Serial.printf("chocolate got noteOn chan %i, note %i, velocity %i\n", channel, note, velocity); Serial.flush();
            // TODO: control looper(s) from here
            // check other notes to see if they are held; if so then trigger extended action
            if (note>=NUM_TRACK_NOTES)
                return;

            note_pressed_at[note] = millis();
            note_released_at[note] = -1;

            already_processed[note] = false;

            #ifdef ENABLE_LOOPER
                if (note==CHOC_NOTE_TOGGLEPLAY) {
                    mpk49_loop_track.toggle_playing();
                } else if (note==CHOC_NOTE_TOGGLEREC) {
                    mpk49_loop_track.toggle_recording();
                } else if (note==CHOC_NOTE_RECORD_MOM) {
                    mpk49_loop_track.start_recording();
                } else if (note==CHOC_NOTE_OVERWRITE_MOM) {
                    mpk49_loop_track.start_overwriting();
                }
            #endif

            /*if (note==0 && note_pressed_at[note+1]>=0) {
                // trigger double action E
                Serial.println("pressed A+B -> E");
            } else if (note==1 && note_pressed_at[note+1]>=0) {
                // trigger double action F
                Serial.println("pressed C+D -> F");
            }*/
            // otherwise, wait for the loop() to fire single events       
        }

        void receive_note_off(uint8_t channel, uint8_t note, uint8_t velocity) override {
            // TODO: control looper(s) from here
            if (note>=NUM_TRACK_NOTES) 
                return;

            unsigned long now = millis();

            note_released_at[note] = now;
            note_pressed_at[note] = -1;

            already_processed[note] = false;

            #ifdef ENABLE_LOOPER
                if (note==CHOC_NOTE_RECORD_MOM) {
                    mpk49_loop_track.stop_recording();
                } else if (note==CHOC_NOTE_OVERWRITE_MOM) {
                    mpk49_loop_track.stop_overwriting();
                }
            #endif

            /*if (note==0 && note_released_at[note+1]>=0) {
                // trigger double action E
                Serial.println("released A+B -> E");
            } else if (note==1 && note_released_at[note+1]>=0) {
                // trigger double action F
                Serial.println("released C+D -> F");
            }*/

            //Serial.printf("chocolate got noteOff chan %i, note %i, velocity %i\n", channel, note, velocity); Serial.flush();
        }

        #define DOUBLE_BOUNCE_TIME 80
        bool together(long *type, int note1, int note2) {
            return type[note1]>=0 && type[note2]>=0 && abs(type[note1] - type[note2]) < DOUBLE_BOUNCE_TIME;
        }

        bool e_held = false, f_held = false;

        /*void loop(uint32_t ticks) override {
            
        }*/

        /*void loop(uint32_t ticks) override {
            if (!already_processed[0]) {
                if (!e_held && note_pressed_at[0]>=0 && note_pressed_at[1]==-1 && millis() - note_pressed_at[0] > DOUBLE_BOUNCE_TIME) {
                    Serial.println("pressed single A");
                    note_pressed_at[0] = -1;
                }
                if (!e_held && note_pressed_at[1]>=0 && note_pressed_at[0]==-1 && millis() - note_pressed_at[1] > DOUBLE_BOUNCE_TIME) {
                    Serial.println("pressed single B");
                    note_pressed_at[1] = -1;
                }

                if (!e_held && note_released_at[0]>=0 && note_released_at[1]==-1 && millis() - note_released_at[0] > DOUBLE_BOUNCE_TIME) {
                    Serial.println("released single A");
                    e_held = false;
                    note_released_at[0] = -1;
                }
                if (!e_held && note_released_at[1]>=0 && note_released_at[0]==-1 && millis() - note_released_at[1] > DOUBLE_BOUNCE_TIME) {
                    Serial.println("released single B");
                    e_held = false;
                    note_released_at[1] = -1;
                }

                if (together(note_pressed_at, 0, 1)) {
                    Serial.println("pressed A+B -> E");
                    e_held = true;
                    note_pressed_at[0] = note_pressed_at[1] = -1;
                }
                if (e_held && (note_released_at[0]>=0 || note_released_at[1]>=0)) {
                //if (together(note_released_at, 0, 1) && e_held) {
                    Serial.println("released A+B -> E");
                    e_held = false;
                    note_released_at[0] = note_released_at[1] = -1;
                }

                already_processed[0] = true;
            }

            return;

            if (together(note_pressed_at, 2, 3)) {
                Serial.println("pressed C+D -> F");
                f_held = true;
                note_pressed_at[2] = note_pressed_at[3] = -1;
            }
            if (e_held && (note_released_at[2]>=0 || note_released_at[3]>=0)) {
                //if (together(note_released_at, 2, 3) && f_held) {
                Serial.println("released C+D -> F");
                f_held = false;
                note_released_at[2] = note_released_at[3] = -1;
            }


            if (!f_held && note_pressed_at[2]>=0 && note_pressed_at[3]==-1 && millis() - note_pressed_at[2] > DOUBLE_BOUNCE_TIME) {
                Serial.println("pressed single C");
                note_pressed_at[2] = -1;
            }
            if (!f_held && note_pressed_at[3]>=0 && note_pressed_at[2]==-1 && millis() - note_pressed_at[3] > DOUBLE_BOUNCE_TIME) {
                Serial.println("pressed single D");
                note_pressed_at[3] = -1;
            }

            if (!e_held && note_released_at[0]>=0 && note_released_at[1]==-1 && millis() - note_released_at[0] > DOUBLE_BOUNCE_TIME) {
                Serial.println("released single A");
                note_released_at[0] = -1;
            }
            if (!e_held && note_released_at[1]>=0 && note_released_at[0]==-1 && millis() - note_released_at[1] > DOUBLE_BOUNCE_TIME) {
                Serial.println("released single B");
                note_released_at[1] = -1;
            }
            if (!f_held && note_released_at[2]>=0 && note_released_at[3]==-1 && millis() - note_released_at[2] > DOUBLE_BOUNCE_TIME) {
                Serial.println("released single C");
                note_released_at[2] = -1;
            }
            if (!f_held && note_released_at[3]>=0 && note_released_at[2]==-1 && millis() - note_released_at[3] > DOUBLE_BOUNCE_TIME) {
                Serial.println("released single D");
                note_released_at[3] = -1;
            }

        }*/

        /*void receive_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
            Serial.printf("chocolate got controlchange chan %i, cc %i, value %i\n", inChannel, inNumber, inValue); Serial.flush();
        }*/
};

extern DeviceBehaviour_Chocolate *behaviour_chocolate;

#endif

#endif