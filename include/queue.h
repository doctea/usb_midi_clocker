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

    bool paused = false;    // true if queue should be held

    Queue() {};

    // add an item to queue, with optional timeout and delay
    void push(DataType data, uint32_t timeout = 0, uint32_t delay = 0) {
        tail++;
        tail %= max_queue_size;
        memcpy(&queue[tail], &data, sizeof(DataType));

        this->delay_at[tail] = millis() + delay;    // record the time that we want to request this
        this->timeout[tail] = timeout;              // record the timeout to allow before pushing on
        //Serial.printf("pushed to tail %i\n", tail);
    }
    // dequeue first queued item, returning a pointer to the popped item or nullptr if queue is empty
    DataType *pop() {
        if (this->isEmpty()) // nothing to do 
            return nullptr;
        head++;
        head %= max_queue_size;
        memcpy(&head_data, &queue[head], sizeof(DataType));
        //Serial.printf("popped from head %i: %02x,%02x\n", head, head_data.pp, head_data.cc);
        this->actual_timeout_at = millis() + timeout[head]; // record time that item was popped, so we can timeout if still paused when that happens
        // todo: set the paused flag here based on whether a timeout was requested, rather than requiring the calling code to handle this?

        return &head_data;
    }

    bool isEmpty() {
        return head==tail;
    }

    // is ready to deliver a value (ie queue not empty and not waiting on a delay; also checks+clears timeout flag if necessary)
    bool isReady() {
        if (this->isEmpty())            
            // not ready if nothing queued!
            return false;
        if (delay_at[head]>millis())    
            // don't send message if we haven't reached its delay time yet
            return false;
        if (this->paused && timeout[head]!=0 && actual_timeout_at<=millis()) {  
            // break out of paused state if timeout has been reached
            Debug_printf("\tqueue timed out (%i vs %i)\n", actual_timeout_at, millis());
            this->paused = false;
        }
        return !paused;
        //return this->paused || this->isEmpty();
    }
    // set the pause override flag
    void setPaused(bool state) {
        this->paused = state;
    }

};