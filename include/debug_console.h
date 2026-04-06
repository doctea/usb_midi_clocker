#pragma once

#include <Arduino.h>
#include <ctype.h>

#include <debug.h>

#include "project.h"
#include "storage.h"

bool pass_debug = true;

char serial_buffer[1024];
unsigned int serial_buffer_index = 0;

bool serial_connected = false;

uint32_t list_files_in_directory(const char *path, int max_depth = 8, int current_depth = 0) {
    if (current_depth > max_depth) {
        Serial.printf("Max directory depth of %i reached at path: %s\n", max_depth, path);
        return 0;
    }
    File dir = SD.open(path);
    if (!dir) {
        Serial.printf("Error opening directory %s\n", path);
        return 0;
    }
    uint32_t total_size = 0;
    while (File entry = dir.openNextFile()) {
        for (int i = 0; i < current_depth; i++)
            Serial.print("\t");
        Serial.printf("%s%s (%lu bytes)\n", entry.name(), entry.isDirectory() ? "/" : "", entry.size());
        total_size += entry.size();
        if (entry.isDirectory()) {
            char subpath[256];
            snprintf(subpath, sizeof(subpath), "%s/%s", path, entry.name());
            total_size += list_files_in_directory(subpath, max_depth, current_depth + 1);
        }
    }
    return total_size;
}

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
        Serial.printf("Current project: %i, selected scene: %i\n", project->current_project_number, project->selected_scene_number);
        Serial.printf("Free RAM: %i bytes\n", freeRam());
        Serial.printf("SD card present: %s\n", SD.begin(BUILTIN_SDCARD) ? "yes" : "no");

        SL_TreeCounts sl_tree_counts_all = sl_count_tree(project->save_tree);
        Serial.printf("Savetree counts - nodes: %lu, settings: %lu, bytes: %lu\n", sl_tree_counts_all.nodes, sl_tree_counts_all.settings, sl_tree_counts_all.bytes);
        SL_TreeCounts sl_tree_counts_scene = sl_count_tree(project->save_tree, false, SL_SCOPE_SCENE);
        SL_TreeCounts sl_tree_counts_project = sl_count_tree(project->save_tree, false, SL_SCOPE_PROJECT);
        SL_TreeCounts sl_tree_counts_routing = sl_count_tree(project->save_tree, false, SL_SCOPE_ROUTING);
        SL_TreeCounts sl_tree_counts_system = sl_count_tree(project->save_tree, false, SL_SCOPE_SYSTEM);
        Serial.printf("Savetree counts by scope:\n");
        Serial.printf("  SCENE - nodes: %lu, settings: %lu, bytes: %lu\n", sl_tree_counts_scene.nodes, sl_tree_counts_scene.settings, sl_tree_counts_scene.bytes);
        Serial.printf("  PROJECT - nodes: %lu, settings: %lu, bytes: %lu\n", sl_tree_counts_project.nodes, sl_tree_counts_project.settings, sl_tree_counts_project.bytes);
        Serial.printf("  ROUTING - nodes: %lu, settings: %lu, bytes: %lu\n", sl_tree_counts_routing.nodes, sl_tree_counts_routing.settings, sl_tree_counts_routing.bytes);
        Serial.printf("  SYSTEM - nodes: %lu, settings: %lu, bytes: %lu\n", sl_tree_counts_system.nodes, sl_tree_counts_system.settings, sl_tree_counts_system.bytes);

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
    } else if (strcmp(command, "list") == 0) {
        // list files and filesize on SD card in given project folder
        // recurse into folders if possible (up to 2 levels deep?) and print files with relative path from project folder
        if (arg1[0] == '\0') {
            Serial.println("Usage: list <project_number>");
            return true;
        }
        int project_number = atoi(arg1);
        if (project_number < 0) {
            Serial.printf("Invalid project number: %i\n", project_number);
            return true;
        }
        char path[MAX_FILEPATH];
        snprintf(path, MAX_FILEPATH, "project%i", project_number);
        Serial.printf("Files in %s:\n", path);

        uint32_t total_size = list_files_in_directory(path, 2, 0);

        Serial.printf("Total storage used in project %i: %lu bytes\n", project_number, total_size);

        return true;
    } else if (strcmp(command, "set") == 0) {
        // switch to project number or scene number
        if (arg1[0] == '\0' || arg2[0] == '\0') {
            Serial.println("Usage: set <project|scene> <number>");
            return true;
        }
        int number = atoi(arg2);
        if (strcmp(arg1, "project") == 0) {
            if (number < 0) {
                Serial.printf("Invalid project number: %i\n", number);
                return true;
            }
            project->setProjectNumber(number);
            Serial.printf("Current project set to %i\n", number);
            return true;
        } else if (strcmp(arg1, "scene") == 0) {
            if (number < 0 || number >= NUM_SCENE_SLOTS_PER_PROJECT) {
                Serial.printf("Invalid scene number: %i\n", number);
                return true;
            }
            project->selected_scene_number = number;
            Serial.printf("Selected scene set to %i\n", number);
            return true;
        } else {
            Serial.printf("Unknown type: %s\n", arg1);
            return true;
        }
    } else if (strcmp(command, "show") == 0) {
        if (arg1[0] == '\0') {
            Serial.println("Usage: show <type> <slot_number>");
            return true;
        }
        char *filename = nullptr;

        if (strcmp(arg1, "scene") == 0) {
            if (arg2[0] == '\0') {
                Serial.println("Usage: show scene <slot_number> (using currently selected slot if slot_number not provided)");
                // show current scene
                sprintf(arg2, "%i", project->selected_scene_number); // default to slot 0 if no slot number provided
                return false;
            }

            int desired_scene_number = atoi(arg2);
            if (desired_scene_number < 0 || desired_scene_number >= NUM_SCENE_SLOTS_PER_PROJECT) {
                Serial.printf("Invalid slot number: %i\n", desired_scene_number);
                return true;
            }
            if (project->is_selected_scene_number_empty(desired_scene_number)) {
                Serial.printf("No scene file found for slot %i\n", desired_scene_number);
                return true;
            }
            // read the appropriate file from disc and output its contents to serial
            filename = storage::get_scene_filename(project->current_project_number, desired_scene_number);
        } else if (strcmp(arg1, "proj") == 0) {
            // show specified project settings

            if (arg2[0] == '\0') {
                Serial.println("Usage: show proj <project_number>");
                return false;
            }

            int desired_project_number = atoi(arg2);
            if (desired_project_number < 0) {
                Serial.printf("Invalid project number: %i\n", desired_project_number);
                return true;
            }
            
            filename = storage::get_project_settings_filename(desired_project_number);
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
        // load a scene, project, etc from a file on the SD card. Usage: load <type> <filename>
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
        } else if (strcmp(arg1, "scene") == 0) {
            // load a scene from the specified file
            Serial.printf("Loading scene from file %s...\n", arg2);

            int desired_scene_number = atoi(arg2);
            if (desired_scene_number < 0 || desired_scene_number >= NUM_SCENE_SLOTS_PER_PROJECT) {
                Serial.printf("Invalid slot number: %i\n", desired_scene_number);
                return true;
            }
            if (project->is_selected_scene_number_empty(desired_scene_number)) {
                Serial.printf("No scene file found for slot %i\n", desired_scene_number);
                return true;
            }

            Serial.printf("Loading scene from file %i...\n", desired_scene_number);
            project->load_scene(desired_scene_number, pass_debug);

            return true;
        } else {
            Serial.printf("Unknown type: %s\n", arg1);
            return true;
        }
    } else if (strcmp(command, "save") == 0) {
        // save a scene, project, etc to a file on the SD card. Usage: save <type> <filename>
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
        } else if (strcmp(arg1, "scene") == 0) {
            if (arg2[0] == '\0') {
                // save current scene to the specified file
                Serial.printf("Saving scene to file %s...\n", arg2);
                project->save_scene();
                return true;
            } else {
                int desired_scene_number = atoi(arg2);
                if (desired_scene_number < 0 || desired_scene_number >= NUM_SCENE_SLOTS_PER_PROJECT) {
                    Serial.printf("Invalid slot number: %i\n", desired_scene_number);
                    return true;
                }
                Serial.printf("Saving scene to slot %i...\n", desired_scene_number);
                if (!project->save_scene(desired_scene_number)) {
                    Serial.printf("Error saving scene to slot %i\n", desired_scene_number);
                } else {
                    Serial.printf("Saved scene to slot %i\n", desired_scene_number);
                }
                return true;
            }
        } else {
            Serial.printf("Unknown type: %s\n", arg1);
            return true;
        }
    // } else if (strcmp(command, "loadline") == 0) {
    //     // load a line from serial input as if it were a line from a file, for testing parsing of 
    //     // scene files without needing to write to the SD card. Usage: loadline <line_contents>
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

    //     if (strcmp(arg1, "scene") == 0) {
    //         // load a line of text as if it were a line from a scene file, and output the result of parsing it
    //         Serial_printf("Parsing line as scene line: '%s'\n", line_buffer);
    //         storage::load_scene_parse_line(line_buffer, &storage::current_state, pass_debug);
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
            // if argument doesn't start with a number, assume SL_SCOPE_SCENE, SL_SCOPE_PROJECT,
            // and parse using sl_scope_from_string, otherwise parse as number
            if (isdigit((unsigned char)arg1[0])) {
                scopemask = (uint8_t)strtoul(arg1, nullptr, 0);
            } else {
                scopemask = sl_scope_from_string(arg1);
            }
        }
        Serial.printf("Dumping settings tree (to depth 8 with scopemask=0x%02X aka %s):\n", scopemask, sl_scope_to_string(scopemask));
        if (settings_root) {
            sl_print_tree_to_print(settings_root, Serial, 8, scopemask);
        } else {
            Serial.println("No settings root found!");
        }

        // output available scopes
        Serial.print("Available scopes:\t");
        for (size_t i = 0 ; i < SL_SCOPE_ENTRY_COUNT ; i++) {
            Serial.printf("  %s (0x%02X)\t", sl_scope_to_string(1 << i), 1 << i);
        }
        Serial.println();

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
        Serial.println("  list <project_number> - List files on the SD card for the given project");
        Serial.println("  set project/scene <number> - Set the current project or scene number");
        Serial.println("  show <scene|proj> <number> - Show the contents of a scene or project settings file");
        Serial.println("  save <scene|proj> [number] - Save the current scene or project settings to the given slot number (or current if not provided)");
        Serial.println("  load <scene|proj> <number> - Load a scene or project settings from the given slot number");
        Serial.println("  [DISABLED WHILE UPGRADING TO SAVELOADLIB] loadline - Load a line of text as if it were from a scene file (for testing parsing)");
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