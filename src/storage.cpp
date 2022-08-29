#include <Arduino.h>
#include "storage.h"

#include "project.h"

#include "SD.h"
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

  const int chipSelect = BUILTIN_SDCARD;

  void make_project_folders(int project_number) {
    char path[255];
    sprintf(path, "project%i", project_number);
    Serial.printf("Checking exists %s..", path);
    if (!SD.exists(path)) {
      Serial.println("making!\n");
      SD.mkdir(path);
    } else {
      Serial.println("exists!\n");
    }
    
    sprintf(path, "project%i/sequences", project_number);
    Serial.printf("Checking exists %s..", path);
    if (!SD.exists(path)) {
      Serial.println("making!\n");
      SD.mkdir(path);
    } else {
      Serial.println("exists!\n");
    }

    sprintf(path, "project%i/loops", project_number);
    Serial.printf("Checking exists %s..", path);
    if (!SD.exists(path)) {
      Serial.println("making!\n");
      SD.mkdir(path);
    } else {
      Serial.println("exists!\n");
    }
  }

  void setup_storage() {
    SD.begin(chipSelect);

    if (!SD.exists("sequences")) {
      Serial.println("Folder 'sequences' doesn't exist on SD, creating!");
      SD.mkdir("sequences");
    }
    if (!SD.exists("loops")) {
      Serial.println("Folder 'loops' doesn't exist on SD, creating!");
      SD.mkdir("loops");
    }
  }

  bool save_sequence(int project_number, uint8_t preset_number, savestate *input) {
    //Serial.println("save_sequence not implemented on teensy");
    File myFile;

    char filename[255] = "";
    sprintf(filename, FILEPATH_SEQUENCE_FORMAT, project_number, preset_number);
    Serial.printf("save_sequence(%i, %i) writing to %s\n", project_number, preset_number, filename);
    if (SD.exists(filename)) {
      Serial.printf("%s exists, deleting first\n", filename);
      SD.remove(filename);
    }
    myFile = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC); //FILE_WRITE_BEGIN);
    if (!myFile) {    
      Serial.printf("Error: couldn't open %s for writing\n", filename);
      return false;
    }
    myFile.println("; begin sequence");
    myFile.printf("id=%i\n",input->id);
    myFile.printf("size_clocks=%i\n",     input->size_clocks);
    myFile.printf("size_sequences=%i\n",  input->size_sequences);
    myFile.printf("size_steps=%i\n",      input->size_steps);
    for (int i = 0 ; i < input->size_clocks ; i++) {
      myFile.printf("clock_multiplier=%i\n", input->clock_multiplier[i]);
    }
    for (int i = 0 ; i < input->size_clocks ; i++) {
      myFile.printf("clock_delay=%i\n", input->clock_delay[i]);
    }
    for (int i = 0 ; i < input->size_sequences ; i++) {
      myFile.printf("sequence_data=");
      for (int x = 0 ; x < input->size_steps ; x++) {
        myFile.printf("%1x", input->sequence_data[i][x]);
      }
      myFile.println("");
    }
    myFile.println("; end sequence");
    myFile.close();
    Serial.println("Finished saving.");
    return true;
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
      load_sequence_parse_line(line, load_state_output);
      Serial.printf("%i: finished load_sequence_parse_line\n", millis());
    } else {
      Serial.printf("%i: Finished loading file\n", millis());
      load_state_current = load_states::NONE;
      load_state_file.close();
      Serial.printf("%i: closed file\n",millis());
    }
  }
  void load_state_start(uint8_t preset_number, savestate *output) {
    //bool debug = false;
    //Serial.println("load_sequence not implemented on teensy");
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

  void load_sequence_parse_line(String line, savestate *output) {
    bool debug = false;
    if (line.charAt(0)==';') 
      return;  // skip comment lines
    if (line.startsWith("id=")) {
      output->id = (uint8_t) line.remove(0,String("id=").length()).toInt();
      if (debug) Serial.printf("Read id %i\n", output->id);
    } else if (line.startsWith("size_clocks=")) {
      output->size_clocks = (uint8_t) line.remove(0,String("size_clocks=").length()).toInt();
      if (debug) Serial.printf("Read size_clocks %i\n", output->size_clocks);
    } else if (line.startsWith("size_sequences=")) {
      output->size_sequences = (uint8_t) line.remove(0,String("size_sequences=").length()).toInt();
      if (debug) Serial.printf("Read size_sequences %i\n", output->size_sequences);
    } else if (line.startsWith("size_steps=")) {
      output->size_steps = (uint8_t) line.remove(0,String("size_steps=").length()).toInt();
      if (debug) Serial.printf("Read size_steps %i\n", output->size_steps);
    } else if (!project.hold_clock_settings && line.startsWith("clock_multiplier=")) {
      if (clock_multiplier_index>NUM_CLOCKS) {
        Serial.println("Skipping clock_multiplier entry as exceeds NUM_CLOCKS");
        return;
      }
      output->clock_multiplier[clock_multiplier_index] = (uint8_t) line.remove(0,String("clock_multiplier=").length()).toInt();
      if (debug) Serial.printf("Read a clock_multiplier: %i\n", output->clock_multiplier[clock_multiplier_index]);
      clock_multiplier_index++;      
    } else if (project.isLoadClockSettings() &&line.startsWith("clock_delay=")) {
      if (clock_delay_index>NUM_CLOCKS) {
        Serial.println("Skipping clock_delay entry as exceeds NUM_CLOCKS");
        return;
      }
      output->clock_delay[clock_delay_index] = (uint8_t) line.remove(0,String("clock_delay=").length()).toInt();      
      if (debug) Serial.printf("Read a clock_delay: %i\n", output->clock_delay[clock_delay_index]);
      clock_delay_index++;
    } else if (project.isLoadSequencerSettings() && line.startsWith("sequence_data=")) {
      if (clock_delay_index>NUM_SEQUENCES) {
        Serial.println("Skipping sequence_data entry as exceeds NUM_CLOCKS");
        return;
      }
      //output->clock_multiplier = (uint8_t) line.remove(0,String("clock_multiplier=").length()).toInt();      
      String data = line.remove(0,String("sequence_data=").length());
      char v[2] = "0";
      if (debug) Serial.printf("Reading sequence %i: [", sequence_data_index);
      for (unsigned int x = 0 ; x < data.length() && x < output->size_steps && data.charAt(x)!='\n' ; x++) {
        v[0] = data.charAt(x);
        if (v[0]=='\n') break;
        output->sequence_data[sequence_data_index][x] = hex2int(v);
        //Serial.printf("%i:%i, ", x, output->sequence_data[sequence_data_index][x]);
        if (debug) Serial.printf("%i", output->sequence_data[sequence_data_index][x]);
      }
      if (debug) Serial.println("]");
      sequence_data_index++;
    }
  }

  bool load_sequence(int project_number, uint8_t preset_number, savestate *output) {
    File myFile;

    char filename[255] = "";
    sprintf(filename, FILEPATH_SEQUENCE_FORMAT, project_number, preset_number);
    Serial.printf("load_sequence(%i,%i) opening %s\n", project_number, preset_number, filename);
    myFile = SD.open(filename, FILE_READ);
    myFile.setTimeout(0);

    clock_multiplier_index = clock_delay_index = sequence_data_index = 0;

    if (!myFile) {
      Serial.printf("Error: Couldn't open %s for reading!\n", filename);
      return false;
    }

    String line;
    while (line = myFile.readStringUntil('\n')) {
      load_sequence_parse_line(line, output);
    }
    Serial.println("Closing file..");
    myFile.close();
    Serial.println("File closed");

    #ifdef ENABLE_APCMINI_DISPLAY
      //redraw_immediately = true;
    #endif

    Serial.printf("Loaded preset from [%s] [%i clocks, %i sequences of %i steps]\n", filename, clock_multiplier_index, sequence_data_index, output->size_steps);
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
    SdFile myOrigFile;
    SdFile myDestFile;

    if (!myOrigFile.open(sourceFile, FILE_READ)) {
      //SD.errorHalt("opening source file for read failed");
      Serial.printf("Error opening source file for copy '%s'\n", sourceFile);
      return false;
    }

    if (!myDestFile.open(destnFile, FILE_WRITE)) {
      //SD.errorHalt("opening destn file for write failed");
      Serial.printf("Error opening dest file for copy '%s'\n", destnFile);
      return false;
    }    

    size_t n;  
    uint8_t buf[64];
    while ((n = myOrigFile.read(buf, sizeof(buf))) > 0) {
      myDestFile.write(buf, n);
    }

    myOrigFile.close();
    myDestFile.close();    

    return true;
  }


  #else
  // Arduino, save to EEPROM
  #include <EEPROM.h>

  void setup_storage() {
    // nothing to do for Arduino
  }

  void save_sequence(uint8_t preset_number, savestate *input) {
    int eeAddress = 16 + (preset_number * sizeof(savestate));
    Serial.print(F("save_sequence at "));
    Serial.println(eeAddress);
    EEPROM.put(eeAddress, *input);
  }

  bool load_sequence(uint8_t preset_number, savestate *output) {
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
