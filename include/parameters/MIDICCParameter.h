#include "parameters/Parameter.h"

#include "midi_out_wrapper.h"

//template<class MIDIOutputWrapper, class DataType>
class MIDICCParameter : public Parameter<MIDIOutputWrapper,byte> {

    //void(MIDIOutputWrapper::*cc_setter)(byte cc_number, byte value, byte channel);
    byte cc_number = 0, channel = 0;

    public:
        MIDICCParameter(char* label, MIDIOutputWrapper *target, byte cc_number, byte channel)
            //, void(MIDIOutputWrapper::*sendControlChange)(byte,byte,byte)) 
            : Parameter(label, target, initial_value) {
                //this->cc_setter = sendControlChange;
                this->cc_number = cc_number;
                this->channel = channel;
            }

        virtual void actual_set_value(double value) override {
            if (this->debug) Serial.printf("MIDICCParameter#actual_set_value passed %f\n", value);
            value = (value + 1.0f) / 2.0;
            if (this->debug) Serial.printf("MIDICCParameter#actual_set_value re-normalised to %f\n", value);
            byte bvalue = value * 127.0;
            if (this->debug) Serial.printf("MIDICCParameter#actual_set_value(%i, %i, %i)\n", cc_number, bvalue, this->channel);
            if (this->target!=nullptr) {
                this->target->sendControlChange(this->cc_number, (byte)bvalue, this->channel);
            } else {
                if (this->debug) Serial.printf("WARNING: No target set in MIDICCParameter#actual_set_value in '%s'!\n", this->label);
            }
        }

};
