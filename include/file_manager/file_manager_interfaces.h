
#include <Arduino.h>
#include <LinkedList.h>

class IParseKeyValueReceiver {
    public:
        virtual bool load_parse_key_value(String key, String value) = 0;
};
class ISaveKeyValueSource {
    public:
        virtual void save_sequence_add_lines(LinkedList<String> *lines) = 0;
};