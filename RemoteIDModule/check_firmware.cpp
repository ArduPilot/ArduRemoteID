#include <Arduino.h>
#include "check_firmware.h"
#include "monocypher.h"
#include "parameters.h"
#include <string.h>

/*
  simple base64 decoder, not particularly efficient, but small
 */
static int32_t base64_decode(const char *s, uint8_t *out, const uint32_t max_len)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const char *p;
    uint32_t n = 0;
    uint32_t i = 0;
    while (*s && (p=strchr(b64,*s))) {
        const uint8_t idx = (p - b64);
        const uint32_t byte_offset = (i*6)/8;
        const uint32_t bit_offset = (i*6)%8;
        out[byte_offset] &= ~((1<<(8-bit_offset))-1);
        if (bit_offset < 3) {
            if (byte_offset >= max_len) {
                break;
            }
            out[byte_offset] |= (idx << (2-bit_offset));
            n = byte_offset+1;
        } else {
            if (byte_offset >= max_len) {
                break;
            }
            out[byte_offset] |= (idx >> (bit_offset-2));
            n = byte_offset+1;
            if (byte_offset+1 >= max_len) {
                break;
            }
            out[byte_offset+1] = (idx << (8-(bit_offset-2))) & 0xFF;
            n = byte_offset+2;
        }
        s++; i++;
    }

    if ((n > 0) && (*s == '=')) {
        n -= 1;
    }

    return n;
}


bool CheckFirmware::check_partition(const uint8_t *flash, uint32_t flash_len,
                                    const uint8_t *lead_bytes, uint32_t lead_length,
                                    const app_descriptor_t *ad, const uint8_t public_key[32])
{
    crypto_check_ctx ctx {};
    crypto_check_ctx_abstract *actx = (crypto_check_ctx_abstract*)&ctx;
    crypto_check_init(actx, ad->sign_signature, public_key);
    if (lead_length > 0) {
        crypto_check_update(actx, lead_bytes, lead_length);
    }
    crypto_check_update(actx, &flash[lead_length], flash_len-lead_length);
    return crypto_check_final(actx) == 0;
}

bool CheckFirmware::check_OTA_partition(const esp_partition_t *part, const uint8_t *lead_bytes, uint32_t lead_length, uint32_t &board_id)
{
    Serial.printf("Checking partition %s\n", part->label);
    spi_flash_mmap_handle_t handle;
    const void *ptr = nullptr;
    auto ret = esp_partition_mmap(part, 0, part->size, SPI_FLASH_MMAP_DATA, &ptr, &handle);
    if (ret != ESP_OK) {
        Serial.printf("mmap failed\n");
        return false;
    }
    const uint8_t sig_rev[] = APP_DESCRIPTOR_REV;
    uint8_t sig[8];
    for (uint8_t i=0; i<8; i++) {
        sig[i] = sig_rev[7-i];
    }
    const app_descriptor_t *ad = (app_descriptor_t *)memmem(ptr, part->size, sig, sizeof(sig));
    if (ad == nullptr) {
        Serial.printf("app_descriptor not found\n");
        spi_flash_munmap(handle);
        return false;
    }
    Serial.printf("app descriptor at 0x%x size=%u id=%u\n", unsigned(ad)-unsigned(ptr), ad->image_size, ad->board_id);
    const uint32_t img_len = uint32_t(uintptr_t(ad) - uintptr_t(ptr));
    if (ad->image_size != img_len) {
        Serial.printf("app_descriptor bad size %u\n", ad->image_size);
        spi_flash_munmap(handle);
        return false;
    }
    board_id = ad->board_id;

    bool no_keys = true;

    for (uint8_t i=0; i<MAX_PUBLIC_KEYS; i++) {
        const char *b64_key = g.public_keys[i].b64_key;
        Serial.printf("Checking public key: '%s'\n", b64_key);
        const char *ktype = "PUBLIC_KEYV1:";
        if (strncmp(b64_key, ktype, strlen(ktype)) != 0) {
            continue;
        }
        no_keys = false;
        b64_key += strlen(ktype);
        uint8_t key[32];
        int32_t out_len = base64_decode(b64_key, key, sizeof(key));
        if (out_len != 32) {
            continue;
        }
        if (check_partition((const uint8_t *)ptr, img_len, lead_bytes, lead_length, ad, key)) {
            Serial.printf("check firmware good for key %u\n", i);
            spi_flash_munmap(handle);
            return true;
        }
        Serial.printf("check failed key %u\n", i);
    }
    spi_flash_munmap(handle);
    if (no_keys) {
        Serial.printf("No public keys - accepting firmware\n");
        return true;
    }
    Serial.printf("firmware failed checks\n");
    return false;
}

bool CheckFirmware::check_OTA_next(const uint8_t *lead_bytes, uint32_t lead_length)
{
    const auto *running_part = esp_ota_get_running_partition();
    if (running_part == nullptr) {
        Serial.printf("No running OTA partition\n");
        return false;
    }
    const auto *part = esp_ota_get_next_update_partition(running_part);
    if (part == nullptr) {
        Serial.printf("No next OTA partition\n");
        return false;
    }
    uint32_t board_id=0;
    bool sig_ok = check_OTA_partition(part, lead_bytes, lead_length, board_id);
    // if app descriptor has a board ID and the ID is wrong then reject
    if (board_id != 0 && board_id != BOARD_ID) {
        return false;
    }
    if (g.lock_level == 0) {
        // if unlocked then accept any firmware
        return true;
    }
    return sig_ok;
}

bool CheckFirmware::check_OTA_running(void)
{
    const auto *running_part = esp_ota_get_running_partition();
    if (running_part == nullptr) {
        Serial.printf("No running OTA partition\n");
        return false;
    }
    uint32_t board_id=0;
    return check_OTA_partition(running_part, nullptr, 0, board_id);
}
        
esp_err_t esp_partition_read_raw(const esp_partition_t* partition,
                                 size_t src_offset, void* dst, size_t size);
    
