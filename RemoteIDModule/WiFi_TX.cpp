/*
  WiFi NAN and Beacon driver

  with thanks to https://github.com/sxjack/uav_electronic_ids for WiFi calls
 */

#include "WiFi_TX.h"
#include <esp_wifi.h>
#include <WiFi.h>
#include <esp_system.h>
#include "parameters.h"

bool WiFi_TX::init(void)
{
    //use a local MAC address to avoid tracking transponders based on their MAC address
    uint8_t mac_addr[6];
    generate_random_mac(mac_addr);

    mac_addr[0] |= 0x02;  // set MAC local bit
    mac_addr[0] &= 0xFE;  // unset MAC multicast bit

    //set MAC address
    esp_base_mac_addr_set(mac_addr);

    if (g.webserver_enable == 0) {
		WiFi.softAP(g.wifi_ssid, g.wifi_password, g.wifi_channel, false, 0); //make it visible and allow no connection
	}
	else {
		WiFi.softAP(g.wifi_ssid, g.wifi_password, g.wifi_channel, false, 1); //make it visible and allow only 1 connection
	}

    if (esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20) != ESP_OK) {
        return false;
    }

    memcpy(WiFi_mac_addr,mac_addr,6); //use generated random MAC address for OpenDroneID messages

    esp_wifi_set_max_tx_power(dBm_to_tx_power(g.wifi_power));

    return true;
}

bool WiFi_TX::transmit_nan(ODID_UAS_Data &UAS_data)
{
    if (!initialised) {
        initialised = true;
        init();
    }
    uint8_t buffer[1024] {};

    int length;
    if ((length = odid_wifi_build_nan_sync_beacon_frame((char *)WiFi_mac_addr,
                  buffer,sizeof(buffer))) > 0) {
        if (esp_wifi_80211_tx(WIFI_IF_AP,buffer,length,true) != ESP_OK) {
            return false;
        }
    }

    if ((length = odid_wifi_build_message_pack_nan_action_frame(&UAS_data,(char *)WiFi_mac_addr,
                  ++send_counter_nan,
                  buffer,sizeof(buffer))) > 0) {
        if (esp_wifi_80211_tx(WIFI_IF_AP,buffer,length,true) != ESP_OK) {
            return false;
        }
    }

    return true;
}

//update the payload of the beacon frames in this function
bool WiFi_TX::transmit_beacon(ODID_UAS_Data &UAS_data)
{
    if (!initialised) {
        initialised = true;
        init();
    }
    uint8_t buffer[1024] {};

    int length;
    if ((length = odid_wifi_build_message_pack_beacon_frame(&UAS_data,(char *)WiFi_mac_addr,
                   "UAS_ID_OPEN", strlen("UAS_ID_OPEN"), //use dummy SSID, as we only extract payload data
                  1000/g.wifi_beacon_rate, ++send_counter_beacon, buffer, sizeof(buffer))) > 0) {

        //set the RID IE element
        uint8_t header_offset = 58;
        vendor_ie_data_t IE_data;
        IE_data.element_id = WIFI_VENDOR_IE_ELEMENT_ID;
        IE_data.vendor_oui[0] = 0xFA;
        IE_data.vendor_oui[1] = 0x0B;
        IE_data.vendor_oui[2] = 0xBC;
        IE_data.vendor_oui_type = 0x0D;
        IE_data.length = length - header_offset + 4; //add 4 as of definition esp_wifi_set_vendor_ie
        memcpy(IE_data.payload,&buffer[header_offset],length - header_offset);

        // so first remove old element, add new afterwards
        if (esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, &IE_data) != ESP_OK){
            return false;
        }

        if (esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, &IE_data) != ESP_OK){
            return false;
        }

        //set the payload also to probe requests, to increase update rate on mobile phones
        // so first remove old element, add new afterwards
        if (esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_PROBE_RESP, WIFI_VND_IE_ID_0, &IE_data) != ESP_OK){
            return false;
        }

        if (esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_PROBE_RESP, WIFI_VND_IE_ID_0, &IE_data) != ESP_OK){
            return false;
        }

        return true;
	}
	else {
		return false;
	}
}


/*
  map dBm to a TX power
 */
uint8_t WiFi_TX::dBm_to_tx_power(float dBm) const
{
    if (dBm < 2) {
        dBm = 2;
    }
    if (dBm > 20) {
        dBm = 20;
    }
    return uint8_t((dBm+1.125) * 4);
}
