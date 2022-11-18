#include "behaviours/behaviour_bamble.h"
#include "menuitems.h"

class BambleTriggerOnSelectorControl : public ObjectSelectorControl<BambleTriggerOnSelectorControl,int> {
    public:
        int envelope_number = -1;
        DeviceBehaviour_Bamble *behaviour = nullptr;
        BambleTriggerOnSelectorControl(char *label, DeviceBehaviour_Bamble *behaviour, int envelope_number, LinkedList<option> *available_values = nullptr) 
            : ObjectSelectorControl(label, this, &BambleTriggerOnSelectorControl::setEnvelopeTriggerOn, &BambleTriggerOnSelectorControl::getEnvelopeTriggerOn, nullptr)
        {
            Serial.printf("BambleTriggerOnSelectorControl constructor %s, %i starting..\n", label, envelope_number); Serial.flush();
            strncpy(this->label, label, MAX_LABEL_LENGTH);
            this->behaviour = behaviour;
            this->envelope_number = envelope_number;

            //this->set_internal_value (this->get_index_for_value( (this->target_object->*this->getter)() ));
            //this->set_internal_value((this->target_object->*this->getter)());

            if (available_values==nullptr)
                this->setup_available_values();
            else
                this->available_values = available_values;

            Serial.printf("BambleTriggerOnSelectorControl constructor %s, %i finished!\n", label, envelope_number); Serial.flush();
        }

        LinkedList<option> *setup_available_values() {
            // envelope trigger selectors
            // Trigger/LFO settings: 
            //  0->19 = trigger #, 
            //  20 = off, 
            //  32->51 = trigger #+loop, 
            //  64->83 = trigger #+invert, 
            //  96->115 = trigger #+loop+invert
            const int num_patterns = (int)(sizeof(behaviour->patterns) / sizeof(bamble_pattern));
            for (int i = 0 ; i < num_patterns ; i++) {
                this->add_available_value(i, behaviour->patterns[i].label);
            }
            this->add_available_value(20, "Off");
            for (int i = 0 ; i < num_patterns ; i++) {
                char label[MAX_LABEL_LENGTH] = "";
                sprintf(label, "%s+loop", behaviour->patterns[i].label);
                this->add_available_value(i+32, label);
            }
            for (int i = 0 ; i < num_patterns ; i++) {
                char label[MAX_LABEL_LENGTH] = "";
                sprintf(label, "%s+invert", behaviour->patterns[i].label);
                this->add_available_value(i+64, label);
            }
            for (int i = 0 ; i < num_patterns ; i++) {
                char label[MAX_LABEL_LENGTH] = "";
                sprintf(label, "%s+loop+invert", behaviour->patterns[i].label);
                this->add_available_value(i+96, label);
            }
            return this->available_values;
        }

        void setEnvelopeTriggerOn(int v) {
            if (envelope_number>=0)
                behaviour->set_envelope_trigger_on(this->envelope_number, v);
        }
        int getEnvelopeTriggerOn() {
            if (envelope_number>=0)
                return behaviour->get_envelope_trigger_on(this->envelope_number);
        }

        virtual void add_available_value(int value, const char *label) override {
            if (this->available_values==nullptr)
                this->available_values = new LinkedList<option>();
            String *l = new String(label);
            available_values->add(option { .value = value, .label = l->c_str() });
            this->minimum_value = 0;
            this->maximum_value = available_values->size() - 1;
        }

        /*virtual void set_current_value(int value) override { 
            if (this->target_object!=nullptr) {
                value = this->get_value_for_index(value);
                (this->target_object->*this->setter)(value);
                this->target_object->set_envelope_trigger_on(this->envelope_number, value);

                char msg[255];
                sprintf(msg, "Set %8s to %s (%i)", this->label, get_label_for_value(value), value);
                msg[this->tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
                menu_set_last_message(msg,GREEN);
            }
            if (this->debug) { Serial.println(F("Done.")); Serial.flush(); }
        }*/


};
