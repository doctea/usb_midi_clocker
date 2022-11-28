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
            Serial.printf("get_clip_for_id(%i) returning item at index %i of %i\n", id, id, this->clips->size());
            return this->clips->get(id);
        }

        void save_clips(int arrangement_number) {
            for (int i = 0 ; i < this->clips->size() ; i++) {
                if (this->clips->get(i)->is_dirty()) {
                    Serial.printf(F("TODO: save clip %i to disk in arrangemnet %i\n"), i, arrangement_number);
                    this->clips->get(i)->set_dirty(true);
                    // 
                }
            }
        }
        void load_clips() {
            // load all the available clip data from disk...
            // get directory listing
            // find all the clip files
            // load them into memory 
        }

        Clip *add_clip(Clip *clip) {
            clip->clip_id = this->get_new_clip_id();
            Serial.printf("add_clip adding clip at index %i with id %i\n", clips->size()-1, clip->clip_id);

            this->clips->add(clip);
            return clip;
        }

};


extern ClipManager *clip_manager;

void setup_clip_manager();

#endif