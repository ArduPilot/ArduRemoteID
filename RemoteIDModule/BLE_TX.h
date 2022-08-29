/*
  BLE BT4/BT5 driver
 */
#pragma once

#include "transmitter.h"

enum BLE_power_level { BLE_PWR_P9_DBM, BLE_PWR_P12_DBM, BLE_PWR_P15_DBM, BLE_PWR_P18_DBM };

class BLE_TX : public Transmitter {
public:
    bool init(void) override;
    bool set_tx_power_level();
    bool transmit_longrange(ODID_UAS_Data &UAS_data);
    bool transmit_legacy_name(ODID_UAS_Data &UAS_data);
    bool transmit_legacy(ODID_UAS_Data &UAS_data);

private:
    uint8_t msg_counters[ODID_MSG_COUNTER_AMOUNT];
    uint8_t legacy_payload[36];
    uint8_t longrange_payload[250];
    bool started;
};
