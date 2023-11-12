#ifndef BEHAVIOUR_BASE__INCLUDED
#define BEHAVIOUR_BASE__INCLUDED

#include <Arduino.h>

#include <MIDI.h>
#include <USBHost_t36.h>

#include "LinkedList.h"

#include "midi/midi_mapper_matrix_types.h"

#include "parameters/Parameter.h"
#include "parameters/MIDICCParameter.h"
#include "ParameterManager.h"

#include "behaviours/SaveableParameters.h"

#include "file_manager/file_manager_interfaces.h"

#ifdef ENABLE_SCREEN
    #include "mymenu/menuitems_harmony.h"
#endif

class MenuItem;
class ArrangementTrackBase;

using namespace midi;

enum BehaviourType {
    undefined,
    usb,             // a USB MIDI device that identifies as a USB MIDI device
    serial,          // a MIDI device connected over a hardware serial port
    usbserial,       // a USB device that connects over serial, but doesn't support MIDI
    usbserialmidi,   // a USB MIDI device that identifies as a SERIAL device (ie OpenTheremin, Arduino device á la Hairless MIDI)
    virt             // a 'virtual' device type that exists only in code (eg CV Input)
};

enum NOTE_MODE {
    IGNORE, TRANSPOSE
};

class DeviceBehaviourUltimateBase : public IMIDIProxiedCCTarget {
    public:

    bool debug = false;

    #ifdef ENABLE_SCREEN
        uint16_t colour = C_WHITE;
    #endif

    source_id_t source_id = -1;
    target_id_t target_id = -1;

    //int force_octave = -1;
    int last_transposed_note = -1, current_transposed_note = -1;
    int current_channel = 0;
    int8_t TUNING_OFFSET = 0;
    //MIDIOutputWrapper *wrapper = nullptr;

    DeviceBehaviourUltimateBase() = default;
    virtual ~DeviceBehaviourUltimateBase() = default;

    virtual const char *get_label() {
        return (const char*)"UltimateBase";
    }

    virtual bool receives_midi_notes()  { return false; }
    virtual bool transmits_midi_notes() { return false; }

    // input/output indicator
    bool indicator_done = false;
    char indicator_text[6];
    virtual const char *get_indicator() {
        if (!indicator_done) {
            snprintf(indicator_text, 5, "%c%c", this->receives_midi_notes()?'I':' ', this->transmits_midi_notes()?'O':' ');
            indicator_done = true;
        }
        return indicator_text;
    }

    virtual int getType() {
        return BehaviourType::undefined;
    }

    //FLASHMEM 
    virtual void setup_callbacks() {};

    virtual bool is_connected() {
        return false;
    }

    virtual void read() {};
    // called every loop
    virtual void loop(uint32_t ticks) {};
    // on_pre_clock called before the clocks are sent
    virtual void on_pre_clock(uint32_t ticks) {};
    // called during tick when behaviour_manager send_clocks is called
    virtual void send_clock(uint32_t ticks) {};
    // called every tick, after clocks sent
    virtual void on_tick(uint32_t ticks) {};
    // called when new bar starts
    virtual void on_bar(int bar_number) {};
    virtual void on_end_bar(int bar_number) {};
    // called when the clock is restarted
    virtual void on_restart() {};
    // called when we change phrase
    virtual void on_phrase(uint32_t phrase) {};
    virtual void on_end_phrase(uint32_t phrase) {};
    // called the end of a phrase, but before clocks are sent - necessary for eg beatstep to send its 'next pattern' sysex?
    virtual void on_end_phrase_pre_clock(uint32_t phrase) {};
    virtual void receive_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    // called when a note_off message is received from the device
    virtual void receive_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    virtual void receive_control_change (uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
    virtual void receive_pitch_bend(uint8_t inChannel, int bend);

    virtual void init() {};

    virtual void killCurrentNote() {
        if (is_valid_note(current_transposed_note)) {
            this->actualSendNoteOff(current_transposed_note, MIDI_MIN_VELOCITY, this->current_channel); //velocity, channel);
            current_transposed_note = NOTE_OFF;
        }
    }
    // tell the device to play a note on
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
        //Serial.println("DeviceBehaviourUltimateBase#sendNoteOn");
        // TODO: this is where ForceOctave check should go..?
        note = this->recalculate_pitch(note);
        if (!is_valid_note(note)) return;
        this->current_transposed_note = note;
        this->current_channel = channel;

        note += this->TUNING_OFFSET;
        if (!is_valid_note(note)) return;

        this->actualSendNoteOn(note, velocity, channel);
    };
    // tell the device to play a note off
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
        //Serial.println("DeviceBehaviourUltimateBase#sendNoteOff");
        // TODO: this is where ForceOctave check should go..?
        note = this->recalculate_pitch(note);
        if (!is_valid_note(note)) return;
        this->last_transposed_note = note;
        if (this->current_transposed_note==note)
            this->current_transposed_note = NOTE_OFF;

