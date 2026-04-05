#pragma once

#ifdef ENABLE_KAWAI_R50
// behaviour for a Kawai R50 drum machine; responds to midi clock, can send midi notes
// to trigger drum sounds.  we want to be able to map incoming midi notes to drum sounds
// on the R50, and also be able to send midi notes from our sequencer to trigger sounds on the R50
// its a midi serial device

#include "behaviours/behaviour_base_serial.h"
#include "behaviours/behaviour_clocked.h"

#include "Drums.h"


/*
// todo: these are the ones we're realy interested in

            this->addDrumNode("Kick",          GM_NOTE_ELECTRIC_BASS_DRUM);
            this->addDrumNode("Stick",         GM_NOTE_SIDE_STICK);
            this->addDrumNode("Clap",          GM_NOTE_HAND_CLAP);
            this->addDrumNode("Snare",         GM_NOTE_ELECTRIC_SNARE);
            this->addDrumNode("Cymbal 1",      GM_NOTE_CRASH_CYMBAL_1);
            this->addDrumNode("Tamb",          GM_NOTE_TAMBOURINE);
            this->addDrumNode("HiTom",         GM_NOTE_HIGH_TOM);
            this->addDrumNode("LoTom",         GM_NOTE_LOW_TOM);
            this->addDrumNode("PHH",           GM_NOTE_PEDAL_HI_HAT);
            this->addDrumNode("OHH",           GM_NOTE_OPEN_HI_HAT);
            this->addDrumNode("CHH",           GM_NOTE_CLOSED_HI_HAT);
            #ifdef ENABLE_ENVELOPES
                this->addNode(new EnvelopeOutput("Cymbal 2",    GM_NOTE_CRASH_CYMBAL_2, MUSO_CC_CV_1, MUSO_CV_CHANNEL, output_target));
                this->addNode(new EnvelopeOutput("Splash",      GM_NOTE_SPLASH_CYMBAL,  MUSO_CC_CV_2, MUSO_CV_CHANNEL, output_target));
                this->addNode(new EnvelopeOutput("Vibra",       GM_NOTE_VIBRA_SLAP,     MUSO_CC_CV_3, MUSO_CV_CHANNEL, output_target));
                this->addNode(new EnvelopeOutput("Ride Bell",   GM_NOTE_RIDE_BELL,      MUSO_CC_CV_4, MUSO_CV_CHANNEL, output_target));
                this->addNode(new EnvelopeOutput("Ride Cymbal", GM_NOTE_RIDE_CYMBAL_1,  MUSO_CC_CV_5, MUSO_CV_CHANNEL, output_target));
*/

class DeviceBehaviour_KawaiR50 : virtual public DeviceBehaviourSerialBase, virtual public DividedClockedBehaviour {

    class DrumMapEntry {
        public:
            char name[32] = "Unknown";

            bool enabled = true;

            uint8_t incoming_midi_note; // midi note we receive that we want to map
            uint8_t *possible_midi_notes = nullptr; // array of possible kawai drum notes that this maps to
            uint8_t num_possible_notes = 0;
            uint8_t current_note_index = 0; // which of the possible notes are we currently using

            DrumMapEntry(
                const char *name,
                uint8_t incoming_midi_note, 
                uint8_t *possible_midi_notes, 
                uint8_t num_possible_notes
            ) {
                strncpy(this->name, name, 31);
                this->incoming_midi_note = incoming_midi_note;
                this->possible_midi_notes = possible_midi_notes;
                this->num_possible_notes = num_possible_notes;
                this->current_note_index = 0;
            }

            int8_t get_mapped_note() {
                if (!this->enabled) return NOTE_OFF;
                if (this->num_possible_notes == 0) return NOTE_OFF;

                //Serial.printf("DrumMapEntry %s: mapping incoming note %i...", this->name, this->incoming_midi_note); Serial.flush();
                uint8_t note = this->possible_midi_notes[this->current_note_index];
                //Serial.printf(" mapped to note %i\n", note); Serial.flush();
                // advance to next note in list TODO: make this configurable (eg random, round robin, etc)
                //this->current_note_index = (this->current_note_index + 1) % this->num_possible_notes;
                return note;
            }
    };

    class DrumMapper {
        public:
            //Hashtable<uint8_t, DrumMapEntry*> drum_map;
            DrumMapEntry *drum_map_storage[MIDI_NUM_NOTES];

