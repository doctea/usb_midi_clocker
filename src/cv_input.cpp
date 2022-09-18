#include "Config.h"

#include "cv_input.h"

#include "ParameterManager.h"
#include "submenuitem.h"

#include "devices/ADCPimoroni24v.h"

//#include "midi_mapper_matrix_manager.h"

#include "behaviour_base.h"
//#include "behaviour_manager.h"
#include "behaviour_craftsynth.h"

ParameterManager parameter_manager = ParameterManager();

// initialise the voltage-reading hardware/librareis and the ParameterManager
void setup_cv_input() {
    Serial.println((char*)"setup_cv_input...");
    tft_print((char*)"...setup_cv_input...\n");

    parameter_manager.init();

    #ifdef ENABLE_CV_INPUT
        parameter_manager.addADCDevice(new ADCPimoroni24v(ENABLE_CV_INPUT, 5.0)); //, 5.0)); //, 2, MAX_INPUT_VOLTAGE_24V));
    #endif

    parameter_manager.auto_init_devices();

    tft_print((char*)"..done setup_cv_input\n");
}

// initialise the input voltage ParameterInputs that can be mapped to Parameters
void setup_parameters() {
    // add the available parameters to a list used globally and later passed to each selector menuitem
    Serial.println(F("==== begin setup_parameters ===="));
    tft_print((char*)"setup_parameters...");

    // initialise the voltage source inputs
    // todo: improve this bit, maybe name the voltage sources?
    VoltageParameterInput<BaseParameter> *vpi1 = new VoltageParameterInput<BaseParameter>('A', parameter_manager.voltage_sources.get(0));
    VoltageParameterInput<BaseParameter> *vpi2 = new VoltageParameterInput<BaseParameter>('B', parameter_manager.voltage_sources.get(1));
    VoltageParameterInput<BaseParameter> *vpi3 = new VoltageParameterInput<BaseParameter>('C', parameter_manager.voltage_sources.get(2));

    vpi3->input_type = UNIPOLAR;

    // todo: set up 1v/oct inputs to map to MIDI source_ids...

    // tell the parameter manager about them
    parameter_manager.addInput(vpi1);
    parameter_manager.addInput(vpi2);
    parameter_manager.addInput(vpi3);

    // get the available target parameters
    // todo: dynamically pull these from all available behaviours
    // todo: dynamically pull them from other things that could have parameters available
    LinkedList<DoubleParameter*> *params = behaviour_craftsynth->get_parameters();
    parameter_manager.addParameters(params);

    // setup the default mappings
    // TODO: make this a property of the Parameter, not the ParameterInputs, so we can have multiple mappings
    // TODO: load this from a saved config file
    /*Serial.println("=========== SETTING DEFAULT PARAMETER MAPS.........");
    vpi1->setTarget(behaviour_craftsynth->getParameterForLabel("Cutoff")); 
    vpi2->setTarget(behaviour_craftsynth->getParameterForLabel("Filter Morph"));
    vpi3->setTarget(behaviour_craftsynth->getParameterForLabel("Distortion")); 
    Serial.println("=========== FINISHED SETTING DEFAULT PARAMETER MAPS");*/
    Serial.println("=========== SETTING DEFAULT PARAMETER MAPS.........");
    behaviour_craftsynth->getParameterForLabel("Cutoff")->connect_input(vpi1, 1.0);
    behaviour_craftsynth->getParameterForLabel("Filter Morph")->connect_input(vpi2, 1.0);
    behaviour_craftsynth->getParameterForLabel("Distortion")->connect_input(vpi3, 1.0);
    Serial.println("=========== FINISHED SETTING DEFAULT PARAMETER MAPS");

    tft_print((char*)"\n");
}

// set up the menus to provide control over the Parameters and ParameterInputs
void setup_parameter_menu() {
    Serial.println("==== setup_parameter_menu starting ====");

    Serial.println("Adding ParameterSelectorControls for available_inputs...");
    // ask ParameterManager to add all the menu items for the ParameterInputs
    parameter_manager.addAllParameterInputMenuItems(menu);

    // ask ParameterManager to add all the menu items for the Parameters
    // todo: dynamically loop over all the available behaviours
    parameter_manager.addParameterSubMenuItems(menu, behaviour_craftsynth->get_label(), behaviour_craftsynth->get_parameters());

    Serial.println("setup_parameter_menu done ==================");
}


