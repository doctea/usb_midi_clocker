#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef ENABLE_APCMINI

#include "behaviours/behaviour_base_usb.h"
#include "midi/midi_apcmini.h"
#include "project.h"
#include "clock.h"

#include "usb/multi_usb_handlers.h"
#include "midi/midi_looper.h"

#include "midi/midi_apcmini_display.h"

// use parameter library adapter to handle cc events
#include "midi/midi_cc_source.h"

#include "behaviours/behaviour_gate_clocks.h"
#include "behaviours/behaviour_gate_sequencer.h"

// config
#define TIME_BETWEEN_APC_REFRESH_MS 50
//#define APCMINI_STOP_ALL_MODE_CLOCK   // make the 'stop all clips' button enable/disable sequencer

class DeviceBehaviour_APCMini : virtual public DeviceBehaviourUSBBase, virtual public MIDI_CC_Source {
    public:
        DeviceBehaviour_APCMini() : DeviceBehaviourUSBBase() {
            // initialise the CCs that this device can translate into ParameterInputs
            this->addParameterInput("Fade1", "APCMini", (byte)APCMINI_FADER_CC_1, (byte)1);
            this->addParameterInput("Fade2", "APCMini", (byte)APCMINI_FADER_CC_2, (byte)1);
            this->addParameterInput("Fade3", "APCMini", (byte)APCMINI_FADER_CC_3, (byte)1);
            this->addParameterInput("Fade4", "APCMini", (byte)APCMINI_FADER_CC_4, (byte)1);
            this->addParameterInput("Fade5", "APCMini", (byte)APCMINI_FADER_CC_5, (byte)1);
            this->addParameterInput("Fade6", "APCMini", (byte)APCMINI_FADER_CC_6, (byte)1);
            this->addParameterInput("Fade7", "APCMini", (byte)APCMINI_FADER_CC_7, (byte)1);
            this->addParameterInput("Fade8", "APCMini", (byte)APCMINI_FADER_CC_8, (byte)1);
        }

        uint16_t vid = 0x09e8, pid = 0x0028;
        virtual uint32_t get_packed_id () override { return (this->vid<<16 | this->pid); }

        virtual const char *get_label() override {
            return "APCMini";
        }
        virtual bool receives_midi_notes() { return true; }

        bool apcmini_shift_held = false;
        uint8_t tempo_fader_cc = APCMINI_FADER_CC_MASTER;

