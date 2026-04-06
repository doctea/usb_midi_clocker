#pragma once

#include <Arduino.h>
#include <ctype.h>

#include <debug.h>

#include "project.h"
#include "storage.h"

bool pass_debug = true;

char serial_buffer[1024];
int serial_buffer_index = 0;

bool serial_connected = false;

bool execute_command(const char *command_line) {
    // parse and execute the command, return true if command was recognized and executed, false if unknown command

    char command[64];
    char arg1[64], arg2[64];

    // split command line into command, arg1 and arg2 (arg2 contains the remainder)
    command[0] = arg1[0] = arg2[0] = '\0';
    const char *p = command_line;
    // skip leading whitespace
    while (*p && isspace((unsigned char)*p)) p++;
    // first word -> command
    const char *start = p;
    while (*p && !isspace((unsigned char)*p)) p++;
    size_t len = p - start;
    if (len > 0) {
        if (len >= sizeof(command)) len = sizeof(command) - 1;
        memcpy(command, start, len);
        command[len] = '\0';
    }
    // skip whitespace before arg1
    while (*p && isspace((unsigned char)*p)) p++;
    // second word -> arg1
    start = p;
    while (*p && !isspace((unsigned char)*p)) p++;
    len = p - start;
    if (len > 0) {
        if (len >= sizeof(arg1)) len = sizeof(arg1) - 1;
        memcpy(arg1, start, len);
        arg1[len] = '\0';
    }
    // skip whitespace before arg2 (the remainder)
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p) {
        // copy the rest of the line into arg2
        strncpy(arg2, p, sizeof(arg2) - 1);
        arg2[sizeof(arg2) - 1] = '\0';
        // trim trailing CR/LF
        char *t = arg2 + strlen(arg2) - 1;
        while (t >= arg2 && (*t == '\n' || *t == '\r')) { *t = '\0'; t--; }
    }

    Serial.printf("Received command: '%s', arg1: '%s', arg2: '%s'\n", command, arg1, arg2);
    
    if (strcmp(command, "debug") == 0) {
        if (strcmp(arg1, "on") == 0) {
            pass_debug = true;
            Serial.println("Debug output enabled");
        } else if (strcmp(arg1, "off") == 0) {
            pass_debug = false;
            Serial.println("Debug output disabled");
        } else {
            Serial.println("Usage: debug <on|off>");
        }
        return true;
    } else if (strcmp(command, "ping") == 0) {
        Serial.println("pong");
        return true;
    } else if (strcmp(command, "status") == 0) {
        Serial.println("Status: All systems nominal.");
        return true;
    } else if (strcmp(command, "reset") == 0) {
        Serial.println("Resetting device...");
        reset_teensy(); // This is a Teensy-specific function to reset the device
        return true;
    } else if (strcmp(command, "free_ram") == 0) {
        debug_free_ram();
        return true;
    } else if (strcmp(command, "crashlog") == 0) {
        dump_crashreport_log();
        return true;
    } else if (strcmp(command, "clearcrashlog") == 0) {
        clear_crashreport_log();
        return true;    
    } else if (strcmp(command, "show") == 0) {
        if (arg1[0] == '\0') {
            Serial.println("Usage: show <type> <filename>");
            return true;
        }
        char *filename = nullptr;

        if (strcmp(arg1, "seq") == 0) {
            if (arg2[0] == '\0') {
                Serial.println("Usage: show seq <slot_number> (using currently selected slot if slot_number not provided)");
                // show current sequence pattern
                sprintf(arg2, "%i", project->selected_pattern_number); // default to slot 0 if no slot number provided
            }

            int desired_pattern_number = atoi(arg2);
            if (desired_pattern_number < 0 || desired_pattern_number >= NUM_PATTERN_SLOTS_PER_PROJECT) {
                Serial.printf("Invalid slot number: %i\n", desired_pattern_number);
                return true;
            }
            if (project->is_selected_pattern_number_empty(desired_pattern_number)) {
                Serial.printf("No pattern file found for slot %i\n", desired_pattern_number);
                return true;
            }
            // read the appropriate file from disc and output its contents to serial
            filename = storage::get_pattern_filename(project->current_project_number, desired_pattern_number);
        } else if (strcmp(arg1, "proj") == 0) {
            // show current project settings
            filename = storage::get_project_settings_filename(project->current_project_number);
        } else {
            Serial.printf("Unknown type: %s\n", arg1);
            return true;
        }

        if (filename == nullptr) {
            Serial.println("Unknown file to show. Usage: show <type> <filename>");
            return true;
        } 

        Serial.printf("------- Contents of %s: -------\n", filename);
        if (!SD.exists(filename) ) {
            Serial.printf("File %s does not exist!\n", filename);
            return true;
        }

        File file = SD.open(filename, FILE_READ);
        if (!file) {
            Serial.printf("Error opening file %s\n", filename);
            return true;
        }
        while (file.available()) {
            Serial.write(file.read());
        }
        Serial.printf("\n------- End of %s -------\n", filename);
        file.close();

        return true;

    } else if (strcmp(command, "load") == 0) {
        // load a pattern, project, etc from a file on the SD card. Usage: load <type> <filename>
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            Serial.println("Usage: load <type> <filename>");
            return true;
        }
        if (arg2[0] == '\0') {
            Serial.println("Usage: load <type> <filename>");
            return true;
        }
        if (strcmp(arg1, "proj") == 0) {
            // load project settings from the specified file
            int project_number = atoi(arg2);
            if (project_number < 0) {
                Serial.printf("Invalid project number: %i\n", project_number);
                return true;
            }
            Serial.printf("Loading project settings from file %i...\n", project_number);
            project->load_project_settings(project_number);
            
            return true;
        } else if (strcmp(arg1, "seq") == 0) {
            // load a sequence pattern from the specified file
            Serial.printf("Loading sequence pattern from file %s...\n", arg2);

            int desired_pattern_number = atoi(arg2);
            if (desired_pattern_number < 0 || desired_pattern_number >= NUM_PATTERN_SLOTS_PER_PROJECT) {
                Serial.printf("Invalid slot number: %i\n", desired_pattern_number);
                return true;
            }
            if (project->is_selected_pattern_number_empty(desired_pattern_number)) {
                Serial.printf("No pattern file found for slot %i\n", desired_pattern_number);
                return true;
            }

            Serial.printf("Loading pattern from file %i...\n", desired_pattern_number);
            project->load_pattern(desired_pattern_number, pass_debug);

            return true;
        } else {
            Serial.printf("Unknown type: %s\n", arg1);
            return true;
        }
    } else if (strcmp(command, "save") == 0) {
        // save a pattern, project, etc to a file on the SD card. Usage: save <type> <filename>
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            Serial.println("Usage: save <type> <filename>");
            return true;
        }
        if (strcmp(arg1, "proj") == 0) {
            if (arg2[0] == '\0') {
                // save current project settings to the specified file
                Serial.printf("Saving project settings to file %s...\n", arg2);
                project->save_project_settings();
                return true;
            } else {
                int desired_project_number = atoi(arg2);
                if (desired_project_number < 0) {
                    Serial.printf("Invalid project number: %i\n", desired_project_number);
                    return true;
                }
                Serial.printf("Saving project settings to slot %i...\n", desired_project_number);
                project->save_project_settings(desired_project_number);
                return true;
            }
        } else if (strcmp(arg1, "seq") == 0) {
            if (arg2[0] == '\0') {
                // save current sequence pattern to the specified file
                Serial.printf("Saving sequence pattern to file %s...\n", arg2);
                project->save_pattern();
                return true;
            } else {
                int desired_pattern_number = atoi(arg2);
                if (desired_pattern_number < 0 || desired_pattern_number >= NUM_PATTERN_SLOTS_PER_PROJECT) {
                    Serial.printf("Invalid slot number: %i\n", desired_pattern_number);
                    return true;
                }
                Serial.printf("Saving sequence pattern to slot %i...\n", desired_pattern_number);
                project->save_pattern(desired_pattern_number);
                return true;
            }
        } else {
            Serial.printf("Unknown type: %s\n", arg1);
            return true;
        }
    // } else if (strcmp(command, "loadline") == 0) {
    //     // load a line from serial input as if it were a line from a file, for testing parsing of 
    //     // pattern files without needing to write to the SD card. Usage: loadline <line_contents>
    //     if (arg1[0] == '\0') {
    //         Serial.println("Usage: loadline <type> <line_contents>");
    //         return true;
    //     }
    //     char line_buffer[512];
    //     if (arg2[0] == '\0') {
    //         Serial.println("Usage: loadline <type> <line_contents>");
    //         return true;
    //     }

    //     snprintf(line_buffer, sizeof(line_buffer), "%s", arg2);

    //     if (strcmp(arg1, "pattern") == 0) {
    //         // load a line of text as if it were a line from a pattern file, and output the result of parsing it
    //         Serial_printf("Parsing line as pattern line: '%s'\n", line_buffer);
    //         storage::load_pattern_parse_line(line_buffer, &storage::current_state, pass_debug);
    //         Serial_println("Done.");
    //     } else if (strcmp(arg1, "project") == 0) {
    //         // load a line of text as if it were a line from a project settings file, and output the result of parsing it
    //         Serial_printf("Parsing line as project settings line: '%s'\n", line_buffer);
    //         project->load_project_parse_line(line_buffer, pass_debug);
    //         Serial_println("Done.");
    //     } else {
    //         Serial.printf("Unknown type: %s\n", arg1);
    //     }
        
    //     return true;
    } else if (strcmp(command, "showtree") == 0) {
        // output the current settings tree as a text dump to the serial console, for debugging
        // usage: showtree [scopemask]
        uint8_t scopemask = 0xFF; // default to showing all scopes
        if (arg1[0] != '\0') {
            scopemask = (uint8_t)strtoul(arg1, nullptr, 0);
        }
        Serial.printf("Dumping settings tree (to depth 8 with scopemask=0x%02X):\n", scopemask);
        if (settings_root) {
            sl_print_tree_to_print(settings_root, Serial, 8, scopemask);
        } else {
            Serial.println("No settings root found!");
        }
        SL_TreeCounts counts = sl_count_tree(settings_root);
        Serial.printf(
            "There are %i settings and %i host nodes in the tree, occupying %i bytes (averaging %i bytes per setting).\n", 
            counts.settings, 
            counts.nodes, 
            counts.bytes,
            counts.settings > 0 ? counts.bytes / counts.settings : 0
        );
        return true;
    } else if (strcmp(command, "info") == 0) {
        Serial.println("Tyrell Corp Nexus6 USB Teensy Clocker");
        Serial.println("Build date: " __DATE__ " " __TIME__);
        Serial.println("Git commit: " COMMIT_INFO);
        Serial.println("Build env: " ENV_NAME);
        return true;
    } else if (strcmp(command, "help") == 0) {
        Serial.println("Available commands:");
        Serial.printf("  debug on/off - Enable or disable debug output (currently %s)\n", pass_debug ? "on" : "off");
        Serial.println("  ping - Check if the device is responsive");
        Serial.println("  status - Get the current status of the device");
        Serial.println("  reset - Reset the device");
        Serial.println("  free_ram - Check available RAM");
        Serial.println("  crashlog - Dump the crash report log (if any)");
        Serial.println("  clearcrashlog - Clear the crash report log (if any)");
        Serial.println("  showtree [scopemask] - Show the current settings tree (optional scopemask, outputs to depth 8)");
        Serial.println("  show - Show contents of a file");
        Serial.println("  load - Load a pattern, project, etc from a file on the SD card");
        Serial.println("  save - Save a pattern, project, etc to a file on the SD card");
        Serial.println("  [DISABLED WHILE UPGRADING TO SAVELOADLIB] loadline - Load a line of text as if it were from a pattern file (for testing parsing)");
        Serial.println("  info - show build info");
        Serial.println("  help - Show this help message");
        return true;
    }
    return false;
}

void update_serial() {

    if (Serial && !serial_connected) {
        Serial.println("[[[[[ Welcome to Intex Systems (help for help) ]]]]]");
        serial_connected = true;
    }

    // read data from serial connection, wait for a new line, and process buffer as a command

    while (Serial.available() > 0) {
        char in_char = Serial.read();
        //Serial.printf("Received char: %c\n", in_char); // debug print each received character
        if (in_char == '\n') {
            serial_buffer[serial_buffer_index] = '\0'; // null-terminate the string
            // process the command in serial_buffer
            //Serial.printf("Received command: %s\n", serial_buffer);
            serial_buffer_index = 0; // reset buffer index for next command

            if (!execute_command(serial_buffer)) {
                Serial.printf("Unknown command: %s\n", serial_buffer);
            }
        } else if (serial_buffer_index < sizeof(serial_buffer) - 1) {
            serial_buffer[serial_buffer_index++] = in_char; // add char to buffer
        } else {
            // buffer overflow, reset index and ignore input
            Serial.println("Serial buffer overflow, command too long!");
            serial_buffer_index = 0;
        }
    }
}