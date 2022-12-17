#include <Arduino.h>

template<class DataType, int max_queue_size>
class Queue {
    public:

    DataType head_data;

    DataType queue[max_queue_size];
    int head = -1;
    int tail = -1;

    Queue() {};

    void push(DataType data) {
        tail = ++tail % max_queue_size;
        memcpy(&queue[tail], &data, sizeof(DataType));
        //Serial.printf("pushed to tail %i\n", tail);
    }
    DataType *pop() {
        if (this->isEmpty()) 
            return nullptr;
        head = ++head % max_queue_size;
        memcpy(&head_data, &queue[head], sizeof(DataType));
        //Serial.printf("popped from head %i: %02x,%02x\n", head, head_data.pp, head_data.cc);

        return &head_data;
    }

    bool isEmpty() {
        return head==tail;
    }

    bool paused = false;
    bool isPaused() {
        return this->paused || this->isEmpty();
    }
    void setPaused(bool state) {
        this->paused = state;
    }

};