/*
  BLE BT4/BT5 driver
 */
#pragma once

#include <Arduino.h>
#include <opendroneid.h>

class BLE_TX {
public:
    bool init(void);
    bool transmit(ODID_UAS_Data &UAS_data);

private:
    int msg_counter;
    uint8_t payload[250];
    uint8_t payload2[255];
    uint8_t legacy_payload[36];
    bool started;
};
