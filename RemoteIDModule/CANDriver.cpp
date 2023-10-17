/*
  CAN driver class for ESP32
 */
#include <Arduino.h>
#include "options.h"

#if AP_DRONECAN_ENABLED

#include "CANDriver.h"

#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "board_config.h"

#define CAN1_TX_IRQ_Handler      ESP32_CAN1_TX_HANDLER
#define CAN1_RX0_IRQ_Handler     ESP32_CAN1_RX0_HANDLER
#define CAN1_RX1_IRQ_Handler     ESP32_CAN1_RX1_HANDLER
#define CAN2_TX_IRQ_Handler      ESP32_CAN2_TX_HANDLER
#define CAN2_RX0_IRQ_Handler     ESP32_CAN2_RX0_HANDLER
#define CAN2_RX1_IRQ_Handler     ESP32_CAN2_RX1_HANDLER

// from canard.h
#define CANARD_CAN_FRAME_EFF                        (1UL << 31U)         ///< Extended frame format
#define CANARD_CAN_FRAME_RTR                        (1UL << 30U)         ///< Remote transmission (not used by UAVCAN)
#define CANARD_CAN_FRAME_ERR                        (1UL << 29U)         ///< Error frame (not used by UAVCAN)

// constructor
CANDriver::CANDriver()
{}

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
static twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

void CANDriver::init(uint32_t bitrate, uint32_t acceptance_code, uint32_t acceptance_mask)
{
    f_config.acceptance_code = acceptance_code;
    f_config.acceptance_mask = acceptance_mask;
    f_config.single_filter = true;
    init_bus(bitrate);
}

static const twai_general_config_t g_config =                      {.mode = TWAI_MODE_NORMAL, .tx_io = PIN_CAN_TX, .rx_io = PIN_CAN_RX, \
                                                                    .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,      \
                                                                    .tx_queue_len = 5, .rx_queue_len = 50,                           \
                                                                    .alerts_enabled = TWAI_ALERT_NONE,  .clkout_divider = 0,        \
                                                                    .intr_flags = ESP_INTR_FLAG_LEVEL2
                                                                   };

void CANDriver::init_once(bool enable_irq)
{
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        Serial.printf("CAN/TWAI Driver installed\n");
    }
    else
    {
        Serial.printf("Failed to install CAN/TWAI driver\n");
        return;
    }

    //Reconfigure alerts to detect rx-related stuff only...
    uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_RX_QUEUE_FULL;
    if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
        Serial.printf("CAN/TWAI Alerts reconfigured\n");
    } else {
        Serial.printf("Failed to reconfigure CAN/TWAI alerts");
    }

    //Start TWAI driver
    if (twai_start() == ESP_OK)
    {
        Serial.printf("CAN/TWAI Driver started\n");
    }
    else
    {
        Serial.printf("Failed to start CAN/TWAI driver\n");
        return;
    }
}

bool CANDriver::init_bus(const uint32_t _bitrate)
{
    bitrate = _bitrate;
    init_once(true);

    Timings timings;
    if (!computeTimings(bitrate, timings)) {
        return false;
    }
    Serial.printf("Timings: presc=%u sjw=%u bs1=%u bs2=%u",
                  unsigned(timings.prescaler), unsigned(timings.sjw), unsigned(timings.bs1), unsigned(timings.bs2));
    return true;
}

