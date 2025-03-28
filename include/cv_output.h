#pragma once

#include "Config.h"

#if defined(ENABLE_CV_OUTPUT) || defined(ENABLE_CV_OUTPUT_2) || defined(ENABLE_CV_OUTPUT_3) || defined(ENABLE_CV_OUTPUT_4) 
    #if __has_include("DAC8574.h")
        #include "DAC8574.h"
    #endif

    #include "parameters/calibration.h"

    #include "interfaces/interfaces.h"

    template<class DACClass>
    class DeviceBehaviour_CVOutput;

    struct cvoutput_config_t {
        uint8_t address;
        uint8_t dac_extended_address;
        GATEBANKS gate_bank;
        int8_t gate_offset;
        const char *calibration_input_names[4];
        DeviceBehaviour_CVOutput<DAC8574> *behaviour = nullptr;
    };

    extern cvoutput_config_t cvoutput_configs[];
    extern unsigned int cvoutput_configs_size;

#endif