#include <Arduino.h>

#include "Config.h"

#ifdef ENABLE_SCREEN
    #include "menu.h"
#endif
#include "parameters/Parameter.h"
#include "parameters/FunctionParameter.h"
#include <LinkedList.h>

#include "parameters/MIDICCParameter.h"

#include "Wire.h"

#ifdef ENABLE_CV_INPUT
    #include "parameter_inputs/VoltageParameterInput.h"
    #include "voltage_sources/ADS24vVoltageSource.h"
#endif

#ifdef ENABLE_SCREEN
    #include "mymenu_items/ParameterMenuItems.h"
#endif

#include "ParameterManager.h"

//#define MAX_INPUT_VOLTAGE_24V 10.0

//FLASHMEM 
void setup_cv_input();
//FLASHMEM
void setup_parameters();
#ifdef ENABLE_SCREEN
void setup_parameter_menu();
#endif

extern ParameterManager *parameter_manager;