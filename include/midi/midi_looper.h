#pragma once

#include "debug.h"

#include <util/atomic.h>

#include <LinkedList.h>
#include "Config.h"

#ifdef ENABLE_LOOPER

#include "ConfigMidi.h"
#include "midi/MidiMappings.h"
//#include "midi/midi_out_wrapper.h"

#include "midi_helpers.h"

#include "midi/midi_mapper_matrix_types.h"
//#include "midi/midi_mapper_matrix_manager.h"

#include "bpm.h"

#include "storage.h"

//#define DEBUG_LOOPER

#define ticks_to_sequence_step(X)  ((X % LOOP_LENGTH_TICKS) / LOOP_LENGTH_STEP_SIZE)

#define MAX_INSTRUCTIONS            100
#define MAX_INSTRUCTION_ARGUMENTS   4

#ifdef ENABLE_SCREEN
    class MenuItem;
#endif

using namespace storage;

extern volatile bool global_load_lock;

//#define LOWEST_LOOPER_NOTE  24
//#define HIGHEST_LOOPER_NOTE 96

// for storing messages for recording+playback by midi looper
// todo: expand to handle other message types (CCs?)
struct midi_message {
    uint8_t message_type;
    uint8_t pitch;
    uint8_t velocity;
    uint8_t channel;
};

struct tracked_note {
    bool playing = false;
    uint8_t velocity = MIDI_MIN_VELOCITY;
    //int32_t started_at = -1;
};


class MIDITrack {
    LinkedList<midi_message> *frames[LOOP_LENGTH_STEPS];

    tracked_note recorded_hanging_notes[MIDI_NUM_NOTES];
    int loaded_recording_number = -1;

    int quantization_value = 4; // 4th of a quarter-note, ie 1 step, ie 6 pulses
    /*  
        # UNTESTED: -2 = quantize to bar          (note)          
        # UNTESTED: -1 = quantize to half-bar     (half-note)     
        # 0 = no quantization
        # 1 = quantize to beat          (quarter-note)
        # 2 = quantize to half-beat     (eighth-note)
        # 3 = quantize to third-beat    (sixteenth-note)
        # 4 = quantize to quarter-beat  (thirty-second-note)
    */
    int find_nearest_quantized_time(int time, int quantization) {
        if (quantization==0) // if quantization is 0 then don't quantize at all
            return time;

        int ticks_per_quant_level;
        if (quantization>0) {
            ticks_per_quant_level = PPQN/quantization;      // get number of ticks per quantized unit
        } else if (quantization==-1) {
            ticks_per_quant_level = PPQN * 2;
        } else if (quantization==-2) {
            ticks_per_quant_level = PPQN * 4;
        } else {
            Serial.printf(F("quantization level %i not known - disabling?\n"), quantization);
            return time;
        }

        int step_num = time / ticks_per_quant_level;        // break ticks into quantized unit
        int step_start_at_tick = step_num * ticks_per_quant_level;  // find the start of the quantized unit
        int diff = time - step_start_at_tick;               // get how many ticks we are away from the start of nearest quantized unit
        //Serial.printf("for time\t%i, got step_num\t%i, step_start_at\t%i, diff\t%i\n", time, step_num, step_start_at_tick, diff);

        int step;
        if (diff < (ticks_per_quant_level/2)) {     // quantize to current quantized unit
            step = step_start_at_tick;
        } else {                                    // quantize to next quantized unit
            step = step_start_at_tick + ticks_per_quant_level;
        }
        //Serial.printf("for time\t%i, got step\t%i\n", time, step);

        int quantized_time = step % LOOP_LENGTH_TICKS;    // wrap around
        //Serial.printf("quantised time\t%i to\t%i\n", time, quantized_time);
        if (debug)
            Serial.printf(F("Quantize level\t%i: quantized time\t%i to\t%i\n"), quantization, time, quantized_time);
        return quantized_time;
    }
    int quantize_time(int time, int quantization = -1) {
        if (quantization==-1)
            quantization = quantization_value;
        int quantized_time = find_nearest_quantized_time(time, quantization) % LOOP_LENGTH_TICKS;
        return quantized_time;
    }

    public: 
        //MIDIOutputWrapper *output = nullptr;

        source_id_t source_id = -1;
        
        bool debug = false;

        bool recording = false;
        bool playing = false;

        bool overwrite = false;

        int last_note = -1;
        int current_note = -1;

        int8_t transpose_amount = 0;

        bool bitmap_enabled = true;

