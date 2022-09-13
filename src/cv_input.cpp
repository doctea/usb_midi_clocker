#include "Config.h"

#include "cv_input.h"

#include "ParameterManager.h"

#include "devices/ADCPimoroni24v.h"

//#include "midi_mapper_matrix_manager.h"

#include "behaviour_base.h"
//#include "behaviour_manager.h"
#include "behaviour_craftsynth.h"

/*LinkedList<VoltageSourceBase*> voltage_sources = LinkedList<VoltageSourceBase*> ();

ADS1015 ADS_OBJECT_24V(0x49);
ADS24vVoltageSource<ADS1015> voltage_source_1_channel_0 = ADS24vVoltageSource<ADS1015>(&ADS_OBJECT_24V, 0, MAX_INPUT_VOLTAGE_24V);
ADS24vVoltageSource<ADS1015> voltage_source_1_channel_1 = ADS24vVoltageSource<ADS1015>(&ADS_OBJECT_24V, 1, MAX_INPUT_VOLTAGE_24V);*/
//ADS24vVoltageSource<ADS1015> voltage_source_1_channel_2 = ADS24vVoltageSource<ADS1015>(&ADS_OBJECT_24V, 2, MAX_INPUT_VOLTAGE_24V);

ParameterManager parameter_manager = ParameterManager();

void setup_cv_input() {
    Serial.println((char*)"setup_cv_input...");
    tft_print((char*)"...setup_cv_input...\n");

    /*//Serial.println("Beginning ADS_x48..");
    Serial.println("Beginning ADS_24V..");
    ADS_OBJECT_24V.begin();
    ADS_OBJECT_24V.setGain(2);
    voltage_sources.add(&voltage_source_1_channel_0);
    voltage_sources.add(&voltage_source_1_channel_1);
    //voltage_sources.add(&voltage_source_1_channel_2);
    voltage_source_1_channel_0.correction_value_1 = 1180.0;    */
    //parameter_manager.debug = true;
    parameter_manager.init();
    parameter_manager.addADCDevice(new ADCPimoroni24v(ENABLE_CV_INPUT)); //, 2, MAX_INPUT_VOLTAGE_24V));

    parameter_manager.auto_init_devices();

    tft_print((char*)"..done setup_cv_input\n");
}

/*
// ParameterInputs, ie wrappers around input mechanism, assignable to a Parameter
LinkedList<BaseParameterInput*> available_inputs            = LinkedList<BaseParameterInput*>();

// Parameters, ie wrappers around destination object
LinkedList<DataParameter*>      available_parameters    = LinkedList<DataParameter*>();

//DataParameter* parameter_a = nullptr;
DataParameter  param_none                              = DataParameter((char*)"None");
*/

void setup_parameters() {
    // add the available parameters to a list used globally and later passed to each selector menuitem
    Serial.println(F("==== begin setup_parameters ===="));
    tft_print((char*)"setup_parameters...");

    MIDICCParameter *parameter_a = new MIDICCParameter(
        (char*)"CS Cutoff", 
        //(MIDIOutputWrapper*)midi_matrix_manager->get_target_for_handle((char*)"USB : CraftSynth : ch 1"), 
        behaviour_craftsynth,
        (byte)34,
        (byte)1
    );    // cutoff
    MIDICCParameter *parameter_b = new MIDICCParameter(
        (char*)"CS Spread", 
        //(MIDIOutputWrapper*)midi_matrix_manager->get_target_for_handle((char*)"USB : CraftSynth : ch 1"), 
        behaviour_craftsynth,
        (byte)20,
        (byte)1
    );    // spread

    parameter_manager.addParameter(parameter_a);
    parameter_manager.addParameter(parameter_b);

    // todo: improve this bit, maybe name the voltage sources?
    VoltageParameterInput<BaseParameter> *vpi1 = new VoltageParameterInput<BaseParameter>('A', parameter_manager.voltage_sources.get(0));
    VoltageParameterInput<BaseParameter> *vpi2 = new VoltageParameterInput<BaseParameter>('B', parameter_manager.voltage_sources.get(1));
    vpi1->setTarget(parameter_a);
    vpi2->setTarget(parameter_b);
    parameter_manager.addInput(vpi1);
    parameter_manager.addInput(vpi2);

    /*VoltageParameterInput<BaseParameter> *vpi1 = new VoltageParameterInput<BaseParameter>('A', voltage_sources.get(0));
    available_inputs.add(vpi1);
    vpi1->setTarget(parameter_a);

    VoltageParameterInput<BaseParameter> *vpi2 = new VoltageParameterInput<BaseParameter>('B', voltage_sources.get(1));
    available_inputs.add(vpi2);
    vpi2->setTarget(parameter_b);

    //vpi1->debug = parameter_a->debug = true;

    //VoltageParameterInput<BaseParameter> *vpi3 = new VoltageParameterInput<BaseParameter>('C', voltage_sources.get(2));
    //available_inputs.add(vpi3);

    available_parameters.add(&param_none);
    available_parameters.add(parameter_a);
    available_parameters.add(parameter_b);

    //vpi2->setTarget(parameter_b);*/

    tft_print((char*)"\n");
}
    