            DrumMapper() {
                for (int i = 0 ; i < MIDI_NUM_NOTES ; i++) {
                    drum_map_storage[i] = nullptr;
                }
            }

            DrumMapEntry *get_mapped_entry_for_incoming_note(uint8_t incoming_note) {
                //Serial.printf("DrumMapper: get_mapped_entry_for_incoming_note for incoming note %i\n", incoming_note); Serial.flush();
                if (!is_valid_note(incoming_note)) {
                    return nullptr;
                }
                if (drum_map_storage[incoming_note] == nullptr) {
                    // Serial.printf("DrumMapper: no mapping for incoming note %i\n", incoming_note); Serial.flush();
                    return nullptr;
                }
                /*Serial.flush();
                Serial.printf("DrumMapper: get_mapped_entry_for_incoming_note found entry for incoming note %i\n", incoming_note);
                Serial.flush();*/
                return drum_map_storage[incoming_note];
            }

            int8_t get_mapped_note_for_incoming_note(uint8_t incoming_note) {
                DrumMapEntry *entry = get_mapped_entry_for_incoming_note(incoming_note);
                //Serial.printf("DrumMapper: found mapping for incoming note %i at %p\n", incoming_note, entry); Serial.flush();

                if (entry==nullptr) {
                    //Serial.printf("DrumMapper: no mapping entry for incoming note %i, returning NOTE_OFF\n", incoming_note);                   Serial.flush();
                    return NOTE_OFF;
                }

                //Serial.printf("DrumMapper: found mapping for incoming note %i at %p\n", incoming_note, entry);                Serial.flush();

                return entry->get_mapped_note();
            }

            void add_drum_map(int8_t note, DrumMapEntry *entry) {
                if (!is_valid_note(note)) {
                    return;
                }
                drum_map_storage[note] = entry;
            }
    };

    DrumMapper drum_mapper;

    // cribbed from https://kawaius.com/wp-content/uploads/2019/04/Kawai-R-50-Digital-Drum-Machine-Manual.pdf
    char *kawair50_note_names[93] = {
        (char *)"---",    // 0
        (char *)"---",    // 1
        (char *)"---",    // 2
        (char *)"---",    // 3
        (char *)"---",    // 4
        (char *)"---",    // 5
        (char *)"---",    // 6
        (char *)"---",    // 7
        (char *)"---",    // 8
        (char *)"---",    // 9
        (char *)"---",    // 10
        (char *)"---",    // 11
        (char *)"---",    // 12
        (char *)"---",    // 13
        (char *)"---",    // 14
        (char *)"---",    // 15
        (char *)"---",    // 16
        (char *)"---",    // 17
        (char *)"---",    // 18
        (char *)"---",    // 19
        (char *)"---",    // 20

        (char *)"BD2",   // 21
        (char *)"BD3",   // 22
        (char *)"BD1",   // 23
        (char *)"SD1",   // 24
        (char *)"HHCL",  // 25
        (char *)"SD2",   // 26
        (char *)"HHOP",  // 27
        (char *)"SD3",   // 28
        (char *)"TOMH",    // 29
        (char *)"CRS1",    // 30
        (char *)"TOMM",    // 31
        (char *)"CLAP",    // 32
        (char *)"TOML",    // 33
        (char *)"COWB",    // 34
        (char *)"TMBL",    // 35
        (char *)"BD2",     // 36
        (char *)"BD3",     // 37
        (char *)"BD1",     // 38
        (char *)"SD3",     // 39
        (char *)"SD1",     // 40
        (char *)"SD2",     // 41
        (char *)"CLAPS",   // 42
        (char *)"TOMH",  // 43
        (char *)"HHCL",    // 44
        (char *)"TOMH",  // 45
        (char *)"HHOP",    // 46
        (char *)"TOMM",    // 47
        (char *)"TOMM",    // 48
        (char *)"CRS1",    // 49
        (char *)"TOML",    // 50
        (char *)"CRS2",    // 51
        (char *)"TOML",    // 52
        (char *)"CONG",    // 53
        (char *)"RID1",    // 54
        (char *)"CONG",    // 55
        (char *)"RID2",    // 56
        (char *)"CONG",    // 57
        (char *)"CHNA",    // 58
        (char *)"SHAK",    // 59
        (char *)"SHAK",    // 60
        (char *)"AGOG",    // 61
        (char *)"SHAK",    // 62
        (char *)"AGOGO",   // 63
        (char *)"COWB",    // 64
        (char *)"COWB",    // 65
        (char *)"TMBL",    // 66
        (char *)"CLAP",    // 67
        (char *)"TMBL",    // 68
        (char *)"CLAP",    // 69
        (char *)"TMBL",    // 70
        (char *)"CLAP",    // 71
        (char *)"TAMB",    // 72
        (char *)"CRS1",    // 73
        (char *)"TAMB",    // 74
        (char *)"CRS1",    // 75
        (char *)"CLVS1",   // 76
        (char *)"CLVS2",   // 77
        (char *)"CLVS3",   // 78
        (char *)"CLVS4",   // 79
        (char *)"CLVS5",   // 80
        (char *)"CLVS6",   // 81
        (char *)"CLVS7",   // 82
        (char *)"CLVS8",   // 83
        (char *)"CLVS9",   // 84
        (char *)"CLVS10",  // 85
        (char *)"CLVS11",  // 86
        (char *)"CLVS12",  // 87
        (char *)"CLVS13",  // 88
        (char *)"CLVS14",  // 89
        (char *)"CLVS15",  // 90
        (char *)"CLVS16",  // 91
        (char *)"CLVS17",  // 92
    };

