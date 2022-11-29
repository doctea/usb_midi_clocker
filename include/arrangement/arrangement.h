#ifndef ARRANGEMENT__INCLUDED
#define ARRANGEMENT__INCLUDED

#include <LinkedList.h>
#include "clips/clips.h"
#include "clips/clip_manager.h"

#define MAX_TRACK_NAME 20

struct clip_instance_t {
    int position;	// which bar/phrase number the specified clip should be used at
    int clip_id;	// project-specific clip number - where to save it on disk
    Clip *clip;	// pointer to the clip in memory
};

class ArrangementTrackBase {
	public:

	char label[MAX_TRACK_NAME];
	int current_song_position = -1;

	ArrangementTrackBase(const char *label) {
		strncpy(this->label, label, MAX_TRACK_NAME);
	}

	// insert a clip at specified song postion; return the index
	virtual int insert_clip_instance(int song_position, Clip *clip);
	//virtual void replace_clip_instance(int song_position, Clip *clip);
	virtual LinkedList<clip_instance_t*> *get_clips_at_time(int song_position);

	virtual bool is_active_for_phrase(int song_position);

	virtual void on_tick(uint32_t tick);
	virtual void on_bar(uint32_t bar);
	virtual void on_phrase(int phrase);

	virtual void debug_track();
};


class ArrangementMultipleTrack : public ArrangementTrackBase {
	public:
	LinkedList<clip_instance_t> *song_structure = new LinkedList<clip_instance_t>();

	int cached_song_position = -1;
	LinkedList<clip_instance_t*> *current_clips = new LinkedList<clip_instance_t*>();

    LinkedList<clip_instance_t*> *clips_at_phrase = new LinkedList<clip_instance_t*>();

	int playhead_index = 0;	// where our internal pointer is currently pointing to

	ArrangementMultipleTrack(const char *label) : ArrangementTrackBase(label) {}

	// 	insert a clip_instance_t at the correct song position
	virtual int insert_clip_instance(int song_position, Clip *clip) override {
		clip_instance_t instance = {
			.position = song_position,
            .clip_id = clip->clip_id,
			.clip = clip
		};
		int index = this->find_index_for_position(song_position);
		song_structure->add(index, instance);
		return index;
	}

	int find_index_for_position(int song_position) {
		// todo: optimise to search forwards/backwards based on requested position's relation to known current position
		for (int i = 0 ; i < song_structure->size() ; i++) {
			if (song_structure->get(i).position >= song_position)
				return i;
		}
		return song_structure->size();
	}

	virtual bool is_active_for_phrase(int song_position) override {
		int index = find_index_for_position(song_position);
        Serial.printf("find_index_for_position(%i) got index %i\n", song_position, index);
		return index < song_structure->size() && song_structure->get(index).position==song_position;
	}

    virtual LinkedList<clip_instance_t*> *get_clips_at_time(int song_position) override {
		if (cached_song_position!=song_position) {
			this->clips_at_phrase->clear();
			int index = find_index_for_position(song_position);
			//Serial.printf("get_clips_at_time(%i)\n", song_position); Serial.flush();
			for (int i = index ; i < this->song_structure->size() && song_structure->get(i).position == song_position ; i++) {
				//Serial.printf("get_clips_at_time(%i) got one at %i\n", song_position, i);
				clips_at_phrase->add(&this->song_structure->get(i));
			}
			this->cached_song_position = song_position;
		}
        return this->clips_at_phrase;
    }

	virtual void on_tick(uint32_t tick) override {
		LinkedList<clip_instance_t*> *clips = get_clips_at_time(current_song_position);
		for (int i = 0 ; i < clips->size() ; i++) {
			this->current_clips->get(i)->clip->on_tick(tick);
		}
	}
	virtual void on_bar(uint32_t bar) override {
		LinkedList<clip_instance_t*> *clips = get_clips_at_time(current_song_position);
		for (int i = 0 ; i < clips->size() ; i++) {
			this->current_clips->get(i)->clip->on_bar(bar);
		}
	}