void setup_parameter_menu() {
    Serial.println("==== setup_parameter_menu starting ====");
    //menu->add(&PulseWidthModulationPanel);
    //menu->add(&testitem);

    Serial.println("Adding ParameterSelectorControls for available_inputs...");
    parameter_manager.addAllParameterInputMenuItems(menu);
    parameter_manager.addAllParameterMenuItems(menu);

    /*for (int i = 0 ; i < available_inputs.size() ; i++) {
        BaseParameterInput *param_input = available_inputs.get(i);
        BaseParameter *param = param_input->target_parameter;

        Serial.printf("About to rename param_input %i '%c'..\n", i, param_input->name);

        param_input->name = i+'A';

        char input_label[20];
        //sprintf(input_label, "Input %i => %c", i<4?0:1, param_input->name); //i+'A');
        sprintf(input_label, "Input %c", param_input->name); //i+'A');

        Serial.printf("%i: Creating control labelled '%s'...\n", i, input_label); Serial.flush();
        ParameterSelectorControl *ctrl = new ParameterSelectorControl(input_label);

        if (param==nullptr) {
            Serial.printf("\t%i: Configuring it with available_inputs item target (no target_parameter already set)..\n", i); Serial.flush();    
        } else {
            Serial.printf("\t%i: Configuring it with available_inputs item target '%s'..\n", i, param->label); Serial.flush();
        }
        //BaseParameterInput *param_input = available_inputs.get(i);
        ctrl->configure(param_input, &available_parameters);

        Serial.println("\tConfigured!  Adding to menu..");
        //ctrl->debug = true;
        menu->add(ctrl);
        Serial.println("\tAdded!"); Serial.flush();
    }*/

    /*
    Serial.println("Starting add available_parameters loop..."); Serial.flush();
    Serial.printf("Got %i parameters..\n", available_parameters.size());
    for (int i = 0 ; i < available_parameters.size() ; i++) {
        DataParameter *p = available_parameters.get(i);
        if (strcmp(p->label,"None")==0) 
            continue;
        Serial.printf("\tinstantiating ParameterMenuItem for item %i aka %s\n", i, p->label); Serial.flush();
        MenuItem *ctrl = p->makeControl();
        Serial.printf("\tadding it to menu...\n", i, p->label); Serial.flush();
        menu->add(ctrl);
    }
    Serial.println("Finished adding available_parameters loop"); Serial.flush();
    */

    Serial.println("setup_parameter_menu done ==================");
}


// called every loop to update the inputs; maybe in future we also eventually want to process internal LFOs or other properties of parameter here
/*void update_parameters() {
    for (int i = 0 ; i < available_inputs.size() ; i++) {
        available_inputs.get(i)->loop();
        //parameter_inputs[i]->loop();
    }
}*/