        MIDITrack() {
            this->wipe_piano_roll_bitmap();
            for(int i = 0 ; i < LOOP_LENGTH_STEPS ; i++) {
                this->frames[i] = new LinkedList<midi_message>();
            }
        };

        /*MIDITrack(MIDIOutputWrapper *default_output) : MIDITrack () {
            output = default_output;
        };*/

        /*void setOutputWrapper(MIDIOutputWrapper *output) {
            if (this->output!=nullptr)
                this->output->stop_all_notes();
            this->output = output;
            Serial.printf("MIDITrack#setOutputWrapper in midi_looper wrapper to '%s'\n", this->output->label);
        }*/

        // for getting and setting from menu
        int get_quantization_value() {
            return quantization_value;
        }
        void set_quantization_value(int qv) {
            quantization_value = qv;
        }

        // set the loop transposition
        void set_transpose(int transpose) {
            stop_all_notes();
            this->transpose_amount = transpose;
        }
        int get_transpose() {
            return this->transpose_amount;
        }

        // for actually storing values into buffer (also used when reloading from save)
        void store_event(midi_message midi_event) {
            unsigned long time = ticks % (LOOP_LENGTH_TICKS);
            //time = quantize_time(time);
            store_event(time, midi_event);
        }
        // for actually storing values into buffer (also used when reloading from save)
        void store_event(unsigned long time, uint8_t instruction_type, /*uint8_t channel,*/ int8_t pitch, uint8_t velocity) {
            midi_message m;
            m.message_type = instruction_type;
            //m.channel = 3; //channel;
            m.pitch = pitch;
            m.velocity = velocity;
            store_event(time, m);
        }
        // for actually storing values into buffer (also used when reloading from save)
        void store_event(unsigned long time, midi_message midi_event) {
            if (this->debug) Serial.printf(F("store_event passed time %i...\n"), time);
            time = quantize_time(time);
            time = ticks_to_sequence_step(time);
            if (this->debug) Serial.printf(F("Recording event at\t%i\n"), time);
            int event_duplicate = is_event_duplicate(midi_event, time);
            if (event_duplicate)
                frames[time]->remove(event_duplicate);
            frames[time]->add(midi_event);
            if (midi_event.message_type==midi::NoteOn) {
                recorded_hanging_notes[midi_event.pitch] = (tracked_note) { 
                    .playing = true, 
                    .velocity = midi_event.velocity, 
                    //.started_at = (int32_t)time 
                };
            } else if (midi_event.message_type==midi::NoteOff) {
                recorded_hanging_notes[midi_event.pitch] = (tracked_note) {
                    .playing        = false,
                    .velocity       = MIDI_MIN_VELOCITY
                    //.started_at     = -1
                };
            }
            //Serial.printf("store_event at %i with pitch %i is recording\n", time, midi_event.pitch);
            pitch_contains_notes[midi_event.pitch] = true;
            if (this->debug) Serial.printf(F("sizeof frames at %i is now %i\n"), time, frames[time]->size());
        }

        unsigned int is_event_duplicate(midi_message midi_event, unsigned long time) {
            for (unsigned int i = 0 ; i < frames[time]->size() ; i++) {
                if(event_matches(midi_event,frames[time]->get(i)))
                    return i;
            }
            return 0;
        }
        bool event_matches(midi_message ev1, midi_message ev2) {
            return (
                ev1.message_type==ev2.message_type && 
                ev1.channel==ev2.channel &&
                ev1.pitch==ev2.pitch //&&
                //ev1.velocity==ev1.velocity
            );
        }

        // get total event count across entire loop
        int count_events() {
            int count = 0;
            for (unsigned int i = 0 ; i < LOOP_LENGTH_STEPS ; i++) {
                count += frames[i]->size();
            }
            return count;
        }

        // clear any notes that we're recording
        void clear_hanging() {
            for (uint_fast8_t i = 0 ; i < MIDI_NUM_NOTES ; i++) {
                recorded_hanging_notes[i] = (tracked_note) {
                    .playing        = false,
                    .velocity       = MIDI_MIN_VELOCITY
                    //.started_at     = -1
                };
            }
        }

        // get the list of events for a specific loop point
        LinkedList<midi_message> get_frames(unsigned long time) {
            return *this->frames[ticks_to_sequence_step(time)]; //time%LOOP_LENGTH];
        }

