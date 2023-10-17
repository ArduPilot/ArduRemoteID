/*
  DroneCAN class for handling OpenDroneID messages
  with thanks to David Buzz for ArduPilot ESP32 HAL
 */
#include <Arduino.h>
#include "board_config.h"
#include "version.h"
#include <time.h>
#include "DroneCAN.h"
#include "parameters.h"
#include <stdarg.h>
#include "util.h"
#include "monocypher.h"

#include <canard.h>
#include <uavcan.protocol.NodeStatus.h>
#include <uavcan.protocol.GetNodeInfo.h>
#include <uavcan.protocol.RestartNode.h>
#include <uavcan.protocol.dynamic_node_id.Allocation.h>
#include <uavcan.protocol.param.GetSet.h>
#include <uavcan.protocol.debug.LogMessage.h>
#include <dronecan.remoteid.BasicID.h>
#include <dronecan.remoteid.Location.h>
#include <dronecan.remoteid.SelfID.h>
#include <dronecan.remoteid.System.h>
#include <dronecan.remoteid.OperatorID.h>
#include <dronecan.remoteid.ArmStatus.h>

#ifndef CAN_BOARD_ID
#define CAN_BOARD_ID 10001
#endif

#ifndef CAN_APP_NODE_NAME
#define CAN_APP_NODE_NAME "ArduPilot RemoteIDModule"
#endif

#define UNUSED(x) (void)(x)


static void onTransferReceived_trampoline(CanardInstance* ins, CanardRxTransfer* transfer);
static bool shouldAcceptTransfer_trampoline(const CanardInstance* ins, uint64_t* out_data_type_signature, uint16_t data_type_id,
        CanardTransferType transfer_type,
        uint8_t source_node_id);

// decoded messages

void DroneCAN::init(void)
{

#if defined(BOARD_BLUEMARK_DB210)
    gpio_reset_pin(GPIO_NUM_19);
    gpio_reset_pin(GPIO_NUM_20);
#endif

    /*
      the ESP32 has a very inefficient CAN (TWAI) stack. If we let it
      receive all message types then when the bus is busy it will
      spend all its time in processRx(). To cope we need to use
      acceptance filters so we don't see most traffic. Unfortunately
      the ESP32 has a very rudimentary acceptance filter system which
      cannot be setup to block the high rate messages (such as ESC
      commands) while still allowing all of the RemoteID messages we
      need. The best we can do is use the message priority. The high
      rate messages all have a low valued priority number (which means
      a high priority). So by setting a single bit acceptance code in
      the top bit of the 3 bit priority field we only see messages
      that have a priority number of 16 or higher (which means
      CANARD_TRANSFER_PRIORITY_MEDIUM or lower priority)
    */
    const uint32_t acceptance_code = 0x10000000U<<3;
    const uint32_t acceptance_mask = 0x0FFFFFFFU<<3;

    can_driver.init(1000000, acceptance_code, acceptance_mask);

    canardInit(&canard, (uint8_t *)canard_memory_pool, sizeof(canard_memory_pool),
               onTransferReceived_trampoline, shouldAcceptTransfer_trampoline, NULL);
    if (g.can_node > 0 && g.can_node < 128) {
        canardSetLocalNodeID(&canard, g.can_node);
    }
    canard.user_reference = (void*)this;
}

void DroneCAN::update(void)
{
    if (do_DNA()) {
        const uint32_t now_ms = millis();
        if (now_ms - last_node_status_ms >= 1000) {
            last_node_status_ms = now_ms;
            node_status_send();
            arm_status_send();
        }
    }
    processTx();
    processRx();
}

void DroneCAN::node_status_send(void)
{
    uint8_t buffer[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE];
    node_status.uptime_sec = millis() / 1000U;
    node_status.vendor_specific_status_code = 0;
    const uint16_t len = uavcan_protocol_NodeStatus_encode(&node_status, buffer);
    static uint8_t tx_id;

    canardBroadcast(&canard,
                    UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE,
                    UAVCAN_PROTOCOL_NODESTATUS_ID,
                    &tx_id,
                    CANARD_TRANSFER_PRIORITY_LOW,
                    (void*)buffer,
                    len);
}

