#include "behaviour_base.h"
#include "midi/midi_mapper_matrix_manager.h"

class MIDIBassBehaviour : virtual public DeviceBehaviourUltimateBase {
    public:

        bool new_bar = true;        // reset on new bar, so that we can pick up only the first note played
        bool drone_enabled = false; // whether or not drone is enabled
        int last_drone_note = -1;   // note that we should drone on
        byte drone_channel = 0;     // channel that we should drone on

        int8_t machinegun = 0;
        int8_t machinegun_current_note = -1;

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
            if (last_drone_note>=0) {
                //if (this->debug) Serial.printf(F("\t\tkill_drone_note(%i, %i, %i)\n"), last_drone_note, 0, drone_channel);
                DeviceBehaviourUltimateBase::sendNoteOff(last_drone_note, 0, drone_channel);
            }
            last_drone_note = -1;
            kill_machinegun_note();
        }
        virtual void send_drone_note() {
            //DeviceBehaviourSerialBase::sendNoteOff(last_drone_note, 0, drone_channel);
            if (last_drone_note>=0) {
                //if (this->debug) Serial.printf(F("\t\tsend_drone_note sending (%i, %i, %i)\n"), last_drone_note, 127, drone_channel);
                DeviceBehaviourUltimateBase::sendNoteOn(last_drone_note, 127, drone_channel);
            }
        }

        virtual void on_tick(uint32_t ticks) override {
            //MIDIOutputWrapper *wrapper = midi_matrix_manager->get_target_for_id(this->target_id);
            int note = -1;
            if (is_valid_note(last_drone_note)) {
                note = last_drone_note;
            } else {
                MIDIOutputWrapper *wrapper = midi_matrix_manager->get_target_for_id(this->target_id);
                if (wrapper!=nullptr && is_valid_note(wrapper->current_transposed_note))
                    note = wrapper->current_transposed_note;
                //if (is_valid_note(this->note))
            }
            if (machinegun && is_valid_note(note)) { //wrapper->current_transposed_note)) {
                int div = machinegun;
                int qt = ticks % PPQN;
                int vel = 127; //constrain(64+(127/qt), 64, 127);   // todo: add some clever velocity stuff here?
                if ((qt+1) % (PPQN/div) == 0) {
                    //Serial.printf("\tqt %i means should kill the note %i?\n", qt, note);
                    kill_machinegun_note();
                } else if (qt % (PPQN/div)==0) {
                    //Serial.printf("\tqt %i means should start the note %i?\n", qt, note);
                    DeviceBehaviourUltimateBase::sendNoteOn(note, vel, drone_channel);
                    this->machinegun_current_note = note;
                } 
            }
        }

        virtual void on_bar(int bar) override {
            //if (this->debug) Serial.printf(F("begin>=DeviceBehaviour_Neutron#on_bar(%i)\n"), bar);
            new_bar = true;
            last_drone_note = -1;
            //Serial.println("end<=DeviceBehaviour_Neutron#on_bar()");
        }
        virtual void on_end_bar(int bar) override {
            //if (this->debug) Serial.println(F("DeviceBehaviour_Neutron#on_end_bar!"));
            if (drone_enabled && last_drone_note>=0)
                this->kill_drone_note();
            if (machinegun>0 && is_valid_note(machinegun_current_note)) {
                kill_machinegun_note();
            }
        }

        virtual void kill_machinegun_note() {
            if (!is_valid_note(machinegun_current_note)) return;

            //Serial.printf("machinegun_current_note is %s, killing\n", get_note_name_c(machinegun_current_note));
            DeviceBehaviourUltimateBase::sendNoteOff(machinegun_current_note, 0, drone_channel);
            machinegun_current_note = -1;
        }

        virtual void sendNoteOn(byte pitch, byte velocity, byte channel = 0) override {
            //Serial.printf("DeviceBehaviour_Neutron#sendNoteOn(%i, %i, %i)\n", pitch, velocity, channel, last_drone_note);
            drone_channel = channel;

            if (drone_enabled) {
                // do drone stuff
                //if (this->debug) Serial.printf(F("DeviceBehaviour_Neutron#sendNoteOn in DRONE mode!\n"));
                if (last_drone_note==-1 && new_bar) {
                    last_drone_note = pitch;
                    //if (this->debug) Serial.printf(F("\tDeviceBehaviour_Neutron#sendNoteOn(%i, %i, %i) got new last_drone_note: %i\n"), pitch, velocity, channel, last_drone_note);
                    new_bar = false;
                    drone_channel = channel;
                    this->send_drone_note();
                }
            } else 
                DeviceBehaviourUltimateBase::sendNoteOn(pitch, velocity, channel);
        }

        virtual void sendNoteOff(byte pitch, byte velocity, byte channel) override {
            if (drone_enabled) {
                //
                if (!is_valid_note(last_drone_note) && machinegun>0 && pitch==machinegun_current_note)
                    kill_machinegun_note();
            } else
                DeviceBehaviourUltimateBase::sendNoteOff(pitch, velocity, channel);
        }

        virtual void setup_saveable_parameters() override {
            if (this->saveable_parameters==nullptr)
                DeviceBehaviourUltimateBase::setup_saveable_parameters();
            this->saveable_parameters->add(new SaveableParameter<MIDIBassBehaviour, bool>("drone", this, &this->drone_enabled, &MIDIBassBehaviour::set_drone, &MIDIBassBehaviour::is_drone));
            this->saveable_parameters->add(new SaveableParameter<MIDIBassBehaviour, int8_t>("machinegun", this, &this->machinegun, &MIDIBassBehaviour::set_machinegun, &MIDIBassBehaviour::get_machinegun));
        }

        /*virtual void save_sequence_add_lines(LinkedList<String> *lines) override {
            lines->add(String(F("drone=")) + String(this->drone_enabled ? F("enabled"):F("disabled")));
            lines->add(String(F("machinegun=")) + String(this->machinegun));
            //ClockedBehaviour::save_sequence_add_lines(lines);
        }
        virtual bool load_parse_key_value(String key, String value) override {
            if (key.equals(F("drone"))) {
                this->set_drone(value.equals(F("enabled")));
                //Serial.printf(F("drone found - setting to %s!\n"), drone_enabled?F("true"):F("false"));
                return true;
            } else if (key.equals(F("machinegun"))) {
                this->set_machinegun(value.toInt());
                return true;
            }
            return DeviceBehaviourUltimateBase::load_parse_key_value(key, value);
        }*/

        #ifdef ENABLE_SCREEN
            LinkedList<MenuItem *> *make_menu_items() override;
        #endif


};