        #ifdef ENABLE_LOOPER
            MIDITrack *loop_track = nullptr;
        #endif

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
            apcmini_clear_display(true);
            redraw_immediately = true;
            //apcmini_update_clock_display();
        }

        virtual void loop(unsigned long ticks) override {
            if (!is_connected()) return;

            #ifdef ENABLE_APCMINI_DISPLAY
                static unsigned long last_processed_tick;

                if (last_processed_tick != ticks) {
                    //Serial.println("about to call apcmini_update_position_display()"); Serial_flush();
                    apcmini_update_position_display(ticks);
                
                    if (redraw_immediately || millis() - last_updated_display > TIME_BETWEEN_APC_REFRESH_MS) {
                        //Serial.println(F("redraw_immediately is set!"));
                        //Serial.println("about to call apcmini_update_clock_display()"); Serial_flush();
                        apcmini_update_clock_display();
                        redraw_immediately = false;
                    }
                    last_processed_tick = ticks;
                }
            #endif
            //Serial.println(F("finished apcmini_loop"));
        }

        bool process_note_on_clock_page(byte inChannel, byte inNumber, byte inVelocity) {
            #ifdef ENABLE_CLOCKS
                if (inNumber==APCMINI_BUTTON_UP) {
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
                    return true;
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
                    return true;
                } else if (inNumber==APCMINI_BUTTON_LEFT) {
                    // shift clock offset left
                    redraw_immediately = true;
                    behaviour_clock_gates->decrease_clock_delay(clock_selected);
                    //redraw_immediately = true;
                    #ifdef ENABLE_APCMINI_DISPLAY
                        redraw_clock_row(clock_selected);
                    #endif
                    return true;
                } else if (inNumber==APCMINI_BUTTON_RIGHT) {
                    // shift clock offset right
                    redraw_immediately = true;
                    behaviour_clock_gates->increase_clock_delay(clock_selected);
                    #ifdef ENABLE_APCMINI_DISPLAY
                        redraw_clock_row(clock_selected);
                    #endif
                    return true;
                } else if (inNumber>=APCMINI_BUTTON_CLIP_STOP && inNumber<=APCMINI_BUTTON_CLIP_STOP+NUM_CLOCKS) { //APCMINI_BUTTON_MUTE) {
                    // button between Clip Stop -> Solo -> Rec arm -> Mute buttons
                    // change divisions/multiplier of corresponding clock
                    byte clock_number = inNumber - APCMINI_BUTTON_CLIP_STOP;  
                    byte old_clock_selected = clock_selected;
                    clock_selected = clock_number;
                    
                    if (apcmini_shift_held) {
                        behaviour_clock_gates->increase_clock_multiplier(clock_number);
                        //clock_multiplier[clock_number] *= 2;   // double the selected clock multiplier -> more pulses
                    } else {
                        behaviour_clock_gates->decrease_clock_multiplier(clock_number);
                        //clock_multiplier[clock_number] /= 2;   // halve the selected clock multiplier -> fewer pulses
                    }
                    
                    /*if (clock_multiplier[clock_number]>CLOCK_MULTIPLIER_MAX)
                    clock_multiplier[clock_number] = CLOCK_MULTIPLIER_MIN;
                    else if (clock_multiplier[clock_number]<CLOCK_MULTIPLIER_MIN) 
                    clock_multiplier[clock_number] = CLOCK_MULTIPLIER_MAX;*/

                    #ifdef ENABLE_APCMINI_DISPLAY
                        redraw_clock_row(clock_selected);
                        //redraw_clock_selected(old_clock_selected, clock_selected);
                    #endif
                    return true;
                }
            #endif
            return false;
        }

        bool process_note_on_sequence_page(byte inChannel, byte inNumber, byte inVelocity) {
            if (inNumber>=0 && inNumber < NUM_SEQUENCES * APCMINI_DISPLAY_WIDTH) {
                byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
                /*Serial.print(F("For inNumber "));
                Serial.print(inNumber);
                Serial.print(F(" got row (ie clock) "));
                Serial.print(row);
                Serial.print(F(" and column "));*/
                byte col = inNumber - (((NUM_SEQUENCES-1)-row)*APCMINI_DISPLAY_WIDTH);
                //Serial.println(col);
                behaviour_sequencer_gates->sequencer_press(row, col, apcmini_shift_held);
                #ifdef ENABLE_APCMINI_DISPLAY
                    redraw_sequence_row(row);
                #endif
                return true;
            } else if (!apcmini_shift_held && inNumber>=APCMINI_BUTTON_CLIP_STOP && inNumber <= APCMINI_BUTTON_STOP_ALL_CLIPS) {
                byte row = inNumber - APCMINI_BUTTON_CLIP_STOP;
                behaviour_sequencer_gates->toggle_track_active(row);
                return true;
            } else if (apcmini_shift_held && inNumber==APCMINI_BUTTON_STOP_ALL_CLIPS) {
                #ifdef APCMINI_STOP_ALL_MODE_CLOCK
                    // start / stop play
                    if (!playing)
                        global_on_restart();
                    
                    //playing = !playing;
                    if (playing)
                        clock_stop();
                    else
                        clock_continue();
                #else
                    behaviour_sequencer_gates->set_sequencer_enabled(!behaviour_sequencer_gates->is_sequencer_enabled());
                #endif
                return true;
            }

            return false;
        }

        bool process_note_on_patterns_page(byte inChannel, byte inNumber, byte inVelocity) {
            if (inNumber>=0 && inNumber < NUM_SEQUENCES * APCMINI_DISPLAY_WIDTH) {
                byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
                byte col = inNumber - (((NUM_SEQUENCES-1)-row)*APCMINI_DISPLAY_WIDTH);
                if (row==0) {
                    if (apcmini_shift_held) {
                        project->save_pattern(col);
                    } else {
                        if (!project->is_selected_pattern_number_empty(col)) {
                            project->load_pattern(col);
                        } else {
                            project->select_pattern_number(col);
                            behaviour_sequencer_gates->sequencer_clear_pattern();
                        }
                    }
                    return true;
                }
            }
            return false;
        }

        bool process_note_on_pads_page(byte inChannel, byte inNumber, byte inVelocity) {
            if (inNumber>=0 && inNumber < NUM_SEQUENCES * APCMINI_DISPLAY_WIDTH) {
                // process drum pad
                byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
                byte col = inNumber - (((NUM_SEQUENCES-1)-row)*APCMINI_DISPLAY_WIDTH);

                Serial_printf("process_note_on_pads_page(%i, %i, %i)\n", inChannel, inNumber, inVelocity);

                if (row>=4 && row <= 7 && col>=0 && col<=3) {
                    row = 7 - row;
                    //row = 3 - row;
                    byte note = 36 + (row*4) + col;
                    DeviceBehaviourUltimateBase::receive_note_on(GM_CHANNEL_DRUMS, note, MIDI_MAX_VELOCITY);
                    return true;
                }
            }
            return false;
        }

        bool process_note_off_pads_page(byte inChannel, byte inNumber, byte inVelocity) {
            if (inNumber>=0 && inNumber < NUM_SEQUENCES * APCMINI_DISPLAY_WIDTH) {
                // process drum pad
                byte row = (NUM_SEQUENCES-1) - (inNumber / APCMINI_DISPLAY_WIDTH);
                byte col = inNumber - (((NUM_SEQUENCES-1)-row)*APCMINI_DISPLAY_WIDTH);

                Serial_printf("process_note_on_pads_page(%i, %i, %i)\n", inChannel, inNumber, inVelocity);

                if (row>=4 && row <= 7 && col>=0 && col<=3) {
                    row = 7 - row;
                    //row = 3 - row;
                    byte note = 36 + (row*4) + col;
                    Serial_printf("sending for note %i: %s\n", note, get_note_name_c(note, GM_CHANNEL_DRUMS));
                    DeviceBehaviourUltimateBase::receive_note_off(GM_CHANNEL_DRUMS, note, MIDI_MAX_VELOCITY);
                    return true;
                }
            }
            return false;
        }

        /*bool process_note_on_defaults(byte inChannel, byte inNumber, byte inVelocity) {
            return false;
        }*/

        virtual void receive_note_on(byte inChannel, byte inNumber, byte inVelocity) override {
            Debug_printf(F("apcmini_note_on for %i, %i, %i\n"), inChannel, inNumber, inVelocity);

            if (get_apc_gate_page()==CLOCKS && process_note_on_clock_page(inChannel, inNumber, inVelocity)) {
                // on CLOCKS page and processed value
                return;
            } else if (get_apc_gate_page()==SEQUENCES && process_note_on_sequence_page(inChannel, inNumber, inVelocity)) {
                // on SEQUENCES page and processed value
                return;
            } else if (get_apc_gate_page()==PATTERNS && process_note_on_patterns_page(inChannel, inNumber, inVelocity)) {
                // on PATTERNS page and processed value
                return;
            } else if (get_apc_gate_page()==PADS && process_note_on_pads_page(inChannel, inNumber, inVelocity)) {
                return;
            } /* else {
                process_note_on_defaults(inChannel, inNumber, inVelocity);
            }*/

            if (false) {    // dummy 
            #ifdef ENABLE_LOOPER
                } else if (inNumber==APCMINI_BUTTON_STOP_ALL_CLIPS && apcmini_shift_held) {
                    #ifdef ENABLE_LOOPER
                        if (this->loop_track!=nullptr) 
                            this->loop_track->clear_all();
                    #endif
                //mpk49_loop_track.clear_all();
            #endif

            } else if (inNumber==BUTTON_RESTART_IMMEDIATELY && apcmini_shift_held) { // up pressed with shift
                // restart/resync immediately
                Debug_println(F("APCmini pressed, restarting downbeat"));
                global_on_restart();
            } else if (inNumber==BUTTON_RESTART_AT_END_OF_BAR && apcmini_shift_held) {
                // restart/resync at end of bar
                Debug_println(F("APCmini pressed, restarting downbeat on next bar"));
                #ifdef ENABLE_APCMINI_DISPLAY
                    //ATOMIC(
                    //sendNoteOn(7, APCMINI_GREEN_BLINK, 1);
                    apcdisplay_sendNoteOn(7, APCMINI_GREEN_BLINK, 1);
                    //)  // turn on the 'going to restart on next bar' flashing indicator
                #endif
                set_restart_on_next_bar(true);

            } else if (inNumber==APCMINI_BUTTON_VOLUME) {
                apcmini_clear_display(false);
                set_apc_gate_page(CLOCKS);
            } else if (inNumber==APCMINI_BUTTON_PAN) {
                apcmini_clear_display(false);
                set_apc_gate_page(SEQUENCES);
            } else if (inNumber==APCMINI_BUTTON_SEND) {
                apcmini_clear_display(false);
                set_apc_gate_page(PATTERNS);
            } else if (inNumber==APCMINI_BUTTON_DEVICE) {
                apcmini_clear_display(false);
                set_apc_gate_page(PADS);
            } else if (inNumber==APCMINI_BUTTON_SHIFT) {
                apcmini_shift_held = true;
                /*  } else if (inNumber==APCMINI_BUTTON_UNLABELED_1) {
                // for debugging -- single-step through a tick
                single_step = true;
                //ticks += 1;
                Serial.print(F("Single-stepped to tick "));
                Serial.println(ticks);*/
            } else if (apcmini_shift_held && inNumber==APCMINI_BUTTON_UNLABELED_1) {
                //load_state_start(project.selected_pattern_number, &project.current_state);
                project->load_pattern(); //project.selected_pattern_number);
                #ifdef ENABLE_APCMINI_DISPLAY
                    apcmini_update_clock_display();
                #endif
            } else if (apcmini_shift_held && inNumber==APCMINI_BUTTON_UNLABELED_2) {
                //save_pattern(project.selected_pattern_number, &project.current_state);
                project->save_pattern(); //project.selected_pattern_number);
            #ifdef ENABLE_DRUM_LOOPER
            } else if (apcmini_shift_held && inNumber==APCMINI_BUTTON_SEND) {
                // toggle drums recording status?
                drums_loop_track.toggle_recording();
            } else if (inNumber==APCMINI_BUTTON_DEVICE) {
                if (apcmini_shift_held) {
                    drums_loop_track.stop_playing();
                } else {
                    drums_loop_track.start_playing();
                }
            #endif
            } else {
                Debug_print(F("Unknown akaiAPC button with note number "));
                Debug_println(inNumber);//if (inNumber<(8*8) && inNumber>=(8*5)) {
            }

            if (redraw_immediately) 
                last_updated_display = 0;
        }

        virtual void receive_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) override {
            if (inNumber==APCMINI_BUTTON_SHIFT) {
                apcmini_shift_held = false;
            }
            if (get_apc_gate_page()==PADS && process_note_off_pads_page(inChannel, inNumber, inVelocity)) {
                return;
            }
        }

        virtual void receive_control_change(uint8_t channel, uint8_t number, uint8_t value) override {
            #ifdef ENABLE_BPM
                if (number==tempo_fader_cc/*APCMINI_FADER_CC_MASTER*/) {   // 56 == "master" fader set bpm 
                    if (clock_mode==CLOCK_INTERNAL)
                        set_bpm(map(value, 0, 127, BPM_MINIMUM, BPM_MAXIMUM)); // scale CC value
                }
            #endif

            this->update_parameter_inputs_cc(number, value, channel);
        }


        virtual void on_tick(volatile uint32_t ticks) override {
            if (device!=nullptr) {
                //this->actualSendNoteOn(ticks % 64, random(APCMINI_YELLOW_BLINK), 1);
                //apcdisplay_sendNoteOn(ticks % 64, random(APCMINI_YELLOW_BLINK), 1);
            }
        }

        virtual void on_restart() override {
            if (device!=nullptr) {
                #ifdef ENABLE_APCMINI_DISPLAY
                    apcdisplay_sendNoteOn(7, APCMINI_OFF, 1);  // turn off the flashing 'going to restart on next bar' indicator
                #endif
            }
        }

        virtual void save_project_add_lines(LinkedList<String> *lines) override {
            DeviceBehaviourUSBBase::save_project_add_lines(lines);
            lines->add(String("tempo_cc=")+String(this->tempo_fader_cc));
        }

        virtual bool parse_project_key_value(String key, String value) override {
            if (key.equals("tempo_cc")) {
                this->tempo_fader_cc = value.toInt();
                return true;
            }
            return DeviceBehaviourUSBBase::parse_project_key_value(key, value);
        }

        //FLASHMEM // causes a section type conflict with virtual void DeviceBehaviour_APCMini::setup_callbacks() 
        LinkedList<MenuItem*> *make_menu_items() override;

};

extern DeviceBehaviour_APCMini *behaviour_apcmini;

void apcmini_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
void apcmini_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
void apcmini_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);

#endif
