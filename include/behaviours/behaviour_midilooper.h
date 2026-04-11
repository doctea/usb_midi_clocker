#pragma once

#include "Config.h"

#ifdef ENABLE_LOOPER

#include "behaviours/behaviour_base.h"
#include "midi/midi_looper.h"

class VirtualBehaviour_MidiLooper : public VirtualBehaviourBase {
    public:

    MIDITrack *track = nullptr;

    VirtualBehaviour_MidiLooper() : VirtualBehaviourBase() {
        // track assigned externally (to &midi_loop_track) after construction
    }

    virtual const char *get_label() override {
        return (const char *)"MIDI Looper";
    }

    virtual int getType() override {
        return BehaviourType::virt;
    }

    // Called every tick from behaviour_manager->do_ticks()
    virtual void on_tick(uint32_t ticks) override {
        track->process_tick(ticks);
    }

    // Source: outgoing notes go from track through midi_matrix_manager
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        midi_matrix_manager->processNoteOn(this->source_id, note, velocity, channel);
    }
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        midi_matrix_manager->processNoteOff(this->source_id, note, velocity, channel);
    }

    // --- Transport wrappers ---
    void start_playing()       { track->start_playing(); }
    void stop_playing()        { track->stop_playing(); }
    void toggle_playing()      { track->toggle_playing(); }

    void start_recording()     { track->start_recording(); }
    void stop_recording()      { track->stop_recording(); }
    void toggle_recording()    { track->toggle_recording(); }

    void start_overwriting()   { track->start_overwriting(); }
    void stop_overwriting()    { track->stop_overwriting(); }
    void toggle_overwriting()  { track->toggle_overwriting(); }

    void clear_all()           { track->clear_all(); }

    bool isPlaying()           { return track->isPlaying(); }
    bool isRecording()         { return track->isRecording(); }
    bool isOverwriting()       { return track->isOverwriting(); }

    // --- Save/load (delegates to MIDITrack bespoke SD logic) ---
    bool load_loop(int project_number, int recording_number) {
        return track->load_loop(project_number, recording_number);
    }
    bool save_loop(int project_number, int recording_number) {
        return track->save_loop(project_number, recording_number);
    }
    int count_events()         { return track->count_events(); }

    #ifdef ENABLE_SCREEN
        virtual LinkedList<MenuItem*> *make_menu_items() override {
            return track->make_menu_items();
        }
    #endif
};

extern VirtualBehaviour_MidiLooper *behaviour_midilooper;

#endif // ENABLE_LOOPER
