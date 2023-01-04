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

#include "mymenu/menu_arrangement.h"
#include "arrangement/arrangement.h"
Arrangement *arrangement = nullptr;

FLASHMEM
void setup_arrangement() {
    //Serial.println("setup_arrangement...");
    Serial.println("========== adding sequence clips");
    arrangement = new Arrangement();
    ArrangementTrackBase *sequencer_track = arrangement->addTrack(new ArrangementSingleTrack("Sequence"));
	for (unsigned int i = 0 ; i < NUM_SEQUENCE_SLOTS_PER_PROJECT ; i++) {
        Serial.printf("setup_arrangement insert_clip_instance for a sequence %i\n", i);
		sequencer_track->insert_clip_instance(i, clip_manager->add_clip(new SequenceClip(7-i))); //new SequenceClip(7-i)); //clip_manager->get_clip_for_id(7-i));
	}
    Serial.println("added sequence clips");

    Serial.println("========== adding loop clips");
    ArrangementMultipleTrack *loop_arrangement = (ArrangementMultipleTrack *)arrangement->addTrack(new ArrangementSingleTrack("Looper"));
    int x = 0;
    for (unsigned int i = 0 ; i < NUM_LOOP_SLOTS_PER_PROJECT ; i++) {
        loop_arrangement->insert_clip_instance(random(0,16), clip_manager->add_clip(new LoopClip(i)));
        loop_arrangement->insert_clip_instance(i*3, clip_manager->add_clip(new LoopClip(i)));
        loop_arrangement->insert_clip_instance(random(0,16), clip_manager->add_clip(new LoopClip(i)));
        loop_arrangement->insert_clip_instance(random(0,16), clip_manager->add_clip(new LoopClip(i)));
    }
    Serial.println("added all looper clips"); Serial_flush();

    // loop over each behaviour, add an ArrangementTrack as appropriate...
    Serial.println("========== adding behaviour tracks");
    const unsigned int behaviours_size = behaviour_manager->behaviours->size();
    for (unsigned int i = 0 ; i < behaviours_size ; i++) {
        ArrangementTrackBase *behaviour_track = behaviour_manager->behaviours->get(i)->create_arrangement_track();
        if (behaviour_track!=nullptr)
            arrangement->addTrack(behaviour_track);
    }
    Serial.println("added behaviour tracks");

    setup_arrangement_menu(arrangement);
    
    // probably ask the behaviour itself to instantiate the track object
}

#include "mymenu/menu_arrangement_track.h"
#include "mymenu.h"
#include "menu.h"

#include "submenuitem_bar.h"

FLASHMEM
void setup_arrangement_menu(Arrangement *arrangement) {
    menu->add_page("Arrangement"); //, 1);

    SubMenuItemBar *transport = new SubMenuItemBar("Transport");
    //Serial.println("instantiated transport bar");    

    transport->add(new NumberControl<bool>("Play", &arrangement->playing, arrangement->playing, 0, 1, true));
    transport->add(new NumberControl<bool>("Loop", &arrangement->looping, arrangement->looping, 0, 1, true));
    transport->add(new NumberControl<int> ("Phrase", &arrangement->current_song_phrase, arrangement->current_song_phrase, 0, 1024, true));
    
    menu->add(transport);
    
    Serial.println("added transport bar");
    
    //menu->add(new ArrangementEditor("Arrangement")); //, arrangement));   

    menu->add(new ArrangementHeader("Arrangement"));
    const unsigned int tracks_size = arrangement->get_tracks()->size();
    for (unsigned int i = 0 ; i < tracks_size ; i++) {
        ArrangementTrackBase* track = arrangement->get_tracks()->get(i);
        menu->add(new ArrangementTrackEditor(track->label, track));
    }

    Serial.println("finished setup_arrangement_menu");

}

