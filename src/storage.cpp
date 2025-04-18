#include <Arduino.h>
#include "storage.h"

#include "project.h"

#include "behaviours/behaviour_manager.h"

#include "mymenu/menu_fileviewers.h"

#include <util/atomic.h>

#ifdef ENABLE_SD
  #include "SD.h"
#endif

//#include "SdFat.h"
#include <SPI.h>
namespace storage {

  savestate current_state; 

  #if defined(__arm__) && defined(CORE_TEENSY)
  // ... Teensy, so save to SD card instead of to EEPROM

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

  FLASHMEM void setup_storage() {
    static bool storage_initialised = false;
    if (storage_initialised) 
      return;
    #ifdef ENABLE_SD
      SD.begin(chipSelect);
      //SD.setMediaDetectPin(0xFF); // disable media detect
      Serial.printf("setup_storage() SD card initialised, media present: %i\n", SD.mediaPresent());
      storage_initialised = true;
    #else
      Serial.println("setup_storage() SD not enabled");
    #endif
  }

  bool save_pattern(int project_number, uint8_t pattern_number, savestate *input, bool debug) {
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

      // save clock + sequence data
      myFile.println(F("; begin sequence"));
      myFile.printf(F("id=%i\n"),input->id);
      myFile.printf(F("size_clocks=%i\n"),     input->size_clocks);
      myFile.printf(F("size_sequences=%i\n"),  input->size_sequences);
      myFile.printf(F("size_steps=%i\n"),      input->size_steps);
      for (unsigned int i = 0 ; i < input->size_clocks ; i++) {
        myFile.printf(F("clock_multiplier=%i\n"), input->clock_multiplier[i]);
      }
      for (unsigned int i = 0 ; i < input->size_clocks ; i++) {
        myFile.printf(F("clock_delay=%i\n"), input->clock_delay[i]);
      }
      for (unsigned int i = 0 ; i < input->size_sequences ; i++) {
        myFile.printf(F("sequence_data="));
        for (int x = 0 ; x < input->size_steps ; x++) {
          myFile.printf("%1x", input->sequence_data[i][x]);
        }
        myFile.println();
      }

      // save behaviour extensions
      myFile.println(F("; behaviour extensions")); 
      LinkedList<String> behaviour_lines = LinkedList<String>();
      if (debug) Serial.println("calling save_pattern_add_lines..");
      behaviour_manager->save_pattern_add_lines(&behaviour_lines);
      if (debug) { Serial.println("got behaviour_lines to save.."); Serial.flush(); }
      for (unsigned int i = 0 ; i < behaviour_lines.size() ; i++) {
        //myFile.printf("behaviour_option_%s\n", behaviour_lines.get(i).c_str());
        if (debug) Serial.printf(F("\tsequence writing behaviour line '%s'\n"), behaviour_lines.get(i).c_str());
        //Serial.flush();
        myFile.printf(F("%s\n"), behaviour_lines.get(i).c_str());
      }
      if (debug) Serial.printf("wrote %i behaviour lines\n", behaviour_lines.size());
      myFile.println(F("; end sequence"));

      // all done -- close the file
      myFile.close();
      //if (irqs_enabled) __enable_irq();
      //Serial.println(F("Finished saving."));

      //sequence_fileviewer->debug = debug;
      update_pattern_filename(String(filename));

      messages_log_add(String("Saved to project : pattern ") + String(project_number) + " : " + String(pattern_number));

      #endif
    }
    if (debug) Serial.println("finishing save_pattern");

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

