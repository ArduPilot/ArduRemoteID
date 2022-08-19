/*
  board configuration file
 */

#pragma once

#ifdef BOARD_ESP32S3_DEV
#define PIN_CAN_TX GPIO_NUM_47
#define PIN_CAN_RX GPIO_NUM_38

#define PIN_UART_TX 18
#define PIN_UART_RX 17

#elif defined(BOARD_ESP32C3_DEV)
#define PIN_CAN_TX GPIO_NUM_5
#define PIN_CAN_RX GPIO_NUM_4

#define PIN_UART_TX 3
#define PIN_UART_RX 2

#elif defined(BOARD_BLUEMARK_DB200)
#define PIN_CAN_TX GPIO_NUM_5
#define PIN_CAN_RX GPIO_NUM_4

#define PIN_UART_TX 3
#define PIN_UART_RX 2

#elif defined(BOARD_BLUEMARK_DB110)
#define PIN_UART_TX 5
#define PIN_UART_RX 4

#else
#error "unsupported board"
#endif