void DroneCAN::arm_status_send(void)
{
    uint8_t buffer[DRONECAN_REMOTEID_ARMSTATUS_MAX_SIZE];
    dronecan_remoteid_ArmStatus arm_status {};

    const uint8_t status = parse_fail==nullptr? MAV_ODID_ARM_STATUS_GOOD_TO_ARM:MAV_ODID_ARM_STATUS_PRE_ARM_FAIL_GENERIC;
    const char *reason = parse_fail==nullptr?"":parse_fail;

    arm_status.status = status;
    arm_status.error.len = strlen(reason);
    strncpy((char*)arm_status.error.data, reason, sizeof(arm_status.error.data));

    const uint16_t len = dronecan_remoteid_ArmStatus_encode(&arm_status, buffer);

    static uint8_t tx_id;
    canardBroadcast(&canard,
                    DRONECAN_REMOTEID_ARMSTATUS_SIGNATURE,
                    DRONECAN_REMOTEID_ARMSTATUS_ID,
                    &tx_id,
                    CANARD_TRANSFER_PRIORITY_LOW,
                    (void*)buffer,
                    len);
}

void DroneCAN::onTransferReceived(CanardInstance* ins,
                                  CanardRxTransfer* transfer)
{
    if (canardGetLocalNodeID(ins) == CANARD_BROADCAST_NODE_ID) {
        if (transfer->transfer_type == CanardTransferTypeBroadcast &&
                transfer->data_type_id == UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID) {
            handle_allocation_response(ins, transfer);
        }
        return;
    }

    const uint32_t now_ms = millis();

    switch (transfer->data_type_id) {
    case UAVCAN_PROTOCOL_GETNODEINFO_ID:
        handle_get_node_info(ins, transfer);
        break;
    case UAVCAN_PROTOCOL_RESTARTNODE_ID:
        Serial.printf("DroneCAN: restartNode\n");
        delay(20);
        esp_restart();
        break;
    case DRONECAN_REMOTEID_BASICID_ID:
        Serial.printf("DroneCAN: got BasicID\n");
        handle_BasicID(transfer);
        break;
    case DRONECAN_REMOTEID_LOCATION_ID:
        Serial.printf("DroneCAN: got Location\n");
        handle_Location(transfer);
        break;
    case DRONECAN_REMOTEID_SELFID_ID:
        Serial.printf("DroneCAN: got SelfID\n");
        handle_SelfID(transfer);
        break;
    case DRONECAN_REMOTEID_SYSTEM_ID:
        Serial.printf("DroneCAN: got System\n");
        handle_System(transfer);
        break;
    case DRONECAN_REMOTEID_OPERATORID_ID:
        Serial.printf("DroneCAN: got OperatorID\n");
        handle_OperatorID(transfer);
        break;
    case UAVCAN_PROTOCOL_PARAM_GETSET_ID:
        handle_param_getset(ins, transfer);
        break;
    case DRONECAN_REMOTEID_SECURECOMMAND_ID:
        handle_SecureCommand(ins, transfer);
        break;
    default:
        //Serial.printf("reject %u\n", transfer->data_type_id);
        break;
    }
}

bool DroneCAN::shouldAcceptTransfer(const CanardInstance* ins,
                                    uint64_t* out_data_type_signature,
                                    uint16_t data_type_id,
                                    CanardTransferType transfer_type,
                                    uint8_t source_node_id)
{
    if (canardGetLocalNodeID(ins) == CANARD_BROADCAST_NODE_ID &&
            data_type_id == UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID) {
        *out_data_type_signature = UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE;
        return true;
    }

#define ACCEPT_ID(name) case name ## _ID: *out_data_type_signature = name ## _SIGNATURE; return true
    switch (data_type_id) {
        ACCEPT_ID(UAVCAN_PROTOCOL_GETNODEINFO);
        ACCEPT_ID(UAVCAN_PROTOCOL_RESTARTNODE);
        ACCEPT_ID(DRONECAN_REMOTEID_BASICID);
        ACCEPT_ID(DRONECAN_REMOTEID_LOCATION);
        ACCEPT_ID(DRONECAN_REMOTEID_SELFID);
        ACCEPT_ID(DRONECAN_REMOTEID_OPERATORID);
        ACCEPT_ID(DRONECAN_REMOTEID_SYSTEM);
        ACCEPT_ID(DRONECAN_REMOTEID_SECURECOMMAND);
        ACCEPT_ID(UAVCAN_PROTOCOL_PARAM_GETSET);
        return true;
    }
    //Serial.printf("%u: reject ID 0x%x\n", millis(), data_type_id);
    return false;
}