        // actually play events for a specified loop point
        /*void play_events(unsigned long time) {
            //if (!specified_output)
            //    specified_output = output;
            int position = time%LOOP_LENGTH;
            //LinkedList<midi_message> frame = get_frames(position);
            #ifdef DEBUG_LOOPER
                Serial.printf("play_events with time %u becomes position %u\n", time, position); Serial_flush();
            #endif

            int number_messages = frames[position].size();
            #ifdef DEBUG_LOOPER
                //Serial.printf("play_events got %i messages\n", number_messages); Serial_flush();
                if (number_messages>0) 
                    Serial.printf("\tfor frame\t%u got\t%i messages to play\n", position, number_messages); Serial_flush();
            #endif

            for (unsigned int i = 0 ; i < number_messages ; i++) {
                #ifdef DEBUG_LOOPER
                    Serial.printf("\tprocessing message number %i/%i..\n", i, number_messages); Serial_flush();
                #endif
                midi_message m = frames[position].get(i);
                
                int pitch = m.pitch + transpose;
                if (pitch<0 || pitch > 127) {
                    if (this->debug) { Serial.printf("\t!!transposed pitch %i (was %i with transpose %i) went out of range!\n", pitch, m.pitch, transpose); Serial_flush(); }
                    return;
                } else {
                    if (this->debug) { Serial.printf("\ttransposed pitch %i (was %i with transpose %i) within range!\n", pitch, m.pitch, transpose); Serial_flush(); }
                }
                if (this->debug) { Serial.printf("\tgot transposed pitch %i from %i + %i\n", pitch, m.pitch, transpose); Serial_flush(); }

                switch (m.message_type) {
                    case midi::NoteOn:
                        current_note = pitch;
                        if (this->debug) { Serial.printf("\t\tSending note on %i at velocity %i\n", pitch, m.velocity); Serial_flush(); }
                        this->sendNoteOn((uint8_t)pitch, (int8_t)m.velocity);
                        track_playing_on(ticks, pitch, m.velocity);
                        break;
                    case midi::NoteOff:
                        if (this->debug) { Serial.printf("\t\tSending note off %i at velocity %i\n", pitch, m.velocity); Serial_flush(); }
                        last_note = pitch;
                        if (m.pitch==current_note) // todo: properly check that there are no other notes playing
                            current_note = -1;
                        this->sendNoteOff((uint8_t)pitch, (int8_t)m.velocity); //, m.channel);
                        track_playing_off(ticks, pitch, m.velocity);
                        break;
                    default:
                        if (this->debug) Serial.printf("\t%i: !!Unhandled message type %i\n", i, 3); Serial_flush(); //m.message_type);
                        break;
                }
            }
        }*/

        // actually play events for specific loop point
        void play_events(unsigned long time) {
            //time = time % LOOP_LENGTH;
            time = ticks_to_sequence_step(time);
            for (int_fast8_t i = 0 ; i < MIDI_NUM_NOTES ; i++) {
                int transposed_pitch = i + transpose_amount;
                if (!is_valid_note(transposed_pitch)) {
                    if (this->debug) { Serial.printf(F("\t!!transposed pitch %i (was %i with transpose %i) went out of range!\n"), transposed_pitch, i, transpose_amount); Serial_flush(); }
                    continue;;
                } else {
                    if (this->debug) { Serial.printf(F("\ttransposed pitch %i (was %i with transpose %i) within range!\n"), transposed_pitch, i, transpose_amount); Serial_flush(); }
                }

                if (track_playing[i].playing && (*piano_roll_bitmap)[time][i]==0) {
                    if (this->debug) Serial.printf(F("NOTE OFF play_events at\t%i: stopping pitch\t%i at vel\t%i\n"), time, i, (*piano_roll_bitmap)[time][i]);

                    this->sendNoteOff(transposed_pitch, 0);
                    track_playing_off(ticks, i, 0);
                    last_note = transposed_pitch;
                    if (i==current_note) // todo: properly check that there are no other notes playing
                        current_note = -1;
                } else if (!track_playing[i].playing && (*piano_roll_bitmap)[time][i]>0) {
                    if (this->debug) Serial.printf(F("NOTE ON play_events at\t%i: playing pitch\t%i at vel\t%i\n"), time, i, (*piano_roll_bitmap)[time][i]);
                    this->sendNoteOn(transposed_pitch, (*piano_roll_bitmap)[time][i]);
                    track_playing_on(ticks, i, (*piano_roll_bitmap)[time][i]);
                    current_note = transposed_pitch;
                }
            }
        }


        // wipe all recorded events
        void clear_all() {
            stop_all_notes();
            Serial.println(F("clearing recording.."));
            for (unsigned int i = 0 ; i < LOOP_LENGTH_STEPS ; i++) {
                // todo: actually free the recorded memory..?
                frames[i]->clear();
                //clear_tick(i);
            }
            //Serial.println(F("cleared"));
            this->wipe_piano_roll_bitmap();
        }

