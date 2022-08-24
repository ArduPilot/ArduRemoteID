/*
  parent class for transmission methods
 */
#pragma once

#include <Arduino.h>
#include <opendroneid.h>

class Transmitter {
public:
    virtual bool init(void);

protected:
    void generate_random_mac(uint8_t mac[6]);
};
