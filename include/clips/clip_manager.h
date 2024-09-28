#ifndef CLIP_MANAGER__INCLUDED
#define CLIP_MANAGER__INCLUDED

#include <Arduino.h>
#include <LinkedList.h>
#include "clips.h"

class ClipManager {
    public:
        ClipManager() {}

        LinkedList<Clip*> *clips = new LinkedList<Clip*>();

        // load and save all available clips to disk..?
        // how are we going to ensure that this stay the same between loads..?
        // just index them per-project i guess.

        int clip_id_count = -1;
        int get_new_clip_id() {
            return ++clip_id_count;
        }

        Clip *get_clip_for_id(int id) {
            //Serial.printf("get_clip_for_id(%i) returning item at index %i of %i\n", id, id, this->clips->size());
            return this->clips->get(id);
        }

        // TODO: save clips from disk - maybe move this to be handled by the tracks
        void save_clips(int arrangement_number = 0) {            
            for (unsigned int i = 0 ; i < this->clips->size() ; i++) {
                Clip *c = this->clips->get(i);
                if (c->is_dirty()) {
                    Serial.printf(F("TODO: save clip %i to disk in arrangement %i\n"), i, arrangement_number);
			        char filepath[255];
                    snprintf(filepath, 255, FILEPATH_CLIP_FILE_FORMAT, project->current_project_number, c->type, c->clip_id);
                    save_file(filepath, c);
                }
            }
        }
        // TODO: load clips from disk - maybe move this to be handled by the tracks
        void load_clips(int arrangement_number = 0) {
            // load all the available clip data from disk...
            // get directory listing
            char path[255];
            snprintf(path, 255, FILEPATH_CLIP_FOLDER_FORMAT, project->current_project_number);
            File clip_folder = SD.open(path);
            if (clip_folder && clip_folder.isDirectory()) {
                // find all the clip files
                File entry = clip_folder.openNextFile();
                while (entry) {
                    // determine the type of clip to create
                    String s = String(entry.name());
                    s = s.replace("clip_","");
                    String clip_type = s.substring(0, s.indexOf('_'));
                    int clip_id = s.substring(s.indexOf('_')+1).toInt();

                    // load them into memory
                    Clip *clip = this->instantiate_clip(clip_type, clip_id);
                    this->clips->add(clip);
                }
            }
        }

        Clip *instantiate_clip(String clip_type, int clip_id) {
            if (clip_type.equals("SequenceClip")) {
                return new SequenceClip();
            } // else if ...
            // TODO: cover other types..
            return nullptr;
        }

        // todo: test whether this even works
        void clear_clips() {
            for (int i = 0; i < this->clips->size() ; i++) {
                delete this->clips->remove(i);
            }
            this->clips->clear();
        }

        Clip *add_clip(Clip *clip) {
            if (clip->clip_id==-1)
                clip->clip_id = this->get_new_clip_id();
            Serial.printf(F("add_clip adding clip at index %i with id %i\n"), clips->size()-1, clip->clip_id);

            this->clips->add(clip);
            
            return clip;
        }

};


extern ClipManager *clip_manager;

void setup_clip_manager();

#endif