        // wipe events at specific tick; only wipe once per tick, so that erase head only wipes previous take and not any new events we've received 
        void clear_tick(uint32_t tick) {
            tick = ticks_to_sequence_step(tick); //tick % LOOP_LENGTH;
            static uint32_t last_cleared_tick = -1;
            if (tick!=last_cleared_tick) {
                if (this->bitmap_enabled) 
                    for (uint_fast8_t i = 0 ; i < MIDI_NUM_NOTES ; i++) {
                        (*piano_roll_bitmap)[tick][i] = false;
                    }
                frames[tick]->clear();
                last_cleared_tick = tick;
            }
        }

        // tell the output device to stop all notes that its playing
        void stop_all_notes();

        // for sending passthrough or recorded noteOns to actual output
        void sendNoteOn (int8_t pitch, uint8_t velocity, uint8_t channel = 0);
        void sendNoteOff(int8_t pitch, uint8_t velocity, uint8_t channel = 0);

        // called by external code to inform the looper about a note being played; looper decides whether to wipe and/or record it
        void in_event(uint32_t ticks, uint8_t message_type, int8_t note, uint8_t velocity) {
            if (this->isOverwriting()) 
                this->clear_tick(ticks);
            if (this->isRecording())
                this->store_event(ticks, message_type, note, velocity);
        }

        // called by external code to inform looper of a tick happening; looper decides whether to wipe and/or play events at current position
        void process_tick(uint32_t ticks) {
            if (this->isOverwriting())
                this->clear_tick(ticks);
            this->update_bitmap(ticks);
            if (this->isPlaying())
                this->play_events(ticks);
        }

    /* playing status stuff*/
        bool isPlaying() {
            return this->playing;
        }
        void toggle_playing() {
            if (this->isPlaying()) 
                this->stop_playing();
            else 
                this->start_playing();
        }
        // enable 'play head'
        void start_playing() {
            Serial.println(F("Looper: Start playing!"));
            this->playing = true;
        }
        // disable 'play head'
        void stop_playing() {
            Serial.println(F("Looper: Stop playing!"));
            if (this->playing)
                this->stop_all_notes();
            this->playing = false;
        }


    /* recording status */
        bool isRecording() {
            return this->recording;
        }
        void toggle_recording() {
            recording = !recording;
            if (recording) {
                this->start_recording();
            } else {
                this->stop_recording();
            }
        }
        // enable 'record head'
        void start_recording() {
            Serial.println(F("Looper: Started recording"));
            // uhhh nothing to do rn?
            recording = true;
        }
        // disable 'record head' and stop playing any notes that
        void stop_recording() {
            Serial.println(F("looper stopped recording"));
            recording = false;
            // send & record note-offs for all notes that are playing due to being recorded
            for (uint_fast8_t i = 0 ; i < MIDI_NUM_NOTES ; i++) {
                if (recorded_hanging_notes[i].playing) {
                    this->sendNoteOff(i, 0);
                    store_event(ticks_to_sequence_step(ticks), midi::NoteOff, i, MIDI_MIN_VELOCITY);
                    recorded_hanging_notes[i].playing = false;
                }
            }
            //this->draw_piano_roll_bitmap();
        }


    /* bitmap processing stuff */
        //int8_t piano_roll_bitmap[LOOP_LENGTH_STEPS][MIDI_NUM_NOTES];    // velocity of note at this moment
        //int8_t (*piano_roll_bitmap)[LOOP_:];    // velocity of note at this moment
        //typedef int8_t track_note_bitmap[LOOP_LENGTH_STEPS][MIDI_NUM_NOTES];
        //track_note_bitmap *piano_roll_bitmap;
        //int8_t (*piano_roll_bitmap)[LOOP_LENGTH_STEPS][MIDI_NUM_NOTES];

        typedef int8_t loop_bitmap[LOOP_LENGTH_STEPS][MIDI_NUM_NOTES];
        loop_bitmap *piano_roll_bitmap = nullptr;       // dynamically allocate RAM for this on first call to wipe_piano_roll_bitmap (in constructor)

        int8_t piano_roll_held[MIDI_NUM_NOTES];
        bool pitch_contains_notes[MIDI_NUM_NOTES];
        int piano_roll_highest = MIDI_MIN_NOTE;
        int piano_roll_lowest = MIDI_MAX_NOTE;

