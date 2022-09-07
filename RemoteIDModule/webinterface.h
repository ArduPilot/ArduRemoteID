/*
  web interface
 */
#pragma once

#include "options.h"
#include <Arduino.h>
#include "version.h"

class WebInterface {
public:
    void init(void);
    void update(void);
private:
    bool initialised = false;
};
