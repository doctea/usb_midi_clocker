#pragma once

#include <Arduino.h>

#include <MIDI.h>
#include <USBHost_t36.h>

#include "LinkedList.h"

#include "midi/midi_mapper_matrix_types.h"

#include "notetracker.h"
#include "Drums.h"

// saveloadlib must be included before Parameter.h because ParameterInput.h inherits SHDynamic
#include "saveload_settings.h"

#include "parameters/Parameter.h"
#include "parameters/ProxyParameter.h"
#include "parameters/MIDICCParameter.h"
#include "ParameterManager.h"

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

enum class PitchBendSupport : uint8_t {
    PASSTHRU,
    MODULATED
};

// enum NOTE_MODE moved to midihelpers and renamed NOTE_LIMIT_MODE

class DeviceBehaviourUltimateBase :
        public virtual IMIDIProxiedCCTarget, 
        public virtual IMIDINoteAndCCTarget, 
        public virtual IMIDIPitchBendTarget,
        public virtual SHDynamic<4, 4> 
    {
    public:

    bool debug = false;

    #ifdef ENABLE_SCREEN
        uint16_t colour = C_WHITE;
    #endif

    source_id_t source_id = -1;
    target_id_t target_id = -1;

    //int force_octave = -1;
    int8_t last_transposed_note = -1, current_transposed_note = -1;
    int8_t current_channel = 0;
    int8_t TUNING_OFFSET = 0;
    //MIDIOutputWrapper *wrapper = nullptr;

    // Tracks only notes sent directly by this behaviour. Matrix-routed notes are
    // paired per source-target connection by MIDIMatrixManager instead.
    NoteTracker note_tracker;

    DeviceBehaviourUltimateBase() = default;
    virtual ~DeviceBehaviourUltimateBase() = default;

    virtual const char *get_label() {
        return (const char*)"UltimateBase";
    }

    virtual bool receives_midi_notes()  { return false; }
    virtual bool transmits_midi_notes() { return false; }
    virtual bool transmits_midi_clock() { return false; }
    virtual bool supports_note_limits() { return false; }

    virtual PitchBendSupport get_pitch_bend_support() const { return PitchBendSupport::PASSTHRU; }
    virtual bool supports_passthru_pitch_bend() const {
        return this->get_pitch_bend_support() == PitchBendSupport::PASSTHRU;
    }
    virtual bool supports_advanced_pitch_bend() const {
        return this->get_pitch_bend_support() == PitchBendSupport::MODULATED;
    }

    virtual int8_t get_pitch_bend_range_semitones() const {
        return 2;
    }

    virtual float pitch_bend_to_normalized(int16_t bend) const {
        return constrain((float)bend / 8192.0f, -1.0f, 1.0f);
    }
    virtual int16_t normalized_to_pitch_bend(float normalized) const {
        normalized = constrain(normalized, -1.0f, 1.0f);
        long bend = (long)(normalized * 8192.0f);
        bend = constrain(bend, -8192L, 8191L);
        return (int16_t)bend;
    }
    virtual float pitch_bend_to_semitones(int16_t bend) const {
        return pitch_bend_to_normalized(bend) * (float)this->get_pitch_bend_range_semitones();
    }
    virtual int16_t semitones_to_pitch_bend(float semitones) const {
        float range = (float)this->get_pitch_bend_range_semitones();
        if (range <= 0.0f)
            return 0;
        return normalized_to_pitch_bend(semitones / range);
    }

    #if defined(ENABLE_ADVANCED_PITCHBEND) && defined(ENABLE_PARAMETERS)
        int16_t last_received_pitch_bend = 0;
        float last_received_pitch_bend_semitones = 0.0f;
        LDataParameter<float> *pitch_bend_parameter = nullptr;

        virtual void emit_effective_pitch_bend_from_semitones(float semitones, uint8_t channel) {
            this->sendPitchBend(semitones_to_pitch_bend(semitones), channel);
        }

        virtual void ensure_advanced_pitch_bend_parameter() {
            if (this->pitch_bend_parameter != nullptr)
                return;

            const int8_t range = this->get_pitch_bend_range_semitones();
            this->pitch_bend_parameter = new LDataParameter<float>(
                "Pitch Bend",
                [this](float semitones) -> void {
                    this->emit_effective_pitch_bend_from_semitones(semitones, this->current_channel);
                },
                [this]() -> float {
                    return this->last_received_pitch_bend_semitones;
                },
                (float)(-range),
                (float)(range)
            );
            for (uint8_t i = 0; i < MAX_SLOT_CONNECTIONS; i++) {
                this->pitch_bend_parameter->connections[i].polar_mode = MOD_SLOT_UNI_CENTERED;
            }
            this->pitch_bend_parameter->float_unit = 'c'; // cents
            this->pitch_bend_parameter->setInitialValueFromData(0.0f);
            this->parameters->add(this->pitch_bend_parameter);
        }

        // Return true once the advanced path has fully handled the event.
        virtual bool handle_modulated_pitch_bend(uint8_t inChannel, int bend) {
            if (!this->supports_advanced_pitch_bend() || !this->transmits_midi_notes())
                return false;

            this->ensure_advanced_pitch_bend_parameter();
            if (this->pitch_bend_parameter == nullptr)
                return false;

            this->current_channel = inChannel;
            this->last_received_pitch_bend = (int16_t)bend;
            this->last_received_pitch_bend_semitones = this->pitch_bend_to_semitones((int16_t)bend);
            this->pitch_bend_parameter->updateValueFromData(this->last_received_pitch_bend_semitones);

            return true;
        }
    #endif

    bool note_limit_parameters_setup = false;
    virtual void ensure_note_limit_parameters();

    // input/output indicator
    bool indicator_done = false;
    char indicator_text[8];
    virtual const char *get_indicator() {
        if (!indicator_done) {
            snprintf(indicator_text, 7, "%c%c%c", this->receives_midi_notes()?'I':' ', this->transmits_midi_notes()?'O':' ', this->transmits_midi_clock()?'C': ' ');
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
    virtual void on_end_beat(int beat_number) {};
    // called when the clock is restarted
    virtual void on_restart() {};
    virtual void release_outputs() {};
    // called when we change phrase
    virtual void on_phrase(uint32_t phrase) {};
    virtual void on_end_phrase(uint32_t phrase) {};
    // called the end of a phrase, but before clocks are sent - necessary for eg beatstep to send its 'next pattern' sysex?
    virtual void on_end_phrase_pre_clock(uint32_t phrase) {};
    virtual void receive_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    // called when a note_off message is received from the device
    virtual void receive_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity);
    virtual void receive_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue);
    virtual void receive_pitch_bend(uint8_t inChannel, int16_t bend);

    virtual void init() {};

    #ifdef ENABLE_SCALES
        virtual int requantise_all_notes();
    #endif

    // abstracted note_tracker access so that we can override to eg track multiple channels separately if we need to
    virtual bool note_tracker_foreach_note(NoteTracker::foreach_func_def func) {
        return note_tracker.foreach_note(func);
    }
    virtual int8_t note_tracker_get_transposed_note_for(int8_t note) {
        return note_tracker.get_transposed_note_for(note);
    }
    virtual bool is_note_held(int8_t note, int8_t channel) {
        return note_tracker.is_note_held(note);
    }
    virtual bool note_tracker_held_note_on(int8_t note, int8_t transposed_note, int8_t channel) {
        return note_tracker.held_note_on(note, transposed_note);
    }
    virtual bool note_tracker_held_note_off(int8_t note, int8_t channel) {
        return note_tracker.held_note_off(note);
    }
    virtual int8_t note_tracker_count_held() {
        return note_tracker.count_held();
    }
    virtual const char *note_tracker_get_held_notes_c() {
        return note_tracker.get_held_notes_c();
    }

    virtual void killCurrentNote() {
        //Serial.println("-=-=-");
        if (is_valid_note(current_transposed_note)) {
            if (debug) Serial_printf("%20s: killCurrentNote() killing TRANSPOSED note %i (%s) on channel %i\n", this->get_label(), current_transposed_note, get_note_name_c(current_transposed_note), this->current_channel);
            this->sendNoteOff(current_transposed_note, MIDI_MIN_VELOCITY, this->current_channel); //velocity, channel);
            //current_transposed_note = NOTE_OFF;
        }
        note_tracker_foreach_note([=](int8_t note, int8_t transposed_note) {
            //this->actualSendNoteOff(note, MIDI_MIN_VELOCITY, this->current_channel);
            if (debug) Serial_printf("%20s: killCurrentNote() killing TRACKED note %i (%s) on channel %i\n", this->get_label(), note, get_note_name_c(note), this->current_channel);
            this->sendNoteOff(note, MIDI_MIN_VELOCITY, this->current_channel);
            //note_tracker.held_note_off(note);
        });
        if (current_transposed_note!=NOTE_OFF) {
            if (debug) Serial_printf("%20s: killCurrentNote() still have current_transposed_note=%i (%s) on channel %i\n", this->get_label(), current_transposed_note, get_note_name_c(current_transposed_note), this->current_channel);
        }
        if (debug) {
            note_tracker_foreach_note([=](int8_t note, int8_t transposed_note) {
                Serial_printf("%20s: killCurrentNote() still have TRACKED note %i (%s) on channel %i\n", this->get_label(), note, get_note_name_c(note), this->current_channel);
            });
        }
        //Serial.println("-=-=-");
    }
    // send note off without doing any of the quantising stuff -- used for killing notes after a harmony etc change where the usual note off would
    // get tranposed to the wrong thing
    virtual void sendNoteOffRaw(uint8_t note, uint8_t velocity, uint8_t channel) {
        this->actualSendNoteOff(note, velocity, channel);
    }

    // This is call-stack state, not held-note state: it is nonzero only while a
    // matrix wrapper is synchronously delivering a note through virtual methods.
    uint8_t routed_note_dispatch_depth = 0;

    class RoutedNoteDispatchScope {
        DeviceBehaviourUltimateBase &behaviour;

    public:
        RoutedNoteDispatchScope(DeviceBehaviourUltimateBase &behaviour) : behaviour(behaviour) {
            ++behaviour.routed_note_dispatch_depth;
        }
        ~RoutedNoteDispatchScope() {
            --behaviour.routed_note_dispatch_depth;
        }
    };

    bool is_processing_routed_note() const {
        return routed_note_dispatch_depth > 0;
    }

    virtual routed_note_output_t resolve_routed_note_output(uint8_t note, uint8_t channel) {
        routed_note_output_t result;
        result.channel = channel;
        result.pitch = apply_note_limits(
            note, this->getLowestNoteMode(), this->getHighestNoteMode(),
            get_effective_lowest_note(), get_effective_highest_note()
        );
        if (!is_valid_note(result.pitch))
            return result;
        result.pitch += this->TUNING_OFFSET;
        result.emitted = is_valid_note(result.pitch);
        return result;
    }

    virtual void send_resolved_routed_note_on(uint8_t note, uint8_t velocity, uint8_t channel) {
        this->actualSendNoteOn(note, velocity, channel);
    }
    virtual void send_resolved_routed_note_off(uint8_t note, uint8_t velocity, uint8_t channel) {
        this->actualSendNoteOff(note, velocity, channel);
    }

    // A behaviour-backed matrix target must still pass through virtual sendNote*
    // overrides (voice allocation, trigger channels, etc.). The scope tells the
    // base implementation not to quantise or track that delivery a second time.
    virtual void sendRoutedNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
        RoutedNoteDispatchScope scope(*this);
        this->sendNoteOn(note, velocity, channel);
    }
    virtual void sendRoutedNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
        RoutedNoteDispatchScope scope(*this);
        this->sendNoteOff(note, velocity, channel);
    }
    // tell the device to play a note on
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override;
    // tell the device to play a note off; handles quantisation and note tracking so that the correct note gets turned off even if quantisation would normally change it, and so that held notes get tracked properly
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        //Serial.println("DeviceBehaviourUltimateBase#sendNoteOff");

        if (debug) Serial_printf("%20s:\tDeviceBehaviourUltimateBase#sendNoteOff(%i, %i, %i)\n", this->get_label(), note, velocity, channel);
        
        // do nothing if passed an invalid note
        if (!is_valid_note(note)) return;

        // The matrix ledger has already selected and remembered the routed pitch.
        // Apply only target-local transforms here; ordinary local notes use the
        // behaviour NoteTracker path below.
        if (is_processing_routed_note()) {
            int8_t output_note = apply_note_limits(note, this->getLowestNoteMode(), this->getHighestNoteMode(), get_effective_lowest_note(), get_effective_highest_note());
            if (!is_valid_note(output_note))
                return;
            output_note += this->TUNING_OFFSET;
            if (is_valid_note(output_note))
                this->actualSendNoteOff(output_note, velocity, channel);
            return;
        }

        // Direct behaviour send: release the pitch remembered at the matching
        // local Note On rather than recalculating against current harmony.
        int8_t quantised_note = note_tracker_get_transposed_note_for(note);
        if (debug) Serial_printf("\t\t note_tracker.get_transposed_note_for(%i) = %i (%s)\n", note, quantised_note, get_note_name_c(quantised_note));

        // remember the transposed note settings (todo: why?  where is this used?)
        this->last_transposed_note = note;
        if (this->current_transposed_note==note)
            this->current_transposed_note = NOTE_OFF;

        note_tracker_held_note_off(note, channel);

        // adjust note according to tuning offset; this is for devices that have a tuning offset setting, eg MIDIMusoCV12
        // todo: is this happening at the right place?  previously had this before the note_tracker_held_off check
        quantised_note += this->TUNING_OFFSET;
        if (!is_valid_note(note)) return;

        if (debug) Serial_printf("%20s:\tDeviceBehaviourUltimateBase#sendNoteOff(%i, %i, %i) -> quantised_note %i, about to call actualSendNoteOff(%i..)\n", this->get_label(), note, velocity, channel, quantised_note, quantised_note);
        this->actualSendNoteOff(quantised_note, velocity, channel);
    };
    // tell the device to send a control change - implements IMIDIProxiedCCTarget
    virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) override {
        //Serial.println("DeviceBehaviourUltimateBase#sendControlChange");
        this->actualSendControlChange(number, value, channel);
    };
    virtual void sendPitchBend(int16_t bend, uint8_t channel) override {
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
        if (this->debug) Serial_printf(F("%s#sendProxiedControlChange(%i, %i, %i)\n"), this->get_label(), cc_number, value, channel);
        this->actualSendControlChange(cc_number, value, channel);
    }

    // use the underlying object to actually send a value
    virtual void actualSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {}
    virtual void actualSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {}
    virtual void actualSendControlChange(uint8_t number, uint8_t value, uint8_t channel) {}
    virtual void actualSendRealTime(uint8_t message) {}
    virtual void actualSendPitchBend(int16_t bend, uint8_t channel) {}

    // parameter handling shit
    ParameterList *parameters = new ParameterList();
    virtual ParameterList *get_parameters () {
        if (parameters==nullptr || parameters->size()==0) {
            this->initialise_parameters();   
        }
        return parameters;
    }
    virtual ParameterList *initialise_parameters() {
        //Serial.printf("behaviour_base#initialise_parameters for '%s'..\n", this->get_label());
        #if defined(ENABLE_ADVANCED_PITCHBEND) && defined(ENABLE_PARAMETERS)
            if (this->supports_advanced_pitch_bend() && this->transmits_midi_notes()) {
                this->ensure_advanced_pitch_bend_parameter();
            }
        #endif
        if (this->supports_note_limits()) {
            this->ensure_note_limit_parameters();
        }
        return parameters;
    }
    virtual bool has_parameters() {
        ParameterList *test_p = this->get_parameters();
        return test_p!=nullptr && test_p->size()>0;
    }
    virtual FloatParameter* getParameterForLabel(const char *label) {
        //Serial.printf(F("getParameterForLabel(%s) in behaviour %s..\n"), label, this->get_label());
        for (auto* p : *parameters) {
            //Serial.printf(F("Comparing '%s' to '%s'\n"), p->label, label);
            if (strcmp(p->label, label)==0) 
                return p;
        }
        Serial_printf(F("WARNING/ERROR in behaviour %s: didn't find a Parameter labelled %s\n"), this->get_label(), label);
        return nullptr;
    }

    virtual void reset_all_mappings() {
        for (auto* p : *this->parameters) {
            p->reset_mappings();
        }
    }

    // ---- saveloadlib-based settings (replaces old SaveableParameters system) ----
    // get_path_segment() returns the behaviour's label as its tree segment.
    // Called by sl_setup_all() via virtual dispatch after full construction.
    virtual const char* get_path_segment() const override { return const_cast<DeviceBehaviourUltimateBase*>(this)->get_label(); }

    virtual void setup_saveable_settings() override {
        ISaveableSettingHost::setup_saveable_settings();
        // path_segment is kept in sync so set_path_segment-based callers still work
        this->set_path_segment(this->get_label());

        if (this->transmits_midi_notes()) {
            register_setting(new LSaveableSetting<int8_t>("lowest_note",       "Note restriction", nullptr,
                [this](int8_t v) { this->setLowestNote(v); },
                [this]() -> int8_t { return this->getLowestNote(); }), SL_SCOPE_SCENE);
            register_setting(new LSaveableSetting<int8_t>("highest_note",      "Note restriction", nullptr,
                [this](int8_t v) { this->setHighestNote(v); },
                [this]() -> int8_t { return this->getHighestNote(); }), SL_SCOPE_SCENE);
            register_setting(new LSaveableSetting<NOTE_LIMIT_MODE>("lowest_note_mode",  "Note restriction", nullptr,
                [this](NOTE_LIMIT_MODE v) { this->setLowestNoteMode(v); },
                [this]() -> NOTE_LIMIT_MODE { return this->getLowestNoteMode(); }), SL_SCOPE_SCENE);
            register_setting(new LSaveableSetting<NOTE_LIMIT_MODE>("highest_note_mode", "Note restriction", nullptr,
                [this](NOTE_LIMIT_MODE v) { this->setHighestNoteMode(v); },
                [this]() -> NOTE_LIMIT_MODE { return this->getHighestNoteMode(); }), SL_SCOPE_SCENE);
        }

        // Register FloatParameters as children so their save/load is handled by the tree
        if (this->has_parameters()) {
            ParameterList *params = this->get_parameters();
            for (auto* p : *params) {
                register_child(p);
            }
        }
    }

    virtual void notify_project_changed(int project_number) {}

    #ifdef ENABLE_SCREEN
        MenuItemList *menuitems = nullptr;
        FLASHMEM
        virtual MenuItemList *make_menu_items();
        //FLASHMEM
        // make menu items for the underlying device type (ie usb, usbserial, serial, virtual)
        virtual MenuItemList *make_menu_items_device() {
            // dummy device menuitems
            return this->menuitems;
        }
        FLASHMEM
        virtual MenuItemList *create_saveable_parameters_recall_selector();

        // override this and set to false if you don't want to have a dedicated parameters page for this behaviour 
        // (eg because you want to put the parameters on the device's main menu page instead, as with MIDIBass)
        virtual bool show_dedicated_parameters_page() {
            return this->get_parameters()!=nullptr && this->get_parameters()->size()>0;
        }

        // Override and return false to prevent the behaviour's menu page from scrolling.
        virtual bool is_page_scrollable() { return true; }

        HarmonyStatus *output_harmony_status = nullptr; // store a pointer to this so we can update it from subclasses, eg MIDIBass
    #endif


    /*virtual void setForceOctave(int octave);
    virtual int getForceOctave() {
        //Serial.println("Beatstep_Behaviour#getForceOctave!"); Serial_flush();
        return this->force_octave;
    }*/

    NOTE_LIMIT_MODE lowest_note_mode  = NOTE_LIMIT_MODE::IGNORE;
    NOTE_LIMIT_MODE highest_note_mode = NOTE_LIMIT_MODE::IGNORE;
    int8_t lowest_note = 0;
    int8_t highest_note = MIDI_MAX_NOTE;
    int8_t effective_lowest_note = 0; // the actual lowest note being applied after taking into account things like force octave
    int8_t effective_highest_note = MIDI_MAX_NOTE; // the actual highest note being

    virtual void begin_note_limit_change() {
        begin_midi_matrix_target_transform_change(this);
    }
    virtual void end_note_limit_change() {
        end_midi_matrix_target_transform_change(this);
        #ifdef ENABLE_SCALES
            DeviceBehaviourUltimateBase::requantise_all_notes();
        #endif
    }

    virtual void setLowestNote(int8_t note) {
        // don't allow highest note to be set higher than highest note
        if (note > this->getHighestNote())
            note = this->getHighestNote();
        if (!is_valid_note(note)) 
            note = MIDI_MIN_NOTE;
        this->lowest_note = note;
    }
    virtual int8_t getLowestNote() {
        return this->lowest_note;
    }
    virtual NOTE_LIMIT_MODE getLowestNoteMode() {
        return lowest_note_mode;
    }
    // mode from NOTE_LIMIT_MODE enum (IGNORE or TRANSPOSE)
    virtual void setLowestNoteMode(NOTE_LIMIT_MODE mode) {
        if (this->lowest_note_mode == mode)
            return;
        this->begin_note_limit_change();
        this->lowest_note_mode = mode;
        this->end_note_limit_change();
    }

    virtual void setHighestNote(int8_t note) {
        // don't allow highest note to be set lower than lowest note
        if (note < this->getLowestNote())
            note = this->getLowestNote();
        if (!is_valid_note(note)) 
            note = MIDI_MAX_NOTE;
        this->highest_note = note;
    }
    virtual int8_t getHighestNote() {
        return this->highest_note;
    }
    virtual NOTE_LIMIT_MODE getHighestNoteMode() {
        return this->highest_note_mode;
    }
    // mode from NOTE_LIMIT_MODE enum (IGNORE or TRANSPOSE)
    virtual void setHighestNoteMode(NOTE_LIMIT_MODE mode) {
        if (this->highest_note_mode == mode)
            return;
        this->begin_note_limit_change();
        this->highest_note_mode = mode;
        this->end_note_limit_change();
    }

    virtual int8_t get_effective_lowest_note() {
        return min(effective_lowest_note, effective_highest_note);
    }
    virtual int8_t get_effective_highest_note() {
        return max(effective_highest_note, effective_lowest_note);
    }


    // ParameterInput support; for registering sources of modulation that this device produces (eg from a MIDI device)
    // for example, MIDI devices might have a Modwheel and a Pitch Bend wheel, and we want to be able to use those as modulation sources for other parameters in the system.
    // might also be other sources of modulation available from the device, eg a CV input, or a touch sensor, or a button, etc.
    // or perhaps virtual things like turning gates into modulation...
    GenericList<ParameterInput*> *parameter_inputs = nullptr;
    virtual GenericList<ParameterInput*> *get_modulation_sources() {
        if (parameter_inputs==nullptr) {
            parameter_inputs = new GenericList<ParameterInput*>();
        }
        return parameter_inputs;
    }

    // remap pitch if force octave is on, TODO: other tranposition modes
    // DONE?: two separate controls: one for lowest pitch/octave, one for highest pitch/octave
    // DONE?: two separate controls: out-of-bounds-lower rule, out-of-bounds-higher-rule
    //          options: ignore (just don't play the note in question); transpose (move the note into allowed range)
    // TODO: move quantisation calculation here...?
    // virtual int recalculate_pitch(byte note) {
    //     /*if (this->force_octave >= 0) {
    //         // send incoming notes from beatstep back out to neutron on serial3, but transposed down
    //         uint8_t note2 = note % 12;
    //         note2 += (force_octave*12); //24;
    //         if(this->debug) {
    //             Serial.printf("\trecalculate_pitch on %i => %i\n", note, note2);
    //         }
    //         note = note2;
    //         return note2;
    //     }*/
    //     if (!is_valid_note(note)) return NOTE_OFF;

    //     if (debug && (getLowestNote()>0 || getHighestNote()<MIDI_NUM_NOTES)) 
    //         Serial_printf("Incoming note is\t%i (%s), bounds are\t%i (%s) to\t%i (%s)\n", note, get_note_name_c(note), getLowestNote(), get_note_name_c(getLowestNote()), getHighestNote(), get_note_name_c(getHighestNote()));

    //     note = apply_note_limits(note, getLowestNoteMode(), getHighestNoteMode(), getLowestNote(), getHighestNote());
            
    //     if (note < getLowestNote()) {
    //         if (this->debug) Serial_printf("\tnote %i (%s)\tis lower than lowest note\t%i (%s)\n", note, get_note_name_c(note), getLowestNote(), get_note_name_c(getLowestNote()));
    //         if (getLowestNoteMode()==NOTE_LIMIT_MODE::IGNORE) {
    //             if (this->debug) Serial.println("\tignoring!");
    //             note = NOTE_OFF;
    //         } else if (getLowestNoteMode()==NOTE_LIMIT_MODE::TRANSPOSE) {
    //             //int8_t octave = note / 12;
    //             /*int8_t lowest_octave = getLowestNote() / 12;
    //             int8_t chromatic_pitch = note % 12;
    //             int8_t note2 = note;
    //             note = (lowest_octave*12) + chromatic_pitch;*/
    //             int8_t note2 = note;
    //             while (is_valid_note(note) && note < getLowestNote()) {
    //                 note += 12;
    //             }
    //             if (this->debug) Serial_printf("\ttransposed from %i (%s) up to\t%i (%s)\n", note2, get_note_name_c(note2), note, get_note_name_c(note));
    //         }
    //     } else if (note > getHighestNote()) {
    //         if (this->debug) Serial_printf("\tnote\t%i (%s)\tis higher than highest note\t%i (%s)\n", note, get_note_name_c(note), getHighestNote(), get_note_name_c(getHighestNote()));
    //         if (getHighestNoteMode()==NOTE_LIMIT_MODE::IGNORE) {
    //             if (this->debug) Serial_println("\tignoring!");
    //             note = NOTE_OFF;
    //         } else if (getHighestNoteMode()==NOTE_LIMIT_MODE::TRANSPOSE) {
    //             //int8_t octave = note / 12;
    //             /*int8_t highest_octave = getHighestNote() / 12;
    //             int8_t chromatic_pitch = note % 12;
    //             int8_t note2 = note;
    //             note = ((highest_octave-1)*12) + chromatic_pitch;*/
    //             int8_t note2 = note;
    //             while (is_valid_note(note) && note > getHighestNote()) {
    //                 note -= 12;
    //             }
    //             if (this->debug) Serial_printf("\ttransposed from %i (%s) down to\t%i (%s)\n", note2, get_note_name_c(note2), note, get_note_name_c(note));
    //             //Serial.printf("\t\t(highest_octave =\t%i, chromatic_pitch =\t%i (%s))\n", highest_octave, chromatic_pitch, note_names[chromatic_pitch]);
    //         }
    //     }
    //     if (!is_valid_note(note) || note < getLowestNote() || note > getHighestNote()) {
    //         if (this->debug) Serial_printf("\t%i (%s) isn't valid or out of bounds\n", note, get_note_name_c(note));
    //         return NOTE_OFF;
    //     }
    //     return note;
    // }

    
};

