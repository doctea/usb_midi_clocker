// thinking about how to restructure things to allow better save+recall of settings / song chaining
// Project contains possible_clips; which ones to be used when queued by an "Arrangement";
// Arrangement is responsible for fetching/writing list of Clips;
// Clip is responsible for recalling + writing its values and telling the Sequence, Parameter, Behaviour what it needs to update...
// Think we need to make Behaviour options into something that can be queried programmatically, ie, something like Parameters that can be saved/loaded
// 	so Parameter knows its own datatype; perhaps we have a ParameterLoader/ParameterSaver type that knows how to read/write a key/value pair and set values accordingly


// abstract base Clip class
class Clip {
	public:
		Clip() = 0;
		~Clip() = default;
};

// a Clip that contains a sequence/clock information
class SequenceClip : public Clip {
	public:

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


class Arrangement {
	public:
	struct clip_instance_t {
		int position;	// which bar/phrase number the specified clip should be used at
		int clip_id;	// project-specific clip number - where to save it on disk
		Clip *clip;	// pointer to the clip in memory
	};
	LinkedList<clip_instance_t> song_structure = new LinkedList<clip_instance_t>();

	// have some helper functions to:-
	// 	return all clip_instance_ts that are queued for a particular song position
	// 	insert a clip_instance_t at the correct position
	// 	generate a GUID for a clip to use as the clip_id?

};
