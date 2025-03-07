#if defined(ENABLE_SCREEN) && defined(ENABLE_PROGRESSIONS)

#include "behaviours/behaviour_progression.h"
#include "midi/midi_apcmini_display.h"

// todo: make a widget that indicates graphically the song structure
// so, loop over playlist; for each playlist position,
// draw the section repeat/2 times
// for each section, draw the chord progression
// each chord progression section should indicate the chord, inversion, and quality .. somehow

// todo: indicate whether the current section has been saved 

int ProgressionPinnedMenuItem::display(Coord pos, bool selected, bool opened) {
    int y = pos.y;
    int x = 0;
    tft->setCursor(0, y);

    // make bar numbers bigger if we're in a scrollable menu
    if (menu->expand_pinned_level()>0) {
        tft->setTextSize(2); //menu->expand_pinned_level());
    } else {
        tft->setTextSize(0);
        tft->println("^^^^^^^^^^^^^^^^^^^^^^^^^");
        return tft->getCursorY()-12;
    }

    tft->setTextColor(C_WHITE, BLACK);
    
    //// headers for the columns
    tft->printf("Pos Cnt Sec Bar ");

    // indicate what page the APC is on
    if (get_apc_gate_page()==CLOCKS)
        tft->printf("Clk");
    else if (get_apc_gate_page()==SEQUENCES)
        tft->printf("Seq");
    else if (get_apc_gate_page()==PATTERNS)
        tft->printf("Pat");
    #ifdef ENABLE_APCMINI_PROGRESSIONS
        else if (get_apc_gate_page()==PROGRESSIONS)
            tft->printf("Prg");
    #endif
    #ifdef ENABLE_ACPMINI_PADS
        else if (get_apc_gate_page()==PADS)
            tft->printf("Pad");
    #endif
    else
        tft->printf("???");
    tft->println();
    
    // show playlist position information - green if playing, red if paused
    if (behaviour_progression->advance_progression_playlist) {
        tft->setTextColor(GREEN, BLACK);
    } else {
        tft->setTextColor(RED, BLACK);
    }
    tft->printf("%i/%i ", behaviour_progression->playlist_position+1, 8);

    tft->setTextColor(C_WHITE, BLACK);
    tft->printf("%i/%i ", behaviour_progression->current_section_plays+1, behaviour_progression->playlist.entries[behaviour_progression->playlist_position].repeats);
    tft->printf("%i/%i ", behaviour_progression->current_section+1, 4);

    // show bar position information - green if playing, red if paused
    if (behaviour_progression->advance_progression_bar) {
        tft->setTextColor(GREEN, BLACK);
    } else {
        tft->setTextColor(RED, BLACK);
    }
    tft->printf("%i/%i ", behaviour_progression->current_bar+1, 8);
    tft->setTextColor(C_WHITE, BLACK);

    // indicate current apc page info
    if (get_apc_gate_page()==PROGRESSIONS) {
        tft->println(   
            behaviour_progression->current_mode==VirtualBehaviour_Progression::MODE::DEGREE ?       "Deg" : 
            behaviour_progression->current_mode==VirtualBehaviour_Progression::MODE::QUALITY ?      "Qlt" : 
            behaviour_progression->current_mode==VirtualBehaviour_Progression::MODE::INVERSION ?    "Inv" : 
            behaviour_progression->current_mode==VirtualBehaviour_Progression::MODE::PLAYLIST ?     "Pls" : "???"
        );
    } else {
        tft->println("---");
    }
    // todo: show current lock status

    y = tft->getCursorY() - 4;
    return y;
}



/*void ProgressionPinnedMenuItem::setup_progression_bar() {
    // show playlist position
    this->add(new NumberControl<int8_t>("PlayPos", &behaviour_progression->playlist_position, 0, 8, true, true));
    // show number of plays of this section
    this->add(new NumberControl<int8_t>("Plays", &behaviour_progression->current_section_plays, 0, 8, true, true));
    // show current section number
    this->add(new NumberControl<int8_t>("Section", &behaviour_progression->current_section, 0, 3, true, true));
    // show current bar number
    this->add(new NumberControl<int8_t>("Bar", &behaviour_progression->current_bar, 0, 7, true, true));
    // show current key, chord, inversion
}
*/

#endif