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

#include "file_manager/file_manager_interfaces.h"

// abstract base Clip class
class Clip : public virtual IParseKeyValueReceiver, public virtual ISaveKeyValueSource {
	public:
		Clip() {};
		~Clip() = default;
		unsigned int clip_id = -1;
		bool dirty = false;

		virtual char get_name() {
			return (char)48 + this->clip_id;
		};

		//virtual void prepare() = 0;
		//virtual void start() = 0;
		virtual void on_phrase(int phrase) {};
		virtual void on_tick(uint32_t tick) {};
		virtual void on_bar(uint32_t bar) {};

		// todo: save/load
		//virtual void save() {}
		//virtual void load() {}
		virtual void set_dirty(bool v = true) {
			this->dirty = v;
		}
		virtual bool is_dirty() {
			return dirty;
		}

		virtual bool load_parse_key_value(String key, String value) override {}
		virtual void add_save_lines(LinkedList<String> *lines) override {}

};

template<class TargetClass, class DataType>
class ObjectValueClip : public Clip {
	public:
		TargetClass *target_object = nullptr;
        bool(TargetClass::*setter_func)(DataType);
        //DataType(TargetClass::*getter_func);

		DataType value = 0;

		ObjectValueClip(TargetClass *target, bool(TargetClass::*setter_func)(DataType), DataType value = 0) {
			this->target_object = target;
			this->setter_func = setter_func;
			this->value = value;
		}

		virtual char get_name() {
			return (char)48 + (char)this->value;
		};

		virtual void setValue(DataType value) {
			this->value = value;
		}

		virtual void activate(DataType value) {
			if (this->target_object==nullptr) 
				return;
			Serial.printf(F("ObjectValueClip#activate(%i)\n"), value);
			(this->target_object->*this->setter_func)(value);
		}

		virtual void on_phrase(int phrase) override {
			//Serial.printf(F("SequenceClip clip_id=%i loading sequence_number %i!\n"), clip_id, sequence_number);
			//project.load_sequence(sequence_number);
			this->activate(value);
		}
		virtual void on_tick(uint32_t tick) override {
			// process the tick for the loaded sequence 
		}
		virtual void on_bar(uint32_t bar) override {

		}
};

class SequenceClip : public ObjectValueClip<Project, int> {
	public:
		SequenceClip() : SequenceClip(0) {}
		SequenceClip(int sequence_number) : ObjectValueClip<Project,int>(project, &Project::load_specific_pattern, sequence_number) {}

		virtual bool load_parse_key_value(String key, String value) override {
			if (key.equals("sequence_number")) {
				this->setValue(value.toInt());
				return true;
			}
			return ObjectValueClip::load_parse_key_value(key, value);
		}

		virtual void add_save_lines(LinkedList<String> *lines) override {
			lines->add(String("sequence_number=") + String(this->value));
		}
};

#ifdef ENABLE_LOOPER
class LoopClip : public ObjectValueClip<Project, int> {
	public:
		LoopClip(int loop_number) : ObjectValueClip<Project,int>(project, &Project::load_specific_loop, loop_number) {}
};
#endif

// a Clip that contains a sequence/clock information
/*class SequenceClip : public Clip {
	public:
		int sequence_number; // the sequence number to load when this clip played

		SequenceClip(int sequence_number) {
			this->sequence_number = sequence_number;
		}

		virtual char get_name() {
			return (char)(sequence_number + 48);
		}

		virtual void on_phrase(int phrase) override {
			Serial.printf(F("SequenceClip clip_id=%i loading sequence_number %i!\n"), clip_id, sequence_number);
			project.load_sequence(sequence_number);
		}
		virtual void on_tick(uint32_t tick) override {
			// process the tick for the loaded sequence 
		}
		virtual void on_bar(uint32_t bar) override {

		}
};*/

// a Clip that contains a sequence/clock information
/*class LoopClip : public Clip {
	public:
		int loop_number; // the sequence number to load when this clip played

		LoopClip(int loop_number) {
			this->loop_number = loop_number;
		}

		virtual char get_name() {
			return (char)(loop_number + 48);
		}

		virtual void on_phrase(int phrase) override {
			Serial.printf(F("LoopClip clip_id=%i loading loop_number %i!\n"), clip_id, loop_number);
			project.load_loop(loop_number);
		}
		virtual void on_tick(uint32_t tick) override {
			// process the tick for the loaded sequence 
		}
		virtual void on_bar(uint32_t bar) override {

		}
};*/

// a Clip that contains automation lanes for one or more Parameters
class ParameterClip : public Clip {
	public:

};

// a Clip that contains *all* preset settings for a Behaviour (including its Parameters?)
/*class BehaviourPresetClip : public Clip {
	public:

};*/

// a Clip that contains ParameterInput<->Parameter mapping information
class ParameterMapClip : public Clip {
	public:
};

// ... compound class, containing many Clips of different types...?
class Scene {
	public:

};

#endif