static void onTransferReceived_trampoline(CanardInstance* ins,
        CanardRxTransfer* transfer)
{
    DroneCAN *dc = (DroneCAN *)ins->user_reference;
    dc->onTransferReceived(ins, transfer);
}

/*
  see if we want to process this packet
 */
static bool shouldAcceptTransfer_trampoline(const CanardInstance* ins,
        uint64_t* out_data_type_signature,
        uint16_t data_type_id,
        CanardTransferType transfer_type,
        uint8_t source_node_id)
{
    DroneCAN *dc = (DroneCAN *)ins->user_reference;
    return dc->shouldAcceptTransfer(ins, out_data_type_signature,
                                    data_type_id,
                                    transfer_type,
                                    source_node_id);
}

void DroneCAN::processTx(void)
{
    for (const CanardCANFrame* txf = NULL; (txf = canardPeekTxQueue(&canard)) != NULL;) {
        CANFrame txmsg {};
        txmsg.dlc = CANFrame::dataLengthToDlc(txf->data_len);
        memcpy(txmsg.data, txf->data, txf->data_len);
        txmsg.id = (txf->id | CANFrame::FlagEFF);

        // push message with 1s timeout
        if (can_driver.send(txmsg)) {
            canardPopTxQueue(&canard);
            tx_fail_count = 0;
        } else {
            if (tx_fail_count < 8) {
                tx_fail_count++;
            } else {
                canardPopTxQueue(&canard);
            }
            break;
        }
    }
}

void DroneCAN::processRx(void)
{
    CANFrame rxmsg;
    uint8_t count = 60;
    while (count-- && can_driver.receive(rxmsg)) {
        CanardCANFrame rx_frame {};
        uint64_t timestamp = micros64();
        rx_frame.data_len = CANFrame::dlcToDataLength(rxmsg.dlc);
        memcpy(rx_frame.data, rxmsg.data, rx_frame.data_len);
        rx_frame.id = rxmsg.id;
        int err = canardHandleRxFrame(&canard, &rx_frame, timestamp);
#if 0
        Serial.printf("%u: FX %08x %02x %02x %02x %02x %02x %02x %02x %02x (%u) -> %d\n",
                      millis(),
                      rx_frame.id,
                      rxmsg.data[0], rxmsg.data[1], rxmsg.data[2], rxmsg.data[3],
                      rxmsg.data[4], rxmsg.data[5], rxmsg.data[6], rxmsg.data[7],
                      rx_frame.data_len,
                      err);
#else
        UNUSED(err);
#endif
    }
}

CANFrame::CANFrame(uint32_t can_id, const uint8_t* can_data, uint8_t data_len, bool canfd_frame) :
    id(can_id)
{
    if ((can_data == nullptr) || (data_len == 0) || (data_len > MaxDataLen)) {
        return;
    }
    memcpy(this->data, can_data, data_len);
    if (data_len <= 8) {
        dlc = data_len;
    } else {
        dlc = 8;
    }
}

uint8_t CANFrame::dataLengthToDlc(uint8_t data_length)
{
    if (data_length <= 8) {
        return data_length;
    } else if (data_length <= 12) {
        return 9;
    } else if (data_length <= 16) {
        return 10;
    } else if (data_length <= 20) {
        return 11;
    } else if (data_length <= 24) {
        return 12;
    } else if (data_length <= 32) {
        return 13;
    } else if (data_length <= 48) {
        return 14;
    }
    return 15;
}


uint8_t CANFrame::dlcToDataLength(uint8_t dlc)
{
    /*
    Data Length Code      9  10  11  12  13  14  15
    Number of data bytes 12  16  20  24  32  48  64
    */
    if (dlc <= 8) {
        return dlc;
    } else if (dlc == 9) {
        return 12;
    } else if (dlc == 10) {
        return 16;
    } else if (dlc == 11) {
        return 20;
    } else if (dlc == 12) {
        return 24;
    } else if (dlc == 13) {
        return 32;
    } else if (dlc == 14) {
        return 48;
    }
    return 64;
}

