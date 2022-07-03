#ifndef BEHAVIOUR_MANAGER__INCLUDED
#define BEHAVIOUR_MANAGER__INCLUDED

#include "behaviour_base.h"

#include <LinkedList.h>

class DeviceBehaviourManager {
    public:
        static DeviceBehaviourManager* getInstance();

        LinkedList<DeviceBehaviourBase *> behaviours = LinkedList<DeviceBehaviourBase *>();

        void registerDevice(DeviceBehaviourBase *device) {
            this->behaviours.add(device);
        }

    private:
        static DeviceBehaviourManager* inst_;
        DeviceBehaviourManager() {}
        DeviceBehaviourManager(const DeviceBehaviourManager&);
        DeviceBehaviourManager& operator=(const DeviceBehaviourManager&);
};

extern DeviceBehaviourManager *behaviour_manager;

void setup_behaviour_manager();

#endif