#ifndef CLIPS__INCLUDED
#define CLIPS__INCLUDED

// thinking about how to restructure things to allow better save+recall of settings / song chaining
// Project contains possible_clips; which ones to be used when queued by an "Arrangement";
// Arrangement is responsible for fetching/writing list of Clips;
// Clip is responsible for recalling + writing its values and telling the Sequence, Parameter, Behaviour what it needs to update...
// Think we need to make Behaviour options into something that can be queried programmatically, ie, something like Parameters that can be saved/loaded
// 	so Parameter knows its own datatype; perhaps we have a ParameterLoader/ParameterSaver type that knows how to read/write a key/value pair and set values accordingly

#include <Arduino.h>
#include <LinkedList.h>

#include "project.h"

// abstract base Clip class
class Clip {
	public:
		Clip() {};
		~Clip() = default;
		int clip_id = -1;

		bool dirty = false;

		//virtual void prepare() = 0;
		//virtual void start() = 0;
		virtual void on_phrase(int phrase) = 0;
		virtual void on_tick(uint32_t tick) = 0;
		virtual void on_bar(uint32_t bar) = 0;
		virtual void set_dirty(bool v = true) {
			this->dirty = v;
		}
		virtual bool is_dirty() {
			return dirty;
		}
};

// a Clip that contains a sequence/clock information
class SequenceClip : public Clip {
	public:
		int sequence_number; // the sequence number to load when this clip played

		SequenceClip(int sequence_number) {
			this->sequence_number = sequence_number;
		}

		/*virtual void prepare() {
			Serial.printf("SequenceClip clip_id=%i loading sequence_number %i!\n", clip_id, sequence_number);
			project.load_sequence(sequence_number);
		}
		virtual void start() {
			// ? nothing to do here... 
		}*/
		virtual void on_phrase(int phrase) {
			Serial.printf("SequenceClip clip_id=%i loading sequence_number %i!\n", clip_id, sequence_number);
			project.load_sequence(sequence_number);
		}
		virtual void on_tick(uint32_t tick) {
			// process the tick for the loaded sequence 
		}
		virtual void on_bar(uint32_t bar) {

		}
};

// a Clip that contains a sequence/clock information
class LoopClip : public Clip {
	public:
		int loop_number; // the sequence number to load when this clip played

		LoopClip(int loop_number) {
			this->loop_number = loop_number;
		}

		virtual void on_phrase(int phrase) {
			Serial.printf("LoopClip clip_id=%i loading loop_number %i!\n", clip_id, loop_number);
			project.load_sequence(loop_number);
		}
		virtual void on_tick(uint32_t tick) {
			// process the tick for the loaded sequence 
		}
		virtual void on_bar(uint32_t bar) {

		}
};

// a Clip that contains automation lanes for one or more Parameters
class ParameterClip : public Clip {
	public:

};

// a Clip that contains *all* preset settings for a Behaviour (including its Parameters?)
class BehaviourPresetClip : public Clip {
	public:

};

// a Clip that contains ParameterInput<->Parameter mapping information
class ParameterMapClip : public Clip {
	public:
};

// ... compound class, containing many Clips of different types...?
class Scene {
	public:

};

#endif