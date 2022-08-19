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

#define PIN_CAN_TX GPIO_NUM_0 // this goes to the TX pin (= input) of the NXP CAN transceiver
#define PIN_CAN_RX GPIO_NUM_5
#define PIN_CAN_EN GPIO_NUM_4 // EN pin of the NXP CAN transceiver
#define PIN_CAN_nSILENT GPIO_NUM_1 // CAN silent pin, active low

// to enable termination resistors uncomment this line
//#define PIN_CAN_TERM EN_NUM_10 // if set to ON, the termination resistors of the CAN bus are enabled

#define PIN_UART_TX 3
#define PIN_UART_RX 2

#define PIN_STATUS_LED GPIO_NUM_8 // LED to signal the status
#define STATUS_LED_ON 1

#elif defined(BOARD_BLUEMARK_DB110)
#define PIN_UART_TX 5
#define PIN_UART_RX 4

#else
#error "unsupported board"
#endif