uint64_t DroneCAN::micros64(void)
{
    uint32_t us = micros();
    if (us < last_micros32) {
        base_micros64 += 0x100000000ULL;
    }
    last_micros32 = us;
    return us + base_micros64;
}

/*
  handle a GET_NODE_INFO request
 */
void DroneCAN::handle_get_node_info(CanardInstance* ins, CanardRxTransfer* transfer)
{
    uint8_t buffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE] {};
    uavcan_protocol_GetNodeInfoResponse pkt {};

    node_status.uptime_sec = millis() / 1000U;

    pkt.status = node_status;
    pkt.software_version.major = FW_VERSION_MAJOR;
    pkt.software_version.minor = FW_VERSION_MINOR;
    pkt.software_version.optional_field_flags = UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_VCS_COMMIT | UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_IMAGE_CRC;
    pkt.software_version.vcs_commit = GIT_VERSION;

    readUniqueID(pkt.hardware_version.unique_id);

    pkt.hardware_version.major = CAN_BOARD_ID >> 8;
    pkt.hardware_version.minor = CAN_BOARD_ID & 0xFF;
    snprintf((char*)pkt.name.data, sizeof(pkt.name.data), "%s", CAN_APP_NODE_NAME);
    pkt.name.len = strnlen((char*)pkt.name.data, sizeof(pkt.name.data));

    uint16_t total_size = uavcan_protocol_GetNodeInfoResponse_encode(&pkt, buffer);
    canardRequestOrRespond(ins,
                           transfer->source_node_id,
                           UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE,
                           UAVCAN_PROTOCOL_GETNODEINFO_ID,
                           &transfer->transfer_id,
                           transfer->priority,
                           CanardResponse,
                           &buffer[0],
                           total_size);
}

void DroneCAN::handle_allocation_response(CanardInstance* ins, CanardRxTransfer* transfer)
{
    // Rule C - updating the randomized time interval
    send_next_node_id_allocation_request_at_ms =
        millis() + UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS +
        random(1, UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);

    if (transfer->source_node_id == CANARD_BROADCAST_NODE_ID) {
        node_id_allocation_unique_id_offset = 0;
        return;
    }

    // Copying the unique ID from the message
    uavcan_protocol_dynamic_node_id_Allocation msg;

    uavcan_protocol_dynamic_node_id_Allocation_decode(transfer, &msg);

    // Obtaining the local unique ID
    uint8_t my_unique_id[sizeof(msg.unique_id.data)] {};
    readUniqueID(my_unique_id);

    // Matching the received UID against the local one
    if (memcmp(msg.unique_id.data, my_unique_id, msg.unique_id.len) != 0) {
        node_id_allocation_unique_id_offset = 0;
        return;
    }

    if (msg.unique_id.len < sizeof(msg.unique_id.data)) {
        // The allocator has confirmed part of unique ID, switching to the next stage and updating the timeout.
        node_id_allocation_unique_id_offset = msg.unique_id.len;
        send_next_node_id_allocation_request_at_ms -= UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS;
    } else {
        // Allocation complete - copying the allocated node ID from the message
        canardSetLocalNodeID(ins, msg.node_id);
        Serial.printf("Node ID allocated: %u\n", unsigned(msg.node_id));
    }
}

