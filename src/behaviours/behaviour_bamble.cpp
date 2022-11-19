#include "behaviours/behaviour_bamble.h"

DeviceBehaviour_Bamble *behaviour_bamble = new DeviceBehaviour_Bamble(); //nullptr;

#ifdef ENABLE_BAMBLE_INPUT
    void bamble_control_change(uint8_t inChannel, uint8_t inNumber, uint8_t inValue) {
        if (behaviour_bamble!=nullptr) behaviour_bamble->receive_control_change(inChannel, inNumber, inValue);
    }

    void bamble_note_on(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
        if (behaviour_bamble!=nullptr) behaviour_bamble->receive_note_on(inChannel, inNumber, inVelocity);
    }

    void bamble_note_off(uint8_t inChannel, uint8_t inNumber, uint8_t inVelocity) {
        if (behaviour_bamble!=nullptr) behaviour_bamble->receive_note_off(inChannel, inNumber, inVelocity);
    }
#endif

#ifdef ENABLE_SCREEN
#include "submenuitem_bar.h"

#include "menuitems_object_multitoggle.h"
#include "menuitems_numbers.h"
#include "mymenu/menu_bamble.h"

class PatternToggle : public MultiToggleItemBase {
    bamble_pattern *target_pattern = nullptr;
    DeviceBehaviour_Bamble *target_object = nullptr;

    public:
        PatternToggle(DeviceBehaviour_Bamble *target_object, bamble_pattern *pattern) 
            : MultiToggleItemBase(pattern->label) {
            this->target_object = target_object;
            this->target_pattern = pattern;
            this->label = pattern->label;
        }
        virtual bool do_getter() override {
            return this->target_pattern->current_state;
        }
        virtual void do_setter(bool state) override {
            target_object->setPatternEnabled(this->target_pattern->cc_number - 32, state);
            //this->target_pattern->current_state = state;
            //this->target_object->sendControlChange(this->target_pattern->cc_number, state ? 127:0, 10);
        }
};



