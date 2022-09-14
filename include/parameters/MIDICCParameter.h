#ifndef MIDICCPARAMETER__INCLUDED
#define MIDICCPARAMETER__INCLUDED

#include "parameters/Parameter.h"

#include "midi_out_wrapper.h"

//template<class MIDIOutputWrapper, class DataType>
//template<class OutputClass>
class MIDICCParameter : public Parameter<DeviceBehaviourUltimateBase,byte> {

    //void(MIDIOutputWrapper::*cc_setter)(byte cc_number, byte value, byte channel);
    byte cc_number = 0, channel = 0;

    public:
        MIDICCParameter(char* label, DeviceBehaviourUltimateBase *target, byte cc_number, byte channel)
            : Parameter(label, target) { //}, initial_value) {
                //this->cc_setter = sendControlChange;
                this->cc_number = cc_number;
                this->channel = channel;
        }

        MIDICCParameter(char* label, DeviceBehaviourUltimateBase *target, byte cc_number, byte channel, byte maximum_value) 
            : MIDICCParameter(label, target, cc_number, channel) {
                this->maximum_value = maximum_value;
        }


        /*virtual const char* parseFormattedDataType(byte value) {
            static char fmt[20] = "              ";
            //sprintf(fmt, "%5i (signed)",      (int)(this->maximum_value*this->getCurrentValue())); //getCurrentValue());
            sprintf(fmt, "%i", this->get_midi_value_for_double(this->getCurrentValue()));
            return fmt;
        }*/

        virtual const char* getFormattedValue() override {
            //return this->parseFormattedDataType(this->get_midi_value_for_double(this->getCurrentValue()));
            static char fmt[20] = "              ";
            sprintf(fmt, "%i    ", this->get_midi_value_for_double(this->getCurrentValue()));
            //Serial.printf("getFormattedValue: '%s'\n", fmt);
            return fmt;
        };

        virtual byte get_midi_value_for_double(double value) {
            if (this->debug) Serial.printf("MIDICCParameter#get_midi_value_for_double passed %f\n", value);
            //value = (value + 1.0f) / 2.0;
            value = value + 0.5f;
            value = constrain(value, 0.0, 1.0);
            if (this->debug) Serial.printf("MIDICCParameter#get_midi_value_for_double re-normalised to %f\n", value);
            byte bvalue = value * this->maximum_value;  //127
            return bvalue;
        }

        virtual void actual_set_value(double value) override {
            /*if (this->debug) Serial.printf("MIDICCParameter#actual_set_value passed %f\n", value);
            value = (value + 1.0f) / 2.0;
            if (this->debug) Serial.printf("MIDICCParameter#actual_set_value re-normalised to %f\n", value);
            byte bvalue = value * 127.0;*/
            //this->debug = true;
            byte bvalue = this->get_midi_value_for_double(value);
            
            if (this->target!=nullptr) {
                if (this->debug) Serial.printf("MIDICCParameter#actual_set_value(%i, %i, %i)\n", cc_number, bvalue, this->channel);
                this->target->sendControlChange(this->cc_number, (byte)bvalue, this->channel);
            } else {
                if (this->debug) Serial.printf("WARNING: No target set in MIDICCParameter#actual_set_value in '%s'!\n", this->label);
            }
        }

};

#endif