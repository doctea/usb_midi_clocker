#include <Arduino.h>

template<class DataType, int max_queue_size>
class Queue {
    public:

    DataType head_data; // the current head data, to be kept in scope

    DataType queue[max_queue_size];     // queued
    uint32_t timeout[max_queue_size];   // timeout values per queued item
    uint32_t delay_at[max_queue_size];  // earliest time to process each queued item
    uint32_t actual_timeout_at = 0;     // earliest time to timeout from current item

    int head = -1;      // read head
    int tail = -1;      // write head

    Queue() {};

    void push(DataType data, uint32_t timeout = 0, uint32_t delay = 0) {
        tail = ++tail % max_queue_size;
        memcpy(&queue[tail], &data, sizeof(DataType));

        //this->timeout_at[tail] = millis() + delay + timeout;
        this->delay_at[tail] = millis() + delay;    // record the time that we want to request this
        this->timeout[tail] = timeout;              // record the timeout to allow before pushing on
        //Serial.printf("pushed to tail %i\n", tail);
    }
    DataType *pop() {
        if (this->isEmpty()) 
            return nullptr;
        head = ++head % max_queue_size;
        memcpy(&head_data, &queue[head], sizeof(DataType));
        //Serial.printf("popped from head %i: %02x,%02x\n", head, head_data.pp, head_data.cc);
        this->actual_timeout_at = millis() + timeout[head]; // record time that item was popped, so we can timeout if still paused when that happens

        return &head_data;
    }

    bool isEmpty() {
        return head==tail;
    }

    bool paused = false;
    bool isReady() {
        if (this->isEmpty())            
            // not ready if nothing queued!
            return false;
        if (delay_at[head]>millis())    
            // don't send message if we haven't reached its delay time yet
            return false;
        if (this->paused && timeout[head]!=0 && actual_timeout_at<=millis()) {  
            // break out of paused state if timeout has been reached
            Serial.printf("\tqueue timed out (%i vs %i)\n", actual_timeout_at, millis());
            this->paused = false;
        }
        return !paused;
        //return this->paused || this->isEmpty();
    }
    void setPaused(bool state) {
        this->paused = state;
    }

};