//FLASHMEM //virtual LinkedList<MenuItem*>* DeviceBehaviour_Bamble::make_menu_items() causes a section type conflict with virtual void DeviceBehaviour_Bamble::setup_callbacks()
//FLASHMEM
LinkedList<MenuItem*> *DeviceBehaviour_Bamble::make_menu_items() {
    //Serial.println(F("\tDividedClockedBehaviour calling DeviceBehaviourUltimateBase::make_menu_items()")); Serial.flush();
    DeviceBehaviourUltimateBase::make_menu_items();

    // Euclidian settings bar /////////////////////////////////////////////////
    SubMenuItemBar *euclidian_options = new SubMenuItemBar("Euclidian settings");

    ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t> *euclidian_mode_control = new ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t>(
        "Mode",
        this,
        &DeviceBehaviour_Bamble::setDemoMode,
        &DeviceBehaviour_Bamble::getDemoMode
    );
    euclidian_mode_control->go_back_on_select = true;
    euclidian_mode_control->add_available_value(0, "Standby");
    euclidian_mode_control->add_available_value(1, "Euclidian");
    euclidian_mode_control->add_available_value(2, "Mutate");
    euclidian_mode_control->add_available_value(3, "Experimental");
    euclidian_mode_control->add_available_value(4, "ArtsEtc");
    euclidian_mode_control->add_available_value(5, "Random");

    ObjectToggleControl<DeviceBehaviour_Bamble> *fills_control = new ObjectToggleControl<DeviceBehaviour_Bamble>(
        "Fills",
        this,
        &DeviceBehaviour_Bamble::setFillsMode,
        &DeviceBehaviour_Bamble::getFillsMode
    );
    
    ObjectNumberControl<DeviceBehaviour_Bamble,float> *euclidian_density_control = new ObjectNumberControl<DeviceBehaviour_Bamble,float>(
        "Density",
        this,
        &DeviceBehaviour_Bamble::setDensity,
        &DeviceBehaviour_Bamble::getDensity,
        nullptr,
        (float)0.0,
        (float)1.0
    );
    euclidian_density_control->go_back_on_select = true;

    euclidian_options->add(euclidian_mode_control);
    euclidian_options->add(fills_control);
    euclidian_options->add(euclidian_density_control);


    // mutate pattern range bar ////////////////////////////////////////////////////////////////
    SubMenuItemBar *mutate_range = new SubMenuItemBar("Mutate pattern range");

    ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t> *minimum_pattern = new ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t>(
        "Low Mutate",
        this,
        &DeviceBehaviour_Bamble::setMinimumPattern,
        &DeviceBehaviour_Bamble::getMinimumPattern
    );
    for (unsigned int i = 0 ; i < sizeof(this->patterns) / sizeof(bamble_pattern) ; i++ ) {
        minimum_pattern->add_available_value(i, patterns[i].label);
    }
    minimum_pattern->go_back_on_select = true;
    mutate_range->add(minimum_pattern);

    ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t> *maximum_pattern = new ObjectSelectorControl<DeviceBehaviour_Bamble,int8_t>(
        "High Mutate",
        this,
        &DeviceBehaviour_Bamble::setMaximumPattern,
        &DeviceBehaviour_Bamble::getMaximumPattern
    );
    for (unsigned int i = 0 ; i < sizeof(this->patterns) / sizeof(bamble_pattern) ; i++ ) {
        maximum_pattern->add_available_value(i, patterns[i].label);
    }
    maximum_pattern->go_back_on_select = true;
    mutate_range->add(maximum_pattern);

    // select patterns on/off ////////////////////////////////////////////////////////////////
    ObjectMultiToggleControl *pattern_selector = new ObjectMultiToggleControl("Enabled drum patterns", true);
    for (unsigned int i = 0 ; i < 16/*sizeof(this->patterns) / sizeof(bamble_pattern)*/ ; i++ ) {
        pattern_selector->addItem(new PatternToggle(this, &patterns[i]));
    }

    ObjectMultiToggleControl *pattern_selector_2 = new ObjectMultiToggleControl("Enabled melody patterns", true);
    for (unsigned int i = 16 ; i < sizeof(this->patterns) / sizeof(bamble_pattern) ; i++ ) {
        pattern_selector_2->addItem(new PatternToggle(this, &patterns[i]));
    }


    //// further euclidian options
    SubMenuItemBar *further_euclidian_options = new SubMenuItemBar("Further Euclidian settings");
    further_euclidian_options->add(new ObjectToggleControl<DeviceBehaviour_Bamble>(
        "Add Phrase", 
        this,
        &DeviceBehaviour_Bamble::setEuclidianSeedUsePhrase,
        &DeviceBehaviour_Bamble::getEuclidianSeedUsePhrase
    ));
    further_euclidian_options->add(new ObjectToggleControl<DeviceBehaviour_Bamble>(
        "Auto-reset",
        this,
        &DeviceBehaviour_Bamble::setEuclidianResetBeforeMutate,
        &DeviceBehaviour_Bamble::getEuclidianResetBeforeMutate
    ));
    further_euclidian_options->add(new ObjectNumberControl<DeviceBehaviour_Bamble,uint16_t>(
        "Seed bank", 
        this, 
        &DeviceBehaviour_Bamble::setEuclidianSeedModifier, 
        &DeviceBehaviour_Bamble::getEuclidianSeedModifier,
        nullptr,
        (uint16_t)0,
        (uint16_t)16384
    ));
    further_euclidian_options->add(new ObjectActionItem<DeviceBehaviour_Bamble>(
       "RaNdOm",
       this,
       &DeviceBehaviour_Bamble::setRandomSeed
    ));

    //Serial.printf("BambleTriggerOnSelectorControl creating...\n"); Serial.flush();
    BambleTriggerOnBar *a = new BambleTriggerOnBar("Envelope PT 1", this, 5);
    BambleTriggerOnBar *b = new BambleTriggerOnBar("Envelope PT 2", this, 6);
    BambleTriggerOnBar *c = new BambleTriggerOnBar("Envelope PT 3", this, 7);
    BambleTriggerOnBar *d = new BambleTriggerOnBar("Envelope PT 4", this, 8);
    b->set_available_values(a->get_available_values());
    c->set_available_values(a->get_available_values());
    d->set_available_values(a->get_available_values());
    /*Serial.printf("BambleTriggerOnSelectorControl created all...\n"); Serial.flush();
    Serial.printf("\t!!!a has %i available_values\n", a->get_available_values()->size());
    Serial.printf("\t!!!b has %i available_values\n", b->get_available_values()->size());*/
    menuitems->add(a);
    menuitems->add(b);
    menuitems->add(c);
    menuitems->add(d);

    //// add all to final menu 
    menuitems->add(euclidian_options);
    menuitems->add(further_euclidian_options);
    menuitems->add(mutate_range);
    menuitems->add(pattern_selector);
    menuitems->add(pattern_selector_2);

    DividedClockedBehaviour::make_menu_items();

    return menuitems;
}

#endif