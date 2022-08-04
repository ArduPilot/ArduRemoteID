/*
  mavlink class for handling OpenDroneID messages
 */
#pragma once

// we have separate helpers disabled to make it possible
// to select MAVLink 1.0 in the arduino GUI build
#define MAVLINK_SEPARATE_HELPERS
#define MAVLINK_NO_CONVERSION_HELPERS

#define MAVLINK_SEND_UART_BYTES(chan, buf, len) comm_send_buffer(chan, buf, len)

// two buffers, one for USB, one for UART. This makes for easier testing with SITL
#define MAVLINK_COMM_NUM_BUFFERS 2

#define MAVLINK_MAX_PAYLOAD_LEN 255

#include <mavlink2.h>

/// MAVLink system definition
extern mavlink_system_t mavlink_system;

void comm_send_buffer(mavlink_channel_t chan, const uint8_t *buf, uint8_t len);

#define MAVLINK_USE_CONVENIENCE_FUNCTIONS
#include <generated/all/mavlink.h>

/*
  abstraction for MAVLink on a serial port
 */
class MAVLinkSerial {
public:
    MAVLinkSerial(HardwareSerial &serial, mavlink_channel_t chan);
    void init(void);
    void update(void);

    const mavlink_open_drone_id_location_t &get_location(void) const {
        return location;
    }

    const mavlink_open_drone_id_basic_id_t &get_basic_id(void) const {
        return basic_id;
    }

    const mavlink_open_drone_id_authentication_t &get_authentication(void) const {
        return authentication;
    }

    const mavlink_open_drone_id_self_id_t &get_self_id(void) const {
        return self_id;
    }

    const mavlink_open_drone_id_system_t &get_system(void) const {
        return system;
    }

    const mavlink_open_drone_id_operator_id_t &get_operator_id(void) const {
        return operator_id;
    }

    // return true when we have the key messages available
    bool initialised(void);
    
private:
    HardwareSerial &serial;
    mavlink_channel_t chan;
    uint32_t last_hb_ms;

    enum {
        PKT_LOCATION       = (1U<<0),
        PKT_BASIC_ID       = (1U<<1),
        PKT_AUTHENTICATION = (1U<<2),
        PKT_SELF_ID        = (1U<<3),
        PKT_SYSTEM         = (1U<<4),
        PKT_OPERATOR_ID    = (1U<<5),
    };

    void update_receive(void);
    void update_send(void);
    void process_packet(mavlink_status_t &status, mavlink_message_t &msg);
    void mav_printf(uint8_t severity, const char *fmt, ...);

    mavlink_open_drone_id_location_t location;
    mavlink_open_drone_id_basic_id_t basic_id;
    mavlink_open_drone_id_authentication_t authentication;
    mavlink_open_drone_id_self_id_t self_id;
    mavlink_open_drone_id_system_t system;
    mavlink_open_drone_id_operator_id_t operator_id;

    uint32_t last_system_msg_ms;

    uint32_t packets_received_mask;
};