bool DroneCAN::do_DNA(void)
{
    if (canardGetLocalNodeID(&canard) != CANARD_BROADCAST_NODE_ID) {
        return true;
    }
    const uint32_t now = millis();
    if (now - last_DNA_start_ms < 1000 && node_id_allocation_unique_id_offset == 0) {
        return false;
    }
    last_DNA_start_ms = now;

    uint8_t node_id_allocation_transfer_id = 0;
    UNUSED(node_id_allocation_transfer_id);
    send_next_node_id_allocation_request_at_ms =
        now + UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS +
        random(1, UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);

    uint8_t allocation_request[CANARD_CAN_FRAME_MAX_DATA_LEN - 1] {};
    allocation_request[0] = 0;
    if (node_id_allocation_unique_id_offset == 0) {
        allocation_request[0] |= 1;
    }

    uint8_t my_unique_id[sizeof(uavcan_protocol_dynamic_node_id_Allocation::unique_id.data)] {};
    readUniqueID(my_unique_id);

    static const uint8_t MaxLenOfUniqueIDInRequest = 6;
    uint8_t uid_size = (uint8_t)(sizeof(uavcan_protocol_dynamic_node_id_Allocation::unique_id.data) - node_id_allocation_unique_id_offset);

    if (uid_size > MaxLenOfUniqueIDInRequest) {
        uid_size = MaxLenOfUniqueIDInRequest;
    }

    memmove(&allocation_request[1], &my_unique_id[node_id_allocation_unique_id_offset], uid_size);

    // Broadcasting the request
    static uint8_t tx_id;
    canardBroadcast(&canard,
                    UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE,
                    UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID,
                    &tx_id,
                    CANARD_TRANSFER_PRIORITY_LOW,
                    &allocation_request[0],
                    (uint16_t) (uid_size + 1));
    node_id_allocation_unique_id_offset = 0;
    return false;
}

void DroneCAN::readUniqueID(uint8_t id[6])
{
    esp_efuse_mac_get_default(id);
}

#define IMIN(a,b) ((a)<(b)?(a):(b))
#define COPY_FIELD(fname) mpkt.fname = pkt.fname
#define COPY_STR(fname) memcpy(mpkt.fname, pkt.fname.data, IMIN(pkt.fname.len, sizeof(mpkt.fname)))

void DroneCAN::handle_BasicID(CanardRxTransfer* transfer)
{
    dronecan_remoteid_BasicID pkt {};
    dronecan_remoteid_BasicID_decode(transfer, &pkt);

    if ((pkt.uas_id.len > 0) && (pkt.id_type > 0) && (pkt.id_type <= MAV_ODID_ID_TYPE_SPECIFIC_SESSION_ID)) {
        //only update if we receive valid data
        auto &mpkt = basic_id;
        memset(&mpkt, 0, sizeof(mpkt));
        COPY_STR(id_or_mac);
        COPY_FIELD(id_type);
        COPY_FIELD(ua_type);
        COPY_STR(uas_id);
        last_basic_id_ms = millis();
    }
}

void DroneCAN::handle_SelfID(CanardRxTransfer* transfer)
{
    dronecan_remoteid_SelfID pkt {};
    auto &mpkt = self_id;
    dronecan_remoteid_SelfID_decode(transfer, &pkt);
    last_self_id_ms = millis();
    memset(&mpkt, 0, sizeof(mpkt));
    COPY_STR(id_or_mac);
    COPY_FIELD(description_type);
    COPY_STR(description);
}

void DroneCAN::handle_System(CanardRxTransfer* transfer)
{
    dronecan_remoteid_System pkt {};
    auto &mpkt = system;
    dronecan_remoteid_System_decode(transfer, &pkt);

    if ((last_system_timestamp != pkt.timestamp) || (pkt.timestamp == 0)) {
        //only update the timestamp if we receive information with a different timestamp
        last_system_ms = millis();
        last_system_timestamp = pkt.timestamp;
	}
    memset(&mpkt, 0, sizeof(mpkt));

    COPY_STR(id_or_mac);
    COPY_FIELD(operator_location_type);
    COPY_FIELD(classification_type);
    COPY_FIELD(operator_latitude);
    COPY_FIELD(operator_longitude);
    COPY_FIELD(area_count);
    COPY_FIELD(area_radius);
    COPY_FIELD(area_ceiling);
    COPY_FIELD(area_floor);
    COPY_FIELD(category_eu);
    COPY_FIELD(class_eu);
    COPY_FIELD(operator_altitude_geo);
    COPY_FIELD(timestamp);
}

void DroneCAN::handle_OperatorID(CanardRxTransfer* transfer)
{
    dronecan_remoteid_OperatorID pkt {};
    auto &mpkt = operator_id;
    dronecan_remoteid_OperatorID_decode(transfer, &pkt);
    last_operator_id_ms = millis();
    memset(&mpkt, 0, sizeof(mpkt));

    COPY_STR(id_or_mac);
    COPY_FIELD(operator_id_type);
    COPY_STR(operator_id);
}