  void load_pattern_parse_line(String line, savestate *output) {
    bool debug = false;
    if (line.charAt(0)==';') {
      return;  // skip comment lines
    } else if (line.startsWith(F("id="))) {
      output->id = (uint8_t) line.remove(0,String(F("id=")).length()).toInt();
      if (debug) Serial.printf(F("Read id %i\n"), output->id);
      return;
    } else if (line.startsWith(F("size_clocks="))) {
      output->size_clocks = (uint8_t) line.remove(0,String(F("size_clocks=")).length()).toInt();
      if (debug) Serial.printf(F("Read size_clocks %i\n"), output->size_clocks);
      return;
    } else if (line.startsWith(F("size_sequences="))) {
      output->size_sequences = (uint8_t) line.remove(0,String(F("size_sequences=")).length()).toInt();
      if (debug) Serial.printf(F("Read size_sequences %i\n"), output->size_sequences);
      return;
    } else if (line.startsWith(F("size_steps="))) {
      output->size_steps = (uint8_t) line.remove(0,String(F("size_steps=")).length()).toInt();
      if (debug) Serial.printf(F("Read size_steps %i\n"), output->size_steps);
      return;
    } else if (project->isLoadClockSettings() && line.startsWith(F("clock_multiplier="))) {
      if (clock_multiplier_index>NUM_CLOCKS) {
        if (debug) Serial.println(F("Skipping clock_multiplier entry as exceeds NUM_CLOCKS"));
        return;
      }
      output->clock_multiplier[clock_multiplier_index] = (uint8_t) line.remove(0,String(F("clock_multiplier=")).length()).toInt();
      if (debug) Serial.printf(F("Read a clock_multiplier: %i\n"), output->clock_multiplier[clock_multiplier_index]);
      clock_multiplier_index++;
      return;
    } else if (project->isLoadClockSettings() && line.startsWith(F("clock_delay="))) {
      if (clock_delay_index>NUM_CLOCKS) {
        if (debug) Serial.println(F("Skipping clock_delay entry as exceeds NUM_CLOCKS"));
        return;
      }
      output->clock_delay[clock_delay_index] = (uint8_t) line.remove(0,String(F("clock_delay=")).length()).toInt();      
      if (debug) Serial.printf(F("Read a clock_delay: %i\n"), output->clock_delay[clock_delay_index]);
      clock_delay_index++;
      return;
    } else if (project->isLoadSequencerSettings() && line.startsWith(F("sequence_data="))) {
      if (clock_delay_index>NUM_SEQUENCES) {
        if (debug) Serial.println(F("Skipping sequence_data entry as exceeds NUM_CLOCKS"));
        return;
      }
      //output->clock_multiplier = (uint8_t) line.remove(0,String("clock_multiplier=").length()).toInt();      
      String data = line.remove(0,String(F("sequence_data=")).length());
      char v[2] = "0";
      if (debug) Serial.printf(F("Reading sequence %i: ["), sequence_data_index);
      for (unsigned int x = 0 ; x < data.length() && x < output->size_steps && data.charAt(x)!='\n' ; x++) {
        v[0] = data.charAt(x);
        if (v[0]=='\n') break;
        output->sequence_data[sequence_data_index][x] = hex2int(v);
        //Serial.printf("%i:%i, ", x, output->sequence_data[sequence_data_index][x]);
        if (debug) Serial.printf(F("%i"), output->sequence_data[sequence_data_index][x]);
      }
      if (debug) Serial.println(']');
      sequence_data_index++;
      return;
    } else if (project->isLoadBehaviourOptions() && behaviour_manager->load_parse_line(line)) {
      return;
    }
    messages_log_add(String("Ignoring line '") + line + String("'"));
  }

  //void update_pattern_filename(String filename);

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

      char filename[MAX_FILEPATH] = "";
      snprintf(filename, MAX_FILEPATH, FILEPATH_PATTERN_FORMAT, project_number, pattern_number);

      update_pattern_filename(String(filename));
      // ^^^ hmm get more frequent intermittent crashes on load in T+A modes if this is enabled...

      File myFile;   
      Serial.printf(F("load_pattern: load_pattern(%i,%i) opening %s\n"), project_number, pattern_number, filename); Serial_flush();
      myFile = SD.open(filename, FILE_READ);
      clock_multiplier_index = clock_delay_index = sequence_data_index = 0;

      /*if(project.isLoadBehaviourOptions()) {
        behaviour_manager->reset_all_mappings();
      }*/

      if (!myFile) {
        Serial.printf(F("load_pattern: Error: Couldn't open %s for reading!\n"), filename);  Serial_flush();
        //if (irqs_enabled) __enable_irq();
        global_load_lock = already_loading = false;
        return false;
      }
      myFile.setTimeout(0);

      String line;
      while (line = myFile.readStringUntil('\n')) {
        load_pattern_parse_line(line, output);
      }
      Serial.println(F("load_pattern: Closing file..")); Serial_flush();
      myFile.close();
      //if (irqs_enabled) __enable_irq();
      Serial.println(F("load_pattern: File closed")); Serial_flush();

      #ifdef ENABLE_APCMINI_DISPLAY
        //redraw_immediately = true;
      #endif

      Serial.printf(F("load_pattern: Loaded pattern from [%s] [%i clocks, %i sequences of %i steps]\n"), filename, clock_multiplier_index, sequence_data_index, output->size_steps); Serial_flush();
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


  #else
  // Arduino, save to EEPROM
  #include <EEPROM.h>

  void setup_storage() {
    // nothing to do for Arduino
  }

  void save_pattern(uint8_t preset_number, savestate *input) {
    int eeAddress = 16 + (preset_number * sizeof(savestate));
    Serial.print(F("save_pattern at "));
    Serial.println(eeAddress);
    EEPROM.put(eeAddress, *input);
  }

  bool load_pattern(uint8_t preset_number, savestate *output) {
    int eeAddress = 16 + (preset_number * sizeof(savestate));
    byte id = EEPROM.read(eeAddress);
    if (id==SAVE_ID_BYTE_V0 || id==SAVE_ID_BYTE_V1) {
      Serial.print(F("Found ID "));
      Serial.print(id);
      Serial.print(F(" at "));
      Serial.print(eeAddress);
      Serial.println(F(" - loading! :D"));
      EEPROM.get(eeAddress, *output);
      if (id==SAVE_ID_BYTE_V0) {  // v0 didn't have clock delays included, so zero them out
        output->clock_delay[0] = output->clock_delay[1] = output->clock_delay[2] = output->clock_delay[3] = 0;
      }
      return true;

    } else {
      Serial.print(F("Didn't find a magic id byte at "));
      //Serial.print(0xD0);
      //Serial.print(F(" at "));
      Serial.print(eeAddress);
      Serial.print(F(" - found "));
      Serial.print(id);
      Serial.println(F(" instead :("));
      return false;
    }    
  }
#endif
}
