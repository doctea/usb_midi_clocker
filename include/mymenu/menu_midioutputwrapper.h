#include "midi/midi_out_wrapper.h"

#ifdef DEBUG_MIDI_WRAPPER

#include "menuitems.h"

class MIDIOutputWrapperDebugMenuItem : public MenuItem {
    public:
        MIDIOutputWrapper *target;

        MIDIOutputWrapperDebugMenuItem(const char *label, MIDIOutputWrapper *target) : MenuItem(label), target(target) {
            if (target!=nullptr)
                target->set_log_message_mode(true);
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            if (target==nullptr) {
                tft->println("Error: No target wrapper set!");
                return tft->getCursorY();
            }

            if (target->message_history==nullptr) {
                tft->println("Debug not initialised");
            }

            tft->printf("Tick : Type  Ch   Pitch   Vel   [%i/%i]\n", target->next_message_history_index, target->message_history_size);

            // so we want to start rendering from the previously logged message...
            int start_at = target->next_message_history_index - 1;
            if (start_at<0) 
                start_at = target->message_history_size - 1;
            for (int i = 0 ; i < target->message_history_size ; i++) {
                int actual = (start_at + i + 1) % target->message_history_size;
                message_history_t *current = &target->message_history[actual];

                if (current->type>0) {
                    char *type_name = "??";
                    if (current->type==midi::NoteOn)
                        type_name = "On";
                    else if (current->type==midi::NoteOff)
                        type_name = "Off";
                    tft->printf("%5u: %3s   ", current->ticks, type_name);
                    tft->printf("%2i  ", current->channel);
                    tft->printf("%3i %3s", current->pitch, (char*)get_note_name_c(current->pitch));
                    tft->printf("  %i\n", current->velocity);
                }
            }

            return tft->getCursorY();
        }
};

#endif