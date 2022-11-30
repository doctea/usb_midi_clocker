#include "clips/clip_manager.h"

ClipManager *clip_manager = nullptr;

void setup_clip_manager() {
    clip_manager = new ClipManager();

    // for initial testing -- add a clip for every sequence slot
    /*for (int i = 0 ; i < NUM_SEQUENCE_SLOTS_PER_PROJECT ; i++) {
        clip_manager->add_clip(new SequenceClip(i));
    }

    for (int i = 0 ; i < NUM_LOOP_SLOTS_PER_PROJECT ; i++) {
        clip_manager->add_clip(new LoopClip(i));
    }*/

}


#include "arrangement/arrangement.h"

Arrangement *arrangement = nullptr;

void setup_arrangement() {
    Serial.println("setup_arrangement...");
    arrangement = new Arrangement();
    ArrangementTrackBase *sequencer_track = arrangement->addTrack(new ArrangementSingleTrack("Sequence"));
	for (int i = 0 ; i < NUM_SEQUENCE_SLOTS_PER_PROJECT ; i++) {
        Serial.printf("setup_arrangement for a sequence %i\n", i);
		sequencer_track->insert_clip_instance(i, clip_manager->add_clip(new SequenceClip(7-i))); //new SequenceClip(7-i)); //clip_manager->get_clip_for_id(7-i));
	}

    ArrangementTrackBase *loop_arrangement = arrangement->addTrack(new ArrangementSingleTrack("Looper"));
    for (int i = 0 ; i < NUM_LOOP_SLOTS_PER_PROJECT ; i++) {
        loop_arrangement->insert_clip_instance(random(0,16), clip_manager->add_clip(new LoopClip(i)));
    }

    /*for (int i = 0 ; i < behaviour_manager->behaviours->size() ; i++) {
        ArrangementTrackBase *behaviour_track = behaviour_manager->behaviours->get(i)->create_arrangement_track();
        if (behaviour_track!=nullptr)
            arrangement->addTrack(behaviour_track);
    }*/

    // TODO: loop over each behaviour, add an ArrangementTrack as appropriate...
    // probably ask the behaviour itself to instantiate the track object
}