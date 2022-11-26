#ifndef ARRANGEMENT__INCLUDED
#define ARRANGEMENT__INCLUDED
/*// sketching out struct for an arrangement/playlist editor
#include "clips/clips.h"

struct clip_instance_t {
    clip_t *clip;

};
*/

#include <LinkedList.h>
#include "clips/clips.h"
#include "clips/clip_manager.h"

struct clip_instance_t {
    int position;	// which bar/phrase number the specified clip should be used at
    int clip_id;	// project-specific clip number - where to save it on disk
    Clip *clip;	// pointer to the clip in memory
};

class Arrangement {
	public:

	LinkedList<clip_instance_t> *song_structure = new LinkedList<clip_instance_t>();
	LinkedList<clip_instance_t*> *current_clips = new LinkedList<clip_instance_t*>();

    LinkedList<clip_instance_t*> *clips_at_phrase = new LinkedList<clip_instance_t*>();

	int playhead_index = 0;	// where our internal pointer is currently pointing to

	// 	insert a clip_instance_t at the correct song position
	void insert_clip_instance(int song_position, Clip *clip) {
		clip_instance_t instance = {
			.position = song_position,
            .clip_id = clip->clip_id,
			.clip = clip
		};
		int index = this->find_index_for_position(song_position);
		song_structure->add(index, instance);
	}

	int find_index_for_position(int song_position) {
		for (int i = 0 ; i < song_structure->size() ; i++) {
			if (song_structure->get(i).position >= song_position)
				return i;
		}
		return song_structure->size();
	}

    LinkedList<clip_instance_t*> *get_clips_at_time(int song_position) {
        this->clips_at_phrase->clear();
        int index = find_index_for_position(song_position);
        //Serial.printf("get_clips_at_time(%i)\n", song_position); Serial.flush();
        for (int i = index ; i < this->song_structure->size() && song_structure->get(i).position == song_position ; i++) {
            //Serial.printf("get_clips_at_time(%i) got one at %i\n", song_position, i);
            clips_at_phrase->add(&this->song_structure->get(i));
        }
        return this->clips_at_phrase;
    }

	void on_tick(uint32_t tick) {
		for (int i = 0 ; i < current_clips->size() ; i++) {
			this->current_clips->get(i)->clip->on_tick(tick);
		}
	}
	void on_bar(uint32_t bar) {
		for (int i = 0 ; i < current_clips->size() ; i++) {
			this->current_clips->get(i)->clip->on_bar(bar);
		}
	}

	void on_phrase(int phrase_number) {
        playhead_index = find_index_for_position(phrase_number);
        Serial.printf("Arrangement#on_phrase(%i) fonud playhead_index %i\n", phrase_number, playhead_index);
		current_clips->clear();
		// find the clips that should be played at this point
		int i = 0;
		for (i = playhead_index ; i < song_structure->size() ; i++) {
            Serial.printf("\t at index %i, found one with position %i\n", i, song_structure->get(i).position);
			if (song_structure->get(i).position == phrase_number) {
				current_clips->add(&song_structure->get(i));
				// this is a clip to load/process
				Clip *item = song_structure->get(i).clip;
				item->prepare();
			} else if (song_structure->get(i).position > phrase_number) {
                break;
            }
		}
        playhead_index = i;
	}

    void debug_arrangement() {
        Serial.println("debug_arrangement:");
        for (int i = 0 ; i < song_structure->size() ; i++) {
            Serial.printf("\tindex %i:\tclip_id=%i,\tposition=%i,\tclip->clip_id=%i\n", i, song_structure->get(i).clip_id, song_structure->get(i).position, song_structure->get(i).clip->clip_id);
        }
        Serial.println("-----");
    }

	/*void save_arrangement(int arrangement_number) {
		clip_manager->save_clips(arrangement_number);
		for (int i = 0 ; i < song_structure->size() ; i++) {
			clip_instance_t *c = &song_structure->get(i);
			Serial.printf(F("TODO: write clip data for arrangement %i: clip_id=%i, song_position=%i\n"), arrangement_number, c->clip->clip_id, c->position);
		}
	}
	void load_arrangement(int arrangement_number) {
        Serial.printf(F("TODO: load arrangement %i\n"), arrangement_number);
		//
	}*/

	// have some helper functions to:-
	//  find the next time that a clip is queued for
	// 	return all clip_instance_ts that are queued for a particular song position
	// 	insert a clip_instance_t at the correct position
	// 	generate a GUID for a clip to use as the clip_id?
};
;
extern Arrangement *arrangement;
extern ClipManager *clip_manager;

void setup_arrangement();

#endif