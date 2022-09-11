#include <Arduino.h>

#include "menu.h"
#include "parameters/Parameter.h"
#include "parameters/FunctionParameter.h"
#include <LinkedList.h>

#include "Wire.h"

#include "voltage_sources/ADS24vVoltageSource.h"

#define MAX_INPUT_VOLTAGE_24V 10.0

LinkedList<VoltageSourceBase*> voltage_sources = LinkedList<VoltageSourceBase*> ();

ADS1015 ADS_OBJECT_24V(0x49);
ADS24vVoltageSource<ADS1015> voltage_source_1_channel_0 = ADS24vVoltageSource<ADS1015>(&ADS_OBJECT_24V, 0, MAX_INPUT_VOLTAGE_24V);
ADS24vVoltageSource<ADS1015> voltage_source_1_channel_1 = ADS24vVoltageSource<ADS1015>(&ADS_OBJECT_24V, 1, MAX_INPUT_VOLTAGE_24V);
ADS24vVoltageSource<ADS1015> voltage_source_1_channel_2 = ADS24vVoltageSource<ADS1015>(&ADS_OBJECT_24V, 2, MAX_INPUT_VOLTAGE_24V);

void setup_cv_input() {
    Serial.println("setup_cv_input...");
    tft_print("setup_cv_input...");

    Serial.println("Beginning ADS_x48..");
    Serial.println("Beginning ADS_24V..");
    ADS_OBJECT_24V.begin();
    ADS_OBJECT_24V.setGain(2);
    voltage_sources.add(&voltage_source_1_channel_0);
    voltage_sources.add(&voltage_source_1_channel_1);
    voltage_sources.add(&voltage_source_1_channel_2);
    voltage_source_1_channel_0.correction_value_1 = 1180.0;    
    Serial.println("..done ADS_x48");
}

void update_voltage_sources() {
    // round robin reading so they get a chance to settle in between adc reads?
    int size = voltage_sources.size();
    if (size==0) return;

    static int last_read = 0;
    voltage_sources.get(last_read)->update();
    Serial.printf("update_voltage_sources read %i from %i\n", voltage_sources.get(last_read).current_value(), last_read);
    last_read++;

    if (last_read>=size)
        last_read = 0;

    voltage_sources.get(last_read)->update();   // pre-read the next one so it has a chance to settle?

    /*for (int i = 0 ; i < voltage_sources.size() ; i++) {
        voltage_sources.get(i)->update();
    }*/
}