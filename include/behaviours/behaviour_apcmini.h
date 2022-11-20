#ifndef BEHAVIOUR_APCMINI__INCLUDED
#define BEHAVIOUR_APCMINI__INCLUDED

#include <Arduino.h>

#include "Config.h"
#include "behaviours/behaviour_base_usb.h"
#include "midi/midi_apcmini.h"
#include "project.h"
#include "clock.h"

#include "multi_usb_handlers.h"
#include "midi/midi_looper.h"

#include "midi/midi_apcmini_display.h"

// use parameter library adapter to handle cc events
#include "midi/midi_cc_source.h"

//extern MIDITrack mpk49_loop_track;
//class MIDITrack;

class DeviceBehaviour_APCMini : public DeviceBehaviourUSBBase, public MIDI_CC_Source {
    public:
        DeviceBehaviour_APCMini() : DeviceBehaviourUSBBase() {
            // initialise the CCs that this device can translate into ParameterInputs
            this->addParameterInput("Fade1", (byte)48, (byte)1);
            this->addParameterInput("Fade2", (byte)49, (byte)1);
            this->addParameterInput("Fade3", (byte)50, (byte)1);
            this->addParameterInput("Fade4", (byte)51, (byte)1);
        }

        uint16_t vid = 0x09e8, pid = 0x0028;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return (char*)"APCMini";
        }
        virtual bool has_input() { return true; }

        bool apcmini_shift_held = false;

        MIDITrack *loop_track = nullptr;

        #ifdef ENABLE_CLOCKS
            byte clock_selected = 0;
        #endif
        bool redraw_immediately = false;
        unsigned long last_updated_display = 0;

        FLASHMEM 
        virtual void setup_callbacks() override {
            //behaviour_apcmini = this;
            if (!is_connected()) return;

            this->device->setHandleControlChange(apcmini_control_change);
            this->device->setHandleNoteOn(apcmini_note_on);
            this->device->setHandleNoteOff(apcmini_note_off);
        };

        FLASHMEM
        virtual void init() override {
            DeviceBehaviourUSBBase::init();
            apcdisplay_initialise_last_sent();
            apcmini_clear_display();
        }

        virtual void loop(unsigned long ticks) override {
            if (!is_connected()) return;

            #ifdef ENABLE_APCMINI_DISPLAY
                static unsigned long last_processed_tick;

                if (last_processed_tick != ticks) {
                    //Serial.println("about to call apcmini_update_position_display()"); Serial_flush();
                    apcmini_update_position_display(ticks);
                
                    if (redraw_immediately || millis() - last_updated_display > 50) {
                        //Serial.println(F("redraw_immediately is set!"));
                        //Serial.println("about to call apcmini_update_clock_display()"); Serial_flush();
                        apcmini_update_clock_display();
                        redraw_immediately = false;
                    }
                    //ATOMIC(
                        last_processed_tick = ticks;
                    //)
                }
            #endif
            //Serial.println(F("finished apcmini_loop"));
        }

        virtual void receive_note_on(byte inChannel, byte inNumber, byte inVelocity) override {
            Serial.printf(F("apcmini_note_on for %i, %i, %i\n"), inChannel, inNumber, inVelocity);
            if (inNumber==APCMINI_BUTTON_STOP_ALL_CLIPS && !apcmini_shift_held) {
                // start / stop play
                if (!playing)
                    global_on_restart();
                
                playing = !playing;    

            #ifdef ENABLE_LOOPER
                } else if (inNumber==APCMINI_BUTTON_STOP_ALL_CLIPS && apcmini_shift_held) {
                    if (this->loop_track!=nullptr) 
                        this->loop_track->clear_all();
                //mpk49_loop_track.clear_all();
            #endif
            } else if (inNumber==BUTTON_RESTART_IMMEDIATELY && apcmini_shift_held) { // up pressed with shift
                // restart/resync immediately
                Serial.println(F("APCmini pressed, restarting downbeat"));
                global_on_restart();
            } else if (inNumber==BUTTON_RESTART_AT_END_OF_BAR && apcmini_shift_held) {
                // restart/resync at end of bar
                Serial.println(F("APCmini pressed, restarting downbeat on next bar"));
                #ifdef ENABLE_APCMINI_DISPLAY
                    //ATOMIC(
                    //sendNoteOn(7, APCMINI_GREEN_BLINK, 1);
                    apcdisplay_sendNoteOn(7, APCMINI_GREEN_BLINK, 1);
                    //)  // turn on the 'going to restart on next bar' flashing indicator
                #endif
                restart_on_next_bar = true;
            #ifdef ENABLE_CLOCKS
                } else if (inNumber==APCMINI_BUTTON_UP) {
                    // move clock selection up
                    byte old_clock_selected = clock_selected;
                    //redraw_immediately = true;
                    if (clock_selected==0)
                    clock_selected = NUM_CLOCKS-1;
                    else
                    clock_selected--;
                    #ifdef ENABLE_APCMINI_DISPLAY
                        redraw_clock_selected(old_clock_selected, clock_selected);
                    #endif
                } else if (inNumber==APCMINI_BUTTON_DOWN) {
                    // move clock selection down
                    byte old_clock_selected  = clock_selected;
                    //redraw_immediately = true;
                    clock_selected++;
                    if (clock_selected>=NUM_CLOCKS) 
                    clock_selected = 0;
                    #ifdef ENABLE_APCMINI_DISPLAY
                        redraw_clock_selected(old_clock_selected, clock_selected);
                    #endif
                } else if (inNumber==APCMINI_BUTTON_LEFT) {
                    // shift clock offset left
                    redraw_immediately = true;
                    decrease_clock_delay(clock_selected);
                    //redraw_immediately = true;
                    #ifdef ENABLE_APCMINI_DISPLAY
                        redraw_clock_row(clock_selected);
                    #endif
                } else if (inNumber==APCMINI_BUTTON_RIGHT) {
                    // shift clock offset right
                    redraw_immediately = true;
                    increase_clock_delay(clock_selected);
                    #ifdef ENABLE_APCMINI_DISPLAY
                        redraw_clock_row(clock_selected);
                    #endif
                } else if (inNumber>=APCMINI_BUTTON_CLIP_STOP && inNumber<=APCMINI_BUTTON_MUTE) {
                    // button between Clip Stop -> Solo -> Rec arm -> Mute buttons
                    // change divisions/multiplier of corresponding clock
                    byte clock_number = inNumber - APCMINI_BUTTON_CLIP_STOP;  
                    byte old_clock_selected = clock_selected;
                    clock_selected = clock_number;
                    
                    if (apcmini_shift_held) {
                        increase_clock_multiplier(clock_number);
                        //clock_multiplier[clock_number] *= 2;   // double the selected clock multiplier -> more pulses
                    } else {
                        decrease_clock_multiplier(clock_number);
                        //clock_multiplier[clock_number] /= 2;   // halve the selected clock multiplier -> fewer pulses
                    }
                    
                    /*if (clock_multiplier[clock_number]>CLOCK_MULTIPLIER_MAX)
                    clock_multiplier[clock_number] = CLOCK_MULTIPLIER_MIN;
                    else if (clock_multiplier[clock_number]<CLOCK_MULTIPLIER_MIN) 
                    clock_multiplier[clock_number] = CLOCK_MULTIPLIER_MAX;*/

                    #ifdef ENABLE_APCMINI_DISPLAY
                        redraw_clock_row(clock_selected);
                        redraw_clock_selected(old_clock_selected, clock_selected);
                    #endif
            #endif
            } else if (inNumber==APCMINI_BUTTON_SHIFT) {
                apcmini_shift_held = true;
                /*  } else if (inNumber==APCMINI_BUTTON_UNLABELED_1) {
                // for debugging -- single-step through a tick
                single_step = true;
                //ticks += 1;
                Serial.print(F("Single-stepped to tick "));
                Serial.println(ticks);*/
            } else if (apcmini_shift_held && inNumber==APCMINI_BUTTON_UNLABELED_1) {
                //load_state_start(project.selected_sequence_number, &project.current_state);
                project.load_sequence(); //project.selected_sequence_number);
                #ifdef ENABLE_APCMINI_DISPLAY
                    apcmini_update_clock_display();
                #endif
            } else if (apcmini_shift_held && inNumber==APCMINI_BUTTON_UNLABELED_2) {
                //save_sequence(project.selected_sequence_number, &project.current_state);
                project.save_sequence(); //project.selected_sequence_number);
                #ifdef ENABLE_SEQUENCER
            } else if (inNumber>=0 && inNumber < NUM_SEQUENCES * APCMINI_DISPLAY_WIDTH) {
                    byte row = 3 - (inNumber / APCMINI_DISPLAY_WIDTH);
                    /*Serial.print(F("For inNumber "));
                    Serial.print(inNumber);
                    Serial.print(F(" got row (ie clock) "));
                    Serial.print(row);
                    Serial.print(F(" and column "));*/
                    byte col = inNumber - ((3-row)*APCMINI_DISPLAY_WIDTH);
                    Serial.println(col);
                    sequencer_press(row, col, apcmini_shift_held);
                    #ifdef ENABLE_APCMINI_DISPLAY
                        redraw_sequence_row(row);
                    #endif
                #endif 
            #ifdef ENABLE_DRUM_LOOPER
            } else if (apcmini_shift_held && inNumber==APCMINI_BUTTON_SEND) {
                // toggle drums recording status?
                drums_loop_track.toggle_recording();
            } else if (inNumber==APCMINI_BUTTON_PAN) {
                if (apcmini_shift_held) {
                    drums_loop_track.stop_playing();
                } else {
                    drums_loop_track.start_playing();
                }
            #endif
            } else {
                Serial.print(F("Unknown akaiAPC button with note number "));
                Serial.println(inNumber);//if (inNumber<(8*8) && inNumber>=(8*5)) {
            }

            if (redraw_immediately) 
                last_updated_display = 0;
        }

        virtual void receive_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) override {
            if (inNumber==APCMINI_BUTTON_SHIFT) {
                apcmini_shift_held = false;
            }
        }

        // called from loop, already inside ATOMIC, so don't use ATOMIC here
        virtual void receive_control_change (uint8_t channel, uint8_t number, uint8_t value) override {
            //ATOMIC(
                /*Serial.print(F("APCMINI CC ch"));
                Serial.print(inChannel);
                Serial.print(F("\tnum "));
                Serial.print(inNumber);
                Serial.print(F("\tvalue: "));
                Serial.println(inValue);*/
            //)
            //debug_free_ram();

            #ifdef ENABLE_BPM
                if (number==APCMINI_FADER_MASTER) {   // 56 == "master" fader set bpm 
                    if (clock_mode==CLOCK_INTERNAL)
                        set_bpm(map(value, 0, 127, BPM_MINIMUM, BPM_MAXIMUM)); // scale CC value
                }
            #endif

            this->update_parameter_inputs_cc(number, value, channel);
        }


        virtual void on_tick(volatile uint32_t ticks) override {
            if (device!=nullptr) {

            }
        }

        virtual void on_restart() override {
            if (device!=nullptr) {
                #ifdef ENABLE_APCMINI_DISPLAY
                    apcdisplay_sendNoteOn(7, APCMINI_OFF, 1);  // turn off the flashing 'going to restart on next bar' indicator
                #endif
            }
        }

};

extern DeviceBehaviour_APCMini *behaviour_apcmini;

void apcmini_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void apcmini_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void apcmini_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

#endif