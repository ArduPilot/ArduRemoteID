/*
  mavlink class for handling OpenDroneID messages
 */
#pragma once
#include "transport.h"
#include "parameters.h"

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
    uint32_t param_request_last_ms;
    const Parameters::Param *param_next;

    void update_receive(void);
    void update_send(void);
    void process_packet(mavlink_status_t &status, mavlink_message_t &msg);
    void mav_printf(uint8_t severity, const char *fmt, ...);
    void handle_secure_command(const mavlink_secure_command_t &pkt);

    void arm_status_send(void);
};
