/*
  WiFi beacon driver
 */
#pragma once

#include "transmitter.h"

class WiFi_TX : public Transmitter {
public:
    bool init(void) override;
    bool transmit_nan(ODID_UAS_Data &UAS_data);
    bool transmit_beacon(ODID_UAS_Data &UAS_data);

private:
    bool initialised;
    char ssid[32];
    uint8_t WiFi_mac_addr[6];
    size_t ssid_length;
    uint8_t send_counter_nan;
    uint8_t send_counter_beacon;
    uint8_t dBm_to_tx_power(float dBm) const;
};
