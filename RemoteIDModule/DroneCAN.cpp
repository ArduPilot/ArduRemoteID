/*
  DroneCAN class for handling OpenDroneID messages
  with thanks to David Buzz for ArduPilot ESP32 HAL
 */
#include <Arduino.h>
#include "board_config.h"
#include "version.h"
#include <time.h>
#include "DroneCAN.h"
#include <canard.h>
#include <uavcan.protocol.NodeStatus.h>
#include <uavcan.protocol.GetNodeInfo.h>
#include <uavcan.protocol.RestartNode.h>
#include <uavcan.protocol.dynamic_node_id.Allocation.h>
#include <dronecan.remoteid.BasicID.h>
#include <dronecan.remoteid.Location.h>
#include <dronecan.remoteid.SelfID.h>
#include <dronecan.remoteid.System.h>
#include <dronecan.remoteid.OperatorID.h>
#include <dronecan.remoteid.ArmStatus.h>

#define BOARD_ID 10001
#ifndef CAN_APP_NODE_NAME
#define CAN_APP_NODE_NAME "ArduPilot RemoteIDModule"
#endif
#define CAN_DEFAULT_NODE_ID 0 // use DNA

#define UNUSED(x) (void)(x)


static void onTransferReceived_trampoline(CanardInstance* ins, CanardRxTransfer* transfer);
static bool shouldAcceptTransfer_trampoline(const CanardInstance* ins, uint64_t* out_data_type_signature, uint16_t data_type_id,
        CanardTransferType transfer_type,
        uint8_t source_node_id);

// decoded messages

void DroneCAN::init(void)
{
    can_driver.init(1000000);

    canardInit(&canard, (uint8_t *)canard_memory_pool, sizeof(canard_memory_pool),
               onTransferReceived_trampoline, shouldAcceptTransfer_trampoline, NULL);
#if CAN_DEFAULT_NODE_ID
    canardSetLocalNodeID(&canard, CAN_DEFAULT_NODE_ID);
#endif
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

    const uint8_t status = parse_fail==nullptr?MAV_ODID_GOOD_TO_ARM:MAV_ODID_PRE_ARM_FAIL_GENERIC;
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
        Serial.printf("RestartNode\n");
        delay(20);
        esp_restart();
        break;
    case DRONECAN_REMOTEID_BASICID_ID:
        Serial.printf("Got BasicID\n");
        handle_BasicID(transfer);
        break;
    case DRONECAN_REMOTEID_LOCATION_ID:
        Serial.printf("Got Location\n");
        handle_Location(transfer);
        break;
    case DRONECAN_REMOTEID_SELFID_ID:
        Serial.printf("Got SelfID\n");
        handle_SelfID(transfer);
        break;
    case DRONECAN_REMOTEID_SYSTEM_ID:
        Serial.printf("Got System\n");
        handle_System(transfer);
        break;
    case DRONECAN_REMOTEID_OPERATORID_ID:
        Serial.printf("Got OperatorID\n");
        handle_OperatorID(transfer);
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
    while (can_driver.receive(rxmsg)) {
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

    pkt.hardware_version.major = BOARD_ID >> 8;
    pkt.hardware_version.minor = BOARD_ID & 0xFF;
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
    auto &mpkt = basic_id;
    dronecan_remoteid_BasicID_decode(transfer, &pkt);
    last_basic_id_ms = millis();
    memset(&mpkt, 0, sizeof(mpkt));
    COPY_STR(id_or_mac);
    COPY_FIELD(id_type);
    COPY_FIELD(ua_type);
    COPY_STR(uas_id);
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
    last_system_ms = millis();
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
    last_location_ms = millis();
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
