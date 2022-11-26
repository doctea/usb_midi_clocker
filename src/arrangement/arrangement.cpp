#include "clips/clip_manager.h"

ClipManager *clip_manager = nullptr;

void setup_clip_manager() {
    clip_manager = new ClipManager();
    for (int i = 0 ; i < NUM_SEQUENCE_SLOTS_PER_PROJECT ; i++) {
        clip_manager->add_clip(new SequenceClip(i));
    }
}

#include "arrangement/arrangement.h"

Arrangement *arrangement = nullptr;

void setup_arrangement() {
    Serial.println("setup_arrangement...");
    arrangement = new Arrangement();
	for (int i = 0 ; i < 8 ; i++) {
        Serial.printf("setup_arrangement %i\n", i);
		arrangement->insert_clip_instance(i, clip_manager->get_clip_for_id(7-i));
	}
}