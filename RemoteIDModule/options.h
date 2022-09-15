/*
  control optional behaviour in the firmware at build time
 */
#pragma once

#include "board_config.h"

// do we support DroneCAN connnection to flight controller?
#define AP_DRONECAN_ENABLED defined(PIN_CAN_TX) && defined(PIN_CAN_RX)

// do we support MAVLink connnection to flight controller?
#define AP_MAVLINK_ENABLED 1