        // what's the lowest pitch that we've got a note for?
        int first_pitch() {
            for (uint_fast8_t i = 0 ; i < MIDI_NUM_NOTES ; i++) {
                if (pitch_contains_notes[i]) return i;
            }
            return 0;
        }
        // what's the highest pitch that we've got a note for?
        int last_pitch() {
            for (uint_fast8_t i = MIDI_MAX_NOTE ; i > 0 ; i--) {
                if (pitch_contains_notes[i]) return i;
            }
            return 0;
        }
        // are there any recorded notes for the given pitch?
        bool does_pitch_contain_notes(int8_t pitch) {
            return pitch_contains_notes[pitch];
        }

        // live update of the 'bitmap' - called from process_tick after erase head - updates for any notes that are currently being recorded
        void update_bitmap(uint32_t ticks) {
            if (!this->bitmap_enabled) return;
            
            for (uint_fast8_t i = 0 ; i < MIDI_NUM_NOTES ; i++) {
                if (recorded_hanging_notes[i].playing) {
                    (*piano_roll_bitmap)[ticks_to_sequence_step(ticks)][i] = recorded_hanging_notes[i].velocity;
                    pitch_contains_notes[i] = true;
                } /*else {
                    piano_roll_bitmap[ticks_to_sequence_step(ticks)][i] = 0;//recorded_hanging_notes[i].velocity;
                }*/
            }
        }

        // wipe all of the bitmap, ready for redrawing etc
        void wipe_piano_roll_bitmap() {
            if (!this->bitmap_enabled) return;

            if (this->piano_roll_bitmap==nullptr)
                this->piano_roll_bitmap = (loop_bitmap*)CALLOC_FUNC(LOOP_LENGTH_STEPS, MIDI_MAX_NOTE);
            //memset(*this->piano_roll_bitmap, 0, LOOP_LENGTH_STEPS*127);
            memset(this->piano_roll_held, 0, MIDI_MAX_NOTE);
            memset(this->pitch_contains_notes, 0, MIDI_MAX_NOTE);
        }

        // render the frames (array of linked list of messages) to a 'bitmap' 2d array of time * pitch
        void draw_piano_roll_bitmap_from_save() {
            if (!this->bitmap_enabled) return;

            if(this->debug) { Serial.println(F("draw_piano_roll_bitmap_from_save")); Serial_flush(); }
            piano_roll_highest = 0;
            piano_roll_lowest = MIDI_MAX_NOTE;

            //if(this->debug) { Serial.println(F("wiping bitmap..")); Serial_flush(); }
            this->wipe_piano_roll_bitmap();

            //if(this->debug) { Serial.println(F("building bitmap..")); Serial_flush(); }
            for (unsigned int x = 0 ; x < LOOP_LENGTH_STEPS ; x++) {   // for each column
                for (unsigned int m = 0 ; m < frames[x]->size() ; m++) {
                    midi_message message = frames[x]->get(m);
                    if (message.message_type==midi::NoteOn) {
                        /*if (piano_roll_highest < message.pitch)
                            piano_roll_lowest = message.pitch;
                        if (piano_roll_lowest > message.pitch)
                            piano_roll_lowest = message.pitch;*/
                        piano_roll_held[message.pitch] = message.velocity;
                        pitch_contains_notes[message.pitch] = true;
                    } else if (message.message_type==midi::NoteOff) {
                        piano_roll_held[message.pitch] = 0;
                    }
                }
                for (uint_fast8_t p = 0 ; p < MIDI_NUM_NOTES ; p++) {
                    (*piano_roll_bitmap)[x][p] = piano_roll_held[p];
                }
            }
            if(this->debug) { Serial.println(F("bitmap built..")); Serial_flush(); }

            /*Serial.println("draw bitmap:");
            for (int p = 0 ; p < 127 ; p++) {
                for (int x = 0 ; x < LOOP_LENGTH ; x++) {
                    if (piano_roll_bitmap[x][p])
                        Serial.print("x");
                    else
                        Serial.print("_");
                }
                Serial.println();
            }*/
        }

    /* erasing status */
        bool isOverwriting() {
            return this->overwrite;
        }
        void toggle_overwriting() {
            if (this->isOverwriting())
                this->stop_overwriting();
            else
                this->start_overwriting();
        }
        // enable 'erase head'
        void start_overwriting() {
            if (this->playing) {
                stop_all_notes();
            }
            //fix_overwrite(ticks%LOOP_LENGTH);
            this->overwrite = true;
        }
        // disable 'erase head'
        void stop_overwriting() {
            this->overwrite = false;
        }

        tracked_note track_playing[MIDI_NUM_NOTES];

