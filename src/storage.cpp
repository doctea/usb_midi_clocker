#include <Arduino.h>
#include "settings_root.h"  // defines SettingsRoot; also pulls in storage.h, project.h, behaviour_manager.h
#include "storage.h"        // idempotent re-include (pragma once)
#include "project.h"        // idempotent
#include "behaviours/behaviour_manager.h"  // idempotent

#include "mymenu/menu_fileviewers.h"

#include <util/atomic.h>

/*
(smaller files are fragments of previous storage approaches)

Files in project0:
sequences/ (0 bytes)
        sequence0.txt (65897 bytes)
        sequence1.txt (65898 bytes)
        sequence2.txt (983 bytes)
        sequence5.txt (984 bytes)
        sequence7.txt (3338 bytes)
        sequence3.txt (983 bytes)
        sequence4.txt (1658 bytes)
        sequence6.txt (3496 bytes)
loops/ (0 bytes)
        loop1.txt (7356 bytes)
        loop0.mid (102 bytes)
        loop2.txt (797 bytes)
        loop4.txt (1901 bytes)
        loop6.txt (1483 bytes)
        loop7.txt (1651 bytes)
        loop5.txt (958 bytes)
        loop3.txt (2148 bytes)
        loop1.mid (262 bytes)
project.txt (18872 bytes)
sections/ (0 bytes)
        section0.txt (467 bytes)
        section2.txt (467 bytes)
        section3.txt (467 bytes)
        section1.txt (467 bytes)
playlist.txt (242 bytes)
Total storage used in project 0: 180877 bytes
*/

#include <vlpp_arena.h>

// // Buffer in EXTMEM (one slow PSRAM malloc); metadata in fast DTCM
// EXTMEM static char vlpp_pool[65536];           // tune this size
// static VLPP_ArenaBase vlpp_arena_obj(vlpp_pool, sizeof(vlpp_pool));


#ifndef READ_FILE_BUF_SIZE
    #define READ_FILE_BUF_SIZE (131072) // should give us a buffer size of 128k for files
#endif

#ifdef ENABLE_SD
  #include "SD.h"
#endif

//#include "SdFat.h"
#include <SPI.h>
namespace storage {

  savestate current_state; 

  /**
   * hex2int - from https://stackoverflow.com/questions/10156409/convert-hex-string-char-to-int
   * take a hex string and convert it to a 32bit number (max 8 hex digits)
   */
  uint32_t hex2int(char *hex) {
      uint32_t val = 0;
      while (*hex) {
          // get current character then increment
          uint8_t byte = *hex++; 
          // transform hex character to the 4bit equivalent number, using the ascii table indexes
          if (byte >= '0' && byte <= '9') byte = byte - '0';
          else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
          else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;    
          // shift 4 to make space for new digit, and add the 4 bits of the new digit 
          val = (val << 4) | (byte & 0xF);
      }
      return val;
  }

  void log_crashreport() {
    #ifdef ENABLE_SD
      if (SD.mediaPresent()) {
        File f = SD.open("crashreport.log", O_WRONLY | O_CREAT);
        if (f) {
          f.println("-----");
          f.print(CrashReport);
          f.println("-----");
          f.close();
        }
      }
    #endif
  }

  void dump_crashreport_log() {
    #ifdef ENABLE_SD
      File f = SD.open("crashreport.log", FILE_READ);
      f.setTimeout(0);
      if (f) {
        while (String line = f.readStringUntil('\n')) {
          f.println(line);
          Serial.print(line);
          Serial.print("\r\n");
        }
      } else {
        Serial.println("dump_crashreport_log: crashreport.log not found");
        messages_log_add("crashreport.log not found");
      }
    #else
      Serial.println("dump_crashreport_log: SD not enabled");
      messages_log_add("SD not enabled");
    #endif
  }

  void clear_crashreport_log() {
    #ifdef ENABLE_SD
      if (!SD.mediaPresent()) {
        messages_log_add("No media present");
        return;
      } else {
        if (SD.remove("crashreport.log")) {
          messages_log_add("Deleted crashreport.log");
        } else {
          messages_log_add("Failed to remove crashreport.log");
        }
      }
    #else
      Serial.println("clear_crashreport_log: SD not enabled");
      messages_log_add("SD not enabled");
    #endif
  }

  // force a crash, for testing CrashReport purposes
  void force_crash() {
    messages_log_add("Forcing crash!");
    Serial_println("Forcing crash!"); Serial_flush();
    *(volatile uint32_t *)0x30000000 = 0;
  }

  #ifdef ENABLE_SD
  const int chipSelect = BUILTIN_SDCARD;
  #endif

