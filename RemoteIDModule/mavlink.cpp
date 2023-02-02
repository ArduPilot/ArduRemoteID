/*
  mavlink class for handling OpenDroneID messages
 */
#include <Arduino.h>
#include "mavlink.h"
#include "board_config.h"
#include "version.h"
#include "parameters.h"

#define SERIAL_BAUD 115200

static HardwareSerial *serial_ports[MAVLINK_COMM_NUM_BUFFERS];

#include <generated/mavlink_helpers.h>

mavlink_system_t mavlink_system = {0, MAV_COMP_ID_ODID_TXRX_1};

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
    mavlink_system.sysid = g.mavlink_sysid;
}

void MAVLinkSerial::update(void)
{
    const uint32_t now_ms = millis();

    if (mavlink_system.sysid != 0) {
        update_send();
    } else if (g.mavlink_sysid != 0) {
        mavlink_system.sysid = g.mavlink_sysid;
    } else if (now_ms - last_hb_warn_ms >= 2000) {
        last_hb_warn_ms = millis();
        serial.printf("Waiting for heartbeat\n");
    }
    update_receive();

    if (param_request_last_ms != 0 && now_ms - param_request_last_ms > 50) {
        param_request_last_ms = now_ms;
        float value;
        if (param_next->get_as_float(value)) {
            mavlink_msg_param_value_send(chan,
                                         param_next->name, value,
                                         MAV_PARAM_TYPE_REAL32,
                                         g.param_count_float(),
                                         g.param_index_float(param_next));
        }
        param_next++;
        if (param_next->ptype == Parameters::ParamType::NONE) {
            param_next = nullptr;
            param_request_last_ms = 0;
        }
    }
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
        if (g.options & OPTIONS_PRINT_RID_MAVLINK) {
            Serial.printf("MAVLink: got Location\n");
        }
        if (last_location_timestamp != location.timestamp) {
            //only update the timestamp if we receive information with a different timestamp
            last_location_ms = millis();
            last_location_timestamp = location.timestamp;
        }
        last_location_ms = now_ms;
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_BASIC_ID: {
        mavlink_open_drone_id_basic_id_t basic_id_tmp;
        if (g.options & OPTIONS_PRINT_RID_MAVLINK) {
            Serial.printf("MAVLink: got BasicID\n");
        }
        mavlink_msg_open_drone_id_basic_id_decode(&msg, &basic_id_tmp);
        if ((strlen((const char*) basic_id_tmp.uas_id) > 0) && (basic_id_tmp.id_type > 0) && (basic_id_tmp.id_type <= MAV_ODID_ID_TYPE_SPECIFIC_SESSION_ID)) {
            //only update if we receive valid data
            basic_id = basic_id_tmp;
            last_basic_id_ms = now_ms;
        }
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_AUTHENTICATION: {
        mavlink_msg_open_drone_id_authentication_decode(&msg, &authentication);
        if (g.options & OPTIONS_PRINT_RID_MAVLINK) {
            Serial.printf("MAVLink: got Auth\n");
        }
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_SELF_ID: {
        mavlink_msg_open_drone_id_self_id_decode(&msg, &self_id);
        if (g.options & OPTIONS_PRINT_RID_MAVLINK) {
            Serial.printf("MAVLink: got SelfID\n");
        }
        last_self_id_ms = now_ms;
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM: {
        mavlink_msg_open_drone_id_system_decode(&msg, &system);
        if (g.options & OPTIONS_PRINT_RID_MAVLINK) {
            Serial.printf("MAVLink: got System\n");
        }
        if ((last_system_timestamp != system.timestamp) || (system.timestamp == 0)) {
            //only update the timestamp if we receive information with a different timestamp
            last_system_ms = millis();
            last_system_timestamp = system.timestamp;
        }
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_SYSTEM_UPDATE: {
        if (g.options & OPTIONS_PRINT_RID_MAVLINK) {
            Serial.printf("MAVLink: got System update\n");
        }
        mavlink_open_drone_id_system_update_t pkt_system_update;
        mavlink_msg_open_drone_id_system_update_decode(&msg, &pkt_system_update);
        system.operator_latitude = pkt_system_update.operator_latitude;
        system.operator_longitude = pkt_system_update.operator_longitude;
        system.operator_altitude_geo = pkt_system_update.operator_altitude_geo;
        system.timestamp = pkt_system_update.timestamp;
        if (last_system_ms != 0) {
            // we can only mark system as updated if we have the other
            // information already
            if ((last_system_timestamp != system.timestamp) || (pkt_system_update.timestamp == 0)) {
                //only update the timestamp if we receive information with a different timestamp
                last_system_ms = millis();
                last_system_timestamp = pkt_system_update.timestamp;
            }
            last_system_ms = now_ms;
        }
        break;
    }
    case MAVLINK_MSG_ID_OPEN_DRONE_ID_OPERATOR_ID: {
        mavlink_msg_open_drone_id_operator_id_decode(&msg, &operator_id);
        if (g.options & OPTIONS_PRINT_RID_MAVLINK) {
            Serial.printf("MAVLink: got OperatorID\n");
        }
        last_operator_id_ms = now_ms;
        break;
    }
    case MAVLINK_MSG_ID_PARAM_REQUEST_LIST: {
        param_next = g.find_by_index_float(0);
        param_request_last_ms = millis();
        break;
    };
    case MAVLINK_MSG_ID_PARAM_REQUEST_READ: {
        mavlink_param_request_read_t pkt;
        mavlink_msg_param_request_read_decode(&msg, &pkt);
        const Parameters::Param *p;
        if (pkt.param_index < 0) {
            p = g.find(pkt.param_id);
        } else {
            p = g.find_by_index_float(pkt.param_index);
        }
        float value;
        if (!p || !p->get_as_float(value)) {
            return;
        }
        if (p != nullptr && (p->flags & PARAM_FLAG_HIDDEN)) {
            return;
        }
        mavlink_msg_param_value_send(chan,
                                     p->name, value,
                                     MAV_PARAM_TYPE_REAL32,
                                     g.param_count_float(),
                                     g.param_index_float(p));
        break;
    }
    case MAVLINK_MSG_ID_PARAM_SET: {
        mavlink_param_set_t pkt;
        mavlink_msg_param_set_decode(&msg, &pkt);
        auto *p = g.find(pkt.param_id);
        float value;
        if (pkt.param_type != MAV_PARAM_TYPE_REAL32 ||
            !p || !p->get_as_float(value)) {
            return;
        }
        p->get_as_float(value);
        if (g.lock_level > 0 &&
            (strcmp(p->name, "LOCK_LEVEL") != 0 ||
             uint8_t(pkt.param_value) <= uint8_t(value))) {
            // only param set allowed is to increase lock level
            mav_printf(MAV_SEVERITY_ERROR, "Parameters locked");
        } else {
            p->set_as_float(pkt.param_value);
        }
        p->get_as_float(value);
        mavlink_msg_param_value_send(chan,
                                     p->name, value,
                                     MAV_PARAM_TYPE_REAL32,
                                     g.param_count_float(),
                                     g.param_index_float(p));
        break;
    }
    case MAVLINK_MSG_ID_SECURE_COMMAND:
    case MAVLINK_MSG_ID_SECURE_COMMAND_REPLY: {
        mavlink_secure_command_t pkt;
        mavlink_msg_secure_command_decode(&msg, &pkt);
        handle_secure_command(pkt);
        break;
    }
    default:
        // we don't care about other packets
        break;
    }
}

void MAVLinkSerial::arm_status_send(void)
{
    const uint8_t status = parse_fail==nullptr?MAV_ODID_ARM_STATUS_GOOD_TO_ARM:MAV_ODID_ARM_STATUS_PRE_ARM_FAIL_GENERIC;
    const char *reason = parse_fail==nullptr?"":parse_fail;
    mavlink_msg_open_drone_id_arm_status_send(
        chan,
        status,
        reason);
}
