/*
  BLE BT4/BT5 driver
 */
#pragma once

#include <Arduino.h>
#include <opendroneid.h>

class BLE_TX {
public:
    bool init(void);
    bool transmit_longrange(ODID_UAS_Data &UAS_data);
    bool transmit_legacy_name(ODID_UAS_Data &UAS_data);
    bool transmit_legacy(ODID_UAS_Data &UAS_data);

private:
    uint8_t msg_counters[ODID_MSG_COUNTER_AMOUNT];
    uint8_t legacy_payload[36];
    uint8_t longrange_payload[250];
    bool started;
};