	virtual void on_phrase(int phrase_number) override {
        playhead_index = find_index_for_position(phrase_number);
		current_song_position = phrase_number;
        //Serial.printf("Arrangement#on_phrase(%i) found playhead_index %i\n", phrase_number, playhead_index);
		LinkedList<clip_instance_t*> *clips = get_clips_at_time(phrase_number);
		for (int i = 0 ; i < clips->size() ; i++) {
            Serial.printf("ArrrangementMultipleTrack#on_phrase(%i) processing clip %i\n", phrase_number, i);
			clips->get(i)->clip->on_phrase(phrase_number);
		}
	}

    virtual void debug_track() override {
        Serial.println("debug_arrangement:");
        for (int i = 0 ; i < song_structure->size() ; i++) {
            Serial.printf("\tindex %i:\tclip_id=%i,\tposition=%i,\tclip->clip_id=%i\n", i, song_structure->get(i).clip_id, song_structure->get(i).position, song_structure->get(i).clip->clip_id);
        }
        Serial.println("-----");
    }

	// have some helper functions to:-
	//  find the next time that a clip is queued for
	// 	return all clip_instance_ts that are queued for a particular song position
	// 	insert a clip_instance_t at the correct position
	// 	generate a GUID for a clip to use as the clip_id?
};

class ArrangementSingleTrack : public ArrangementMultipleTrack {
	public:

	ArrangementSingleTrack(const char *label) : ArrangementMultipleTrack(label) {}

	virtual int replace_clip_instance(int song_position, Clip *clip) {
		// todo: use LinkedList 'set' instead to overwrite existing data instead of forcing relinking of list
		int index = ArrangementMultipleTrack::insert_clip_instance(song_position, clip);
		song_structure->remove(index+1);
		return index; 
	}
	virtual int insert_clip_instance(int song_position, Clip *clip) override {
		return this->replace_clip_instance(song_position, clip);
	}
};



class Arrangement {
	public:
		Arrangement() {}

		int current_song_phrase = -1;

		LinkedList<ArrangementTrackBase*> *tracks = new LinkedList<ArrangementTrackBase*> ();
		LinkedList<ArrangementTrackBase*> *current_tracks = new LinkedList<ArrangementTrackBase*> ();

		ArrangementTrackBase* addTrack(ArrangementTrackBase* track) {
			this->tracks->add(track);
			return track;
		}

		LinkedList<ArrangementTrackBase*> *get_active_tracks(int current_song_phrase) {
            Serial.printf("get_active_tracks(%i)...\n", current_song_phrase);
			//if (this->current_song_phrase!=current_song_phrase) {
				this->current_tracks->clear();
				for (int i = 0 ; i < tracks->size() ; i++) {
                    Serial.printf("\tchecking track %s..", tracks->get(i)->label);
					if (tracks->get(i)->is_active_for_phrase(current_song_phrase)) {
                        Serial.print("is_active!");
						current_tracks->add(tracks->get(i));
                    }
                    Serial.println();
				}
			//}

			return this->current_tracks;
		}

		LinkedList<ArrangementTrackBase*> *get_tracks() {
			return this->tracks;
		}

		void on_tick(uint32_t ticks) {
			if (current_song_phrase<0) return;

			LinkedList<ArrangementTrackBase*> *current_tracks = get_active_tracks(current_song_phrase);
			for (int i = 0 ; i < current_tracks->size() ; i++) {
				current_tracks->get(i)->on_tick(ticks);
			}
		}

		void on_bar(int bar_number) {
			if (current_song_phrase<0) return;

			LinkedList<ArrangementTrackBase*> *current_tracks = get_active_tracks(current_song_phrase);
			for (int i = 0 ; i < current_tracks->size() ; i++) {
				current_tracks->get(i)->on_bar(bar_number);
			}
		}

		void on_phrase(int phrase_number) {
            current_song_phrase = phrase_number;
			if (current_song_phrase<0) return;

			LinkedList<ArrangementTrackBase*> *current_tracks = get_active_tracks(current_song_phrase);
            Serial.printf("Arrangement#on_phrase(%i) got %i tracks\n", phrase_number, current_tracks->size());

			for (int i = 0 ; i < current_tracks->size() ; i++) {
				current_tracks->get(i)->on_phrase(phrase_number);
			}
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
		void debug_arrangement() {
			Serial.println("debug_arrangement:");
			for (int i = 0 ; i < tracks->size() ; i++) {
				tracks->get(i)->debug_track();
			}
			Serial.println("-----");
		}
};

extern Arrangement *arrangement;
extern ClipManager *clip_manager;

void setup_arrangement();

#endif
