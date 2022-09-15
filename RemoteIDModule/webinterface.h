/*
  web interface
 */
#pragma once

#include "options.h"
#include <Arduino.h>
#include "version.h"

typedef struct {
    String name;
    String value;
} json_table_t;

class WebInterface {
public:
    void init(void);
    void update(void);
private:
    bool initialised = false;

    // first 16 bytes for flashing, skip buffer in updater
    uint8_t lead_bytes[16];
    uint8_t lead_len;
};
