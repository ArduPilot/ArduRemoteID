/*
  mavlink message definitions
 */
#pragma once

// we have separate helpers disabled to make it possible
// to select MAVLink 1.0 in the arduino GUI build
#define MAVLINK_SEPARATE_HELPERS
#define MAVLINK_NO_CONVERSION_HELPERS

#define MAVLINK_SEND_UART_BYTES(chan, buf, len) comm_send_buffer(chan, buf, len)

// two buffers, one for USB, one for UART. This makes for easier testing with SITL
#define MAVLINK_COMM_NUM_BUFFERS 2

#define MAVLINK_MAX_PAYLOAD_LEN 255

#include <mavlink2.h>

/// MAVLink system definition
extern mavlink_system_t mavlink_system;

void comm_send_buffer(mavlink_channel_t chan, const uint8_t *buf, uint8_t len);

#define MAVLINK_USE_CONVENIENCE_FUNCTIONS
#include <generated/all/mavlink.h>
