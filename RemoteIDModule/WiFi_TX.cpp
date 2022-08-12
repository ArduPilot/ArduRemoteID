/*
  WiFi NAN driver

  with thanks to https://github.com/sxjack/uav_electronic_ids for WiFi calls
 */

#include "WiFi_TX.h"
#include <esp_wifi.h>
#include <WiFi.h>
#include <esp_system.h>

bool WiFi_NAN::init(const char *_ssid)
{
    int            i;
    int8_t         wifi_power;
    wifi_config_t  wifi_config;

    strncpy(ssid, _ssid, sizeof(ssid));
    ssid[sizeof(ssid)-1] = 0;

    ssid_length = strlen(ssid);
    WiFi.softAP(ssid, NULL, wifi_channel);

    esp_wifi_get_config(WIFI_IF_AP, &wifi_config);
  
    wifi_config.ap.ssid_hidden = 1;
    esp_err_t status = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);

    // esp_wifi_set_country();
    status = esp_wifi_set_bandwidth(WIFI_IF_AP,WIFI_BW_HT20);

    // esp_wifi_set_max_tx_power(78);
    esp_wifi_get_max_tx_power(&wifi_power);

    String address = WiFi.macAddress();
    status = esp_read_mac(WiFi_mac_addr,ESP_MAC_WIFI_STA);

    return true;
}

bool WiFi_NAN::transmit(ODID_UAS_Data &UAS_data)
{
    int            length;
    esp_err_t      wifi_status;
    char           text[128];
    uint8_t        buffer[1024];

    if ((length = odid_wifi_build_nan_sync_beacon_frame((char *)WiFi_mac_addr,
                                                        buffer,sizeof(buffer))) > 0) {
        wifi_status = esp_wifi_80211_tx(WIFI_IF_AP,buffer,length,true);
    }

    
    if ((length = odid_wifi_build_message_pack_nan_action_frame(&UAS_data,(char *)WiFi_mac_addr,
                                                                ++send_counter,
                                                                buffer,sizeof(buffer))) > 0) {
        wifi_status = esp_wifi_80211_tx(WIFI_IF_AP,buffer,length,true);
    }

    return true;
}
