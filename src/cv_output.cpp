#include "Config.h"
#include "cv_output.h"

cvoutput_config_t cvoutput_configs[] = {
    /*#ifdef ENABLE_CV_OUTPUT_1
        { ENABLE_CV_OUTPUT, ENABLE_CV_OUTPUT_EXTENDED_ADDRESS, ENABLE_CV_OUTPUT_1_GATE_BANK, ENABLE_CV_OUTPUT_1_GATE_BANK_OFFSET, ENABLE_CV_OUTPUT_CALIBRATION_INPUT_NAMES },
    #endif
    #ifdef ENABLE_CV_OUTPUT_2
        { ENABLE_CV_OUTPUT_2, ENABLE_CV_OUTPUT_2_EXTENDED_ADDRESS, ENABLE_CV_OUTPUT_2_GATE_BANK, ENABLE_CV_OUTPUT_2_GATE_BANK_OFFSET, ENABLE_CV_OUTPUT_2_CALIBRATION_INPUT_NAMES },
    #endif
    #ifdef ENABLE_CV_OUTPUT_3
        { ENABLE_CV_OUTPUT_3, ENABLE_CV_OUTPUT_3_EXTENDED_ADDRESS, ENABLE_CV_OUTPUT_3_GATE_BANK, ENABLE_CV_OUTPUT_2_GATE_BANK_OFFSET, ENABLE_CV_OUTPUT_3_CALIBRATION_INPUT_NAMES } }
    #endif*/
    #ifdef CONFIG_CV_OUTPUT_1
        CONFIG_CV_OUTPUT_1,
    #endif
    #ifdef CONFIG_CV_OUTPUT_2
        CONFIG_CV_OUTPUT_2,
    #endif
    #ifdef CONFIG_CV_OUTPUT_3
        CONFIG_CV_OUTPUT_3
    #endif
};

unsigned int cvoutput_configs_size = sizeof(cvoutput_configs)/sizeof(cvoutput_config_t);