class BehaviourNoteLimitParameter : public ProxyNoteParameter<int8_t> {
    DeviceBehaviourUltimateBase *behaviour;

public:
    BehaviourNoteLimitParameter(const char *label, DeviceBehaviourUltimateBase *behaviour,
                                int8_t *source, int8_t *target)
        : ProxyNoteParameter<int8_t>(label, source, target, MIDI_MIN_NOTE, MIDI_MAX_NOTE),
          behaviour(behaviour) {}

    virtual void setTargetValueFromData(int8_t value, bool force = false) override {
        if (*this->target == value) {
            ProxyNoteParameter<int8_t>::setTargetValueFromData(value, force);
            return;
        }
        behaviour->begin_note_limit_change();
        ProxyNoteParameter<int8_t>::setTargetValueFromData(value, force);
        behaviour->end_note_limit_change();
    }

    virtual void updateValueFromData(int8_t value) override {
        if (*this->source == value && *this->target == value)
            return;
        behaviour->begin_note_limit_change();
        ProxyNoteParameter<int8_t>::updateValueFromData(value);
        behaviour->end_note_limit_change();
    }

    virtual void updateValueFromNormal(float value) override {
        this->updateValueFromData(this->normalToData(value));
    }
};



class VirtualBehaviourBase : public virtual DeviceBehaviourUltimateBase {
    public:
    virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) override {
        if (number==MIDI_CC_ALL_NOTES_OFF) {
            this->killCurrentNote();
        }
    }

    virtual int getType() override {
        return BehaviourType::virt;
    }
};