/*
  BLE TX driver

  Many thanks to Roel Schiphorst from BlueMark for his assistance with the Bluetooth code

  Also many thanks to chegewara for his sample code:
  https://github.com/Tinyu-Zhao/Arduino-Borads/blob/b584d00a81e4ac6d7b72697c674ca1bbcb8aae69/libraries/BLE/examples/BLE5_multi_advertising/BLE5_multi_advertising.ino
 */

#include "BLE_TX.h"
#include "options.h"
#include <esp_system.h>

#include <BLEDevice.h>
#include <BLEAdvertising.h>

// set max power
static const int8_t tx_power = ESP_PWR_LVL_P18;

//interval min/max are configured for 1 Hz update rate. Somehow dynamic setting of these fields fails
//shorter intervals lead to more BLE transmissions. This would result in increased power consumption and can lead to more interference to other radio systems.
static esp_ble_gap_ext_adv_params_t legacy_adv_params = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_NONCONN,
    .interval_min = 125, //(unsigned int) 0.75*1000/(OUTPUT_RATE_HZ*6); //allow ble controller to have some room for transmission.
    .interval_max = 167, //(unsigned int) 1000/(OUTPUT_RATE_HZ*6);
    .channel_map = ADV_CHNL_ALL,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST, // we want unicast non-connectable transmission
    .tx_power = tx_power,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_1M,
    .sid = 0,
    .scan_req_notif = false,
};

static esp_ble_gap_ext_adv_params_t ext_adv_params_coded = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_NONCONN_NONSCANNABLE_UNDIRECTED, //set to unicast advertising
     .interval_min = 750, //(unsigned int) 0.75*1000/(OUTPUT_RATE_HZ); //allow ble controller to have some room for transmission.
    .interval_max = 1000, // (unsigned int) 1000/(OUTPUT_RATE_HZ);
    .channel_map = ADV_CHNL_ALL,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST, // we want unicast non-connectable transmission
    .tx_power = tx_power,
    .primary_phy = ESP_BLE_GAP_PHY_CODED,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_CODED,
    .sid = 1,
    .scan_req_notif = false,
};

static BLEMultiAdvertising advert(2);

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

    advert.setAdvertisingParams(0, &legacy_adv_params);
    advert.setInstanceAddress(0, mac_addr);
    advert.setDuration(0);

    advert.setAdvertisingParams(1, &ext_adv_params_coded);
    advert.setDuration(1);
    advert.setInstanceAddress(1, mac_addr);

    // prefer S8 coding
    if (esp_ble_gap_set_prefered_default_phy(ESP_BLE_GAP_PHY_OPTIONS_PREF_S8_CODING, ESP_BLE_GAP_PHY_OPTIONS_PREF_S8_CODING) != ESP_OK) {
        Serial.printf("Failed to setup S8 coding\n");
    }

    legacy_msg_counter = 0;
    longrange_msg_counter = 0;
    return true;
}

#define IMIN(a,b) ((a)<(b)?(a):(b))

