#ifdef ENABLE_APCMINI_DISPLAY

#ifdef ENABLE_CLOCKS
void redraw_clock_row(byte c) {
    byte start_row = 64-((c+1)*APCMINI_DISPLAY_WIDTH);
    for (byte i = 0 ; i < APCMINI_DISPLAY_WIDTH ; i++) {
      byte io = (i - clock_delay[c]) % APCMINI_DISPLAY_WIDTH; //) % APCMINI_DISPLAY_WIDTH;   // TODO: this doesn't display properly?
      if (clock_multiplier[c]<=0.5) {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, APCMINI_RED, 1));
      } else if (clock_multiplier[c]<=1.0) {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, APCMINI_GREEN, 1));
      } else if (io%(byte)clock_multiplier[c]==0) {
        byte colour = clock_multiplier[c]   > 8.0 ? APCMINI_RED : 
                      (clock_multiplier[c]  > 4.0 ? APCMINI_YELLOW : APCMINI_GREEN);
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, colour, 1));
      } else {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, APCMINI_OFF, 1));  // turn the led off
      }
    }
}

void redraw_clock_selected(byte old_clock_selected, byte clock_selected) {
  ATOMIC(midi_apcmini->sendNoteOn(APCMINI_BUTTON_CLIP_STOP + old_clock_selected, APCMINI_OFF, 1);)
  ATOMIC(midi_apcmini->sendNoteOn(APCMINI_BUTTON_CLIP_STOP + clock_selected,     APCMINI_ON,  1);)
}
#endif

#ifdef ENABLE_SEQUENCER
void redraw_sequence_row(byte c) {
    byte start_row = 32-((c+1)*APCMINI_DISPLAY_WIDTH);
    for (byte i = 0 ; i < APCMINI_DISPLAY_WIDTH ; i++) {
      if (read_sequence(c,i)) { //should_trigger_sequence(i*PPQN,c)) {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, APCMINI_YELLOW, 1);)
      } else {
        ATOMIC(midi_apcmini->sendNoteOn(start_row+i, APCMINI_OFF, 1);)
      }
    }
}
#endif

#endif
