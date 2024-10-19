
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
#include "behaviours/behaviour_manager.h"

#include "parameter_inputs/VirtualParameterInput.h"
#include "parameter_inputs/MixerParameterInput.h"

ParameterManager *parameter_manager = new ParameterManager(LOOP_LENGTH_TICKS);

// initialise the voltage-reading hardware/libraries and the ParameterManager
//FLASHMEM
void setup_cv_input() {
    //Serial.println("setup_cv_input...");
    tft_print("...setup_cv_input...\n");

    parameter_manager->init();

    Wire.begin();

    #ifdef ENABLE_CV_INPUT
        tft_print("...adding ADCPimoroni24v #1!\n");
        parameter_manager->addADCDevice(new ADCPimoroni24v(ENABLE_CV_INPUT, &Wire, 5.0));
    #endif
    #ifdef ENABLE_CV_INPUT_2
        tft_print("...adding ADCPimoroni24v #2!\n");
        parameter_manager->addADCDevice(new ADCPimoroni24v(ENABLE_CV_INPUT_2, &Wire, 5.0));
    #endif

    parameter_manager->auto_init_devices();

    tft_print("..done setup_cv_input\n");
}

// initialise the input voltage ParameterInputs that can be mapped to Parameters
//FLASHMEM 
void setup_parameters() {
    //Serial.println(F("==== begin setup_parameters ====")); Serial_flush();
    //tft_print("..setup_parameters...");

    // initialise the voltage source inputs
    // todo: improve this bit, maybe name the voltage sources?
    #ifdef ENABLE_CV_INPUT
        tft_print("...adding VoltageParameterInputs for ADC1 CV source!\n");
        VoltageParameterInput *vpi1 = new VoltageParameterInput((char*)"A", "ADC1", parameter_manager->voltage_sources->get(0));
        VoltageParameterInput *vpi2 = new VoltageParameterInput((char*)"B", "ADC1", parameter_manager->voltage_sources->get(1));
        VoltageParameterInput *vpi3 = new VoltageParameterInput((char*)"C", "ADC1", parameter_manager->voltage_sources->get(2));
        //vpi3->input_type = UNIPOLAR;

        // tell the parameter manager about them
        parameter_manager->addInput(vpi1);
        parameter_manager->addInput(vpi2);
        parameter_manager->addInput(vpi3);
    #endif

    #ifdef ENABLE_CV_INPUT_2
        tft_print("...adding VoltageParameterInputs for ADC2 CV source!\n");
        VoltageParameterInput *vpi4 = new VoltageParameterInput((char*)"D", "ADC2", parameter_manager->voltage_sources->get(3));
        VoltageParameterInput *vpi5 = new VoltageParameterInput((char*)"E", "ADC2", parameter_manager->voltage_sources->get(4));
        VoltageParameterInput *vpi6 = new VoltageParameterInput((char*)"F", "ADC2", parameter_manager->voltage_sources->get(5));
        //vpi3->input_type = UNIPOLAR;

        // tell the parameter manager about them
        parameter_manager->addInput(vpi4);
        parameter_manager->addInput(vpi5);
        parameter_manager->addInput(vpi6);
    #endif

    VirtualParameterInput *virtpi1 = new VirtualParameterInput((char*)"LFO sync", "LFOs", LFO_LOCKED);
    VirtualParameterInput *virtpi2 = new VirtualParameterInput((char*)"LFO free", "LFOs", LFO_FREE);
    VirtualParameterInput *virtpi3 = new VirtualParameterInput((char*)"Random",   "LFOs", RAND);
    parameter_manager->addInput(virtpi1);
    parameter_manager->addInput(virtpi2);
    parameter_manager->addInput(virtpi3);

    VirtualMixerParameterInput *mixerpi1 = new VirtualMixerParameterInput((char*)"Mixer", "Mixers");
    parameter_manager->addInput(mixerpi1);

    // get the available target parameters
    // todo: dynamically pull them from other things that could have parameters available

    // add Parameters from registered Behaviours
    // todo: move this to the behaviour initialising!
    for(unsigned int i = 0 ; i < behaviour_manager->behaviours->size() ; i++) {
        //Serial.printf("\tdoing addParameters for behaviour %i/%i\n", i+1, behaviour_manager->behaviours->size());
        if (behaviour_manager->behaviours->get(i)==nullptr) {
            //Serial.printf("\tbehaviour %i is nullptr!!\n", i);
            continue;
        } else {
            //Serial.printf("\tbehaviour %s\n", behaviour_manager->behaviours->get(i)->get_label());
        }
        parameter_manager->addParameters(behaviour_manager->behaviours->get(i)->get_parameters());
    }
    //Serial.println("finished allParameters.");

    //Serial.println("about to parameter_manager->setDefaultParameterConnections()..");
    parameter_manager->setDefaultParameterConnections();

    tft_print("done.\n");
}

#ifdef ENABLE_SCREEN
// set up the menus to provide control over the ParameterInputs and VoltageSources
//FLASHMEM 
void setup_parameter_menu() {
    Serial.println(F("==== setup_parameter_menu starting ===="));
    Serial.println(F("Adding ParameterSelectorControls for available_inputs..."));
    // ask ParameterManager to add all the menu items for the ParameterInputs
    parameter_manager->addAllParameterInputMenuItems(menu);

    // ask ParameterManager to add all the menu items for the Voltage Sources
    parameter_manager->addAllVoltageSourceCalibrationMenuItems(menu, true);

    //DirectNumberControl<int> *mixer_profile = new DirectNumberControl<int>("Mixer profiling", &parameter_manager->profile_update_mixers, parameter_manager->profile_update_mixers, (int)0, (int)1000000, nullptr);
    //menu->add(mixer_profile);

    Serial.println(F("setup_parameter_menu done =================="));
}
#endif