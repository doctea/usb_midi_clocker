#ifndef ARRANGEMENT__INCLUDED
#define ARRANGEMENT__INCLUDED

#include "Config.h"

#include <LinkedList.h>
#include "clips/clips.h"
#include "clips/clip_manager.h"

#include "colours.h"

#define MAX_TRACK_NAME 20

struct clip_instance_t {
    unsigned int position;	// which bar/phrase number the specified clip should be used at
    unsigned int clip_id;	// project-specific clip number - where to save it on disk
    Clip *clip;	// pointer to the clip in memory
};

class ArrangementTrackBase {
	public:

	char label[MAX_TRACK_NAME];
	int current_song_position = -1;
    uint16_t colour = C_WHITE;

	ArrangementTrackBase(const char *label) {
		strncpy(this->label, label, MAX_TRACK_NAME);
	}
    ~ArrangementTrackBase() = default;

	// insert a clip at specified song postion; return the index
	virtual int insert_clip_instance(unsigned int song_position, Clip *clip) = 0;
	//virtual void replace_clip_instance(int song_position, Clip *clip);
	virtual LinkedList<clip_instance_t*> *get_clips_at_time(unsigned int song_position) = 0;

	virtual bool is_active_for_phrase(unsigned int song_position) = 0;

	virtual void on_tick(uint32_t tick) = 0;
	virtual void on_bar(uint32_t bar) = 0;
	virtual void on_phrase(int phrase) = 0;

	virtual void debug_track() = 0;


	virtual int get_num_lanes() = 0;
	virtual clip_instance_t *get_clip_at_time_lane(unsigned int phrase_number, unsigned int lane_number) = 0;

};

class ArrangementMultipleTrack : public ArrangementTrackBase {
	public:
	LinkedList<clip_instance_t> *song_structure = new LinkedList<clip_instance_t>();

	int cached_song_position = -1;
	LinkedList<clip_instance_t*> *current_clips = new LinkedList<clip_instance_t*>();

    LinkedList<clip_instance_t*> *clips_at_phrase = new LinkedList<clip_instance_t*>();

	int playhead_index = 0;	// where our internal pointer is currently pointing to

	ArrangementMultipleTrack(const char *label) : ArrangementTrackBase(label) {}
	~ArrangementMultipleTrack() = default;

	// 	insert a clip_instance_t at the correct song position
	virtual int insert_clip_instance(unsigned int song_position, Clip *clip) override {
		clip_instance_t instance = {
			.position = song_position,
            .clip_id = clip->clip_id,
			.clip = clip
		};
		int index = this->find_index_for_position(song_position);
		song_structure->add(index, instance);
		return index;
	}

	int find_index_for_position(unsigned int song_position) {
		// todo: optimise to search forwards/backwards based on requested position's relation to known current position
		for (unsigned int i = 0 ; i < song_structure->size() ; i++) {
			if (song_structure->get(i).position >= song_position)
				return i;
		}
		return song_structure->size();
	}

	virtual bool is_active_for_phrase(unsigned int song_position) override {
		unsigned int index = find_index_for_position(song_position);
        //Serial.printf("find_index_for_position(%i) got index %i\n", song_position, index);
		return index < song_structure->size() && song_structure->get(index).position==song_position;
	}

	virtual int get_num_lanes() {
		return 1;
	}
	virtual clip_instance_t *get_clip_at_time_lane(unsigned int phrase_number, unsigned int lane_number) {
		return this->get_clips_at_time(phrase_number)->get(lane_number);
	}

    virtual LinkedList<clip_instance_t*> *get_clips_at_time(unsigned int song_position) override {
		if (cached_song_position!=(int)song_position) {
			this->clips_at_phrase->clear();
			unsigned int index = find_index_for_position(song_position);
			//Serial.printf("get_clips_at_time(%i)\n", song_position); Serial.flush();
			for (unsigned int i = index ; i < this->song_structure->size() && song_structure->get(i).position == song_position ; i++) {
				//Serial.printf("get_clips_at_time(%i) got one at %i\n", song_position, i);
				clips_at_phrase->add(&this->song_structure->get(i));
			}
			this->cached_song_position = song_position;
		}
        return this->clips_at_phrase;
    }

	virtual void on_tick(uint32_t tick) override {
		LinkedList<clip_instance_t*> *clips = get_clips_at_time(current_song_position);
		for (unsigned int i = 0 ; i < clips->size() ; i++) {
			this->current_clips->get(i)->clip->on_tick(tick);
		}
	}
	virtual void on_bar(uint32_t bar) override {
		LinkedList<clip_instance_t*> *clips = get_clips_at_time(current_song_position);
		for (unsigned int i = 0 ; i < clips->size() ; i++) {
			this->current_clips->get(i)->clip->on_bar(bar);
		}
	}

	virtual void on_phrase(int current_song_phrase) override {
		current_song_position = current_song_phrase;
        playhead_index = find_index_for_position(current_song_position);
		 //phrase_number;
        //Serial.printf("Arrangement#on_phrase(%i) found playhead_index %i\n", phrase_number, playhead_index);
		LinkedList<clip_instance_t*> *clips = get_clips_at_time(current_song_position);
		for (unsigned int i = 0 ; i < clips->size() ; i++) {
            //Serial.printf("ArrrangementMultipleTrack#on_phrase(%i) processing clip %i\n", phrase_number, i);
			clips->get(i)->clip->on_phrase(current_song_position);
		}
	}

