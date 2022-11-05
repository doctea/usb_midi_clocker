#ifndef DRUMS_INCLUDED
#define DRUMS_INCLUDED

#include "Config.h"

/*#define TRIGGER_IS_ON   true
#define TRIGGER_IS_OFF  false
*/
#define GM_CHANNEL_DRUMS 10

#define GM_NOTE_MINIMUM 35
//#define GM_NOTE_SLAP 28   // extended?
#define GM_NOTE_ACOUSTIC_BASS_DRUM 35
#define GM_NOTE_ELECTRIC_BASS_DRUM 36
#define GM_NOTE_SIDE_STICK 37
#define GM_NOTE_ACOUSTIC_SNARE 38
#define GM_NOTE_HAND_CLAP 39
#define GM_NOTE_ELECTRIC_SNARE 40
#define GM_NOTE_LOW_FLOOR_TOM 41
#define GM_NOTE_CLOSED_HI_HAT 42
#define GM_NOTE_HIGH_FLOOR_TOM 43
#define GM_NOTE_PEDAL_HI_HAT 44
#define GM_NOTE_LOW_TOM 45
#define GM_NOTE_OPEN_HI_HAT 46
#define GM_NOTE_LOW_MID_TOM 47
#define GM_NOTE_HI_MID_TOM 48
#define GM_NOTE_CRASH_CYMBAL_1 49
#define GM_NOTE_HIGH_TOM 50
#define GM_NOTE_RIDE_CYMBAL_1 51
#define GM_NOTE_CHINESE_CYMBAL 52
#define GM_NOTE_RIDE_BELL 53
#define GM_NOTE_TAMBOURINE 54
#define GM_NOTE_SPLASH_CYMBAL 55
#define GM_NOTE_COWBELL 56
#define GM_NOTE_CRASH_CYMBAL_2 57
#define GM_NOTE_VIBRA_SLAP 58
#define GM_NOTE_RIDE_CYMBAL_2 59
#define GM_NOTE_HIGH_BONGO 60
#define GM_NOTE_LOW_BONGO 61
#define GM_NOTE_MUTE_HIGH_CONGA 62
#define GM_NOTE_OPEN_HIGH_CONGA 63
#define GM_NOTE_LOW_CONGA 64
#define GM_NOTE_HIGH_TIMBALE 65
#define GM_NOTE_LOW_TIMBALE 66
#define GM_NOTE_HIGH_AGOGO 67
#define GM_NOTE_LOW_AGOGO 68
#define GM_NOTE_CABASA 69
#define GM_NOTE_MARACAS 70
#define GM_NOTE_SHORT_WHISTLE 71
#define GM_NOTE_LONG_WHISTLE 72
#define GM_NOTE_SHORT_GUIRO 73
#define GM_NOTE_LONG_GUIRO 74
#define GM_NOTE_CLAVES 75
#define GM_NOTE_HIGH_WOODBLOCK 76
#define GM_NOTE_LOW_WOODBLOCK 77
#define GM_NOTE_MUTE_CUICA 78
#define GM_NOTE_OPEN_CUICA 79
#define GM_NOTE_MUTE_TRIANGLE 80
#define GM_NOTE_OPEN_TRIANGLE 81
#define GM_NOTE_MAXIMUM 81

// the midimuso-cv12 gate and cv output mappings - taken from http://midimuso.co.uk/wp-content/uploads/2017/08/CV_12_ORAC_Manual.pdf
// for mode 0B (11 gates, 5 CVs + pitch bend)
//#define DEFAULT_MUSO_GATE_CHANNEL   16   // channel to output triggers (ie gate triggers to notes on this channel)
//int midi_channel_muso_gate = DEFAULT_MUSO_GATE_CHANNEL;
/*#define MUSO_GATE_CHANNEL   (midi_channel_muso_gate)
#define MUSO_NOTE_MINIMUM   60
#define MUSO_NOTE_GATE_1    60    
#define MUSO_NOTE_GATE_2    61
#define MUSO_NOTE_GATE_3    62
#define MUSO_NOTE_GATE_4    63
#define MUSO_NOTE_GATE_5    64
#define MUSO_NOTE_GATE_6    65
#define MUSO_NOTE_GATE_7    66
#define MUSO_NOTE_GATE_8    67
#define MUSO_NOTE_GATE_9    68
#define MUSO_NOTE_GATE_10   69    // will lose these in mode 1B
#define MUSO_NOTE_GATE_11   70    // will lose these in mode 1B

#define MUSO_MODE_0B  1
#define MUSO_MODE_1B  2
#define MUSO_MODE_2B  3
#define MUSO_MODE_0B_AND_2A	4
// todo: work out and explain wtf is going on here and how the different modes work in the code*/

//#if MUSO_MODE==MUSO_MODE_0B
//#define NUM_TRIGGERS        (MUSO_NOTE_GATE_11 - MUSO_NOTE_MINIMUM + 1)
//#define MUSO_NOTE_MAXIMUM   (MUSO_NOTE_MINIMUM + NUM_TRIGGERS)
/*#elif MUSO_MODE==MUSO_MODE_2B
#define NUM_TRIGGERS        (MUSO_NOTE_GATE_7 - MUSO_NOTE_MINIMUM + 1)
#define MUSO_NOTE_MAXIMUM   (MUSO_NOTE_MINIMUM + NUM_TRIGGERS)
#endif*/

// defines to help with which trigger number/euclidian track number maps to which
#define TRIGGER_KICK        0
#define TRIGGER_SIDESTICK   1
#define TRIGGER_CLAP        2
#define TRIGGER_SNARE       3
#define TRIGGER_CRASH_1     4
#define TRIGGER_TAMB        5
#define TRIGGER_HITOM       6
#define TRIGGER_LOTOM       7
#define TRIGGER_PEDALHAT    8
#define TRIGGER_OPENHAT     9
#define TRIGGER_CLOSEDHAT   10
#define TRIGGER_CRASH_2     11
#define TRIGGER_SPLASH      12
#define TRIGGER_VIBRA       13
#define TRIGGER_RIDE_BELL   14
#define TRIGGER_RIDE_CYM    15
#define TRIGGER_BASS_CH4    16
#define TRIGGER_BITSBOX_CH3 17
#define TRIGGER_PITCH_1_CH1 18
#define TRIGGER_PITCH_2_CH2 19

/*
// the CC values that the midimuso translates into the CV outputs
#define MUSO_CC_CV_1  1
#define MUSO_CC_CV_2  7
#define MUSO_CC_CV_3  11
#define MUSO_CC_CV_4  71
#define MUSO_CC_CV_5  74
#define MUSO_CC_CV_6  93    // extended envelope, used for the midi note-triggered envelopes
#define MUSO_CC_CV_7  94    // extended envelope, used for the midi note-triggered envelopes
#define MUSO_CC_CV_8  73    // extended envelope, used for the midi note-triggered envelopes
#define MUSO_CC_CV_9  72    // extended envelope, used for the midi note-triggered envelopes
#define MUSO_USE_PITCH_FOR  MUSO_CC_CV_5        // because my CV5 output doesn't work!

#define MUSO_CV_CHANNEL     1   // channel to output CV CC's (ie envelopes to MidiMuso on this channel)

#define MIDI_CC_ALL_NOTES_OFF 123

// tracking what triggers are currently active, for the sake of pixel output 
int trigger_status[NUM_TRIGGERS];*/

#endif
