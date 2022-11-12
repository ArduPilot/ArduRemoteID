#pragma once

#include "options.h"
#include <stdint.h>
#include <esp_ota_ops.h>

// reversed app descriptor. Reversed used to prevent it appearing in flash
#define APP_DESCRIPTOR_REV { 0x19, 0x75, 0xe2, 0x46, 0x37, 0xf1, 0x2a, 0x43 }

class CheckFirmware {
public:
    typedef struct {
        uint8_t sig[8];
        uint32_t  board_id;
        uint32_t image_size;
        uint8_t sign_signature[64];
    } app_descriptor_t;

    // check the firmware on the partition which will be updated by OTA
    static bool check_OTA_next(const esp_partition_t *part, const uint8_t *lead_bytes, uint32_t lead_length);
    static bool check_OTA_running(void);

private:
    static bool check_OTA_partition(const esp_partition_t *part, const uint8_t *lead_bytes, uint32_t lead_length, uint32_t &board_id);
    static bool check_partition(const uint8_t *flash, uint32_t flash_len,
                                const uint8_t *lead_bytes, uint32_t lead_length,
                                const app_descriptor_t *ad, const uint8_t public_key[32]);
};