    public:
    DeviceBehaviour_KawaiR50() : DeviceBehaviourSerialBase() {
        // setup drum mapping

        // kick
        uint8_t *kick_notes = new uint8_t[6]{21, 22, 23, 36, 37, 38};
        drum_mapper.add_drum_map(
            GM_NOTE_ELECTRIC_BASS_DRUM, 
            new DrumMapEntry("Kick", GM_NOTE_ELECTRIC_BASS_DRUM, kick_notes, sizeof(kick_notes))
        );
        drum_mapper.add_drum_map(
            GM_NOTE_ACOUSTIC_BASS_DRUM, 
            drum_mapper.drum_map_storage[GM_NOTE_ELECTRIC_BASS_DRUM] // share same mapping as electric bass drum
        );

        // snare
        uint8_t *snare_notes = new uint8_t[6]{24, 26, 28, 39, 40, 41};
        drum_mapper.add_drum_map(
            GM_NOTE_ACOUSTIC_SNARE, 
            new DrumMapEntry("Snare", GM_NOTE_ACOUSTIC_SNARE, snare_notes, sizeof(snare_notes))
        );
        drum_mapper.add_drum_map(
            GM_NOTE_ELECTRIC_SNARE, 
            drum_mapper.drum_map_storage[GM_NOTE_ACOUSTIC_SNARE] // share same mapping as acoustic snare
        );

        // hand clap
        uint8_t *clap_notes = new uint8_t[6]{32, 42, 67, 69, 71};
        drum_mapper.add_drum_map(
            GM_NOTE_HAND_CLAP, 
            new DrumMapEntry("Hand Clap", GM_NOTE_HAND_CLAP, clap_notes, sizeof(clap_notes))
        );

        // closed hi-hat
        uint8_t *chh_notes = new uint8_t[2]{25, 44};
        drum_mapper.add_drum_map(
            GM_NOTE_CLOSED_HI_HAT, 
            new DrumMapEntry("Closed Hi-Hat", GM_NOTE_CLOSED_HI_HAT, chh_notes, sizeof(chh_notes))
        );

        // open hi-hat
        uint8_t *ohh_notes = new uint8_t[2]{27,46};
        drum_mapper.add_drum_map(
            GM_NOTE_OPEN_HI_HAT, 
            new DrumMapEntry("Open Hi-Hat", GM_NOTE_OPEN_HI_HAT, ohh_notes, sizeof(ohh_notes))
        );

        // low tom
        uint8_t *lowtom_notes = new uint8_t[3]{33, 50, 52};
        drum_mapper.add_drum_map(
            GM_NOTE_LOW_TOM, 
            new DrumMapEntry("Low Tom", GM_NOTE_LOW_TOM, lowtom_notes, sizeof(lowtom_notes))
        );

        // high tom
        uint8_t *hitom_notes = new uint8_t[3]{29, 43, 45};
        drum_mapper.add_drum_map(
            GM_NOTE_HI_MID_TOM, 
            new DrumMapEntry("High Tom", GM_NOTE_HI_MID_TOM, hitom_notes, sizeof(hitom_notes))
        );

        // crash cymbal 1
        uint8_t *crash1_notes = new uint8_t[4]{30, 49, 73, 75};
        drum_mapper.add_drum_map(
            GM_NOTE_CRASH_CYMBAL_1, 
            new DrumMapEntry("Crash Cymbal 1", GM_NOTE_CRASH_CYMBAL_1, crash1_notes, sizeof(crash1_notes))
        );

        // ride cymbal 1
        uint8_t *ride1_notes = new uint8_t[2]{54, 56};
        drum_mapper.add_drum_map(
            GM_NOTE_RIDE_CYMBAL_1, 
            new DrumMapEntry("Ride Cymbal 1", GM_NOTE_RIDE_CYMBAL_1, ride1_notes, sizeof(ride1_notes))
        );

        // tamb
        uint8_t *tamb_notes = new uint8_t[2]{72, 74};
        drum_mapper.add_drum_map(
            GM_NOTE_TAMBOURINE, 
            new DrumMapEntry("Tamb", GM_NOTE_TAMBOURINE, tamb_notes, sizeof(tamb_notes))
        );

        // bell
        uint8_t *bell_notes = new uint8_t[3]{34, 64, 65};
        drum_mapper.add_drum_map(
            GM_NOTE_RIDE_BELL, 
            new DrumMapEntry("Bell", GM_NOTE_RIDE_BELL, bell_notes, sizeof(bell_notes))
        );

    }

