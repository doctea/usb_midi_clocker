#include "behaviours/behaviour_bamble.h"
#include "menuitems.h"
#include "submenuitem_bar.h"
#include "menuitems_object_selector.h"

class BambleTriggerOnBar;


class BambleTriggerOnSelectorControl : public ObjectSelectorControl<BambleTriggerOnSelectorControl,int8_t> {
    public:
        int8_t envelope_number = -1;
        DeviceBehaviour_Bamble *behaviour = nullptr;
        BambleTriggerOnSelectorControl(const char *label, DeviceBehaviour_Bamble *behaviour, int8_t envelope_number, LinkedList<option> *available_values = nullptr) 
            : ObjectSelectorControl(label, this, &BambleTriggerOnSelectorControl::setEnvelopeTriggerOn, &BambleTriggerOnSelectorControl::getEnvelopeTriggerOn, nullptr)
            , behaviour(behaviour), envelope_number(envelope_number)
        {
            this->go_back_on_select = true;
        }

        LinkedList<option> *setup_available_values() override {
            ObjectSelectorControl::setup_available_values();

            const int num_patterns = (int)(sizeof(behaviour->patterns) / sizeof(bamble_pattern));
            for (unsigned int i = 0 ; i < num_patterns ; i++) {
                this->add_available_value(i, behaviour->patterns[i].label);
            }
            this->add_available_value(20, "Off");

            return this->available_values;
        }

        int8_t getEnvelopeTriggerOn() {
            return this->behaviour->get_envelope_trigger_on(this->envelope_number);
        }
        void setEnvelopeTriggerOn(int8_t v) {
            this->behaviour->set_envelope_trigger_on(this->envelope_number, v);
        }

        virtual void add_available_value(int8_t value, const char *label) override {
            //Serial.printf("add_available_values(%i, %s)\n", value, label);
            if (this->available_values==nullptr) {
                this->setup_available_values();
                /*Serial.printf("add_available_values(%i, %s) instantiating list\n", value, label);
                this->available_values = new LinkedList<option>();
                Serial.printf("add_available_values(%i, %s) instantiated list!\n", value, label);*/
            }
            String *l = new String(label);
            available_values->add(option { .value = value, .label = l->c_str() });
            this->minimum_value = 0;
            this->maximum_value = available_values->size() - 1;
        }

        /*virtual void set_current_value(int value) override { 
            if (this->behaviour!=nullptr) {
                value = this->get_value_for_index(value);
                //(this->target_object->*this->setter)(value);
                this->behaviour->set_envelope_trigger_on(this->envelope_number, value);

                char msg[255];
                sprintf(msg, "Set %8s to %s (%i)", this->label, get_label_for_value(value), value);
                msg[this->tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
                menu_set_last_message(msg,GREEN);
            }
            if (this->debug) { Serial.println(F("Done.")); Serial_flush(); }
        }*/
};



class BambleTriggerOnBar : public SubMenuItemBar {
    public:
        int envelope_number = -1;
        DeviceBehaviour_Bamble *behaviour = nullptr;

        BambleTriggerOnSelectorControl *pattern_selector = nullptr;
        ObjectToggleControl<BambleTriggerOnBar> *loop_toggler = nullptr;
        ObjectToggleControl<BambleTriggerOnBar> *invert_toggler = nullptr;

        BambleTriggerOnBar(const char *label, DeviceBehaviour_Bamble *behaviour, int envelope_number) : SubMenuItemBar(label) {
            this->behaviour = behaviour;
            this->envelope_number = envelope_number;
            //this->debug = true;

            this->pattern_selector = new BambleTriggerOnSelectorControl("Trigger on", behaviour, envelope_number); //, LinkedList<option> *available_values = nullptr));
            //this->pattern_selector->debug = true;
            this->loop_toggler =     new ObjectToggleControl<BambleTriggerOnBar>("Loop", this,   &BambleTriggerOnBar::setLoop,   &BambleTriggerOnBar::getLoop);
            this->invert_toggler =   new ObjectToggleControl<BambleTriggerOnBar>("Invert", this, &BambleTriggerOnBar::setInvert, &BambleTriggerOnBar::getInvert);
            this->add(pattern_selector);
            this->add(loop_toggler);
            this->add(invert_toggler);
        }

        void setEnvelopeTriggerOn(int v) {
            if (envelope_number>=0)
                behaviour->set_envelope_trigger_on(this->envelope_number, v);
        }
        int getEnvelopeTriggerOn() {
            if (envelope_number>=0)
                return behaviour->get_envelope_trigger_on(this->envelope_number);
        }
        void setLoop(bool value) {
            behaviour->set_envelope_trigger_loop(this->envelope_number, value);
        }
        bool getLoop() {
            return behaviour->is_envelope_trigger_loop(this->envelope_number);
        }
        void setInvert(bool value) {
            behaviour->set_envelope_trigger_invert(this->envelope_number, value);
        }
        bool getInvert() {
            return behaviour->is_envelope_trigger_invert(this->envelope_number);
        }

        virtual LinkedList<BambleTriggerOnSelectorControl::option> *get_available_values() {
            //Serial.printf("get_available_values() in %s", this->label);
            return this->pattern_selector->get_available_values();
        }
        virtual void set_available_values(LinkedList<BambleTriggerOnSelectorControl::option> *available_values) {
            this->pattern_selector->set_available_values(available_values);
        }

        virtual inline int get_max_pixel_width(int item_number) override {
            switch(item_number) {
                case 1: case 2: return tft->width()/4;
                case 0: return tft->width()/2;
            }
            return tft->width() / tft->characterWidth();
        }
};


