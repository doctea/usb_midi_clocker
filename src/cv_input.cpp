
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
    #ifdef ENABLE_CV_INPUT
        Wire.begin();
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

    int8_t used_sources = 0;

    // initialise the voltage source inputs
    // todo: improve this bit, maybe name the voltage sources?
    #ifdef ENABLE_CV_INPUT
        tft_print("...adding VoltageParameterInputs for ADC1 CV source!\n");
        #ifndef ENABLE_CV_INPUT_ORDER
            #define ENABLE_CV_INPUT_ORDER {0,1,2} 
        #endif
        {
        int8_t input_orders[3] = ENABLE_CV_INPUT_ORDER;
        VoltageParameterInput *vpi1 = new VoltageParameterInput((char*)"A", "ADC1", parameter_manager->voltage_sources->get(input_orders[0]+used_sources));
        VoltageParameterInput *vpi2 = new VoltageParameterInput((char*)"B", "ADC1", parameter_manager->voltage_sources->get(input_orders[1]+used_sources));
        VoltageParameterInput *vpi3 = new VoltageParameterInput((char*)"C", "ADC1", parameter_manager->voltage_sources->get(input_orders[2]+used_sources));
        //vpi3->input_type = UNIPOLAR;

        // tell the parameter manager about them
        parameter_manager->addInput(vpi1);
        parameter_manager->addInput(vpi2);
        parameter_manager->addInput(vpi3);

        used_sources += 3;
        }
    #endif

    #ifdef ENABLE_CV_INPUT_2
        tft_print("...adding VoltageParameterInputs for ADC2 CV source!\n");
        #ifndef ENABLE_CV_INPUT_2_ORDER
            #define ENABLE_CV_INPUT_2_ORDER {0,1,2}
        #endif
        {
        int8_t input_orders[3] = ENABLE_CV_INPUT_2_ORDER;
        VoltageParameterInput *vpi4 = new VoltageParameterInput((char*)"D", "ADC2", parameter_manager->voltage_sources->get(input_orders[0]+used_sources));
        VoltageParameterInput *vpi5 = new VoltageParameterInput((char*)"E", "ADC2", parameter_manager->voltage_sources->get(input_orders[1]+used_sources));
        VoltageParameterInput *vpi6 = new VoltageParameterInput((char*)"F", "ADC2", parameter_manager->voltage_sources->get(input_orders[2]+used_sources));
        //vpi3->input_type = UNIPOLAR;

        // tell the parameter manager about them
        parameter_manager->addInput(vpi4);
        parameter_manager->addInput(vpi5);
        parameter_manager->addInput(vpi6);

        used_sources += 3;
        }
    #endif

    VirtualParameterInput *virtpi1 = new VirtualParameterInput((char*)"LFO sync", "LFOs", LFO_LOCKED);
    VirtualParameterInput *virtpi2 = new VirtualParameterInput((char*)"LFO free", "LFOs", LFO_FREE);
    VirtualParameterInput *virtpi3 = new VirtualParameterInput((char*)"Random",   "LFOs", RAND);
    parameter_manager->addInput(virtpi1);
    parameter_manager->addInput(virtpi2);
    parameter_manager->addInput(virtpi3);

    VirtualMixerParameterInput *mixerpi1 = new VirtualMixerParameterInput((char*)"Mix 1", "Mixers");
    VirtualMixerParameterInput *mixerpi2 = new VirtualMixerParameterInput((char*)"Mix 2", "Mixers");
    VirtualMixerParameterInput *mixerpi3 = new VirtualMixerParameterInput((char*)"Mix 3", "Mixers");
    parameter_manager->addInput(mixerpi1);
    parameter_manager->addInput(mixerpi2);
    parameter_manager->addInput(mixerpi3);

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

#ifdef ENABLE_CV_OUTPUT
    // todo: move this to its own file out of cv_input.cpp?
    #include "behaviours/behaviour_cvoutput.h"
    extern DeviceBehaviour_CVOutput<DAC8574> *behaviour_cvoutput_1, *behaviour_cvoutput_2, *behaviour_cvoutput_3;

    void setup_cv_output_parameter_inputs() {
        #if defined(ENABLE_CV_OUTPUT)
            #ifdef ENABLE_CV_OUTPUT_CALIBRATION_INPUT_NAMES
                const char *CV_OUTPUT_CALIBRATION_INPUT_NAMES[] = ENABLE_CV_OUTPUT_CALIBRATION_INPUT_NAMES;
                behaviour_cvoutput_1->set_calibration_parameter_input(0, CV_OUTPUT_CALIBRATION_INPUT_NAMES[0]);
                behaviour_cvoutput_1->set_calibration_parameter_input(1, CV_OUTPUT_CALIBRATION_INPUT_NAMES[1]);
                behaviour_cvoutput_1->set_calibration_parameter_input(2, CV_OUTPUT_CALIBRATION_INPUT_NAMES[2]);
                behaviour_cvoutput_1->set_calibration_parameter_input(3, CV_OUTPUT_CALIBRATION_INPUT_NAMES[3]);
            #endif
        #endif
        #if defined(ENABLE_CV_OUTPUT_2)
            #ifdef ENABLE_CV_OUTPUT_2_CALIBRATION_INPUT_NAMES
                const char *CV_OUTPUT_2_CALIBRATION_INPUT_NAMES[] = ENABLE_CV_OUTPUT_2_CALIBRATION_INPUT_NAMES;
                behaviour_cvoutput_2->set_calibration_parameter_input(0, CV_OUTPUT_2_CALIBRATION_INPUT_NAMES[0]);
                behaviour_cvoutput_2->set_calibration_parameter_input(1, CV_OUTPUT_2_CALIBRATION_INPUT_NAMES[1]);
                behaviour_cvoutput_2->set_calibration_parameter_input(2, CV_OUTPUT_2_CALIBRATION_INPUT_NAMES[2]);
                behaviour_cvoutput_2->set_calibration_parameter_input(3, CV_OUTPUT_2_CALIBRATION_INPUT_NAMES[3]);
            #endif
        #endif
    }
#endif

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

    #ifdef ENABLE_CV_OUTPUT
        setup_cv_output_parameter_inputs();
        // ask ParameterManager to add all the menu items for the CVOutputs
        parameter_manager->addAllCVOutputCalibrationMenuItems(menu);
    #endif

    //DirectNumberControl<int> *mixer_profile = new DirectNumberControl<int>("Mixer profiling", &parameter_manager->profile_update_mixers, parameter_manager->profile_update_mixers, (int)0, (int)1000000, nullptr);
    //menu->add(mixer_profile);

    Serial.println(F("setup_parameter_menu done =================="));
}
#endif