bool BLE_TX::transmit_longrange(ODID_UAS_Data &UAS_data)
{
    // create a packed UAS data message
    uint8_t payload[250];
    int length = odid_message_build_pack(&UAS_data, payload, 255);
    if (length <= 0) {
        return false;
    }

    // setup ASTM header
    const uint8_t header[] { uint8_t(length+5), 0x16, 0xfa, 0xff, 0x0d, uint8_t(longrange_msg_counter++) };

    // combine header with payload
    memcpy(longrange_payload, header, sizeof(header));
    memcpy(&longrange_payload[sizeof(header)], payload, length);
    int longrange_length = sizeof(header) + length;

    advert.setAdvertisingData(1, longrange_length, longrange_payload);

    // we start advertising when we have the first lot of data to send
    if (!started) {
        advert.start();
    }
    started = true;

    return true;
}

 bool BLE_TX::transmit_legacy(ODID_UAS_Data &UAS_data)
 {
	static uint8_t legacy_phase = 0;
	int legacy_length = 0;
	// setup ASTM header
	const uint8_t header[] { 0x1e, 0x16, 0xfa, 0xff, 0x0d, uint8_t(legacy_msg_counter++) };
	// combine header with payload
	memset(legacy_payload, 0, sizeof(legacy_payload));
	memcpy(legacy_payload, header, sizeof(header));

	switch (legacy_phase)
    {
		case  0:
			ODID_Location_encoded location_encoded;
			memset(&location_encoded, 0, sizeof(location_encoded));
			if (encodeLocationMessage(&location_encoded, &UAS_data.Location) != ODID_SUCCESS) {
				break;
			}

			memcpy(&legacy_payload[sizeof(header)], &location_encoded, sizeof(location_encoded));
			legacy_length = sizeof(header) + sizeof(location_encoded);

			break;

		case  1:
			ODID_BasicID_encoded basicid_encoded;
			memset(&basicid_encoded, 0, sizeof(basicid_encoded));
			if (encodeBasicIDMessage(&basicid_encoded, &UAS_data.BasicID[0]) != ODID_SUCCESS) {
				break;
			}

			memcpy(&legacy_payload[sizeof(header)], &basicid_encoded, sizeof(basicid_encoded));
			legacy_length = sizeof(header) + sizeof(basicid_encoded);

			break;

		case  2:
			ODID_SelfID_encoded selfid_encoded;
			memset(&selfid_encoded, 0, sizeof(selfid_encoded));
			if (encodeSelfIDMessage(&selfid_encoded, &UAS_data.SelfID) != ODID_SUCCESS) {
				break;
			}

			memcpy(&legacy_payload[sizeof(header)], &selfid_encoded, sizeof(selfid_encoded));
			legacy_length = sizeof(header) + sizeof(selfid_encoded);

			break;

		case  3:
			ODID_System_encoded system_encoded;
			memset(&system_encoded, 0, sizeof(system_encoded));
			if (encodeSystemMessage(&system_encoded, &UAS_data.System) != ODID_SUCCESS) {
				break;
			}

			memcpy(&legacy_payload[sizeof(header)], &system_encoded, sizeof(system_encoded));
			legacy_length = sizeof(header) + sizeof(system_encoded);

			break;

		case  4:
			ODID_OperatorID_encoded operatorid_encoded;
			memset(&operatorid_encoded, 0, sizeof(operatorid_encoded));
			if (encodeOperatorIDMessage(&operatorid_encoded, &UAS_data.OperatorID) != ODID_SUCCESS) {
				break;
			}

			memcpy(&legacy_payload[sizeof(header)], &operatorid_encoded, sizeof(operatorid_encoded));
			legacy_length = sizeof(header) + sizeof(operatorid_encoded);

			break;

		case  5: //broadcast BLE name
			char legacy_name[28] {};
			const char *UAS_ID = (const char *)UAS_data.BasicID[0].UASID;
			const uint8_t ID_len = strlen(UAS_ID);
			const uint8_t ID_tail = IMIN(4, ID_len);
			snprintf(legacy_name, sizeof(legacy_name), "ArduRemoteID_%s", &UAS_ID[ID_len-ID_tail]);

			memset(legacy_payload, 0, sizeof(legacy_payload));
			const uint8_t legacy_header_ble_name[] { 0x02, 0x01, 0x06, uint8_t(strlen(legacy_name)+1), ESP_BLE_AD_TYPE_NAME_SHORT};

			memcpy(legacy_payload, legacy_header_ble_name, sizeof(legacy_header_ble_name));
			memcpy(&legacy_payload[sizeof(legacy_header_ble_name)], legacy_name, strlen(legacy_name) + 1);
			legacy_length = sizeof(legacy_header_ble_name) + strlen(legacy_name);

			break;
	}

	legacy_phase++;
    legacy_phase %= 6;

	advert.setAdvertisingData(0, legacy_length, legacy_payload);

    if (!started) {
        advert.start();
    }
    started = true;

    return true;
}
