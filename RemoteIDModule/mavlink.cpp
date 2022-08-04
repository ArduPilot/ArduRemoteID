/*
  mavlink class for handling OpenDroneID messages
 */
#include <Arduino.h>
#include "mavlink.h"

#define SERIAL_BAUD 115200

static HardwareSerial *serial_ports[MAVLINK_COMM_NUM_BUFFERS];

#include <generated/mavlink_helpers.h>

mavlink_system_t mavlink_system = {2,1};

#define dev_printf(fmt, args ...)  do {Serial.printf(fmt, ## args); } while(0)

/*
  send a buffer out a MAVLink channel
 */
void comm_send_buffer(mavlink_channel_t chan, const uint8_t *buf, uint8_t len)
{
    if (chan >= MAVLINK_COMM_NUM_BUFFERS || serial_ports[uint8_t(chan)] == nullptr) {
        return;
    }
    auto &serial = *serial_ports[uint8_t(chan)];
    serial.write(buf, len);
}

/*
  abstraction for MAVLink on a serial port
 */
MAVLinkSerial::MAVLinkSerial(HardwareSerial &_serial, mavlink_channel_t _chan) :
    serial(_serial),
    chan(_chan)
{
    serial_ports[uint8_t(_chan - MAVLINK_COMM_0)] = &serial;
}

void MAVLinkSerial::init(void)
{
}

void MAVLinkSerial::update(void)
{
    update_send();
    update_receive();
}

void MAVLinkSerial::update_send(void)
{
    uint32_t now_ms = millis();
    if (now_ms - last_hb_ms >= 1000) {
        last_hb_ms = now_ms;
        mavlink_msg_heartbeat_send(
            chan,
            MAV_TYPE_ODID,
            MAV_AUTOPILOT_ARDUPILOTMEGA,
            0,
            0,
            0);
    }
}

void MAVLinkSerial::update_receive(void)
{
    // receive new packets
    mavlink_message_t msg;
    mavlink_status_t status;
    uint32_t now_ms = millis();

    status.packet_rx_drop_count = 0;

    const uint16_t nbytes = serial.available();
    for (uint16_t i=0; i<nbytes; i++) {
        const uint8_t c = (uint8_t)serial.read();
        // Try to get a new message
        if (mavlink_parse_char(chan, c, &msg, &status)) {
            process_packet(status, msg);
        }
    }
}

/*
  printf via MAVLink STATUSTEXT for debugging
 */
void MAVLinkSerial::mav_printf(uint8_t severity, const char *fmt, ...)
{
    va_list arg_list;
    char text[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN+1] {};
    va_start(arg_list, fmt);
    vsnprintf(text, sizeof(text), fmt, arg_list);
    va_end(arg_list);
    mavlink_msg_statustext_send(chan,
                                severity,
                                text,
                                0, 0);
}

void MAVLinkSerial::process_packet(mavlink_status_t &status, mavlink_message_t &msg)
{
    switch (msg.msgid) {
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_LOCATION: {
        dev_printf("Got OPEN_DRONE_ID_LOCATION\n");
        mavlink_msg_open_drone_id_location_decode(&msg, &location);
        packets_received_mask |= PKT_LOCATION;
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_BASIC_ID: {
        dev_printf("Got OPEN_DRONE_ID_BASIC_ID\n");
        mavlink_msg_open_drone_id_basic_id_decode(&msg, &basic_id);
        packets_received_mask |= PKT_BASIC_ID;
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_AUTHENTICATION: {
        dev_printf("Got OPEN_DRONE_ID_AUTHENTICATION\n");
        mavlink_msg_open_drone_id_authentication_decode(&msg, &authentication);
        packets_received_mask |= PKT_AUTHENTICATION;
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_SELF_ID: {
        dev_printf("Got OPEN_DRONE_ID_SELF_ID\n");
        mavlink_msg_open_drone_id_self_id_decode(&msg, &self_id);
        packets_received_mask |= PKT_SELF_ID;
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM: {
        dev_printf("Got OPEN_DRONE_ID_SYSTEM\n");
        mavlink_msg_open_drone_id_system_decode(&msg, &system);
        packets_received_mask |= PKT_SYSTEM;
        last_system_msg_ms = millis();
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_OPERATOR_ID: {
        dev_printf("Got OPEN_DRONE_ID_OPERATOR_ID\n");
        mavlink_msg_open_drone_id_operator_id_decode(&msg, &operator_id);
        packets_received_mask |= PKT_OPERATOR_ID;
        break;
    }
    default:
        // we don't care about other packets
        break;
    }
}

/*
  return true when we have the base set of packets
 */
bool MAVLinkSerial::initialised(void)
{
    const uint32_t required = PKT_LOCATION | PKT_BASIC_ID | PKT_SELF_ID | PKT_SYSTEM | PKT_OPERATOR_ID;
    return (packets_received_mask & required) == required;
}