void DroneCAN::handle_Location(CanardRxTransfer* transfer)
{
    dronecan_remoteid_Location pkt {};
    auto &mpkt = location;
    dronecan_remoteid_Location_decode(transfer, &pkt);
    if (last_location_timestamp != pkt.timestamp) {
        //only update the timestamp if we receive information with a different timestamp
        last_location_ms = millis();
        last_location_timestamp = pkt.timestamp;
    }
    memset(&mpkt, 0, sizeof(mpkt));

    COPY_STR(id_or_mac);
    COPY_FIELD(status);
    COPY_FIELD(direction);
    COPY_FIELD(speed_horizontal);
    COPY_FIELD(speed_vertical);
    COPY_FIELD(latitude);
    COPY_FIELD(longitude);
    COPY_FIELD(altitude_barometric);
    COPY_FIELD(altitude_geodetic);
    COPY_FIELD(height_reference);
    COPY_FIELD(height);
    COPY_FIELD(horizontal_accuracy);
    COPY_FIELD(vertical_accuracy);
    COPY_FIELD(barometer_accuracy);
    COPY_FIELD(speed_accuracy);
    COPY_FIELD(timestamp);
    COPY_FIELD(timestamp_accuracy);
}

/*
  handle parameter GetSet request
 */
void DroneCAN::handle_param_getset(CanardInstance* ins, CanardRxTransfer* transfer)
{
    uavcan_protocol_param_GetSetRequest req;
    if (uavcan_protocol_param_GetSetRequest_decode(transfer, &req)) {
        return;
    }

    uavcan_protocol_param_GetSetResponse pkt {};

    const Parameters::Param *vp = nullptr;

    if (req.name.len != 0 && req.name.len > PARAM_NAME_MAX_LEN) {
        vp = nullptr;
    } else if (req.name.len != 0 && req.name.len <= PARAM_NAME_MAX_LEN) {
        memcpy((char *)pkt.name.data, (char *)req.name.data, req.name.len);
        vp = Parameters::find((char *)pkt.name.data);
    } else {
        vp = Parameters::find_by_index(req.index);
    }
    if (vp != nullptr && (vp->flags & PARAM_FLAG_HIDDEN)) {
        vp = nullptr;
    }
    if (vp != nullptr && req.name.len != 0 &&
        req.value.union_tag != UAVCAN_PROTOCOL_PARAM_VALUE_EMPTY) {
        if (g.lock_level > 0) {
            can_printf("Parameters locked");
        } else {
            // param set
            switch (vp->ptype) {
            case Parameters::ParamType::UINT8:
                if (req.value.union_tag != UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE) {
                    return;
                }
                vp->set_uint8(uint8_t(req.value.integer_value));
                break;
            case Parameters::ParamType::INT8:
                if (req.value.union_tag != UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE) {
                    return;
                }
                vp->set_int8(int8_t(req.value.integer_value));
                break;
            case Parameters::ParamType::UINT32:
                if (req.value.union_tag != UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE) {
                    return;
                }
                vp->set_uint32(uint32_t(req.value.integer_value));
                break;
            case Parameters::ParamType::FLOAT:
                if (req.value.union_tag != UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE) {
                    return;
                }
                vp->set_float(req.value.real_value);
                break;
            case Parameters::ParamType::CHAR20: {
                if (req.value.union_tag != UAVCAN_PROTOCOL_PARAM_VALUE_STRING_VALUE) {
                    return;
                }
                char v[21] {};
                strncpy(v, (const char *)&req.value.string_value.data[0], req.value.string_value.len);
                if (vp->min_len > 0 && strlen(v) < vp->min_len) {
                    can_printf("%s too short - min %u", vp->name, vp->min_len);
                } else {
                    vp->set_char20(v);
                }
                break;
            }
            case Parameters::ParamType::CHAR64: {
                if (req.value.union_tag != UAVCAN_PROTOCOL_PARAM_VALUE_STRING_VALUE) {
                    return;
                }
                char v[65] {};
                strncpy(v, (const char *)&req.value.string_value.data[0], req.value.string_value.len);
                vp->set_char64(v);
                break;
            }
            default:
                return;
            }
        }
    }
    if (vp != nullptr) {
        switch (vp->ptype) {
        case Parameters::ParamType::UINT8:
            pkt.value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            pkt.value.integer_value = vp->get_uint8();
            pkt.default_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            pkt.default_value.integer_value = uint8_t(vp->default_value);
            pkt.min_value.union_tag = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_INTEGER_VALUE;
            pkt.min_value.integer_value = uint8_t(vp->min_value);
            pkt.max_value.union_tag = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_INTEGER_VALUE;
            pkt.max_value.integer_value = uint8_t(vp->max_value);
            break;
        case Parameters::ParamType::INT8:
            pkt.value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            pkt.value.integer_value = vp->get_int8();
            pkt.default_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            pkt.default_value.integer_value = int8_t(vp->default_value);
            pkt.min_value.union_tag = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_INTEGER_VALUE;
            pkt.min_value.integer_value = int8_t(vp->min_value);
            pkt.max_value.union_tag = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_INTEGER_VALUE;
            pkt.max_value.integer_value = int8_t(vp->max_value);
            break;
        case Parameters::ParamType::UINT32:
            pkt.value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            pkt.value.integer_value = vp->get_uint32();
            pkt.default_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE;
            pkt.default_value.integer_value = uint32_t(vp->default_value);
            pkt.min_value.union_tag = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_INTEGER_VALUE;
            pkt.min_value.integer_value = uint32_t(vp->min_value);
            pkt.max_value.union_tag = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_INTEGER_VALUE;
            pkt.max_value.integer_value = uint32_t(vp->max_value);
            break;
        case Parameters::ParamType::FLOAT:
            pkt.value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE;
            pkt.value.real_value = vp->get_float();
            pkt.default_value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE;
            pkt.default_value.real_value = vp->default_value;
            pkt.min_value.union_tag = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_REAL_VALUE;
            pkt.min_value.real_value = vp->min_value;
            pkt.max_value.union_tag = UAVCAN_PROTOCOL_PARAM_NUMERICVALUE_REAL_VALUE;
            pkt.max_value.real_value = vp->max_value;
            break;
        case Parameters::ParamType::CHAR20: {
            pkt.value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_STRING_VALUE;
            const char *s = vp->get_char20();
            if (vp->flags & PARAM_FLAG_PASSWORD) {
                s = "********";
            }
            strncpy((char*)pkt.value.string_value.data, s, sizeof(pkt.value.string_value.data));
            pkt.value.string_value.len = strlen(s);
            break;
        }
        case Parameters::ParamType::CHAR64: {
            pkt.value.union_tag = UAVCAN_PROTOCOL_PARAM_VALUE_STRING_VALUE;
            const char *s = vp->get_char64();
            strncpy((char*)pkt.value.string_value.data, s, sizeof(pkt.value.string_value.data));
            pkt.value.string_value.len = strlen(s);
            break;
        }
        default:
            return;
        }
        pkt.name.len = strlen(vp->name);
        strncpy((char *)pkt.name.data, vp->name, sizeof(pkt.name.data));
    }

    uint8_t buffer[UAVCAN_PROTOCOL_PARAM_GETSET_RESPONSE_MAX_SIZE] {};
    uint16_t total_size = uavcan_protocol_param_GetSetResponse_encode(&pkt, buffer);

    canardRequestOrRespond(ins,
                           transfer->source_node_id,
                           UAVCAN_PROTOCOL_PARAM_GETSET_SIGNATURE,
                           UAVCAN_PROTOCOL_PARAM_GETSET_ID,
                           &transfer->transfer_id,
                           transfer->priority,
                           CanardResponse,
                           &buffer[0],
                           total_size);
}

