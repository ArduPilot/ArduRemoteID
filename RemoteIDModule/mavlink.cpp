/*
  mavlink class for handling OpenDroneID messages
 */
#include <Arduino.h>
#include "mavlink.h"
#include "board_config.h"
#include "version.h"

#define SERIAL_BAUD 115200

static HardwareSerial *serial_ports[MAVLINK_COMM_NUM_BUFFERS];

#include <generated/mavlink_helpers.h>

mavlink_system_t mavlink_system = {0,MAV_COMP_ID_ODID_TXRX_1};

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
    // print banner at startup
    serial.printf("ArduRemoteID version %u.%u %08x\n",
                  FW_VERSION_MAJOR, FW_VERSION_MINOR, GIT_VERSION);
}

void MAVLinkSerial::update(void)
{
    if (mavlink_system.sysid != 0) {
        update_send();
    } else if (millis() - last_hb_warn_ms >= 2000) {
        last_hb_warn_ms = millis();
        serial.printf("Waiting for heartbeat\n");
    }
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
            MAV_AUTOPILOT_INVALID,
            0,
            0,
            0);

        // send arming status
        arm_status_send();
    }
}

void MAVLinkSerial::update_receive(void)
{
    // receive new packets
    mavlink_message_t msg;
    mavlink_status_t status;
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
    const uint32_t now_ms = millis();
    switch (msg.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: {
        mavlink_heartbeat_t hb;
        if (mavlink_system.sysid == 0) {
            mavlink_msg_heartbeat_decode(&msg, &hb);
            if (msg.sysid > 0 && hb.type != MAV_TYPE_GCS) {
                mavlink_system.sysid = msg.sysid;
            }
        }
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_LOCATION: {
        mavlink_msg_open_drone_id_location_decode(&msg, &location);
        last_location_ms = now_ms;
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_BASIC_ID: {
        mavlink_msg_open_drone_id_basic_id_decode(&msg, &basic_id);
        last_basic_id_ms = now_ms;
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_AUTHENTICATION: {
        mavlink_msg_open_drone_id_authentication_decode(&msg, &authentication);
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_SELF_ID: {
        mavlink_msg_open_drone_id_self_id_decode(&msg, &self_id);
        last_self_id_ms = now_ms;
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM: {
        mavlink_msg_open_drone_id_system_decode(&msg, &system);
        last_system_ms = now_ms;
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM_UPDATE: {
        mavlink_open_drone_id_system_update_t pkt_system_update;
        mavlink_msg_open_drone_id_system_update_decode(&msg, &pkt_system_update);
        system.operator_latitude = pkt_system_update.operator_latitude;
        system.operator_longitude = pkt_system_update.operator_longitude;
        system.operator_altitude_geo = pkt_system_update.operator_altitude_geo;
        system.timestamp = pkt_system_update.timestamp;
        if (last_system_ms != 0) {
            // we can only mark system as updated if we have the other
            // information already
            last_system_ms = now_ms;
        }
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_OPERATOR_ID: {
        mavlink_msg_open_drone_id_operator_id_decode(&msg, &operator_id);
        last_operator_id_ms = now_ms;
        break;
    }
    default:
        // we don't care about other packets
        break;
    }
}

void MAVLinkSerial::arm_status_send(void)
{
    const uint8_t status = parse_fail==nullptr?MAV_ODID_GOOD_TO_ARM:MAV_ODID_PRE_ARM_FAIL_GENERIC;
    const char *reason = parse_fail==nullptr?"":parse_fail;
    mavlink_msg_open_drone_id_arm_status_send(
        chan,
        status,
        reason);
}
