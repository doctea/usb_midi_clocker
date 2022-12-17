#include <Arduino.h>

template<class DataType, int max_queue_size>
class Queue {
    public:

    DataType head_data;

    DataType queue[max_queue_size];
    uint32_t timeout_at[max_queue_size];
    uint32_t delay_at[max_queue_size];
    int head = -1;
    int tail = -1;

    Queue() {};

    void push(DataType data, uint32_t timeout = 0, uint32_t delay = 0) {
        tail = ++tail % max_queue_size;
        memcpy(&queue[tail], &data, sizeof(DataType));

        // todo: rewrite this timeout/delay thing so that it runs from the time that item becomes head (timeout) and the time that it was added (delay)
        this->timeout_at[tail] = millis() + delay + timeout;
        this->delay_at[tail] = millis() + delay;
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
    bool isReady() {
        if (this->isEmpty())
            return false;
        if (delay_at[head]>millis())
            return false;
        if (this->paused && timeout_at[head]!=-1 && timeout_at[head]<=millis()) {
            Serial.printf("\tqueue timed out (%i vs %i)\n", timeout_at[head], millis());
            this->paused = false;
        }
        return !paused;
        //return this->paused || this->isEmpty();
    }
    void setPaused(bool state) {
        this->paused = state;
    }

};