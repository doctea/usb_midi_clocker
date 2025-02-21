

#include "behaviours/behaviour_progression.h"

void ProgressionPinnedMenuItem::setup_progression_bar() {
    // show playlist position
    this->progression_bar->add(new NumberControl<int8_t>("PlayPos", &behaviour_progression->playlist_position, 0, 8, true, true));
    // show number of plays of this section
    this->progression_bar->add(new NumberControl<int8_t>("Plays", &behaviour_progression->current_section_plays, 0, 8, true, true));
    // show current section number
    this->progression_bar->add(new NumberControl<int8_t>("Section", &behaviour_progression->current_section, 0, 3, true, true));
    // show current bar number
    this->progression_bar->add(new NumberControl<int8_t>("Bar", &behaviour_progression->current_bar, 0, 7, true, true));
    // show current key, chord, inversion
}
