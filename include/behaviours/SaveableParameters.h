#ifndef SAVEABLE_PARAMETERS__INCLUDED
#define SAVEABLE_PARAMETERS__INCLUDED

#include <Arduino.h>

class SaveableParameterBase {
    public:
    const char *label = nullptr;
    SaveableParameterBase(const char *label, bool *variable_recall_enabled = nullptr, bool *variable_save_enabled = nullptr) {
        this->label = label;
        this->variable_recall_enabled = variable_recall_enabled;
        this->variable_save_enabled = variable_save_enabled;
    }
    virtual String get_line() { return String("; nop"); }
    virtual bool parse_key_value(String key, String value) {
        return false;
    }

    bool *variable_recall_enabled = nullptr;
    bool *variable_save_enabled = nullptr;
    virtual bool is_recall_enabled() { 
        if (variable_recall_enabled==nullptr || (*variable_recall_enabled)) 
            return true; 
        return false;
    }
    virtual bool is_save_enabled() { 
        if (variable_save_enabled==nullptr || (*variable_save_enabled)) 
            return true; 
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

        bool(TargetClass::*is_recall_enabled_func)();
        bool(TargetClass::*is_save_enabled_func)();

        SaveableParameter(
            const char *label, 
            TargetClass *target, 
            DataType *variable,
            bool *variable_recall_enabled = nullptr,
            bool *variable_save_enabled = nullptr,
            void(TargetClass::*setter_func)(DataType) = nullptr,
            DataType(TargetClass::*getter_func)() = nullptr,
            bool(TargetClass::*is_recall_enabled_func)() = nullptr,
            bool(TargetClass::*is_save_enabled_func)() = nullptr
        ) : SaveableParameterBase(label, variable_recall_enabled, variable_save_enabled) {
            this->target = target;
            this->variable = variable;
            this->setter_func = setter_func;
            this->getter_func = getter_func;
            this->is_recall_enabled_func = is_recall_enabled_func;
            this->is_save_enabled_func = is_save_enabled_func;
        }

        virtual bool is_recall_enabled () override {
            if (this->is_save_enabled_func!=nullptr) 
                return (this->target->*is_save_enabled_func)();
            return SaveableParameterBase::is_recall_enabled();
        }

        virtual String get_line() {
            if (this->getter_func!=nullptr)
                return String(this->label) + String("=") + String((this->target->*getter_func)());
            else 
                return String(this->label) + String("=") + String(*this->variable);
        }
        virtual bool parse_key_value(String key, String value) {
            if (key.equals(this->label)) {
                this->set((DataType)0,value);
                return true;
            }
            return false;
        }
        // todo: rewrite to use constexpr?
        void setInt(String value) {
            if (setter_func!=nullptr)
                (this->target->*setter_func)(value.toInt());
            else if (variable!=nullptr)
                *this->variable = value.toInt();
        }
        void setBool(bool value) {
            if (setter_func!=nullptr)
                (this->target->*setter_func)(value);
            else if (variable!=nullptr)
                *this->variable = value;
        }
        void setFloat(float value) {
            if (setter_func!=nullptr)
                (this->target->*setter_func)(value);
            else if (variable!=nullptr)
                *this->variable = value;
        }

        virtual void set(signed int, String value) {
            setInt(value);
        }
        virtual void set(unsigned int, String value) {
            setInt(value);
        }
        virtual void set(signed long, String value) {
            setInt(value);
        }
        virtual void set(unsigned long, String value) {
            setInt(value);
        }
        virtual void set(bool, String value) {
            setBool((value.equals("true") || value.equals("enabled")));
        }
        virtual void set(float, String value) {
            this->setFloat(value.toFloat());
        }
};

/*
#include "parameters/Parameter.h"
class SaveableParameterWrapper : public SaveableParameterBase {
    DoubleParameter *target = nullptr;
    SaveableParameterWrapper(DoubleParameter *target) : SaveableParameterBase(target->label) {
        this->target = target;
    }
    virtual String get_line() {
        return String(this->target->label) + String("=") + String(this->target->getCurrentNormalValue());
    }
    virtual bool parse_key_value(String key, String value) {
        if (key.equals(this->target->label)) {
            this->target->updateValueFromNormal(value.toFloat());
            //this->target->upd
        }
    }
};
*/

#endif