#include "Config.h"

#if defined(ENABLE_MICROLIDIAN) && defined(ENABLE_XIAOSERIAL) && defined(ENABLE_USBSERIAL)

#include "behaviours/behaviour_xiaoserial.h"

DeviceBehaviour_XiaoSerial *behaviour_xiaoserial = new DeviceBehaviour_XiaoSerial();

#endif                              