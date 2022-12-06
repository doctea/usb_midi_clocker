#include "file_manager/file_manager.h"
#include "file_manager/file_manager_interfaces.h"

bool load_file(char *filename, IParseKeyValueReceiver *target) {
    static volatile bool already_loading = false;
    if (already_loading) 
        return false;
    already_loading = true;

    File myFile;
    Debug_printf(F("load_file(%s)...\n"), filename);

    myFile = SD.open(filename, FILE_READ);
    myFile.setTimeout(0);

    if (!myFile) {
      Debug_printf(F("Error: Couldn't open %s for reading!\n"), filename);  Serial_flush();
      //if (irqs_enabled) __enable_irq();
      already_loading = false;
      return false;
    }

    String line;
    while (line = myFile.readStringUntil('\n')) {
        if (line.charAt(0)==';')
            continue;
        int split = line.indexOf("=");
        String key = line.substring(0, split);
        String value = line.substring(split+1);
        target->load_parse_key_value(key, value);
    }

    Serial.println(F("Closing file..")); Serial_flush();
    myFile.close();
    return true;
}

bool save_file(char *filename, ISaveKeyValueSource *source) {
    LinkedList<String> lines;// = new LinkedList<String>();

    File myFile;

    //char filename[255] = "";
    //sprintf(filename, FILEPATH_SEQUENCE_FORMAT, project_number, preset_number);
    Serial.printf(F("save_file(%s)\n"), filename);
    if (SD.exists(filename)) {
      Serial.printf(F("%s exists, deleting first\n"), filename);
      SD.remove(filename);
    }
    myFile = SD.open(filename, FILE_WRITE_BEGIN | (uint8_t)O_TRUNC); //FILE_WRITE_BEGIN);
    if (!myFile) {    
      Serial.printf(F("Error: couldn't open %s for writing\n"), filename);
      //if (irqs_enabled) __enable_irq();
      return false;
    }
    myFile.println(F("; begin"));

    source->save_sequence_add_lines(&lines);
    
    for (int i = 0 ; i < lines.size() ; i++) {
        myFile.println(lines.get(i));
    }
    myFile.println(F("; end"));

    myFile.close();
    Serial.println(F("Finished saving."));
    return true;
}
