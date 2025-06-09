#pragma once 

#include <Arduino.h>
#include <parameters.h>

#define serial_log(format, args...) \
                if (g.options & OPTIONS_PRINT_RID_DEBUG) { \
                    Serial.printf(format, ## args); \
                }