bool CANDriver::computeTimings(uint32_t target_bitrate, Timings& out_timings)
{
    if (target_bitrate < 1) {
        return false;
    }

    /*
     * Hardware configuration
     */
    const uint32_t pclk = 100000;

    static const int MaxBS1 = 16;
    static const int MaxBS2 = 8;

    /*
     * Ref. "Automatic Baudrate Detection in CANopen Networks", U. Koppe, MicroControl GmbH & Co. KG
     *      CAN in Automation, 2003
     *
     * According to the source, optimal quanta per bit are:
     *   Bitrate        Optimal Maximum
     *   1000 kbps      8       10
     *   500  kbps      16      17
     *   250  kbps      16      17
     *   125  kbps      16      17
     */
    const int max_quanta_per_bit = (target_bitrate >= 1000000) ? 10 : 17;

    static const int MaxSamplePointLocation = 900;

    /*
     * Computing (prescaler * BS):
     *   BITRATE = 1 / (PRESCALER * (1 / PCLK) * (1 + BS1 + BS2))       -- See the Reference Manual
     *   BITRATE = PCLK / (PRESCALER * (1 + BS1 + BS2))                 -- Simplified
     * let:
     *   BS = 1 + BS1 + BS2                                             -- Number of time quanta per bit
     *   PRESCALER_BS = PRESCALER * BS
     * ==>
     *   PRESCALER_BS = PCLK / BITRATE
     */
    const uint32_t prescaler_bs = pclk / target_bitrate;

    /*
     * Searching for such prescaler value so that the number of quanta per bit is highest.
     */
    uint8_t bs1_bs2_sum = uint8_t(max_quanta_per_bit - 1);

    while ((prescaler_bs % (1 + bs1_bs2_sum)) != 0) {
        if (bs1_bs2_sum <= 2) {
            return false;          // No solution
        }
        bs1_bs2_sum--;
    }

    const uint32_t prescaler = prescaler_bs / (1 + bs1_bs2_sum);
    if ((prescaler < 1U) || (prescaler > 1024U)) {
        return false;              // No solution
    }

    /*
     * Now we have a constraint: (BS1 + BS2) == bs1_bs2_sum.
     * We need to find the values so that the sample point is as close as possible to the optimal value.
     *
     *   Solve[(1 + bs1)/(1 + bs1 + bs2) == 7/8, bs2]  (* Where 7/8 is 0.875, the recommended sample point location *)
     *   {{bs2 -> (1 + bs1)/7}}
     *
     * Hence:
     *   bs2 = (1 + bs1) / 7
     *   bs1 = (7 * bs1_bs2_sum - 1) / 8
     *
     * Sample point location can be computed as follows:
     *   Sample point location = (1 + bs1) / (1 + bs1 + bs2)
     *
     * Since the optimal solution is so close to the maximum, we prepare two solutions, and then pick the best one:
     *   - With rounding to nearest
     *   - With rounding to zero
     */
    struct BsPair {
        uint8_t bs1;
        uint8_t bs2;
        uint16_t sample_point_permill;

        BsPair() :
            bs1(0),
            bs2(0),
            sample_point_permill(0)
        { }

        BsPair(uint8_t bs1_bs2_sum, uint8_t arg_bs1) :
            bs1(arg_bs1),
            bs2(uint8_t(bs1_bs2_sum - bs1)),
            sample_point_permill(uint16_t(1000 * (1 + bs1) / (1 + bs1 + bs2)))
        {}

        bool isValid() const
        {
            return (bs1 >= 1) && (bs1 <= MaxBS1) && (bs2 >= 1) && (bs2 <= MaxBS2);
        }
    };

    // First attempt with rounding to nearest
    BsPair solution(bs1_bs2_sum, uint8_t(((7 * bs1_bs2_sum - 1) + 4) / 8));

    if (solution.sample_point_permill > MaxSamplePointLocation) {
        // Second attempt with rounding to zero
        solution = BsPair(bs1_bs2_sum, uint8_t((7 * bs1_bs2_sum - 1) / 8));
    }

    if ((target_bitrate != (pclk / (prescaler * (1 + solution.bs1 + solution.bs2)))) || !solution.isValid()) {
        return false;
    }

    Serial.printf("Timings: quanta/bit: %d, sample point location: %.1f%%",
                  int(1 + solution.bs1 + solution.bs2), float(solution.sample_point_permill) / 10.F);

    out_timings.prescaler = uint16_t(prescaler - 1U);
    out_timings.sjw = 0;                                        // Which means one
    out_timings.bs1 = uint8_t(solution.bs1 - 1);
    out_timings.bs2 = uint8_t(solution.bs2 - 1);
    return true;
}

bool CANDriver::send(const CANFrame &frame)
{
    if (frame.isErrorFrame() || frame.dlc > 8) {
        return false;
    }

    twai_message_t message {};
    message.identifier = frame.id;
    message.extd = frame.isExtended() ? 1 : 0;
    message.data_length_code = frame.dlc;
    memcpy(message.data, frame.data, 8);

    twai_status_info_t info {};
    twai_get_status_info(&info);
    switch (info.state) {
    case TWAI_STATE_STOPPED:
        twai_start();
        break;
    case TWAI_STATE_RUNNING:
    case TWAI_STATE_RECOVERING:
        break;
    case TWAI_STATE_BUS_OFF: {
        uint32_t now = millis();
        if (now - last_bus_recovery_ms > 2000) {
            last_bus_recovery_ms = now;
            twai_initiate_recovery();
        }
        break;
    }
    }

    const esp_err_t sts = twai_transmit(&message, pdMS_TO_TICKS(5));
    if (sts == ESP_OK) {
        last_bus_recovery_ms = 0;
    }

    return (sts == ESP_OK);
}

bool CANDriver::receive(CANFrame &out_frame)
{
    twai_message_t message {};
    esp_err_t recverr = twai_receive(&message, pdMS_TO_TICKS(5));
    if (recverr != ESP_OK) {
        return false;
    }

    memcpy(out_frame.data, message.data, 8);// copy new data
    out_frame.dlc = message.data_length_code;
    out_frame.id = message.identifier;
    if (message.extd) {
        out_frame.id |= CANARD_CAN_FRAME_EFF;
    }
    if (out_frame.id & CANFrame::FlagERR) { // same as a message.isErrorFrame() if done later.
        return false;
    }
    return true;
}

#endif // AP_DRONECAN_ENABLED