        // track when a playing note began
        void track_playing_on(uint32_t ticks, int8_t pitch, int8_t velocity) {
            track_playing[pitch].playing = true;
            //track_playing[pitch].started_at = ticks;
            track_playing[pitch].velocity = velocity;
        }
        // stop tracking a playing note
        void track_playing_off(uint32_t ticks, int8_t pitch, int8_t velocity) {
            track_playing[pitch].playing = false;
            //track_playing[pitch].started_at = -1;
            track_playing[pitch].velocity = velocity;
        }
        // check all playing notes and write a note off at current position
        /*void fix_overwrite(uint32_t ticks) {
            // TODO: for this to work properly, we need to know what notes might be playing at this time
            // TODO: so we need a data structure that will allow to easily look up whether the note is playing at point 'ticks'
            // TODO: i guess the bitmap structure would work for this...?
            // TODO: so for that to work, we'll need to incrementally redraw the bitmap from store_event and clear_tick (and from here, too)
            // TODO: and also i guess from process_tick -- ie if a note is being recorded, we don't yet know when it will end, so we need to update the bitmap on a tick-by-tick basis
            for (unsigned int i = 0 ; i < 127 ; i++) {
                if (piano_roll_bitmap[ticks%LOOP_LENGTH][i])
                    this->store_event(ticks%LOOP_LENGTH, midi::NoteOff, i, 0);
            }
        }*/
        /*void fix_overwrite_pitch(uint32_t ticks, int8_t pitch) {
            if (piano_roll_bitmap[ticks][pitch])
                this->store_event(ticks, midi::NoteOff, pitch, 0);
        }*/

        // convert bitmap to the linkedlist-of-messages save format
        void convert_from_bitmap() {
            if (!this->bitmap_enabled) return;

            if (this->debug) Serial.println(F(("Converting from bitmap...")));

            // store current quantization setting so that we don't quantize during conversion, else shit gets all fucked up!
            int previous_quant = this->quantization_value;  
            this->quantization_value = 0;

            bool held_state[MIDI_NUM_NOTES];   // for tracking what notes are held
            int note_on_count = 0, note_off_count = 0;

            memset(held_state, false, MIDI_MAX_NOTE);
            /*for (unsigned int i = 0 ; i < 127; i++) {
                held_state[i] = false;
            }*/

            // todo: fix notes that wrap around from end of loop?
            //          maybe just set initial held_state to be the last frame..?
            for (int t = 0 ; t < LOOP_LENGTH_STEPS ; t++) {
                //Serial.printf(F("doing time %i:\n"), t);
                frames[t]->clear();
                for (uint_fast8_t p = 0 ; p < MIDI_NUM_NOTES ; p++) {
                    if ((*piano_roll_bitmap)[t][p]>0           && !held_state[p]) { // note on
                    //if (piano_roll_bitmap[t][p]>0           && piano_roll_bitmap[(t-1)%LOOP_LENGTH][p]==0) { // note on
                        //if (this->debug) Serial.printf(F("Found note on with\tpitch %i\t"), p);
                        held_state[p] = true;
                        note_on_count++;
                        this->store_event(t * LOOP_LENGTH_STEP_SIZE, midi::NoteOn, p, (*piano_roll_bitmap)[t][p]);
                    } else if ((*piano_roll_bitmap)[t][p]==0   && held_state[p]) {
                        //if (this->debug) Serial.printf(F("\tFound note off with\tpitch %i\t\n"), p);
                    //} else if (piano_roll_bitmap[t][p]==0   && piano_roll_bitmap[(t-1)%LOOP_LENGTH][p]>0) {
                        held_state[p] = false;
                        note_off_count++;
                        this->store_event(t * LOOP_LENGTH_STEP_SIZE, midi::NoteOff, p, 0);
                    } else {
                        //Serial.printf("\tno change - held_state is %i\n", held_state[p]);
                        if (this->debug) Serial.print(F("."));
                    }
                }
            }
            //if (this->debug) Serial.printf(F("Converted: found %i note ons, %i note offs\n"), note_on_count, note_off_count);
            if (note_on_count>note_off_count) {
                Serial.printf(F("WARNING in MIDITrack#convert_from_bitmap: found more note ons than note offs!"));
                // todo: fix note on/off mismatches
            }

            /*if (this->debug) { 
                for (int t = 0 ; t < LOOP_LENGTH_STEPS ; t++) {
                    if (frames[t]->size()>0) {
                        Serial.printf(F("frames[%i] now has %i events:\n"), t, frames[t]->size());
                        for (unsigned int i = 0 ; i < frames[t]->size() ; i++) {
                            Serial.printf(F("\tmessage type = %i, pitch = %i, velocity = %i\n"), frames[t]->get(i).message_type, frames[t]->get(i).pitch, frames[t]->get(i).velocity);
                        }
                    }
                }
                Serial.println(F("Done convert_from_bitmap"));
            }*/

            this->quantization_value = previous_quant;  // restore original quantization setting
        }