    virtual const char *get_label() override {
        return "Kawai R50";
    }

    virtual bool receives_midi_notes() override {
      return false;
    }
    virtual bool transmits_midi_notes() override {
      return true;
    }
    virtual bool transmits_midi_clock() override {
      return true;
    }

    int8_t get_kawai_note_for_gm_drum_note(int8_t gm_note) {
        // use drummapper to map incoming GM drum note to Kawai R50 note
        return drum_mapper.get_mapped_note_for_incoming_note(gm_note);
    }

    // override SendNoteOn to translate GM_DRUM notes to Kawai R50 notes
    // and send via the DeviceBehaviourSerialBase output device
    virtual void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // translate GM_DRUM note to Kawai R50 note
        if (note >= 21 && note <= 92) {
            // Kawai R50 uses a different note mapping
            //Serial.printf("KawaiR50: sendNoteOn GM note %i vel %i ch %i\n", note, velocity, channel); Serial.flush();
            note = get_kawai_note_for_gm_drum_note(note);
            if (!is_valid_note(note)) {
                //Serial.printf("KawaiR50: mapped to invalid note %i, not sending\n", note); Serial.flush();
                return;
            }
            //Serial.printf("KawaiR50: mapped to Kawai note %i (%s)\n", note, kawair50_note_names[note]); Serial.flush();
            DeviceBehaviourSerialBase::actualSendNoteOn(note, velocity, GM_CHANNEL_DRUMS);
        }
    }
    virtual void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override {
        // translate GM_DRUM note to Kawai R50 note
        if (note >= 21 && note <= 92) {
            // Kawai R50 uses a different note mapping
            // we'll just pass through the note for now
            note = get_kawai_note_for_gm_drum_note(note);
            DeviceBehaviourSerialBase::actualSendNoteOff(note, velocity, GM_CHANNEL_DRUMS);
        }
    }

    virtual int getType() override {
      return BehaviourType::serial;
    }

    #ifdef ENABLE_SCREEN
        //FLASHMEM 
        virtual LinkedList<MenuItem*> *make_menu_items() override;
    #endif

    virtual void setup_saveable_settings() override {
        DeviceBehaviourUltimateBase::setup_saveable_settings();
        DividedClockedBehaviour::setup_saveable_settings();

        for (uint8_t i = 0; i < MIDI_NUM_NOTES ; i++) {
            DrumMapEntry *entry = drum_mapper.drum_map_storage[i];
            if (entry != nullptr) {
                register_setting(new LSaveableSetting<bool>(
                    (String("kawair50_drummap_") + String(entry->incoming_midi_note) + String("_enabled")).c_str(),
                    "KawaiR50",
                    &entry->enabled
                ));
            }
        }
    }

};

extern DeviceBehaviour_KawaiR50 *behaviour_kawair50;

#endif