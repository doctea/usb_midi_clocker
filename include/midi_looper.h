#ifndef MIDI_LOOPER__INCLUDED
#define MIDI_LOOPER__INCLUDED

#include <LinkedList.h>
#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"
#include "midi_out_wrapper.h"

#include "bpm.h"

#include "storage.h"

//#define DEBUG_LOOPER

#define LOOP_LENGTH (PPQN*4*4)
#define MAX_INSTRUCTIONS            100
#define MAX_INSTRUCTION_ARGUMENTS   4

using namespace storage;

// for storing messages for recording+playback by midi looper
// todo: expand to handle other message types (CCs?)
struct midi_message {
    uint8_t message_type;
    uint8_t pitch;
    uint8_t velocity;
    uint8_t channel;
};

class MIDITrack {
    LinkedList<midi_message> frames[LOOP_LENGTH];

    bool recorded_hanging_notes[127];
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
            Serial.printf("quantization level %i not known - disabling?\n", quantization);
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

        int quantized_time = step % LOOP_LENGTH;    // wrap around
        //Serial.printf("quantised time\t%i to\t%i\n", time, quantized_time);
        if (debug)
            Serial.printf("Quantize level\t%i: quantized time\t%i to\t%i\n", quantization, time, quantized_time);
        return quantized_time;
    }
    int quantize_time(int time, int quantization = -1) {
        if (quantization==-1)
            quantization = quantization_value;
        int quantized_time = find_nearest_quantized_time(time, quantization) % LOOP_LENGTH;
        return quantized_time;
    }

    public: 
        MIDIOutputWrapper *output = nullptr;
        //MIDIOutputWrapper **output_deferred = nullptr;

        bool debug = false;

        bool recording = false;
        bool playing = false;

        bool overwrite = false;

        int last_note = -1;
        int current_note = -1;

        int transpose = 0;

        MIDITrack() {};

        MIDITrack(MIDIOutputWrapper *default_output) {
            output = default_output;
        };

        /*MIDITrack(MIDIOutputWrapper **default_output_deferred) {
            output_deferred = default_output_deferred;
        };*/

        void setOutputWrapper(MIDIOutputWrapper *output) {
            if (this->output!=nullptr)
                this->output->stop_all_notes();
            this->output = output;
            Serial.printf("MIDITrack#setOutputWrapper in midi_looper wrapper to '%s'\n", this->output->label);
        }

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
            this->transpose = transpose;
        }
        int get_transpose() {
            return this->transpose;
        }

        // for actually storing values into buffer (also used when reloading from save)
        void record_event(midi_message midi_event) {
            unsigned long time = ticks % (LOOP_LENGTH);
            //time = quantize_time(time);
            record_event(time, midi_event);
        }
        // for actually storing values into buffer (also used when reloading from save)
        void record_event(unsigned long time, uint8_t instruction_type, /*uint8_t channel,*/ uint8_t pitch, uint8_t velocity) {
            midi_message m;
            m.message_type = instruction_type;
            //m.channel = 3; //channel;
            m.pitch = pitch;
            m.velocity = velocity;
            record_event(time, m);
        }
        // for actually storing values into buffer (also used when reloading from save)
        void record_event(unsigned long time, midi_message midi_event) {
            //Serial.printf("Recording event at\t%i", time);
            time = quantize_time(time);
            frames[time%LOOP_LENGTH].add(midi_event);
            if (midi_event.message_type==midi::NoteOn) {
                recorded_hanging_notes[midi_event.pitch] = true;
            } else if (midi_event.message_type==midi::NoteOff) {
                recorded_hanging_notes[midi_event.pitch] = false;
            }
        }

        // get total event count across entire loop
        int count_events() {
            int count = 0;
            for (int i = 0 ; i < LOOP_LENGTH ; i++) {
                count += frames[i].size();
            }
            return count;
        }

        // clear any notes that we're recording
        void clear_hanging() {
            for (int i = 0 ; i < 127 ; i++) {
                recorded_hanging_notes[i] = false;
            }
        }

        // get the list of events for a specific loop point
        LinkedList<midi_message> get_frames(unsigned long time) {
            return this->frames[time%LOOP_LENGTH];
        }

        // actually play events for a specified loop point
        void play_events(unsigned long time) {
            //if (!specified_output)
            //    specified_output = output;
            int position = time%LOOP_LENGTH;
            //LinkedList<midi_message> frame = get_frames(position);
            #ifdef DEBUG_LOOPER
                Serial.printf("play_events with time %u becomes position %u\n", time, position); Serial.flush();
            #endif

            int number_messages = frames[position].size();
            #ifdef DEBUG_LOOPER
                //Serial.printf("play_events got %i messages\n", number_messages); Serial.flush();
                if (number_messages>0) 
                    Serial.printf("\tfor frame\t%u got\t%i messages to play\n", position, number_messages); Serial.flush();
            #endif

            for (int i = 0 ; i < number_messages ; i++) {
                #ifdef DEBUG_LOOPER
                    Serial.printf("\tprocessing message number %i/%i..\n", i, number_messages); Serial.flush();
                #endif
                midi_message m = frames[position].get(i);
                
                int pitch = m.pitch + transpose;
                if (pitch<0 || pitch > 127) {
                    if (this->debug) { Serial.printf("\t!!transposed pitch %i (was %i with transpose %i) went out of range!\n", pitch, m.pitch, transpose); Serial.flush(); }
                    return;
                } else {
                    if (this->debug) { Serial.printf("\ttransposed pitch %i (was %i with transpose %i) within range!\n", pitch, m.pitch, transpose); Serial.flush(); }
                }
                if (this->debug) { Serial.printf("\tgot transposed pitch %i from %i + %i\n", pitch, m.pitch, transpose); Serial.flush(); }

                switch (m.message_type) {
                    case midi::NoteOn:
                        current_note = pitch;
                        if (this->debug) { Serial.printf("\t\tSending note on %i at velocity %i\n", pitch, m.velocity); Serial.flush(); }
                        this->sendNoteOn((uint8_t)pitch, (byte)m.velocity);
                        track_playing_on(ticks, pitch, m.velocity);
                        break;
                    case midi::NoteOff:
                        if (this->debug) { Serial.printf("\t\tSending note off %i at velocity %i\n", pitch, m.velocity); Serial.flush(); }
                        last_note = pitch;
                        if (m.pitch==current_note) // todo: properly check that there are no other notes playing
                            current_note = -1;
                        this->sendNoteOff((uint8_t)pitch, (byte)m.velocity); //, m.channel);
                        track_playing_off(ticks, pitch, m.velocity);
                        break;
                    default:
                        if (this->debug) Serial.printf("\t%i: !!Unhandled message type %i\n", i, 3); Serial.flush(); //m.message_type);
                        break;
                }
            }
        }

        // wipe all recorded events
        void clear_all() {
            stop_all_notes();
            Serial.println("clearing recording");
            for (int i = 0 ; i < LOOP_LENGTH ; i++) {
                // todo: actually free the recorded memory..?
                //frames[i].clear();
                clear_tick(i);
            }
        }

        // wipe events at specific tick; only wipe once per tick so that erase head only wipes previous take and not any new events we've received 
        void clear_tick(uint32_t tick) {
            tick = tick % LOOP_LENGTH;
            static uint32_t last_cleared_tick = -1;
            if (tick!=last_cleared_tick) {
                frames[tick].clear();
                last_cleared_tick = tick;
            }
        }

        void stop_all_notes() {
            if (output!=nullptr)
                this->output->stop_all_notes();
        }

        // for sending passthrough or recorded noteOns to actual output
        void sendNoteOn(int pitch, int velocity, int channel = 0) {
            if (output!=nullptr) 
                output->sendNoteOn(pitch, velocity, channel);
            //if (output_deferred!=nullptr && (*output_deferred)!=nullptr) 
            //    (*output_deferred)->sendNoteOn(pitch, velocity, channel);
        }
        // for sending passthrough or recorded noteOffs to actual output
        void sendNoteOff(int pitch, int velocity, int channel = 0) {
            //Serial.printf("sendNoteOff: output is %p, output_deferred is %p, *output_deferred is %p\n", output, output_deferred, *output_deferred);
            if (output!=nullptr) 
                output->sendNoteOff(pitch, velocity, channel);
            //if (output_deferred!=nullptr && (*output_deferred)!=nullptr) 
                //(*output_deferred)->sendNoteOff(pitch, velocity, channel);
        }


        // called by external code to inform the looper about a note being played; looper decides whether to record, overwrite
        void in_event(uint32_t ticks, byte message, byte note, byte velocity) {
            if (this->isOverwriting()) 
                this->clear_tick(ticks);
            if (this->isRecording())
                this->record_event(ticks%LOOP_LENGTH, midi::NoteOn, note, velocity);
        }

        // called by external code to inform looper of a tick happening; looper decides whether to play, overwrite
        void process_tick(uint32_t ticks) {
            if (this->isOverwriting())
                this->clear_tick(ticks);
            if (this->isPlaying())
                this->play_events(ticks);
        }


        bool isPlaying() {
            return this->playing;
        }
        bool isRecording() {
            return this->recording;
        }
        bool isOverwriting() {
            return this->overwrite;
        }

        /* playing status */
        void toggle_playing() {
            if (this->isPlaying()) 
                this->stop_playing();
            else 
                this->start_playing();
        }
        // enable 'play head'
        void start_playing() {
            Serial.println("Looper: Start playing!");
            this->playing = true;
        }
        // disable 'play head'
        void stop_playing() {
            Serial.println("Looper: Stop playing!");
            if (this->playing)
                this->stop_all_notes();
            this->playing = false;
        }


        /* recording status */
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
            Serial.println("Looper: Started recording");
            // uhhh nothing to do rn?
            recording = true;
        }
        // disable 'record head' and stop playing any notes that
        void stop_recording() {
            Serial.println("looper stopped recording");
            recording = false;
            // send & record note-offs for all notes that are playing due to being recorded
            for (byte i = 0 ; i < 127 ; i++) {
                if (recorded_hanging_notes[i]) {
                    if (output!=nullptr)
                        this->sendNoteOff(i, 0);
                    record_event(ticks%LOOP_LENGTH, midi::NoteOff, i, 0);
                    recorded_hanging_notes[i] = false;
                }
            }
            this->draw_piano_roll_bitmap();
        }
        /*void toggle_recording() {
            bool previously_recording = this->is_recording;
            this->is_recording = !this->is_recording;
            if (this->is_recording==true && !previously_recording)
                this->start_recording();
            if (!this->is_recording && previously_recording) 
                this->stop_recording();
        }*/

        bool piano_roll_bitmap[LOOP_LENGTH][127];
        bool piano_roll_held[127];
        int piano_roll_highest = 0;
        int piano_roll_lowest = 127;

        // render a bitmap of the loop to array
        void draw_piano_roll_bitmap() {
            piano_roll_highest = 0;
            piano_roll_lowest = 127;

            for (int p = 0 ; p < 127; p++) {
                for (int x = 0  ; x < LOOP_LENGTH ; x++) {
                    piano_roll_bitmap[x][p] = false;
                }
                piano_roll_held[p] = false;
            }
            for (int x = 0 ; x < LOOP_LENGTH ; x++) {   // for each column
                for (int m = 0 ; m < frames[x].size() ; m++) {
                    midi_message message = frames[x].get(m);
                    if (message.message_type==midi::NoteOn) {
                        if (piano_roll_highest < message.pitch)
                            piano_roll_lowest = message.pitch;
                        if (piano_roll_lowest > message.pitch)
                            piano_roll_lowest = message.pitch;
                        piano_roll_held[message.pitch] = true;
                    } else if (message.message_type==midi::NoteOff) {
                        piano_roll_held[message.pitch] = false;
                    }
                }
                for (int p = 0 ; p < 127 ; p++) {
                    piano_roll_bitmap[x][p] = piano_roll_held[p];
                }
            }

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
            fix_overwrite(ticks%LOOP_LENGTH);
            this->overwrite = true;
        }
        // disable 'erase head'
        void stop_overwriting() {
            this->overwrite = false;
        }

        struct tracked_note {
            bool playing = false;
            uint32_t started_at = -1;
        };
        tracked_note track_playing[127];

        // track when a playing note began
        void track_playing_on(uint32_t ticks, byte pitch, byte velocity) {
            track_playing[pitch].playing = true;
            track_playing[pitch].started_at = ticks;
        }
        // stop tracking a playing note
        void track_playing_off(uint32_t ticks, byte pitch, byte velocity) {
            track_playing[pitch].playing = false;
            track_playing[pitch].started_at = -1;
        }
        // check all playing notes and write a note off at current position
        void fix_overwrite(uint32_t ticks) {
            for (int i = 0 ; i < 127 ; i++) {
                if (track_playing[i].playing) {
                    fix_overwrite_pitch(ticks, i);
                }
            }
        }
        void fix_overwrite_pitch(uint32_t ticks, byte pitch) {
            this->record_event(ticks, midi::NoteOff, pitch, 0);
        }

        /* save+load stuff to filesystem */
        bool save_loop(int project_number, int recording_number) {
            //Serial.println("save_sequence not implemented on teensy");
            File myFile;

            char filename[255] = "";
            sprintf(filename, FILEPATH_LOOP_FORMAT, project_number, recording_number);
            Serial.printf("midi_looper::save_sequence(%i) writing to %s\n", recording_number, filename);
            if (SD.exists(filename)) {
                Serial.printf("%s exists, deleting first\n", filename);
                SD.remove(filename);
            }
            myFile = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC); //FILE_WRITE_BEGIN);
            if (!myFile) {    
                Serial.printf("Error: couldn't open %s for writing\n", filename);
                return false;
            }
            myFile.println("; begin loop");
            //myFile.printf("id=%i\n",input->id);
            myFile.println("starts_at=0");
            myFile.printf("transpose=%i\n", transpose);
            bool last_written = false;
            int lines_written = 0;
            for (int x = 0 ; x < LOOP_LENGTH ; x++) {
                int size = frames[x].size();
                /*if (!last_written) {
                    myFile.printf("starts_at=%i\n",x);
                }*/
                //myFile.printf("%1x", input->sequence_data[i][x]);
                if (size==0) {      // only write lines that have data
                    last_written = false;
                    continue;
                } else if (!last_written) {
                    myFile.printf("starts_at=%i\n",x);
                }
                myFile.printf("loop_data="); //%2x:", frames[x].size());
                
                for(int i = 0 ; i < size ; i++) {
                    midi_message m = frames[x].get(i);
                    myFile.printf("%02x%02x%02x%02x,", m.message_type, m.channel, m.pitch, m.velocity);
                }
                lines_written++;
                myFile.println("");
            }
            myFile.println("; end loop");
            myFile.close();
            Serial.printf("Finished saving, %i lines written!\n", lines_written);

            if (lines_written==0) {
                // TODO:  delete the empty file / update project availability so it displays the slot as empty
            }

            loaded_recording_number = recording_number;
            return true;
        }

        // load file on disk into loop
        bool load_loop(int project_number, int recording_number) {
            File myFile;

            char filename[255] = "";
            sprintf(filename, FILEPATH_LOOP_FORMAT, project_number, recording_number);
            Serial.printf("midi_looper::load_sequence(%i) opening %s\n", recording_number, filename);
            myFile = SD.open(filename, FILE_READ);
            myFile.setTimeout(0);

            if (!myFile) {
                Serial.printf("Error: Couldn't open %s for reading!\n", filename);
                /*#ifdef ENABLE_SCREEN
                    menu.set_last_message("Error loading recording!");//, recording_number);
                    menu.set_message_colour(ST77XX_RED);
                #endif*/
                return false;
            }

            clear_all();

            int total_frames = 0, total_messages = 0;
            String line;
            int time = 0;
            while (line = myFile.readStringUntil('\n')) {
                //load_sequence_parse_line(line, output);
                if (line.startsWith("starts_at=")) {
                    time =      line.remove(0,String("starts_at=").length()).toInt();
                } if (line.startsWith("transpose=")) {
                    transpose = line.remove(0,String("transpose=").length()).toInt();
                } else if (line.startsWith("loop_data=")) {
                    Serial.printf("reading line %s\n", line.c_str());
                    total_frames++;
                    //if (debug) Serial.printf("Read id %i\n", output->id);
                    line = line.remove(0,String("loop_data=").length());
                    line = line.remove(line.length()-1,1);
                    char c_line[(1+sizeof(midi_message)) * MAX_INSTRUCTIONS];// = line.c_str();
                    strcpy(c_line, line.c_str());
                    midi_message m;
                    int messages_count = 0;

                    char *tok;
                    tok = strtok(c_line,",;:");
                    while (tok!=NULL && messages_count<MAX_INSTRUCTIONS) {
                        #ifdef DEBUG_LOOP_LOADER
                            Serial.printf("at time %i: for token '%s', sizeof is already %i, ", time, tok, frames[time].size());
                        #endif
                        int tmp_message_type, tmp_channel, tmp_pitch, tmp_velocity;
                        sscanf(tok, "%02x%02x%02x%02x", &tmp_message_type, &tmp_channel, &tmp_pitch, &tmp_velocity);
                        m.message_type = tmp_message_type;
                        m.channel = tmp_channel;
                        m.pitch = tmp_pitch;
                        m.velocity = tmp_velocity;
                        #ifdef DEBUG_LOOP_LOADER
                            Serial.printf("read message bytes: %02x, %02x, %02x, %02x\n", m.message_type, m.channel, m.pitch, m.velocity);
                        #endif
                        record_event(time, m);
                        messages_count++;
                        tok = strtok(NULL,",;:");
                    }
                    #ifdef DEBUG_LOOP_LOADER
                        Serial.printf("for time\t%i read\t%i messages\n", time, messages_count);
                    #endif
                    total_messages += messages_count;
                    time++;
                }
            }
            #ifdef DEBUG_LOOP_LOADER
                Serial.println("Closing file..");
            #endif
            myFile.close();
            #ifdef DEBUG_LOOP_LOADER
                Serial.println("File closed");
            #endif

            //Serial.printf("Loaded preset from [%s] [%i clocks, %i sequences of %i steps]\n", filename, clock_multiplier_index, sequence_data_index, output->size_steps);
            /*#ifdef ENABLE_SCREEN
                menu.set_last_message("Loaded recording %i"); //, recording_number);
                menu.set_message_colour(ST77XX_GREEN);
            #endif*/
            Serial.printf("Loaded recording from [%s] - [%i] frames with total [%i] messages\n", filename, total_frames, total_messages);
            
            loaded_recording_number = recording_number;
            clear_hanging();

            draw_piano_roll_bitmap();

            return true;
        }       

};

#endif