        /* save+load stuff to filesystem - linkedlist-of-message format */
        bool save_loop(int project_number, int recording_number) {
            #ifdef ENABLE_SD
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            //Serial.println("save_pattern not implemented on teensy");
            //bool irqs_enabled = __irq_enabled();
            //__disable_irq();
            File f;
            clear_hanging();
            convert_from_bitmap();

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_LOOP_FORMAT, project_number, recording_number);
            Serial.printf(F("save_loop(%i) writing to %s\n"), recording_number, filename);
            if (SD.exists(filename)) {
                Serial.printf(F("\t%s exists, deleting first\n"), filename);
                SD.remove(filename);
            }
            f = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC); //FILE_WRITE_BEGIN);
            if (!f) {    
                //if (irqs_enabled) __enable_irq();
                Serial.printf(F("\tError: couldn't open %s for writing\n"), filename);
                return false;
            }
            f.println(F("; begin loop"));
            f.printf(F("step_size=%i\n"), LOOP_LENGTH_STEP_SIZE);
            //myFile.printf("id=%i\n",input->id);
            f.println(F("starts_at=0"));
            f.printf(F("transpose=%i\n"), transpose_amount);
            bool last_written = false;
            int lines_written = 0;
            for (unsigned int x = 0 ; x < LOOP_LENGTH_STEPS ; x++) {
                unsigned int size = frames[x]->size();
                /*if (!last_written) {
                    myFile.printf("starts_at=%i\n",x);
                }*/
                //myFile.printf("%1x", input->sequence_data[i][x]);
                if (size==0) {      // only write lines that have data
                    last_written = false;
                    continue;
                } else if (!last_written || LOOP_LENGTH_STEP_SIZE>1) {
                    f.printf(F("starts_at=%i\n"),(x*LOOP_LENGTH_STEP_SIZE));
                }
                f.printf(F("loop_data=")); //%2x:", frames[x].size());
                
                for(unsigned int i = 0 ; i < size ; i++) {
                    midi_message m = frames[x]->get(i);
                    f.printf("%02x%02x%02x%02x,", m.message_type, m.channel, m.pitch, m.velocity);
                }
                lines_written++;
                f.println();
            }
            f.println(F("; end loop"));
            f.close();
            //if (irqs_enabled) __enable_irq();
            Serial.printf(F("Finished saving, %i lines written!\n"), lines_written);

            if (lines_written==0) {
                // TODO:  delete the empty file / update project availability so it displays the slot as empty
            }

            Serial.println(F("Re-drawing bitmap from save..."));
            clear_hanging();
            this->draw_piano_roll_bitmap_from_save();
            clear_hanging();

            loaded_recording_number = recording_number;
            }
            return true;
            #else
            return false;
            #endif
        }

        //#define DEBUG_LOOP_LOADER

        // load file on disk into loop - linked-list-of-messages format
        bool load_loop(int project_number, int recording_number) {
            #ifdef ENABLE_SD
            //this->debug = true;
            Serial.println("load_loop: top of load_loop()"); Serial_flush();
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            if (global_load_lock) {
                Serial.println("load_loop: global_load_lock is set, so returning!");
                return false;
            }
            static volatile bool already_loading = false;
            if (already_loading) {
                Serial.println("load_loop: global_load_lock is set, so returning!");
                return false;
            }
            already_loading = true;
            global_load_lock = true;

            char filename[MAX_FILEPATH] = "";
            snprintf(filename, MAX_FILEPATH, FILEPATH_LOOP_FORMAT, project_number, recording_number);
            Serial.printf(F("load_loop: midi_looper::load_loop(%i) opening %s\n"), recording_number, filename); Serial_flush();

            File f;
            f = SD.open(filename, FILE_READ);
            if (!f) {
                Serial.printf(F("\tload_loop: Error: Couldn't open %s for reading!\n"), filename); Serial_flush();
                /*#ifdef ENABLE_SCREEN
                    menu.set_last_message("Error loading recording!");//, recording_number);
                    menu.set_message_colour(ST77XX_RED);
                #endif*/
                //if (irqs_enabled) __enable_irq();
                global_load_lock = false; already_loading = false;

                return false;
            }
            f.setTimeout(0);

            clear_all();

            Serial.printf("\tload_loop: Entering load loop for project %i and recording_number %i..\n", project_number, recording_number);
            Serial_flush();

            int loop_length_size = 1;   // default to 1-to-1 time:event time mapping, like in old format

            int total_frames = 0, total_messages = 0;
            String line;
            int time = 0;
            while (line = f.readStringUntil('\n')) {
                if (this->debug) { Serial.printf(F("--reading line %s\n"), line.c_str()); Serial_flush(); }
                //load_pattern_parse_line(line, output);
                if (line.startsWith(F("starts_at="))) {
                    time =      line.remove(0,String(F("starts_at=")).length()).toInt() * loop_length_size;
                } else if (line.startsWith(F("step_size="))) {
                    loop_length_size = line.remove(0,String(F("step_size=")).length()).toInt();
                    //Serial.printf("read loop_length_size %i!\n", loop_length_size);
                } else if (line.startsWith(F("transpose="))) {
                    transpose_amount = line.remove(0,String(F("transpose=")).length()).toInt();
                } else if (line.startsWith(F("loop_data="))) {
                    //Serial.printf("processing a loop_data line..\n");
                    total_frames++;
                    //if (debug) Serial.printf("Read id %i\n", output->id);
                    line = line.remove(0,String(F("loop_data=")).length());
                    line = line.remove(line.length()-1,1);
                    int MAX_LENGTH = (1+sizeof(midi_message)) * MAX_INSTRUCTIONS;
                    char c_line[MAX_LENGTH];// = line.c_str();
                    strncpy(c_line, line.c_str(), MAX_LENGTH);
                    midi_message m;
                    int messages_count = 0;

                    char *tok;
                    tok = strtok(c_line,",;:");
                    while (tok!=NULL && messages_count<MAX_INSTRUCTIONS) {
                        //#ifdef DEBUG_LOOP_LOADER
                        //    Serial.printf(F("at time %i: for token '%s', sizeof is already %i, "), time, tok, frames[ticks_to_sequence_step(time)].size());
                        //#endif
                        int tmp_message_type, tmp_channel, tmp_pitch, tmp_velocity;
                        sscanf(tok, "%02x%02x%02x%02x", &tmp_message_type, &tmp_channel, &tmp_pitch, &tmp_velocity);
                        m.message_type = tmp_message_type;
                        m.channel = tmp_channel;
                        m.pitch = tmp_pitch;
                        m.velocity = tmp_velocity;
                        #ifdef DEBUG_LOOP_LOADER
                            Serial.printf(F("read message int8_ts: %02x, %02x, %02x, %02x\n"), m.message_type, m.channel, m.pitch, m.velocity); Serial_flush();
                        #endif
                        if (this->debug) { Serial.printf(F("storing event at %i\n"), time); Serial_flush(); }
                        store_event(time, m);
                        messages_count++;
                        tok = strtok(NULL,",;:");
                    }
                    #ifdef DEBUG_LOOP_LOADER
                        Serial.printf(F("load_loop: for time\t%i read\t%i messages\n"), time, messages_count);
                    #endif
                    total_messages += messages_count;
                    time++;
                }
            }
            #ifdef DEBUG_LOOP_LOADER
                Serial.println(F("load_loop: Closing file.."));
            #endif
            f.close();
            //if (irqs_enabled) __enable_irq();
            #ifdef DEBUG_LOOP_LOADER
                Serial.println(F("load_loop: File closed"));
            #endif

            //Serial.printf("Loaded preset from [%s] [%i clocks, %i sequences of %i steps]\n", filename, clock_multiplier_index, sequence_data_index, output->size_steps);
            /*#ifdef ENABLE_SCREEN
                menu.set_last_message("Loaded recording %i"); //, recording_number);
                menu.set_message_colour(ST77XX_GREEN);
            #endif*/
            Serial.printf(F("load_loop: Loaded recording from [%s] - [%i] frames with total [%i] messages\n"), filename, total_frames, total_messages); Serial_flush();
            
            loaded_recording_number = recording_number;
            clear_hanging();

            this->draw_piano_roll_bitmap_from_save();

            //clear_hanging();
            global_load_lock = false; 
            already_loading = false;

            }
            //this->debug = false;

            return true;
            #else
            return false;
            #endif
        }       

        #ifdef ENABLE_SCREEN
            virtual LinkedList<MenuItem*> *make_menu_items();
        #endif

};

extern MIDITrack midi_looper;

#endif