/*
  mavlink class for handling OpenDroneID messages
 */
#pragma once
#include "transport.h"

/*
  abstraction for MAVLink on a serial port
 */
class MAVLinkSerial : public Transport {
public:
    using Transport::Transport;
    MAVLinkSerial(HardwareSerial &serial, mavlink_channel_t chan);
    void init(void) override;
    void update(void) override;

private:
    HardwareSerial &serial;
    mavlink_channel_t chan;
    uint32_t last_hb_ms;
    uint32_t last_hb_warn_ms;

    void update_receive(void);
    void update_send(void);
    void process_packet(mavlink_status_t &status, mavlink_message_t &msg);
    void mav_printf(uint8_t severity, const char *fmt, ...);

    void arm_status_send(void);
};
