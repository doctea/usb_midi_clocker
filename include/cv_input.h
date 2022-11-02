#include <Arduino.h>

#include "Config.h"

#include "menu.h"
#include "parameters/Parameter.h"
#include "parameters/FunctionParameter.h"
#include <LinkedList.h>

#include "parameter_inputs/VoltageParameterInput.h"

#include "parameters/MIDICCParameter.h"

#include "Wire.h"

#include "voltage_sources/ADS24vVoltageSource.h"

#include "mymenu_items/ParameterMenuItems.h"

#include "ParameterManager.h"

//#define MAX_INPUT_VOLTAGE_24V 10.0

//FLASHMEM 
void setup_cv_input();
//FLASHMEM
void setup_parameters();
void setup_parameter_menu();

extern ParameterManager *parameter_manager;