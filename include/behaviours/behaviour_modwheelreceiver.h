#include "behaviours/behaviour_base.h"

#include "parameters/MIDICCParameter.h"

// add a mappable proxy parameter for modwheel output
class ModwheelReceiver : public virtual DeviceBehaviourUltimateBase {
    public:
        MIDICCProxyParameter *modwheel_proxy = nullptr;

        virtual LinkedList<DoubleParameter*> *initialise_parameters() override {
           this->modwheel_proxy = new MIDICCProxyParameter(
                (char*)"Modwheel",      
                this,   
                (byte)midi::ModulationWheel,
                (byte)midi_matrix_manager->getDefaultChannelForTargetId(this->target_id)
           );
           this->parameters->add(this->modwheel_proxy);
           return this->parameters;
        }

        virtual bool process(byte cc_number, byte value, byte channel = 0) {
            if (this->modwheel_proxy!=nullptr && this->modwheel_proxy->responds_to(cc_number, channel)) {
                this->modwheel_proxy->updateValueFromData(value);
                return true;
            }
            return false;
        } 

        /* // to use, put
           //     ModwheelReceiver::initialise_parameters();
           //   in behaviour's initialise_parameters()
           // and override sendControlChange() like this:
           // see behaviour_neutron for example
        virtual void sendControlChange(byte cc_number, byte value, byte channel = 0) {
            Serial.printf("behaviour_neutron sendControlChange(cc=%i, value=%i, channel=%i)\n", cc_number, value, channel);
            // if we receive a value from another device, then update the proxy parameter, which will handle the actual sending
            //if (cc_number==this->modwheel_proxy->cc_number)
            if (!ModwheelReceiver::process(cc_number, value, channel))
                DeviceBehaviourUltimateBase::sendControlChange(cc_number, value, channel);
        }*/
};
