
#include "CANDriver.h"
#include <canard.h>

#include <canard.h>
#include <uavcan.protocol.NodeStatus.h>
#include <dronecan.remoteid.BasicID.h>
#include <dronecan.remoteid.Location.h>
#include <dronecan.remoteid.SelfID.h>
#include <dronecan.remoteid.System.h>
#include <dronecan.remoteid.OperatorID.h>

#define CAN_POOL_SIZE 4096

class DroneCAN {
public:
    DroneCAN();
    void init(void);
    void update(void);

    void set_parse_fail(const char *msg) {
        parse_fail = msg;
    }
    
private:
    uint32_t last_node_status_ms;
    CANDriver can_driver;
    CanardInstance canard;
    uint32_t canard_memory_pool[CAN_POOL_SIZE/sizeof(uint32_t)];

    void node_status_send(void);
    void arm_status_send(void);

    uint8_t tx_fail_count;

    void processTx(void);
    void processRx(void);

    uint64_t micros64();
    uint64_t base_micros64;
    uint32_t last_micros32;

    void handle_get_node_info(CanardInstance* ins, CanardRxTransfer* transfer);

    void readUniqueID(uint8_t id[6]);

    bool do_DNA(void);
    void handle_allocation_response(CanardInstance* ins, CanardRxTransfer* transfer);

    uint32_t send_next_node_id_allocation_request_at_ms;
    uint32_t node_id_allocation_unique_id_offset;
    uint32_t last_DNA_start_ms;

    uavcan_protocol_NodeStatus node_status;
    dronecan_remoteid_BasicID msg_BasicID;
    dronecan_remoteid_Location msg_Location;
    dronecan_remoteid_SelfID msg_SelfID;
    dronecan_remoteid_System msg_System;
    dronecan_remoteid_OperatorID msg_OperatorID;

    uint32_t last_location_ms;
    uint32_t last_basic_id_ms;
    uint32_t last_self_id_ms;
    uint32_t last_operator_id_ms;
    uint32_t last_system_ms;

    const char *parse_fail;
    
public:
    void onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer);
    bool shouldAcceptTransfer(const CanardInstance* ins,
                              uint64_t* out_data_type_signature,
                              uint16_t data_type_id,
                              CanardTransferType transfer_type,
                              uint8_t source_node_id);

    const dronecan_remoteid_BasicID &get_basic_id(void) { return msg_BasicID; }
    const dronecan_remoteid_Location &get_location(void) { return msg_Location; }
    const dronecan_remoteid_SelfID &get_self_id(void) { return msg_SelfID; }
    const dronecan_remoteid_System &get_system(void) { return msg_System; }
    const dronecan_remoteid_OperatorID &get_operator_id(void) { return msg_OperatorID; }

    bool system_valid(void);
    bool location_valid(void);
};
