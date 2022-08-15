/*
  control optional behaviour in the firmware at build time
 */

// enable WiFi NAN support
#define AP_WIFI_NAN_ENABLED 1

// enable bluetooth 4 and 5 support
#define AP_BLE_ENABLED 1

// start sending packets as soon we we power up,
// not waiting for location data from flight controller
#define AP_BROADCAST_ON_POWER_UP 1

// do we support DroneCAN connnection to flight controller?
#define AP_DRONECAN_ENABLED 1

// do we support MAVLink connnection to flight controller?
#define AP_MAVLINK_ENABLED 1