/*
  handle SecureCommand
 */
void DroneCAN::handle_SecureCommand(CanardInstance* ins, CanardRxTransfer* transfer)
{
    dronecan_remoteid_SecureCommandRequest req;
    if (dronecan_remoteid_SecureCommandRequest_decode(transfer, &req)) {
        return;
    }

    dronecan_remoteid_SecureCommandResponse reply {};
    reply.result = DRONECAN_REMOTEID_SECURECOMMAND_RESPONSE_RESULT_UNSUPPORTED;
    reply.sequence = req.sequence;
    reply.operation = req.operation;

    if (!check_signature(req.sig_length, req.data.len-req.sig_length,
                         req.sequence, req.operation, req.data.data)) {
        reply.result = DRONECAN_REMOTEID_SECURECOMMAND_RESPONSE_RESULT_DENIED;
        goto send_reply;
    }

    switch (req.operation) {
    case DRONECAN_REMOTEID_SECURECOMMAND_REQUEST_SECURE_COMMAND_GET_REMOTEID_SESSION_KEY: {
        make_session_key(session_key);
        memcpy(reply.data.data, session_key, sizeof(session_key));
        reply.data.len = sizeof(session_key);
        reply.result = DRONECAN_REMOTEID_SECURECOMMAND_RESPONSE_RESULT_ACCEPTED;
        break;
    }
    case DRONECAN_REMOTEID_SECURECOMMAND_REQUEST_SECURE_COMMAND_SET_REMOTEID_CONFIG: {
        Serial.printf("SECURE_COMMAND_SET_REMOTEID_CONFIG\n");
        int16_t data_len = req.data.len - req.sig_length;
        req.data.data[data_len] = 0;
        /*
          command buffer is nul separated set of NAME=VALUE pairs
         */
        reply.result = DRONECAN_REMOTEID_SECURECOMMAND_RESPONSE_RESULT_ACCEPTED;
        char *command = (char *)req.data.data;
        while (data_len > 0) {
            uint8_t cmdlen = strlen(command);
            Serial.printf("set_config %s", command);
            char *eq = strchr(command, '=');
            if (eq != nullptr) {
                *eq = 0;
                if (!g.set_by_name_string(command, eq+1)) {
                    reply.result = DRONECAN_REMOTEID_SECURECOMMAND_RESPONSE_RESULT_FAILED;
                }
            }
            command += cmdlen+1;
            data_len -= cmdlen+1;
        }
        break;
    }
    }

send_reply:
    uint8_t buffer[UAVCAN_PROTOCOL_PARAM_GETSET_RESPONSE_MAX_SIZE] {};
    uint16_t total_size = dronecan_remoteid_SecureCommandResponse_encode(&reply, buffer);

    canardRequestOrRespond(ins,
                           transfer->source_node_id,
                           DRONECAN_REMOTEID_SECURECOMMAND_SIGNATURE,
                           DRONECAN_REMOTEID_SECURECOMMAND_ID,
                           &transfer->transfer_id,
                           transfer->priority,
                           CanardResponse,
                           &buffer[0],
                           total_size);
}

