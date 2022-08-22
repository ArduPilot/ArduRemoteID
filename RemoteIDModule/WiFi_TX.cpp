/*
  WiFi NAN driver

  with thanks to https://github.com/sxjack/uav_electronic_ids for WiFi calls
 */

#include "WiFi_TX.h"
#include <esp_wifi.h>
#include <WiFi.h>
#include <esp_system.h>

bool WiFi_NAN::init(void)
{
    wifi_config_t wifi_config {};

    WiFi.softAP("", NULL, wifi_channel);

    esp_wifi_get_config(WIFI_IF_AP, &wifi_config);

    wifi_config.ap.ssid_hidden = 1;

    if (esp_wifi_set_config(WIFI_IF_AP, &wifi_config) != ESP_OK) {
        return false;
    }
    if (esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20) != ESP_OK) {
        return false;
    }
    if (esp_read_mac(WiFi_mac_addr, ESP_MAC_WIFI_STA) != ESP_OK) {
        return false;
    }

    return true;
}

bool WiFi_NAN::transmit(ODID_UAS_Data &UAS_data)
{
    uint8_t buffer[1024] {};

    int length;
    if ((length = odid_wifi_build_nan_sync_beacon_frame((char *)WiFi_mac_addr,
                  buffer,sizeof(buffer))) > 0) {
        if (esp_wifi_80211_tx(WIFI_IF_AP,buffer,length,true) != ESP_OK) {
            return false;
        }
    }

    if ((length = odid_wifi_build_message_pack_nan_action_frame(&UAS_data,(char *)WiFi_mac_addr,
                  ++send_counter,
                  buffer,sizeof(buffer))) > 0) {
        if (esp_wifi_80211_tx(WIFI_IF_AP,buffer,length,true) != ESP_OK) {
            return false;
        }
    }

    return true;
}
