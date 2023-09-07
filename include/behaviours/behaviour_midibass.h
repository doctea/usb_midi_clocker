#ifndef BEHAVIOUR_MIDIBASS__INCLUDED
#define BEHAVIOUR_MIDIBASS__INCLUDED

#include "behaviour_base.h"
#include "midi/midi_mapper_matrix_manager.h"

class MIDIBassBehaviour : virtual public DeviceBehaviourUltimateBase {
    public:

        bool new_bar = true;        // reset on new bar, so that we can pick up only the first note played
        bool drone_enabled = false; // whether or not drone is enabled
        int last_drone_note = NOTE_OFF;   // note that we should drone on
        byte drone_channel = 0;     // channel that we should drone on

        int8_t machinegun = 0;
        int8_t machinegun_current_note = NOTE_OFF;

        void set_machinegun(int8_t value) {
            this->machinegun = value;
            if (!machinegun)
                kill_machinegun_note();
        }
        int8_t get_machinegun() {
            return this->machinegun;
        }

        virtual bool is_drone() {
            return drone_enabled;
        }
        virtual void set_drone(bool value) {
            if (this->drone_enabled && !value && this->last_drone_note>=0) {
                // turning drone off and there is a note to kill
                //if (this->debug) Serial.println(F("set_drone setting drone_enabled off - calling kill_drone_note!"));
                this->kill_drone_note();
            } else if (!this->drone_enabled && value) {
                // turning drone on
                // should kill any (non-drone) notes that we have playing... except thats handled by the MIDIOutputWrapper that's wrapped around this behaviour, rather than the output that this is connected to.  
                // hmmm.  so, how do we untangle this?  should DeviceBehaviours be subclasses of MIDIOutputWrapper too?
                    // TODO: yeah this ugly af kludge should work for now
                    //if (this->target_id == -1)     // at least cache it
                    //    target_id = midi_matrix_manager->get_target_id_for_handle("S3 : Neutron : ch 4");
                if (this->target_id == -1)
                    Serial.printf(F("WARNING: MIDIBassBehaviour#set_drone has target_id of -1 in %s!\n"), this->get_label());
                midi_matrix_manager->stop_all_notes_for_target(this->target_id);
            }
            this->drone_enabled = value;
        }
        virtual int get_drone_note() {
            return this->last_drone_note;
        }

        virtual void kill_drone_note() {            
            if (is_valid_note(last_drone_note)) {
                if (this->debug) Serial.printf(F("\t\tkill_drone_note(%i, %i, %i)>>>>\n"), last_drone_note, 0, drone_channel);
                DeviceBehaviourUltimateBase::sendNoteOff(last_drone_note, MIDI_MIN_VELOCITY, drone_channel);
                if (this->debug) Serial.printf(F("\t\t<<<< after kill_drone_note(%i, %i, %i)\n"), last_drone_note, 0, drone_channel);
            }
            last_drone_note = NOTE_OFF;
            if (machinegun>0)
                kill_machinegun_note();
        }
        virtual void send_drone_note() {
            //DeviceBehaviourSerialBase::sendNoteOff(last_drone_note, 0, drone_channel);
            if (last_drone_note>=0) {
                if (this->debug) Serial.printf(F("\t\tsend_drone_note sending (p=%s, v=%i, c=%i)\n"), get_note_name_c(last_drone_note), 127, drone_channel);
                DeviceBehaviourUltimateBase::sendNoteOn(last_drone_note, MIDI_MAX_VELOCITY, drone_channel);
            }
        }

        virtual void killCurrentNote() override {
            this->kill_drone_note();
            this->kill_machinegun_note();
            DeviceBehaviourUltimateBase::killCurrentNote();
        }

        // midibassproxy drone works, but machinegun doesn't, if we use on_tick here
        //virtual void on_tick(uint32_t ticks) override {
        virtual void on_pre_clock(uint32_t ticks) override {    // both drone and machinegun seem to work if we do on_pre_clock here?
            //MIDIOutputWrapper *wrapper = midi_matrix_manager->get_target_for_id(this->target_id);
            int note = NOTE_OFF;
            if (is_valid_note(last_drone_note)) {
                //if (this->debug) Serial.printf("%s#on_tick(%i) with last_drone_note %s\n", this->get_label(), ticks%24, get_note_name_c(last_drone_note));
                note = last_drone_note;
            } else {
                //MIDIOutputWrapper *wrapper = midi_matrix_manager->get_target_for_id(this->target_id);
                //if (wrapper!=nullptr && is_valid_note(current_transposed_note))
                //    note = current_transposed_note;
                //if (this->debug) Serial.printf("%s#on_tick(%i) with current_transposed_note %s\n", this->get_label(), ticks%24, get_note_name_c(note));
                //if (is_valid_note(this->note))
                note = current_transposed_note;
            }
            if (machinegun && is_valid_note(note)) { //wrapper->current_transposed_note)) {
                //if (this->debug) Serial.printf("%s#on_tick(%i) with machinegun %i and note %s\n", this->get_label(), ticks%24, machinegun, get_note_name_c(note));
                int div = machinegun;
                int qt = ticks % PPQN;
                int vel = MIDI_MAX_VELOCITY; //constrain(64+(127/qt), 64, 127);   // todo: add some clever velocity stuff here?
                if ((qt+1) % (PPQN/div) == 0) {
                    if (debug) Serial.printf("%s:\tqt %i means should kill the note %s?\n", this->get_label(), qt, get_note_name_c(note));
                    /// TODO: fix this workaround -- for some reason only necessary for MIDIBassProxy, as Neutron works OK?! 
                    //      if this workaround isn't in place then we don't seem to send out note offs for machine gun properly, like machinegun_current_note doesn't get set properly..?
                    int prev_note = this->machinegun_current_note;
                    this->machinegun_current_note = note;
                    kill_machinegun_note();
                    this->machinegun_current_note = prev_note;
                } else if (qt % (PPQN/div)==0) {
                    if (debug) Serial.printf("%s:\tqt %i means should start the note %s?\n", this->get_label(), qt, get_note_name_c(note));
                    DeviceBehaviourUltimateBase::sendNoteOn(note, vel, drone_channel);
                    this->machinegun_current_note = note;
                } 
            } else {
                //if (this->debug) Serial.printf("%s#on_tick(%i) isn't machinegun or isn't valid note %s\n", this->get_label(), ticks%24, get_note_name_c(note));
            }
        }

