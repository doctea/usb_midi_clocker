#ifndef MIDI_LOOPER__INCLUDED
#define MIDI_LOOPER__INCLUDED

#include <LinkedList.h>
#include "Config.h"
#include "ConfigMidi.h"
#include "MidiMappings.h"
#include "midi_out_wrapper.h"

#include "storage.h"
//#include "menu.h"
//#include "mymenu.h"

//#include <MIDI.h>
//#include "USBHost_t36.h"

#define LOOP_LENGTH (PPQN*4*4)
#define MAX_INSTRUCTIONS            100
#define MAX_INSTRUCTION_ARGUMENTS   4

using namespace storage;
/*
void recordInstruction(byte instruction_type, byte channel, byte arg0, byte arg1);
void playInstruction(int index);
void clear_recording();
void stop_all_notes();*/

typedef struct midi_message {
    uint8_t message_type;
    uint8_t pitch;
    uint8_t velocity;
    uint8_t channel;
};


class midi_track {
    //LinkedList<midi_frame> frames = LinkedList<midi_frame> ();
    //midi_frame frames[LOOP_LENGTH];
    LinkedList<midi_message> frames[LOOP_LENGTH];
    midi_output_wrapper *output;

    //bool playing_notes[127];    // track what notes are playing so we can turn them off / record ends appropriately
    bool recorded_hanging_notes[127];
    int loaded_recording_number = -1;