    virtual void debug_track() override {
        Serial.println(F("debug_arrangement:"));
        for (unsigned int i = 0 ; i < song_structure->size() ; i++) {
            Serial.printf(F("\tindex %i:\tclip_id=%i,\tposition=%i,\tclip->clip_id=%i\n"), i, song_structure->get(i).clip_id, song_structure->get(i).position, song_structure->get(i).clip->clip_id);
        }
        Serial.println(F("-----"));
    }

	// have some helper functions to:-
	//  find the next time that a clip is queued for
	// 	return all clip_instance_ts that are queued for a particular song position
	// 	insert a clip_instance_t at the correct position
	// 	generate a GUID for a clip to use as the clip_id?
};

class ArrangementSingleTrack : public ArrangementMultipleTrack {
	public:

	~ArrangementSingleTrack() = default;
    ArrangementSingleTrack(const char *label) : ArrangementMultipleTrack(label) {}
	ArrangementSingleTrack(const char *label, uint16_t colour) : ArrangementSingleTrack(label) {
        this->colour = colour;
    }

	virtual int replace_clip_instance(int song_position, Clip *clip) {
		// todo: use LinkedList 'set' instead to overwrite existing data instead of forcing relinking of list
		int index = ArrangementMultipleTrack::insert_clip_instance(song_position, clip);
		song_structure->remove(index+1);
		return index; 
	}
	virtual int insert_clip_instance(unsigned int song_position, Clip *clip) override {
		return this->replace_clip_instance(song_position, clip);
	}
};



class Arrangement {
	public:
		Arrangement() {}

		bool looping = false, playing = true;
		int current_song_phrase = -1;

		LinkedList<ArrangementTrackBase*> *tracks = new LinkedList<ArrangementTrackBase*> ();
		LinkedList<ArrangementTrackBase*> *current_tracks = new LinkedList<ArrangementTrackBase*> ();

		ArrangementTrackBase* addTrack(ArrangementTrackBase* track) {
            if (track!=nullptr)
			    this->tracks->add(track);
			return track;
		}

		LinkedList<ArrangementTrackBase*> *get_active_tracks(int current_song_phrase) {
            //Serial.printf(F("get_active_tracks(%i)...\n"), current_song_phrase);
			//if (this->current_song_phrase!=current_song_phrase) {
				this->current_tracks->clear();
				for (unsigned int i = 0 ; i < tracks->size() ; i++) {
                    //Serial.printf(F("\tchecking track %s.."), tracks->get(i)->label);
					if (tracks->get(i)->is_active_for_phrase(current_song_phrase)) {
                        //Serial.print(F("is_active!"));
						current_tracks->add(tracks->get(i));
                    }
                    //Serial.println();
				}
			//}

			return this->current_tracks;
		}

		LinkedList<ArrangementTrackBase*> *get_tracks() {
			return this->tracks;
		}

		void on_tick(uint32_t ticks) {
			if (!playing)
				return;
			if (current_song_phrase<0) 
				return;

			LinkedList<ArrangementTrackBase*> *current_tracks = get_active_tracks(current_song_phrase);
			for (unsigned int i = 0 ; i < current_tracks->size() ; i++) {
				current_tracks->get(i)->on_tick(ticks);
			}
		}

		void on_bar(int bar_number) {
			if (!playing)
				return;
			if (current_song_phrase<0) 
				return;

			LinkedList<ArrangementTrackBase*> *current_tracks = get_active_tracks(current_song_phrase);
			for (unsigned int i = 0 ; i < current_tracks->size() ; i++) {
				current_tracks->get(i)->on_bar(bar_number);
			}
		}

		void on_phrase(int phrase_number) {
			//if (current_song_phrase<0) current_song_phrase = 0;
			if (!this->playing)
				return;
			if (!this->looping)
            	this->current_song_phrase++; // = phrase_number;
			if (this->current_song_phrase<0) 
			 	current_song_phrase = 0;
				//return;

			Serial.printf(F("Arrangement#on_phrase(%i) with current_song_phrase=%i\n"), phrase_number, current_song_phrase);

			LinkedList<ArrangementTrackBase*> *current_tracks = get_active_tracks(current_song_phrase);
            Serial.printf(F("Arrangement#on_phrase(%i) got %i tracks\n"), phrase_number, current_tracks->size());

			for (unsigned int i = 0 ; i < current_tracks->size() ; i++) {
				current_tracks->get(i)->on_phrase(current_song_phrase);
			}
		}

		void on_restart() {
			this->current_song_phrase = 0; //-1;
		}

		/*void save_arrangement(int arrangement_number) {
			clip_manager->save_clips(arrangement_number);
			for (unsigned int i = 0 ; i < song_structure->size() ; i++) {
				clip_instance_t *c = &song_structure->get(i);
				Serial.printf(F("TODO: write clip data for arrangement %i: clip_id=%i, song_position=%i\n"), arrangement_number, c->clip->clip_id, c->position);
			}
		}
		void load_arrangement(int arrangement_number) {
			Serial.printf(F("TODO: load arrangement %i\n"), arrangement_number);
			//
		}*/
		void debug_arrangement() {
			Serial.println(F("debug_arrangement:"));
			for (unsigned int i = 0 ; i < tracks->size() ; i++) {
				tracks->get(i)->debug_track();
			}
			Serial.println(F("-----"));
		}
};

extern Arrangement *arrangement;
extern ClipManager *clip_manager;

void setup_arrangement();

#endif