// printf to CAN LogMessage for debugging
void DroneCAN::can_printf(const char *fmt, ...)
{
    uavcan_protocol_debug_LogMessage pkt {};
    uint8_t buffer[UAVCAN_PROTOCOL_DEBUG_LOGMESSAGE_MAX_SIZE] {};
    va_list ap;
    va_start(ap, fmt);
    uint32_t n = vsnprintf((char*)pkt.text.data, sizeof(pkt.text.data), fmt, ap);
    va_end(ap);
    pkt.text.len = n;
    if (sizeof(pkt.text.data) < n) {
        pkt.text.len = sizeof(pkt.text.data);
    }

    uint32_t len = uavcan_protocol_debug_LogMessage_encode(&pkt, buffer);
    static uint8_t tx_id;

    canardBroadcast(&canard,
                    UAVCAN_PROTOCOL_DEBUG_LOGMESSAGE_SIGNATURE,
                    UAVCAN_PROTOCOL_DEBUG_LOGMESSAGE_ID,
                    &tx_id,
                    CANARD_TRANSFER_PRIORITY_LOW,
                    buffer,
                    len);
}



#if 0
// xprintf is useful when debugging in C code such as libcanard
extern "C" {
    void xprintf(const char *fmt, ...);
}

void xprintf(const char *fmt, ...)
{
    char buffer[200] {};
    va_list ap;
    va_start(ap, fmt);
    uint32_t n = vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    Serial.printf("%s", buffer);
}
#endif
