#pragma once

#include <Arduino.h>
#include "Config.h"
#include "saveload_settings.h"  // ISaveableSettingHost, SHStorage, LSaveableSetting, sl_scope_t etc.

#ifdef ENABLE_SD
  #include "SD.h"
#endif


void setup_saveloadlib();
namespace storage {

  #define FILEPATH_PROJECT_FOLDER_FORMAT    "project%i"
  #define FILEPATH_PATTERN_FORMAT           "project%i/sequences/sequence%i.txt"
  #define FILEPATH_SECTION_FORMAT           "project%i/sections/section%i.txt"
  #define FILEPATH_PLAYLIST_FORMAT          "project%i/playlist.txt"
  #define FILEPATH_PROJECT_SETTINGS_FORMAT  "project%i/project.txt"
  #define FILEPATH_LOOP_FORMAT              "project%i/loops/loop%i.txt"
  #define FILEPATH_CALIBRATION_FORMAT       "calibration_voltage_source_%i.txt"

  #define MAX_FILEPATH 255

  //#include <SD.h>
  //#include <SdFat.h>

  //#define NUM_STATES 8    // number of states that we can save..?  possibly should be per-project too

  /*#define NUM_CLOCKS    4
  #define NUM_SEQUENCES 4
  #define NUM_STEPS     8*/

  #define SAVE_ID_BYTE_V0 0xD0
  #define SAVE_ID_BYTE_V1 0xD1

  // ---------------------------------------------------------------------------
  // SequenceRowSetting — custom SaveableSetting that serialises / parses one row
  // of step data as nibble-encoded hex.  Holds a pointer to the live size_steps
  // field so it reads the correct length even after loading a different step count.
  // ---------------------------------------------------------------------------
  struct SequenceRowSetting : public SaveableSettingBase {
    uint8_t*       row;
    const uint8_t* steps_ptr;   // points to the live savestate::size_steps field

    SequenceRowSetting(const char* lbl, const char* cat, uint8_t* row_ptr, const uint8_t* num_steps_ptr)
        : row(row_ptr), steps_ptr(num_steps_ptr) {
      set_label(lbl);
      set_category(cat ? cat : "");
    }

    uint8_t get_steps() const { return steps_ptr ? *steps_ptr : (uint8_t)NUM_STEPS; }

    const char* get_line() override {
      uint8_t n   = get_steps();
      int     pos = snprintf(linebuf, SL_MAX_LINE, "%s=", label);
      for (uint8_t i = 0; i < n && pos < SL_MAX_LINE - 1; i++)
        linebuf[pos++] = "0123456789abcdef"[row[i] & 0xF];
      linebuf[pos] = '\0';
      return linebuf;
    }

    bool parse_key_value(const char* key, const char* value) override {
      if (strcmp(key, label) != 0) return false;
      uint8_t n = get_steps();
      for (uint8_t i = 0; i < n && value[i]; i++) {
        const char nibble[2] = { value[i], '\0' };
        row[i] = (uint8_t)strtol(nibble, nullptr, 16);
      }
      return true;
    }

    virtual size_t heap_size() const override { return sizeof(SequenceRowSetting); }
  };

  // ---------------------------------------------------------------------------
  // savestate — the clock + sequence data for one scene.
  //
  // Inherits SHStorage so all fields are registered as saveloadlib settings under
  // the path segment "scene" with SL_SCOPE_PATTERN mask.  setup_saveable_settings()
  // must be called once (done by setup_sd()) before any save/load.
  //
  // File format produced: "scene~key=value" lines (new format).
  // Old flat "key=value" lines are still accepted on load for backward compatibility.
  //
  // Quick scene switching (future): pre-load scenes as in-RAM LinkedList<String>
  // or shadow savestate instances at project-open time; apply_scene() then replays
  // them through load_line() with no SD I/O required.
  //
  // Note: SHStorage adds virtual methods, making savestate non-trivially-destructible.
  //       The legacy Arduino EEPROM path at the bottom of storage.cpp is incompatible.
  // ---------------------------------------------------------------------------
  struct savestate : public SHStorage<0, 4 + NUM_CLOCKS + NUM_CLOCKS + NUM_SEQUENCES> {
    uint8_t id             = SAVE_ID_BYTE_V1;
    uint8_t size_clocks    = NUM_CLOCKS;
    uint8_t size_sequences = NUM_SEQUENCES;
    uint8_t size_steps     = NUM_STEPS;
    uint8_t clock_multiplier[NUM_CLOCKS] = {
      //7, 6, 5, 4, 3, 2, 1, 0
      5, 4, 3, 2, 1, 3, 4, 7
    };
    uint8_t sequence_data[NUM_SEQUENCES][NUM_STEPS];
    uint8_t clock_delay[NUM_CLOCKS] = { 0, 0, 0, 0,
      #if NUM_CLOCKS>4
        0,
      #endif
      #if NUM_CLOCKS>5
        0,
      #endif
      #if NUM_CLOCKS>6
        0,
      #endif
      #if NUM_CLOCKS>7
        0
      #endif
    };

    // Registers all fields as SL_SCOPE_PATTERN settings.  Called by setup_sd().
    virtual void setup_saveable_settings() override;
  };

  char *get_pattern_filename(int project_number, int pattern_number);
  char *get_project_settings_filename(int project_number);

  bool save_pattern(int project_number, uint8_t preset_number, savestate *input, bool debug = false);
  bool load_pattern(int project_number, uint8_t preset_number, savestate *input, bool debug = false);
  /*void load_state_update();
  void load_state_start(uint8_t preset_number, savestate *input);*/
  bool load_pattern_parse_line(String line, savestate *output, bool debug = false);
  FLASHMEM void setup_sd();

  FLASHMEM void log_crashreport();
  FLASHMEM void dump_crashreport_log();
  FLASHMEM void clear_crashreport_log();
  FLASHMEM void force_crash();

  void make_project_folders(int project_number);

  bool copy_file(char *src, char *dst);

  extern savestate current_state;
}

class SettingsRoot : public SHStorage<16,16> {
    public:
        SettingsRoot() {
          this->set_path_segment("root");
        }

        virtual void setup_saveable_settings() override {
            //this->register_setting(new LSaveableSetting<uint8_t>("test_setting", "Root", &test_setting));

            register_child(project);

            //register_child(midi_matrix_manager);

            register_child(&storage::current_state);
        }

        uint8_t test_setting = 42;
};


extern SettingsRoot *settings_root;

