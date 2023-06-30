
#include "Config.h"

#include "cv_input.h"

#include "ParameterManager.h"
#ifdef ENABLE_SCREEN
    #include "colours.h"
    #include "submenuitem.h"
#endif

#ifdef ENABLE_CV_INPUT
    #include "devices/ADCPimoroni24v.h"
#endif

#include "behaviours/behaviour_base.h"
#include "behaviours/behaviour_craftsynth.h"

ParameterManager *parameter_manager = new ParameterManager(LOOP_LENGTH_TICKS);

// initialise the voltage-reading hardware/libraries and the ParameterManager
FLASHMEM
void setup_cv_input() {
    Serial.println("setup_cv_input...");
    tft_print("...setup_cv_input...\n");

    parameter_manager->init();

    #ifdef ENABLE_CV_INPUT
        parameter_manager->addADCDevice(new ADCPimoroni24v(ENABLE_CV_INPUT, 5.0)); //, 5.0)); //, 2, MAX_INPUT_VOLTAGE_24V));
    #endif

    parameter_manager->auto_init_devices();

    tft_print("..done setup_cv_input\n");
}

// initialise the input voltage ParameterInputs that can be mapped to Parameters
FLASHMEM 
void setup_parameters() {
    //parameter_manager = new ParameterManager();
    // add the available parameters to a list used globally and later passed to each selector menuitem
    Serial.println(F("==== begin setup_parameters ====")); Serial_flush();
    tft_print("..setup_parameters...");

    // initialise the voltage source inputs
    // todo: improve this bit, maybe name the voltage sources?
    #ifdef ENABLE_CV_INPUT
        VoltageParameterInput *vpi1 = new VoltageParameterInput((char*)"A", parameter_manager->voltage_sources->get(0));
        VoltageParameterInput *vpi2 = new VoltageParameterInput((char*)"B", parameter_manager->voltage_sources->get(1));
        VoltageParameterInput *vpi3 = new VoltageParameterInput((char*)"C", parameter_manager->voltage_sources->get(2));

        //vpi3->input_type = UNIPOLAR;
        // todo: set up 1v/oct inputs to map to MIDI source_ids...

        // tell the parameter manager about them
        parameter_manager->addInput(vpi1);
        parameter_manager->addInput(vpi2);
        parameter_manager->addInput(vpi3);
    #endif

    // get the available target parameters
    // todo: dynamically pull these from all available behaviours
    // todo: dynamically pull them from other things that could have parameters available
    // todo: move this to the behaviour initialising!
    /*#ifdef ENABLE_CRAFTSYNTH_USB
        Serial.println(F("setup_parameters() about to do get_parameters on behaviour_craftsynth..")); Serial_flush();
        LinkedList<FloatParameter*> *params = behaviour_craftsynth->get_parameters();
        Serial.println(F("setup_parameters() just did get_parameters on behaviour_craftsynth.. about to addParameters()")); Serial_flush();
        parameter_manager->addParameters(params);
        Serial.println(F("setup_parameters() just did parameter_manager->addParameters(params)")); Serial_flush();

        // setup the default mappings
        // TODO: load this from a saved config file
        // hmmm if this section is uncommented then it causes 'conflicting section type' problems due to FLASHMEM..?
        //Serial.println(F("=========== SETTING DEFAULT PARAMETER MAPS.........")); Serial_flush();
        //behaviour_craftsynth->getParameterForLabel((char*)F("Filter Cutoff"))->set_slot_0_amount(1.0); //->connect_input(vpi1, 1.0);
        //behaviour_craftsynth->getParameterForLabel((char*)F("Filter Morph"))->set_slot_1_amount(1.0); //connect_input(vpi2, 1.0);
        //behaviour_craftsynth->getParameterForLabel((char*)F("Distortion"))->set_slot_2_amount(1.0); //connect_input(vpi3, 1.0);
        //Serial.println(F("=========== FINISHED SETTING DEFAULT PARAMETER MAPS")); Serial_flush();
    #endif*/
    Serial.println("starting allParameters...");
    for(unsigned int i = 0 ; i < behaviour_manager->behaviours->size() ; i++) {
        Serial.printf("\tdoing addParameters for behaviour %i/%i\n", i+1, behaviour_manager->behaviours->size());
        if (behaviour_manager->behaviours->get(i)==nullptr) {
            Serial.printf("\tbehaviour %i is nullptr!!\n", i);
            continue;
        } else {
            Serial.printf("\tbehaviour %s\n", behaviour_manager->behaviours->get(i)->get_label());
        }
        parameter_manager->addParameters(behaviour_manager->behaviours->get(i)->get_parameters());
    }
    Serial.println("finished allParameters.");

    Serial.println("about to parameter_manager->setDefaultParameterConnections()..");
    parameter_manager->setDefaultParameterConnections();


    tft_print("done.\n");
}

#ifdef ENABLE_SCREEN
// set up the menus to provide control over the Parameters and ParameterInputs
FLASHMEM void setup_parameter_menu() {
    Serial.println(F("==== setup_parameter_menu starting ===="));

    Serial.println(F("Adding ParameterSelectorControls for available_inputs..."));
    // ask ParameterManager to add all the menu items for the ParameterInputs
    parameter_manager->addAllParameterInputMenuItems(menu, "CV Input ");

    // ask ParameterManager to add all the menu items for the Parameters
    // todo: dynamically loop over all the available behaviours
    /*#ifdef ENABLE_CRAFTSYNTH_USB
        parameter_manager->addParameterSubMenuItems(
            menu, 
            behaviour_craftsynth->get_label(), 
            behaviour_craftsynth->get_parameters()
        );
    #endif*/

    //parameter_manager->addAllVoltageSourceMenuItems(menu);
    parameter_manager->addAllVoltageSourceCalibrationMenuItems(menu);

    //DirectNumberControl<int> *mixer_profile = new DirectNumberControl<int>("Mixer profiling", &parameter_manager->profile_update_mixers, parameter_manager->profile_update_mixers, (int)0, (int)1000000, nullptr);
    //menu->add(mixer_profile);

    Serial.println(F("setup_parameter_menu done =================="));
}
#endif