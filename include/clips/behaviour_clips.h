#ifndef BEHAVIOUR_CLIPS__INCLUDED
#define BEHAVIOUR_CLIPS__INCLUDED

#include "clips/clips.h"
//#include "behaviours/behaviour_base.h"

class DeviceBehaviourUltimateBase;

//#include "file_manager/file_manager.h"

class BehaviourPresetClip : public Clip {
    public:

    DeviceBehaviourUltimateBase *behaviour = nullptr;

    BehaviourPresetClip(DeviceBehaviourUltimateBase *behaviour) {
        this->behaviour = behaviour;
    }
    
    virtual void on_phrase(int phrase) override {
        // load the preset for this clip_id
    }
	virtual void on_tick(uint32_t tick) override {};
	virtual void on_bar(uint32_t bar) override {};
    //virtual char get_name() { return 48 + this->clip_id; }
};

/*class ArrangementTrackBehaviour {
    public:
    const char *get_label();
};*/

#endif