        virtual void on_bar(int bar) override {
            //if (this->debug) Serial.printf(F("begin>=DeviceBehaviour_Neutron#on_bar(%i)\n"), bar);
            new_bar = true;
            last_drone_note = NOTE_OFF;
            //Serial.println("end<=DeviceBehaviour_Neutron#on_bar()");
        }
        virtual void on_end_bar(int bar) override {
            if (this->debug) Serial.printf(F("%s#on_end_bar!\n"), this->get_label());
            if (drone_enabled && is_valid_note(last_drone_note)) {
                if (debug) Serial.printf("%s#on_end_bar(%i) doing kill_drone_note() for\t %i\n", this->get_label(), bar, last_drone_note);
                this->kill_drone_note();
            }
            if (machinegun > 0 && is_valid_note(this->machinegun_current_note)) {
                if (debug) Serial.printf("%s#on_end_bar(%i) doing kill_machinegun_note() for\t %i\n", this->get_label(), bar, machinegun_current_note);
                this->kill_machinegun_note();
            }
        }

        virtual void kill_machinegun_note() {
            if (!is_valid_note(this->machinegun_current_note)) {
                if (this->debug) Serial.printf("%s#kill_machinegun_note(): machinegun_current_note %i is not valid, ignoring!\n", this->get_label(), this->machinegun_current_note);
                return;
            }

            if (this->debug) Serial.printf("%s#kill_machinegun_note(): note is %s, killing!\n", this->get_label(), get_note_name_c(this->machinegun_current_note));
            DeviceBehaviourUltimateBase::sendNoteOff(this->machinegun_current_note, MIDI_MIN_VELOCITY, drone_channel);
            this->machinegun_current_note = NOTE_OFF;
        }

        virtual void sendNoteOn(byte pitch, byte velocity, byte channel = 0) override {
            if (this->debug) Serial.printf("%s#sendNoteOn\t(p=%3s,\tv=%3i,\tc=%2i)\n", this->get_label(), get_note_name_c(pitch), velocity, channel);

            if (drone_enabled) {
                drone_channel = channel;

                // do drone stuff
                //if (this->debug) Serial.printf(F("DeviceBehaviour_Neutron#sendNoteOn in DRONE mode!\n"));
                if (!is_valid_note(last_drone_note) && new_bar) {
                    if (this->machinegun)
                        this->machinegun_current_note = pitch;
                    last_drone_note = pitch;
                    //if (this->debug) Serial.printf(F("\tDeviceBehaviour_Neutron#sendNoteOn(%i, %i, %i) got new last_drone_note: %i\n"), pitch, velocity, channel, last_drone_note);
                    new_bar = false;
                    drone_channel = channel;
                    this->send_drone_note();
                }
            } else 
                DeviceBehaviourUltimateBase::sendNoteOn(pitch, velocity, channel);
        }

        virtual void sendNoteOff(byte pitch, byte velocity, byte channel = 0) override {
            if (drone_enabled) {
                //
                if (!is_valid_note(last_drone_note) && machinegun>0 && pitch==this->machinegun_current_note)
                    kill_machinegun_note();
            } else
                DeviceBehaviourUltimateBase::sendNoteOff(pitch, velocity, channel);
        }

        virtual void setup_saveable_parameters() override {
            if (this->saveable_parameters==nullptr)
                DeviceBehaviourUltimateBase::setup_saveable_parameters();

            //MIDIOutputWrapper *my_wrapper = midi_matrix_manager->get_target_for_id(this->target_id);
            //this->saveable_parameters->add(new SaveableParameter<DeviceBehaviourUltimateBase, int>("octave", "MIDI Bass", this, nullptr, nullptr, nullptr, &DeviceBehaviourUltimateBase::setForceOctave, &DeviceBehaviourUltimateBase::getForceOctave));
            this->saveable_parameters->add(new SaveableParameter<MIDIBassBehaviour, bool>("drone", "MIDI Bass", this, &this->drone_enabled, nullptr, nullptr, &MIDIBassBehaviour::set_drone, &MIDIBassBehaviour::is_drone));
            this->saveable_parameters->add(new SaveableParameter<MIDIBassBehaviour, int8_t>("machinegun", "MIDI Bass", this, &this->machinegun, nullptr, nullptr, &MIDIBassBehaviour::set_machinegun, &MIDIBassBehaviour::get_machinegun));
        }

        #ifdef ENABLE_SCREEN
            LinkedList<MenuItem *> *make_menu_items() override;
        #endif


};

#endif