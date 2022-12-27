#ifndef SAVEABLE_PARAMETERS__INCLUDED
#define SAVEABLE_PARAMETERS__INCLUDED

#include <Arduino.h>

class SaveableParameterBase {
    public:
    const char *label = nullptr;
    SaveableParameterBase(const char *label) {
        this->label = label;
    }
    virtual String get_line() { return String("; nop"); }
    virtual bool parse_key_value(String key, String value) {
        return false;
    }
};

template<class TargetClass, class DataType>
class SaveableParameter : public SaveableParameterBase {
    public:
        TargetClass *target;
        DataType *variable;
        void(TargetClass::*setter_func)(DataType);
        DataType(TargetClass::*getter_func)();

        SaveableParameter(
            const char *label, 
            TargetClass *target, 
            DataType *variable,
            void(TargetClass::*setter_func)(DataType),
            DataType(TargetClass::*getter_func)()
        ) : SaveableParameterBase(label) {
            this->target = target;
            this->variable = variable;
            this->setter_func = setter_func;
            this->getter_func = getter_func;
        }

        virtual String get_line() {
            return String(this->label) + String("=") + String((this->target->*getter_func)());
        }
        virtual bool parse_key_value(String key, String value) {
            if (key.equals(this->label)) {
                this->set((DataType)0,value);
                return true;
            }
            return false;
        }
        // todo: rewrite to use constexpr?
        virtual void set(signed int, String value) {
            (this->target->*setter_func)(value.toInt());
        }
        virtual void set(unsigned int, String value) {
            (this->target->*setter_func)(value.toInt());
        }
        virtual void set(signed long, String value) {
            (this->target->*setter_func)(value.toInt());
        }
        virtual void set(unsigned long, String value) {
            (this->target->*setter_func)(value.toInt());
        }
        virtual void set(bool, String value) {
            (this->target->*setter_func)(value.equals("true") || value.equals("enabled"));
        }
        virtual void set(float, String value) {
            (this->target->*setter_func)(value.toFloat());
        }
};

#endif