    int quantization_value = 4; // 4th of a quarter-note, ie 1 step, ie 6 pulses
    /*  
        # -2 = quantize to bar          (note)          TODO
        # -1 = quantize to half-bar     (half-note)     TODO
        # 0 = no quantization
        # 1 = quantize to beat          (quarter-note)
        # 2 = quantize to half-beat     (eighth-note)
        # 3 = quantize to third-beat    (sixteenth-note)
        # 4 = quantize to quarter-beat  (thirty-second-note)
    */
    int find_nearest_quantized_time(int time, int quantization) {
        if (quantization==0) // if quantization is 0 then don't quantize at all
            return time;

        int ticks_per_quant_level = PPQN/quantization;      // get number of ticks per quantized unit

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
        bool debug = false;

        int last_note = -1;
        int current_note = -1;

        int transpose = 0;

        midi_track(midi_output_wrapper *default_output) {
            output = default_output;
            //frames[0].time = 0;
            /*for (int i = 0 ; i < LOOP_LENGTH ; i++) {
                frames[i] = new LinkedList<midi_message>();
            }*/
        };

        // for getting and setting from menu
        int get_quantization_value() {
            return quantization_value;
        }
        void set_quantization_value(int qv) {
            quantization_value = qv;
        }

        void set_transpose(int transpose) {
            stop_all_notes();
            this->transpose = transpose;
        }

        // for recording values (also used for reloading)
        void record_event(midi_message midi_event) {
            unsigned long time = ticks % (LOOP_LENGTH);
            //time = quantize_time(time);
            record_event(time, midi_event);
        }
        void record_event(unsigned long time, uint8_t instruction_type, /*uint8_t channel,*/ uint8_t pitch, uint8_t velocity) {
            midi_message m;
            m.message_type = instruction_type;
            //m.channel = 3; //channel;
            m.pitch = pitch;
            m.velocity = velocity;
            //frames[time%LOOP_LENGTH].add(m);
            record_event(time, m);
        }
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

        LinkedList<midi_message> get_frames(unsigned long time) {
            return this->frames[time];
        }

        void play_events(unsigned long time) { //, midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> *specified_output = nullptr) {
            //if (!specified_output)
            //    specified_output = output;

            int position = time%LOOP_LENGTH;
            int number_messages = frames[position].size();

            if (frames[position].size()>0) 
                Serial.printf("for frame\t%i got\t%i messages to play\n", position, number_messages);

            for (int i = 0 ; i < number_messages ; i++) {
                midi_message m = frames[position].get(i);
                
                int pitch = m.pitch + transpose;

                switch (m.message_type) {
                    case midi::NoteOn:
                        current_note = pitch;
                        output->sendNoteOn(pitch, m.velocity); //, m.channel);
                        //playing_notes[pitch] = true;
                        break;
                    case midi::NoteOff:
                        last_note = pitch;
                        if (m.pitch==current_note) // todo: properly check that there are no other notes playing
                            current_note = -1;
                        output->sendNoteOff(pitch, m.velocity); //, m.channel);
                        //playing_notes[pitch] = false;
                        break;
                    default:
                        Serial.printf("\t%i: Unhandled message type %i\n", i, 3); //m.message_type);
                        break;
                }
            }
        }

        void clear_all() {
            stop_all_notes();
            Serial.println("clearing recording");
            for (int i = 0 ; i < LOOP_LENGTH ; i++) {
                // todo: actually free the recorded memory..?
                frames[i].clear();
            }
        }

        void stop_all_notes() {
            // todo: probably move this into the wrapper, so that can have multiple sources playing into the same output?
            this->output->stop_all_notes();
            /*for (byte i = 0 ; i < 127 ; i++) {
                if (playing_notes[i]) {
                    output->sendNoteOff(i, 0);
                    playing_notes[i] = false;
                }
            }*/
        }

        void stop_recording() {
            Serial.println("mpk49 stopped recording");
            // send & record note-offs for all notes that are playing due to being recorded
            for (byte i = 0 ; i < 127 ; i++) {
                if (recorded_hanging_notes[i]) {
                    output->sendNoteOff(i, 0);
                    record_event(ticks%LOOP_LENGTH, midi::NoteOff, i, 0);
                    recorded_hanging_notes[i] = false;
                }
            }
        }

        void start_recording() {
            Serial.println("mpk49 started recording");
            // uhhh nothing to do rn?
        }

        // save+load stuff !
        bool save_sequence(int recording_number) {
            //Serial.println("save_sequence not implemented on teensy");
            File myFile;

            char filename[255] = "";
            sprintf(filename, FILEPATH_LOOP, recording_number);
            Serial.printf("midi_looper::save_sequence(%i) writing to %s\n", recording_number, filename);
            if (SD.exists(filename)) {
                Serial.printf("%s exists, deleting first\n", filename);
                SD.remove(filename);
            }
            myFile = SD.open(filename, FILE_WRITE_BEGIN | O_TRUNC); //FILE_WRITE_BEGIN);
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
                /*if (!last_written) {
                    myFile.printf("starts_at=%i\n",x);
                }*/
                //myFile.printf("%1x", input->sequence_data[i][x]);
                if (frames[x].size()==0) {      // only write lines that have data
                    last_written = false;
                    continue;
                } else if (!last_written) {
                    myFile.printf("starts_at=%i\n",x);
                }
                myFile.printf("loop_data="); //%2x:", frames[x].size());
                for(int i = 0 ; i < frames[x].size() ; i++) {
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

        bool load_state(int recording_number) {
            File myFile;

            char filename[255] = "";
            sprintf(filename, FILEPATH_LOOP, recording_number);
            Serial.printf("midi_looper::load_state(%i) opening %s\n", recording_number, filename);
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
                //load_state_parse_line(line, output);
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
                        Serial.printf("at time %i: for token '%s', sizeof is already %i, ", time, tok, frames[time].size());
                        int tmp_message_type, tmp_channel, tmp_pitch, tmp_velocity;
                        sscanf(tok, "%02x%02x%02x%02x", &tmp_message_type, &tmp_channel, &tmp_pitch, &tmp_velocity);
                        m.message_type = tmp_message_type;
                        m.channel = tmp_channel;
                        m.pitch = tmp_pitch;
                        m.velocity = tmp_velocity;
                        Serial.printf("read message bytes: %02x, %02x, %02x, %02x\n", m.message_type, m.channel, m.pitch, m.velocity);
                        record_event(time, m);
                        messages_count++;
                        tok = strtok(NULL,",;:");
                    }
                    Serial.printf("for time\t%i read\t%i messages\n", time, messages_count);
                    total_messages += messages_count;
                    time++;
                }
            }
            Serial.println("Closing file..");
            myFile.close();
            Serial.println("File closed");

            //Serial.printf("Loaded preset from [%s] [%i clocks, %i sequences of %i steps]\n", filename, clock_multiplier_index, sequence_data_index, output->size_steps);
            /*#ifdef ENABLE_SCREEN
                menu.set_last_message("Loaded recording %i"); //, recording_number);
                menu.set_message_colour(ST77XX_GREEN);
            #endif*/
            Serial.printf("Loaded recording from [%s] - [%i] frames with total [%i] messages\n", filename, total_frames, total_messages);
            
            loaded_recording_number = recording_number;
            clear_hanging();

            return true;
        }       

};

extern midi_track mpk49_loop_track;

/**
 * @var {byte} the maximum number of instructions
 *      its possible to loop around while loops are still playing, if so instructions in the lower loop will be overwritten
 */
//const unsigned long MAX_INSTRUCTIONS = (LOOP_LENGTH); //100;


#endif