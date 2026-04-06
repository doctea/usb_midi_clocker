#include <Arduino.h>
#include "settings_root.h"  // defines SettingsRoot; also pulls in storage.h, project.h, behaviour_manager.h
#include "storage.h"        // idempotent re-include (pragma once)
#include "project.h"        // idempotent
#include "behaviours/behaviour_manager.h"  // idempotent

#include "mymenu/menu_fileviewers.h"

#include <util/atomic.h>

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
    Serial_println("Forcing crash!");
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
  // Registers all scene fields under path segment "scene" with SL_SCOPE_PATTERN
  // mask.  Called once by setup_sd() on current_state.
  // ---------------------------------------------------------------------------
  void savestate::setup_saveable_settings() {
    set_path_segment("scene");

    // Scene metadata
    register_setting(new LSaveableSetting<uint8_t>("scene_id",    "Scene", &id),            false, SL_SCOPE_PATTERN);
    register_setting(new LSaveableSetting<uint8_t>("size_clocks", "Scene", &size_clocks),   false, SL_SCOPE_PATTERN);
    register_setting(new LSaveableSetting<uint8_t>("size_seqs",   "Scene", &size_sequences), false, SL_SCOPE_PATTERN);
    register_setting(new LSaveableSetting<uint8_t>("size_steps",  "Scene", &size_steps),    false, SL_SCOPE_PATTERN);

    // Clock multipliers — packed as 2-hex-chars-per-byte: "clock_mult=07060504..."
    register_setting(new PackedByteArraySetting("clock_mult",  "Clock", clock_multiplier, NUM_CLOCKS), false, SL_SCOPE_PATTERN);
    // Clock delays — same format
    register_setting(new PackedByteArraySetting("clock_delay", "Clock", clock_delay,      NUM_CLOCKS), false, SL_SCOPE_PATTERN);

    // Sequence rows — one nibble-hex setting per row, length from live size_steps pointer
    char lbl[20];
    for (uint8_t i = 0; i < NUM_SEQUENCES; i++) {
      snprintf(lbl, sizeof(lbl), "seq_%u", i);
      register_setting(new SequenceRowSetting(lbl, "Sequence", sequence_data[i], &size_steps), false, SL_SCOPE_PATTERN);
    }
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

  bool save_pattern(int project_number, uint8_t pattern_number, savestate *input, bool debug) {
    uint32_t micros_start = micros();
    #ifdef ENABLE_SD
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      //bool debug = false;
      #ifdef ENABLE_SD
      File myFile;

      // check + create project folders, if they don't already exist
      make_project_folders(project_number);

      // check + remove sequence file if it already exists
      char filename[MAX_FILEPATH] = "";
      snprintf(filename, MAX_FILEPATH, FILEPATH_PATTERN_FORMAT, project_number, pattern_number);
      if (debug) Serial.printf(F("save_pattern(%i, %i) writing to %s\n"), project_number, pattern_number, filename);
      if (SD.exists(filename)) {
        //Serial.printf(F("%s exists, deleting first\n"), filename); Serial.flush();
        SD.remove(filename);
        //Serial.println("deleted"); Serial.flush();
      }

      myFile = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC); //FILE_WRITE_BEGIN);
      if (!myFile) {    
        if (debug) Serial.printf(F("Error: couldn't open %s for writing\n"), filename);
        //if (irqs_enabled) __enable_irq();
        return false;
      }
      if (debug) { Serial.println("Starting data write.."); Serial_flush(); }

      myFile.println(F("; begin scene"));

      // Save scene fields (clock multipliers, delays, sequence data) via saveloadlib.
      // Each line is prefixed "scene~key=value".
      LinkedList<String> scene_lines = LinkedList<String>();
      sl_save_to_linkedlist(input, scene_lines, SL_SCOPE_PATTERN);
      for (unsigned int i = 0 ; i < scene_lines.size() ; i++) {
        myFile.println(scene_lines.get(i).c_str());
      }
      if (debug) Serial.printf("wrote %u scene lines\n", scene_lines.size());

      // Save behaviour pattern extensions via saveloadlib.
      // Each behaviour writes its SL_SCOPE_PATTERN settings prefixed by its label.
      if (debug) { Serial_println(F("Saving behaviour extensions..")); Serial_flush(); }
      myFile.println(F("; behaviour extensions"));
      LinkedList<String> behaviour_lines = LinkedList<String>();
      if (debug) Serial.println("calling sl_save_to_linkedlist for behaviour_manager..");
      sl_save_to_linkedlist(behaviour_manager, behaviour_lines, SL_SCOPE_PATTERN);
      if (debug) { Serial.println("got behaviour_lines to save.."); Serial.flush(); }
      for (unsigned int i = 0 ; i < behaviour_lines.size() ; i++) {
        if (debug) {
          Serial_printf(F("\twriting behaviour line [%i/%i] '%s'\n"), i+1, behaviour_lines.size(), behaviour_lines.get(i).c_str());
          Serial.flush();
        }
        myFile.println(behaviour_lines.get(i).c_str());
      }
      if (debug) {
        Serial.printf("wrote %i behaviour lines\n", behaviour_lines.size()); Serial_flush();
      }
      myFile.println(F("; end scene"));

      // TODO: save parameter input states via saveloadlib
      // parameter_manager->save_pattern_parameter_inputs_add_lines() removed - vestige of old save/load mechanism
      
      // all done -- close the file
      myFile.close();
      //if (irqs_enabled) __enable_irq();
      //Serial.println(F("Finished saving."));

      //sequence_fileviewer->debug = debug;
      //update_pattern_filename(String(filename));

      messages_log_add(String("Saved to project : pattern ") + String(project_number) + " : " + String(pattern_number));

      #endif
    }
    if (debug) Serial.println("finishing save_pattern");

    Serial.printf("save_pattern took %i micros\n", micros()-micros_start);

    return true;
    #endif
    return false;
  }

  enum load_states {
    NONE,
    LOADING
  };

  byte load_state_current = load_states::NONE;
  savestate *load_state_output;
  File load_state_file;
  uint8_t clock_multiplier_index = 0;
  uint8_t clock_delay_index = 0;
  uint8_t sequence_data_index = 0;

