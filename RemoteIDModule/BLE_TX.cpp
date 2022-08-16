/*
  BLE TX driver

  Many thanks to Roel Schiphorst from BlueMark for his assistance with the Bluetooth code

  Also many thanks to chegewara for his sample code:
  https://github.com/Tinyu-Zhao/Arduino-Borads/blob/b584d00a81e4ac6d7b72697c674ca1bbcb8aae69/libraries/BLE/examples/BLE5_multi_advertising/BLE5_multi_advertising.ino
 */

#include "BLE_TX.h"
#include <esp_system.h>

#include <BLEDevice.h>
#include <BLEAdvertising.h>

// set max power
static const int8_t tx_power = ESP_PWR_LVL_P18;

static esp_ble_gap_ext_adv_params_t ext_adv_params_1M = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE,
    .interval_min = 0x30,
    .interval_max = 0x30,
    .channel_map = ADV_CHNL_ALL,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .tx_power = tx_power,
    .primary_phy = ESP_BLE_GAP_PHY_CODED,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_1M,
    .sid = 0,
    .scan_req_notif = false,
};

static esp_ble_gap_ext_adv_params_t legacy_adv_params = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND,
    .interval_min = 0x45,
    .interval_max = 0x45,
    .channel_map = ADV_CHNL_ALL,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .tx_power = tx_power,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_1M,
    .sid = 2,
    .scan_req_notif = false,
};

static esp_ble_gap_ext_adv_params_t ext_adv_params_coded = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_SCANNABLE,
    .interval_min = 0x50,
    .interval_max = 0x50,
    .channel_map = ADV_CHNL_ALL,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    .tx_power = tx_power,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_CODED,
    .sid = 3,
    .scan_req_notif = false,
};

static BLEMultiAdvertising advert(3);

bool BLE_TX::init(void)
{
    BLEDevice::init("");

    // generate random mac address
    uint8_t mac_addr[6];
    for (uint8_t i=0; i<6; i++) {
        mac_addr[i] = uint8_t(random(256));
    }
    // set as a bluetooth random static address
    mac_addr[0] |= 0xc0;

    advert.setAdvertisingParams(0, &ext_adv_params_1M);
    advert.setInstanceAddress(0, mac_addr);
    advert.setDuration(0);

    advert.setAdvertisingParams(1, &legacy_adv_params);
    advert.setInstanceAddress(1, mac_addr);
    advert.setDuration(1);

    advert.setAdvertisingParams(2, &ext_adv_params_coded);
    advert.setDuration(2);
    advert.setInstanceAddress(2, mac_addr);

    // prefer S8 coding
    if (esp_ble_gap_set_prefered_default_phy(ESP_BLE_GAP_PHY_OPTIONS_PREF_S8_CODING, ESP_BLE_GAP_PHY_OPTIONS_PREF_S8_CODING) != ESP_OK) {
        Serial.printf("Failed to setup S8 coding\n");
    }

    return true;
}

#define MIN(a,b) ((a)<(b)?(a):(b))

bool BLE_TX::transmit(ODID_UAS_Data &UAS_data)
{
    // create a packed UAS data message
    int length = odid_message_build_pack(&UAS_data, payload, 255);
    if (length <= 0) {
        return false;
    }

    // setup ASTM header
    const uint8_t header[] { uint8_t(length+5), 0x16, 0xfa, 0xff, 0x0d, uint8_t(msg_counter++) };

    // combine header with payload
    memcpy(payload2, header, sizeof(header));
    memcpy(&payload2[sizeof(header)], payload, length);
    int length2 = sizeof(header) + length;

    char legacy_name[28] {};
    const char *UAS_ID = (const char *)UAS_data.BasicID[0].UASID;
    const uint8_t ID_len = strlen(UAS_ID);
    const uint8_t ID_tail = MIN(4, ID_len);
    snprintf(legacy_name, sizeof(legacy_name), "DroneBeacon_%s", &UAS_ID[ID_len-ID_tail]);

    memset(legacy_payload, 0, sizeof(legacy_payload));
    const uint8_t legacy_header[] { 0x02, 0x01, 0x06, uint8_t(strlen(legacy_name)+1), ESP_BLE_AD_TYPE_NAME_SHORT};

    memcpy(legacy_payload, legacy_header, sizeof(legacy_header));
    memcpy(&legacy_payload[sizeof(legacy_header)], legacy_name, strlen(legacy_name));
    const uint8_t legacy_length = sizeof(legacy_header) + strlen(legacy_name);
    
    // setup advertising data
    advert.setAdvertisingData(0, length2, payload2);
    advert.setScanRspData(1, legacy_length, legacy_payload);
    advert.setAdvertisingData(1, legacy_length, legacy_payload);
    advert.setScanRspData(2, length2, payload2);

    // we start advertising when we have the first lot of data to send
    if (!started) {
        advert.start();
    }
    started = true;

    return true;
}
