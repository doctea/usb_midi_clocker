#pragma once

//#define ENABLE_DEBUG_SERIAL
#define SERIAL_FLUSH_REALLY

#include <Arduino.h>

#include "shared_debug.h"

void debug_free_ram();

void reset_teensy();

#include "menu_messages.h"

