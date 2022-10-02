/*
  BLE BT4/BT5 driver
 */
#pragma once

#include "transmitter.h"

class BLE_TX : public Transmitter {
public:
    bool init(void) override;
    bool transmit_longrange(ODID_UAS_Data &UAS_data);
    bool transmit_legacy(ODID_UAS_Data &UAS_data);

private:
    bool initialised;
    uint8_t msg_counters[ODID_MSG_COUNTER_AMOUNT];
    uint8_t legacy_payload[36];
    uint8_t longrange_payload[250];
    bool started;

    uint8_t dBm_to_tx_power(float dBm) const;
};
