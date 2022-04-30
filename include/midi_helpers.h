#ifndef MIDI_HELPERS__INCLUDED
#define MIDI_HELPERS__INCLUDED

String get_note_name(int pitch) {
  if (pitch==-1 || pitch>127) {
    String s = "_"; //note_names[chromatic_degree] + String(octave);
    return s;
  }
  int octave = pitch / 12;
  int chromatic_degree = pitch % 12; 
  const String note_names[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
  };
  
  String s = note_names[chromatic_degree] + String(octave);
  return s;
}

#endif

String get_note_name(int pitch);