
#include "CANDriver.h"
#include <canard.h>

#define CAN_POOL_SIZE 4096

class DroneCAN {
public:
    DroneCAN();
    void init(void);
    void update(void);

private:
    uint32_t last_node_status_ms;
    CANDriver can_driver;
    CanardInstance canard;
    uint32_t canard_memory_pool[CAN_POOL_SIZE/sizeof(uint32_t)];

    void node_status_send(void);

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

public:
    void onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer);
    bool shouldAcceptTransfer(const CanardInstance* ins,
                              uint64_t* out_data_type_signature,
                              uint16_t data_type_id,
                              CanardTransferType transfer_type,
                              uint8_t source_node_id);
};