  FLASHMEM
  void make_project_folders(int project_number) {
    #ifdef ENABLE_SD
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      char path[MAX_FILEPATH];
      snprintf(path, MAX_FILEPATH, "project%i", project_number);
      //Serial.printf(F("Checking exists %s.."), path);
      if (!SD.exists(path)) {
        //Serial.println(F("making!\n"));
        SD.mkdir(path);
      } else {
        //Serial.println(F("exists!\n"));
      }
      
      snprintf(path, MAX_FILEPATH, "project%i/sequences", project_number);
      //Serial.printf("Checking exists %s..", path);
      if (!SD.exists(path)) {
        //Serial.println(F("making!\n"));
        SD.mkdir(path);
      } else {
        //Serial.println(F("exists!\n"));
      }

      snprintf(path, MAX_FILEPATH, "project%i/loops", project_number);
      //Serial.printf("Checking exists %s..", path);
      if (!SD.exists(path)) {
        //Serial.println(F("making!\n"));
        SD.mkdir(path);
      } else {
        //Serial.println(F("exists!\n"));
      }

      snprintf(path, MAX_FILEPATH, "project%i/sections", project_number);
      //Serial.printf("Checking exists %s..", path);
      if (!SD.exists(path)) {
        //Serial.println(F("making!\n"));
        SD.mkdir(path);
      } else {
        //Serial.println(F("exists!\n"));
      }
    }
    #endif
  }

  // ---------------------------------------------------------------------------
  // savestate::setup_saveable_settings()
  // Registers all scene fields under path segment "scene" with SL_SCOPE_SCENE
  // mask.  Called once by setup_sd() on current_state.
  // ---------------------------------------------------------------------------
  void savestate::setup_saveable_settings() {
    set_path_segment("scene");

    //vlpp_set_arena(&vlpp_arena_obj);   // call BEFORE sl_setup_all() / menus / etc.

    // Scene metadata
    register_setting(new LSaveableSetting<uint8_t>("scene_id",    "Scene", &id),            SL_SCOPE_SCENE, false);
    register_setting(new LSaveableSetting<uint8_t>("size_clocks", "Scene", &size_clocks),   SL_SCOPE_SCENE, false);
    register_setting(new LSaveableSetting<uint8_t>("size_seqs",   "Scene", &size_sequences), SL_SCOPE_SCENE, false);
    register_setting(new LSaveableSetting<uint8_t>("size_steps",  "Scene", &size_steps),    SL_SCOPE_SCENE, false);

    // Clock multipliers — packed as 2-hex-chars-per-byte: "clock_mult=07060504..."
    register_setting(new PackedByteArraySetting("clock_mult",  "Clock", clock_multiplier, NUM_CLOCKS), SL_SCOPE_SCENE, false);
    // Clock delays — same format
    register_setting(new PackedByteArraySetting("clock_delay", "Clock", clock_delay,      NUM_CLOCKS), SL_SCOPE_SCENE, false);

    // Sequence rows — one nibble-hex setting per row, length from live size_steps pointer
    char lbl[20];
    for (uint8_t i = 0; i < NUM_SEQUENCES; i++) {
      snprintf(lbl, sizeof(lbl), "seq_%u", i);
      register_setting(new SequenceRowSetting(lbl, "Sequence", sequence_data[i], &size_steps), SL_SCOPE_SCENE, false);
    }

    // // After setup, check usage:
    // Serial.printf("vlpp arena: %u / %u bytes\n",
    //               vlpp_arena_obj.bytes_used(), vlpp_arena_obj.capacity);

  }

  FLASHMEM void setup_sd() {
    static bool storage_initialised = false;
    if (storage_initialised) 
      return;
    #ifdef ENABLE_SD
      SD.begin(chipSelect);
      //SD.setMediaDetectPin(0xFF); // disable media detect
      Serial.printf("setup_sd() SD card initialised, media present: %i\n", SD.mediaPresent());
      storage_initialised = true;
    #else
      Serial.println("setup_sd() SD not enabled");
    #endif
    // Register savestate settings so current_state can save/load via saveloadlib
    current_state.setup_saveable_settings();
  }

  bool save_scene(int project_number, uint8_t scene_number, savestate *input, bool debug) {
    #ifdef ENABLE_SD
    make_project_folders(project_number);

    char filename[MAX_FILEPATH] = "";
    snprintf(filename, MAX_FILEPATH, FILEPATH_SCENE_FORMAT, project_number, scene_number);
    if (debug) Serial.printf(F("save_scene(%i, %i) writing to %s\n"), project_number, scene_number, filename);
    if (SD.exists(filename)) SD.remove(filename);

    uint32_t micros_start = micros();
    if (!sl_save_to_file(project->save_tree, filename, SL_SCOPE_SCENE)) {
      if (debug) Serial.printf(F("save_scene: sl_save_to_file failed for %s\n"), filename);
      return false;
    }
    Serial.printf("save_scene took %lu micros\n", micros() - micros_start);

    update_scene_filename(String(filename));
    messages_log_add(String("Saved scene ") + String(project_number) + ":" + String(scene_number));
    return true;
    #endif
    return false;
  }

  //void update_scene_filename(String filename);

  char *get_scene_filename(int project_number, int scene_number) {
    static char filename[MAX_FILEPATH];
    snprintf(filename, MAX_FILEPATH, FILEPATH_SCENE_FORMAT, project_number, scene_number);
    return filename;
  }
  char *get_project_settings_filename(int project_number) {
    static char filename[MAX_FILEPATH];
    snprintf(filename, MAX_FILEPATH, FILEPATH_PROJECT_SETTINGS_FORMAT, project_number);
    return filename;
  }

  bool load_scene(int project_number, uint8_t scene_number, savestate *output, bool debug) {
    #ifdef ENABLE_SD
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      static volatile bool already_loading = false;
      if (already_loading) return false;
      if (global_load_lock) return false;
      global_load_lock = already_loading = true;

      messages_log_add(String("Loading scene ") + String(scene_number));

      char *filename = get_scene_filename(project_number, scene_number);
      update_scene_filename(String(filename));

      if (debug) Serial_printf(F("load_scene(%i,%i) from %s\n"), project_number, scene_number, filename);
      uint32_t micros_start = micros();
      bool ok = sl_load_from_file(filename, SL_SCOPE_SCENE);
      Serial_printf(F("sl_load_from_file for scene %i:%i took %lu micros\n"), project_number, scene_number, micros() - micros_start);

      global_load_lock = already_loading = false;
      return ok;
    }
    #endif
    return false;
  }

  /*bool copy_directory(char *sourceDirectory, char *destinationDirectory) {
    SdFile dir;
    if (!dir.open(sourceDirectory)){
      Serial.printf("copy_directory: error opening source %s\n", sourceDirectory);
    }
    while (dir.openNext(&dir, O_RDONLY)) {
      copy_file(dir.getName(), destinationDirectory);
    }
  }*/

  bool copy_file(const char *sourceFile, const char *destnFile) {
    //bool irqs_enabled = __irq_enabled();
    //__disable_irq();
    SdFile myOrigFile;
    SdFile myDestFile;

    if (!myOrigFile.open(sourceFile, FILE_READ)) {
      //SD.errorHalt("opening source file for read failed");
      Serial.printf(F("Error opening source file for copy '%s'\n"), sourceFile);
      //if (irqs_enabled) __enable_irq();
      return false;
    }

    if (!myDestFile.open(destnFile, FILE_WRITE)) {
      //SD.errorHalt("opening destn file for write failed");
      Serial.printf(F("Error opening dest file for copy '%s'\n"), destnFile);
      //if (irqs_enabled) __enable_irq();
      return false;
    }    

    size_t n;  
    uint8_t buf[64];
    while ((n = myOrigFile.read(buf, sizeof(buf))) > 0) {
      myDestFile.write(buf, n);
    }

    myOrigFile.close();
    myDestFile.close();    

    //if (irqs_enabled) __enable_irq();
    return true;
  }
}



void setup_saveloadlib() {
    // Register settings with saveloadlib by calling setup_saveable_settings() on an instance of each ISaveableSettingHost subclass.

    // set up pool for allocation of settings -- improves speed significantly compared to slow EXTMEM new()
    EXTMEM static SL_Arena<524288> sl_arena;
    sl_set_setting_arena(&sl_arena);

    // set up file read buffer for sl_load_from_file -- also much faster than EXTMEM new() on demand
    EXTMEM static char sl_file_buf[READ_FILE_BUF_SIZE];
    sl_set_file_read_buffer(sl_file_buf, READ_FILE_BUF_SIZE);

    settings_root = new SettingsRoot();

    sl_register_root(settings_root);

    uint32_t start_time = millis();
    sl_setup_all(settings_root);
    Serial.printf("setup_saveloadlib: Finished sl_setup_all in %i millis.\n", millis()-start_time);

    //sl_validate_tree(settings_root, Serial);

    // Wire the save tree into Project so Project::save_project_settings() can
    // call sl_save_to_file without needing to know the full SettingsRoot type.
    project->save_tree = settings_root;
}

SettingsRoot *settings_root;