        note += this->TUNING_OFFSET;
        if (!is_valid_note(note)) return;

        this->actualSendNoteOff(note, velocity, channel);
    };
    // tell the device to send a control change - implements IMIDIProxiedCCTarget
    virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) {
        //Serial.println("DeviceBehaviourUltimateBase#sendControlChange");
        this->actualSendControlChange(number, value, channel);
    };
    virtual void sendPitchBend(int16_t bend, uint8_t channel) {
        this->actualSendPitchBend(bend, channel);
    }
    // tell the device to send a realtime message
    virtual void sendRealTime(uint8_t message) {
        //Serial.println("DeviceBehaviourUltimateBase#sendRealTime");
        this->actualSendRealTime(message);
    };    
    virtual void sendNow() {};

    // implements IMIDIProxiedCCTarget
    virtual void sendProxiedControlChange(byte cc_number, byte value, byte channel = 0) {
        if (this->debug) Serial.printf(F("%s#sendProxiedControlChange(%i, %i, %i)\n"), this->get_label(), cc_number, value, channel);
        this->actualSendControlChange(cc_number, value, channel);
    }

    // use the underlying object to actually send a value
    virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {}
    virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {}
    virtual void actualSendControlChange(uint8_t number, uint8_t value, uint8_t channel) {}
    virtual void actualSendRealTime(uint8_t message) {}
    virtual void actualSendPitchBend(int16_t bend, uint8_t channel) {}

    // parameter handling shit
    LinkedList<FloatParameter*> *parameters = new LinkedList<FloatParameter*>();
    virtual LinkedList<FloatParameter*> *get_parameters () {
        if (parameters==nullptr || parameters->size()==0)
            this->initialise_parameters();
        return parameters;
    }
    virtual LinkedList<FloatParameter*> *initialise_parameters() {
        return parameters;
    }
    virtual bool has_parameters() {
        return this->get_parameters()->size()>0;
    }
    virtual FloatParameter* getParameterForLabel(const char *label) {
        //Serial.printf(F("getParameterForLabel(%s) in behaviour %s..\n"), label, this->get_label());
        for (unsigned int i = 0 ; i < parameters->size() ; i++) {
            //Serial.printf(F("Comparing '%s' to '%s'\n"), parameters->get(i)->label, label);
            if (strcmp(parameters->get(i)->label, label)==0) 
                return parameters->get(i);
        }
        Serial.printf(F("WARNING/ERROR in behaviour %s: didn't find a Parameter labelled %s\n"), this->get_label(), label);
        return nullptr;
    }

    virtual void reset_all_mappings() {
        for (unsigned int i = 0 ; i < this->parameters->size() ; i++) {
            this->parameters->get(i)->reset_mappings();
        }
    }

    virtual bool has_saveable_parameters() {
        return this->saveable_parameters!=nullptr && this->saveable_parameters->size()>0;
    }

    // saveable parameter handling shit
    LinkedList<SaveableParameterBase*> *saveable_parameters = nullptr;
    virtual void setup_saveable_parameters() {
        if (this->saveable_parameters==nullptr) {
            Debug_println("instantiating saveable_parameters list");
            this->saveable_parameters = new LinkedList<SaveableParameterBase*> ();
        }

        if(this->transmits_midi_notes()) {
            this->saveable_parameters->add(new LSaveableParameter<int8_t>("lowest_note",       "Note restriction", nullptr, [=](int8_t v) -> void { this->setLowestNote(v); },     [=]() -> int8_t { return this->getLowestNote(); }));
            this->saveable_parameters->add(new LSaveableParameter<int8_t>("highest_note",      "Note restriction", nullptr, [=](int8_t v) -> void { this->setHighestNote(v); },    [=]() -> int8_t { return this->getHighestNote(); }));
            this->saveable_parameters->add(new LSaveableParameter<int8_t>("lowest_note_mode",  "Note restriction", nullptr, [=](int8_t v) -> void { this->setLowestNoteMode(v); }, [=]() -> int8_t { return this->getLowestNoteMode(); }));
            this->saveable_parameters->add(new LSaveableParameter<int8_t>("highest_note_mode", "Note restriction", nullptr, [=](int8_t v) -> void { this->setHighestNoteMode(v); },[=]() -> int8_t { return this->getHighestNoteMode(); }));
        }

        // todo: add all the modulatable parameters via a wrapped class
        /*if (this->has_parameters()) {
            for (unsigned int i = 0 ; i < parameters->size() ; i++) {
                this->saveable_parameters->add(new SaveableParameterWrapper(parameters->get(i)));
            }
        }*/
    }
    virtual bool load_parse_key_value_saveable_parameters(String key, String value) {
        for (unsigned int i = 0 ; i < saveable_parameters->size() ; i++) {
            if (saveable_parameters->get(i)->is_recall_enabled() && saveable_parameters->get(i)->parse_key_value(key, value))
                return true;
        }
        return false;
    }
    virtual void save_sequence_add_lines_saveable_parameters(LinkedList<String> *lines) {
        for (unsigned int i = 0 ; i < saveable_parameters->size() ; i++) {
            Debug_printf("%s#save_sequence_add_lines_saveable_parameters() processing %i aka '%s'..\n", this->get_label(), i, saveable_parameters->get(i)->label);
            if (saveable_parameters->get(i)->is_save_enabled()) {
                Debug_printf("\t\tis_save_enabled() returned true, getting line!");
                lines->add(saveable_parameters->get(i)->get_line());
            }
        }
    }

    virtual void save_project_add_lines(LinkedList<String> *lines) {

    }
    virtual bool parse_project_key_value(String key, String value) {
        return false;
    }

    // save all the parameter mapping settings; override in subclasses, which should call back up the chain
    virtual void save_sequence_add_lines(LinkedList<String> *lines) {
        this->save_sequence_add_lines_parameters(lines);
        this->save_sequence_add_lines_saveable_parameters(lines);
    }

    virtual void save_sequence_add_lines_parameters(LinkedList<String> *lines) {
        Debug_println("save_sequence_add_lines_parameters..");
        if (this->has_parameters()) {
            LinkedList<FloatParameter*> *parameters = this->get_parameters();
            for (unsigned int i = 0 ; i < parameters->size() ; i++) {
                FloatParameter *parameter = parameters->get(i);

                parameter->save_sequence_add_lines(lines);
            }
        }
        Serial.println("finished save_sequence_add_lines_parameters.");
    }

    // ask behaviour to process the key/value pair
    virtual bool load_parse_key_value(String key, String value) {
        if (this->load_parse_key_value_saveable_parameters(key, value)) {
            return true;
        }
        Debug_printf(F("PARAMETERS\tload_parse_key_value passed '%s' => '%s'...\n"), key.c_str(), value.c_str());
        //static String prefix = String("parameter_" + this->get_label());

        // todo: can optimise this for most cases by remembering the last-found parameter and starting our search there instead
        //              eg something like parameter_manager->fast_load_parse_key_value(this->parameters, key, value)
        const char *prefix = "parameter_";
        if (this->has_parameters() && key.startsWith(prefix)) {
            /*for (unsigned int i = 0 ; i < parameters->size() ; i++) {
                if (parameters->get(i)->load_parse_key_value(key, value)) 
                    return true;
            }*/
            if (parameter_manager->fast_load_parse_key_value(key, value, this->parameters))
                return true;
        }
        ///Serial.printf(F("...load_parse_key_value(%s, %s) isn't a parameter!\n"));
        return false;
    }

    #ifdef ENABLE_SCREEN
        LinkedList<MenuItem*> *menuitems = nullptr;
        FLASHMEM
        virtual LinkedList<MenuItem*> *make_menu_items();
        //FLASHMEM
        virtual LinkedList<MenuItem*> *make_menu_items_device() {
            // dummy device menuitems
            return this->menuitems;
        }
        FLASHMEM
        virtual LinkedList<MenuItem*> *create_saveable_parameters_recall_selector();

        HarmonyStatus *output_harmony_status = nullptr; // store a pointer to this so we can update it from subclasses, eg MIDIBass
    #endif


    /*virtual void setForceOctave(int octave);
    virtual int getForceOctave() {
        //Serial.println("Beatstep_Behaviour#getForceOctave!"); Serial_flush();
        return this->force_octave;
    }*/

    int8_t lowest_note_mode  = NOTE_MODE::IGNORE;
    int8_t highest_note_mode = NOTE_MODE::IGNORE;
    int8_t lowest_note = 0;
    int8_t highest_note = MIDI_MAX_NOTE;

    virtual void setLowestNote(int8_t note) {
        // don't allow highest note to be set higher than highest note
        if (note > this->getHighestNote())
            note = this->getHighestNote();
        if (!is_valid_note(note)) 
            note = MIDI_MIN_NOTE;
        // if the currently playing note doesn't fit within the new bounds, kill it
        //if (is_valid_note(this->current_transposed_note) && this->current_transposed_note < note)
        //if (note > getLowestNote()) // if new lower-note-limit is higher than existing lower-note-limit, force kill of note
        // TODO: hmmm so, restricting this kill command as in the above two commented-out lines results in stuck notes on Neutron -- weird, why?
            this->killCurrentNote();
            //this->sendNoteOff(this->current_transposed_note, 0, 0);
        this->lowest_note = note;
    }
    virtual int8_t getLowestNote() {
        return this->lowest_note;
    }
    virtual int8_t getLowestNoteMode() {
        return lowest_note_mode;
    }
    virtual void setLowestNoteMode(int8_t mode) {
        if (is_valid_note(this->current_transposed_note) && this->current_transposed_note < this->getLowestNote())
            this->killCurrentNote();
            //this->sendNoteOff(this->current_transposed_note, 0, 0);
        this->lowest_note_mode = mode;
    }

    virtual void setHighestNote(int8_t note) {
        // don't allow highest note to be set lower than lowest note
        if (note < this->getLowestNote())
            note = this->getLowestNote();
        if (!is_valid_note(note)) 
            note = MIDI_MAX_NOTE;
        // if the currently playing note doesn't fit within the new bounds, kill it
        //if (is_valid_note(this->current_transposed_note) && this->current_transposed_note > note)
        //if (note < getHighestNote()) // if new higher-note-limit is lower than existing higher-note-limit, force kill of note
        // TODO: hmmm so, restricting this kill command as in the above two commented-out lines results in stuck notes on Neutron -- weird, why?
            this->killCurrentNote();
            //this->sendNoteOff(this->current_transposed_note, MIDI_MIN_VELOCITY, this->current_channel);
        this->highest_note = note;
    }
    virtual int8_t getHighestNote() {
        return this->highest_note;
    }
    virtual int8_t getHighestNoteMode() {
        return this->highest_note_mode;
    }
    virtual void setHighestNoteMode(int8_t mode) {
        if (is_valid_note(this->current_transposed_note) && this->current_transposed_note > this->getHighestNote())
            this->killCurrentNote();
            //this->sendNoteOff(this->current_transposed_note, 0, 0);
        this->highest_note_mode = mode;
    }

    // remap pitch if force octave is on, TODO: other tranposition modes
    // TODO: two separate controls: one for lowest pitch/octave, one for highest pitch/octave
    // TODO: two separate controls: out-of-bounds-lower rule, out-of-bounds-higher-rule
    //          options: ignore (just don't play the note in question); transpose (move the note into allowed range)
    // TODO: move quantisation calculation here...?
    virtual int recalculate_pitch(byte note) {
        /*if (this->force_octave >= 0) {
            // send incoming notes from beatstep back out to neutron on serial3, but transposed down
            uint8_t note2 = note % 12;
            note2 += (force_octave*12); //24;
            if(this->debug) {
                Serial.printf("\trecalculate_pitch on %i => %i\n", note, note2);
            }
            note = note2;
            return note2;
        }*/
        if (!is_valid_note(note)) return NOTE_OFF;

        if (debug && (getLowestNote()>0 || getHighestNote()<127)) 
            Serial.printf("Incoming note is\t%i (%s), bounds are\t%i (%s) to\t%i (%s)\n", note, get_note_name_c(note), getLowestNote(), get_note_name_c(getLowestNote()), getHighestNote(), get_note_name_c(getHighestNote()));
            
        if (note < getLowestNote()) {
            if (this->debug) Serial.printf("\tnote %i (%s)\tis lower than lowest note\t%i (%s)\n", note, get_note_name_c(note), getLowestNote(), get_note_name_c(getLowestNote()));
            if (getLowestNoteMode()==NOTE_MODE::IGNORE) {
                if (this->debug) Serial.println("\tignoring!");
                note = NOTE_OFF;
            } else if (getLowestNoteMode()==NOTE_MODE::TRANSPOSE) {
                //int8_t octave = note / 12;
                /*int8_t lowest_octave = getLowestNote() / 12;
                int8_t chromatic_pitch = note % 12;
                int8_t note2 = note;
                note = (lowest_octave*12) + chromatic_pitch;*/
                int8_t note2 = note;
                while (is_valid_note(note) && note < getLowestNote()) {
                    note += 12;
                }
                if (this->debug) Serial.printf("\ttransposed from %i (%s) up to\t%i (%s)\n", note2, get_note_name_c(note2), note, get_note_name_c(note));
            }
        } else if (note > getHighestNote()) {
            if (this->debug) Serial.printf("\tnote\t%i (%s)\tis higher than highest note\t%i (%s)\n", note, get_note_name_c(note), getHighestNote(), get_note_name_c(getHighestNote()));
            if (getHighestNoteMode()==NOTE_MODE::IGNORE) {
                if (this->debug) Serial.println("\tignoring!");
                note = NOTE_OFF;
            } else if (getHighestNoteMode()==NOTE_MODE::TRANSPOSE) {
                //int8_t octave = note / 12;
                /*int8_t highest_octave = getHighestNote() / 12;
                int8_t chromatic_pitch = note % 12;
                int8_t note2 = note;
                note = ((highest_octave-1)*12) + chromatic_pitch;*/
                int8_t note2 = note;
                while (is_valid_note(note) && note > getHighestNote()) {
                    note -= 12;
                }
                if (this->debug) Serial.printf("\ttransposed from %i (%s) down to\t%i (%s)\n", note2, get_note_name_c(note2), note, get_note_name_c(note));
                //Serial.printf("\t\t(highest_octave =\t%i, chromatic_pitch =\t%i (%s))\n", highest_octave, chromatic_pitch, note_names[chromatic_pitch]);
            }
        }
        if (!is_valid_note(note) || note < getLowestNote() || note > getHighestNote()) {
            if (this->debug) Serial.printf("\t%i (%s) isn't valid or out of bounds\n", note, get_note_name_c(note));
            return NOTE_OFF;
        }
        return note;
    }

    
};

#endif