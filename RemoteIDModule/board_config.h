/*
  board configuration file
 */

#pragma once

#ifdef BOARD_ESP32S3_DEV
#define BOARD_ID 1
#define PIN_CAN_TX GPIO_NUM_47
#define PIN_CAN_RX GPIO_NUM_38

#define PIN_UART_TX 18
#define PIN_UART_RX 17

#define WS2812_LED_PIN GPIO_NUM_48

#elif defined(BOARD_ESP32C3_DEV)
#define BOARD_ID 2
#define PIN_CAN_TX GPIO_NUM_5
#define PIN_CAN_RX GPIO_NUM_4

#define PIN_UART_TX 3
#define PIN_UART_RX 2

#define WS2812_LED_PIN GPIO_NUM_8

#elif defined(BOARD_BLUEMARK_DB200)
#define BOARD_ID 3

#define PIN_CAN_TX GPIO_NUM_0 // this goes to the TX pin (= input) of the NXP CAN transceiver
#define PIN_CAN_RX GPIO_NUM_5
#define PIN_CAN_EN GPIO_NUM_4 // EN pin of the NXP CAN transceiver
#define PIN_CAN_nSILENT GPIO_NUM_1 // CAN silent pin, active low

#define CAN_APP_NODE_NAME "BlueMark DB200"

// to enable termination resistors uncomment this line
//#define PIN_CAN_TERM GPIO_NUM_10 // if set to ON, the termination resistors of the CAN bus are enabled

#define PIN_UART_TX 3
#define PIN_UART_RX 2

#define PIN_STATUS_LED GPIO_NUM_8 // LED to signal the status
// LED off when ready to arm
#define STATUS_LED_OK 0

#elif defined(BOARD_BLUEMARK_DB110)
#define BOARD_ID 4
#define PIN_UART_TX 5
#define PIN_UART_RX 4

#define PIN_STATUS_LED GPIO_NUM_8 // LED to signal the status
// LED off when ready to arm
#define STATUS_LED_OK 0

#elif defined(BOARD_JW_TBD)
#define BOARD_ID 5
#define PIN_CAN_TX GPIO_NUM_47
#define PIN_CAN_RX GPIO_NUM_38

#define PIN_UART_TX 18
#define PIN_UART_RX 17

#define PIN_STATUS_LED GPIO_NUM_5 // LED to signal the status
#define LED_MODE_FLASH 1 // flashing LED configuration
// LED on when ready to arm
#define STATUS_LED_OK 1

#define CAN_APP_NODE_NAME "JW TBD"

#elif defined(BOARD_MRO_RID)
#define BOARD_ID 6
#define PIN_CAN_TX GPIO_NUM_0
#define PIN_CAN_RX GPIO_NUM_1

#define PIN_UART_TX 4
#define PIN_UART_RX 5

#define WS2812_LED_PIN GPIO_NUM_2
#define CAN_APP_NODE_NAME "mRobotics RemoteID"

#elif defined(BOARD_JWRID_ESP32S3)
#define BOARD_ID 7
#define PIN_CAN_TX GPIO_NUM_47
#define PIN_CAN_RX GPIO_NUM_38

#define PIN_UART_TX 37
#define PIN_UART_RX 36

#define WS2812_LED_PIN GPIO_NUM_5

#define CAN_APP_NODE_NAME "JWRID_ESP32S3"

#elif defined(BOARD_BLUEMARK_DB202)
#define BOARD_ID 8

#define PIN_UART_TX 7
#define PIN_UART_RX 6

#define PIN_STATUS_LED GPIO_NUM_8 // LED to signal the status
// LED off when ready to arm
#define STATUS_LED_OK 0

#else
#error "unsupported board"
#endif
