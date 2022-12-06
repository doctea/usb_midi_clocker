#ifndef FILE_MANAGER_INTERFACES__INCLUDED
#define FILE_MANAGER_INTERFACES__INCLUDED

#include <Arduino.h>
#include <LinkedList.h>

class IParseKeyValueReceiver {
    public:
        //FLASHMEM
        virtual bool load_parse_key_value(String key, String value) = 0;
};
class ISaveKeyValueSource {
    public:
        //FLASHMEM 
        virtual void add_save_lines(LinkedList<String> *lines) = 0;
};

#endif