/*
  void load_state_update() {
    if (load_state_current!=load_states::LOADING || !load_state_file)
      return;

    String line;
    if(line = load_state_file.readStringUntil('\n')) {
      Serial.printf("%i: parsing line %s\n", millis(), line.c_str());
      load_pattern_parse_line(line, load_state_output);
      Serial.printf("%i: finished load_pattern_parse_line\n", millis());
    } else {
      Serial.printf("%i: Finished loading file\n", millis());
      load_state_current = load_states::NONE;
      load_state_file.close();
      Serial.printf("%i: closed file\n",millis());
    }
  }
  void load_state_start(uint8_t preset_number, savestate *output) {
    //bool debug = false;
    //Serial.println("load_pattern not implemented on teensy");
    if (load_state_current==load_states::LOADING) {
      load_state_file.close();
      load_state_current = load_states::NONE;
    }
    clock_multiplier_index = clock_delay_index = sequence_data_index = 0;

    char filename[255] = "";
    sprintf(filename, "sequences/sequence%i.txt", preset_number);
    Serial.printf("load_state_start(%i) opening %s\n", preset_number, filename);
    load_state_file = SD.open(filename, FILE_READ);

    if (!load_state_file) {
      Serial.printf("Error: Couldn't open %s for reading!\n", filename);
      return;
    }

    load_state_file.setTimeout(0);
    load_state_current = load_states::LOADING;
    load_state_output = output;
  }*/

  bool load_pattern_parse_line(String line, savestate *output, bool debug = false) {
    line.replace('\n', "");
    line.replace('\r', "");

    if (line.charAt(0) == ';') return true;  // comment

    int eq = line.indexOf('=');
    if (eq < 0) return false;

    String left  = line.substring(0, eq);
    String value = line.substring(eq + 1);

    int tilde = left.indexOf('~');
    if (tilde >= 0) {
      // -----------------------------------------------------------------------
      // saveloadlib tilde-delimited format: "segment~key=value"
      // Route "scene~..." to the savestate settings tree;
      // everything else to behaviour_manager ("BehaviourLabel~key=value").
      // -----------------------------------------------------------------------
      String segment = left.substring(0, tilde);
      String rest    = left.substring(tilde + 1);

      if (segment.equals(F("scene"))) {
        static char restbuf[SL_MAX_LINE];
        static char valbuf[SL_MAX_LINE];
        rest.toCharArray(restbuf,  sizeof(restbuf));
        value.toCharArray(valbuf, sizeof(valbuf));
        static char* segs[16];
        int cnt = sl_tokenise_inplace(restbuf, segs, 16);
        if (cnt > 0) return output->load_line(segs, cnt, valbuf, SL_SCOPE_PATTERN);
        return false;
      }
      // } else if (project->isLoadBehaviourOptions()) {
      //   if (debug) Serial_println(F("Routing to behaviour_manager"));
      //   return behaviour_manager->load_parse_line(line);
      // }
      return false;
    }

    // -------------------------------------------------------------------------
    // Legacy flat format fallback — handles save files written before the
    // saveloadlib migration.  Positional counters (clock_multiplier_index etc.)
    // are reset at the top of load_pattern() before parsing begins.
    // -------------------------------------------------------------------------
    if (debug) Serial.printf(F("load_pattern_parse_line: legacy format: '%s'\n"), line.c_str());

    /*if (left.equals(F("scene_id"))) {
      output->scene_id = (uint8_t)value.toInt();
      if (debug) Serial.printf(F("Read scene_id %i\n"), output->sceid);
      return true;
    } else*/ if (left.equals(F("size_clocks"))) {
      output->size_clocks = (uint8_t)value.toInt();
      if (debug) Serial.printf(F("Read size_clocks %i\n"), output->size_clocks);
      return true;
    } else if (left.equals(F("size_sequences"))) {
      output->size_sequences = (uint8_t)value.toInt();
      if (debug) Serial.printf(F("Read size_sequences %i\n"), output->size_sequences);
      return true;
    } else if (left.equals(F("size_steps"))) {
      output->size_steps = (uint8_t)value.toInt();
      if (debug) Serial.printf(F("Read size_steps %i\n"), output->size_steps);
      return true;
    } else if (project->isLoadClockSettings() && left.equals(F("clock_multiplier"))) {
      if (clock_multiplier_index < NUM_CLOCKS) {
        output->clock_multiplier[clock_multiplier_index] = (uint8_t)value.toInt();
        if (debug) Serial.printf(F("Read clock_multiplier[%i]=%i\n"), clock_multiplier_index, output->clock_multiplier[clock_multiplier_index]);
        clock_multiplier_index++;
      }
      return true;
    } else if (project->isLoadClockSettings() && left.equals(F("clock_delay"))) {
      if (clock_delay_index < NUM_CLOCKS) {
        output->clock_delay[clock_delay_index] = (uint8_t)value.toInt();
        if (debug) Serial.printf(F("Read clock_delay[%i]=%i\n"), clock_delay_index, output->clock_delay[clock_delay_index]);
        clock_delay_index++;
      }
      return true;
    } else if (project->isLoadSequencerSettings() && left.equals(F("sequence_data"))) {
      if (sequence_data_index < NUM_SEQUENCES) {
        if (debug) Serial.printf(F("Reading legacy sequence %i: ["), sequence_data_index);
        char v[2] = "0";
        for (unsigned int x = 0; x < (unsigned)value.length() && x < output->size_steps; x++) {
          v[0] = value.charAt(x);
          if (v[0] == '\n') break;
          output->sequence_data[sequence_data_index][x] = hex2int(v);
          if (debug) Serial.printf(F("%i"), output->sequence_data[sequence_data_index][x]);
        }
        if (debug) Serial.println(']');
        sequence_data_index++;
      }
      return true;
    }
    // } else if (project->isLoadBehaviourOptions() && behaviour_manager->load_parse_line(line)) {
    //   if (debug) Serial_println(F("Parsed a legacy line with behaviour_manager"));
    //   return true;
    // }
    // TODO: reload parameter input options via saveloadlib
    // parameter_manager->load_parse_line(line) removed - vestige of old mechanism

    if (debug) Serial.printf(F("Unrecognised line in scene file: '%s'\n"), line.c_str());
    messages_log_add(String("Ignoring scene line '") + line + String("'"));
    return false;
  }

  //void update_pattern_filename(String filename);

  char *get_pattern_filename(int project_number, int pattern_number) {
    static char filename[MAX_FILEPATH];
    snprintf(filename, MAX_FILEPATH, FILEPATH_PATTERN_FORMAT, project_number, pattern_number);
    return filename;
  }
  char *get_project_settings_filename(int project_number) {
    static char filename[MAX_FILEPATH];
    snprintf(filename, MAX_FILEPATH, FILEPATH_PROJECT_SETTINGS_FORMAT, project_number);
    return filename;
  }

  bool load_pattern(int project_number, uint8_t pattern_number, savestate *output, bool debug) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      #ifdef ENABLE_SD
      static volatile bool already_loading = false;
      if (already_loading) return false;
      if (global_load_lock) return false;
      global_load_lock = true; 
      already_loading = true;

      messages_log_add(String("Loading pattern ") + String(pattern_number));

      //bool irqs_enabled = __irq_enabled();
      //__disable_irq();

      char *filename = get_pattern_filename(project_number, pattern_number);

      update_pattern_filename(String(filename));
      // ^^^ hmm get more frequent intermittent crashes on load in T+A modes if this is enabled...

      File myFile;   
      if (debug) { Serial_printf(F("load_pattern: load_pattern(%i,%i) opening %s\n"), project_number, pattern_number, filename); Serial_flush(); }
      uint32_t start_time = micros();
      myFile = SD.open(filename, FILE_READ);
      uint32_t time_to_open = micros()-start_time;
      if (debug) { Serial_printf(F("load_pattern: opened file in %i micros\n"), time_to_open); Serial_flush(); }
     
      clock_multiplier_index = clock_delay_index = sequence_data_index = 0;

      /*if(project.isLoadBehaviourOptions()) {
        behaviour_manager->reset_all_mappings();
      }*/

      if (!myFile) {
        if (debug) Serial.printf(F("load_pattern: Error: Couldn't open %s for reading!\n"), filename);  Serial_flush();
        //if (irqs_enabled) __enable_irq();
        global_load_lock = already_loading = false;
        return false;
      }
      myFile.setTimeout(0);
      /*char *file_contents = (char *)malloc(myFile.size()+1);
      if (!file_contents) {
        Serial_println(F("load_pattern: Error: couldn't allocate memory for file_contents")); Serial_flush();
        myFile.close();
        global_load_lock = already_loading = false;
        return false;
      }

      myFile.readBytes(file_contents, myFile.size());
      myFile.close();

      char *line_start = file_contents;
      char *line_end = nullptr;
      while (line_start && *line_start) {
        line_end = strchr(line_start, '\n');
        if (line_end) {
          *line_end = '\0';
        }
        String line = String(line_start);
        //Serial.printf(F("load_pattern: parsing line: %s\n"), line.c_str()); Serial_flush();
        load_pattern_parse_line(line, output);
        //Serial.printf(F("load_pattern: finished load_pattern_parse_line\n")); Serial_flush();
        if (line_end) {
          line_start = line_end + 1;
        } else {
          line_start = nullptr;
        }
      }

      free(file_contents);
      */

      String line;
      int line_count = 0;
      int failed_line_count = 0;
      while (line = myFile.readStringUntil('\n')) {
        if (debug) Serial.printf(F("load_pattern: parsing line %3i: '%s'\n"), line_count, line.c_str()); Serial_flush();
        failed_line_count += load_pattern_parse_line(line, output, debug) ? 0 : 1;
        if (debug) Serial.printf(F("load_pattern: finished load_pattern_parse_line %3i\n"), line_count); Serial_flush(); Serial_flush();
        line_count++;
      }
      if (debug) Serial_printf(F("load_pattern: Closing file after %3i lines (%3i failed)\n"), line_count, failed_line_count); Serial_flush();
      myFile.close();
      //if (irqs_enabled) __enable_irq();
      if (debug) Serial_println(F("load_pattern: File closed")); Serial_flush();

      #ifdef ENABLE_APCMINI_DISPLAY
        //redraw_immediately = true;
      #endif

      if (debug) Serial_printf(F("load_pattern: Loaded pattern from [%s] [%i clocks, %i sequences of %i steps]\n"), filename, clock_multiplier_index, sequence_data_index, output->size_steps); Serial_flush();
      global_load_lock = already_loading = false;
      #endif
    }
    return true;
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

    settings_root = new SettingsRoot();

    sl_register_root(settings_root);

    sl_setup_all(settings_root);

    // Wire the save tree into Project so Project::save_project_settings() can
    // call sl_save_to_file without needing to know the full SettingsRoot type.
    project->save_tree = settings_root;
}

SettingsRoot *settings_root;
