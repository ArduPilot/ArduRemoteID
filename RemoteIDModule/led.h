#pragma once

#include <stdint.h>
#include "board_config.h"

#ifdef WS2812_LED_PIN
#include <Adafruit_NeoPixel.h>
#endif

class Led {
public:
    enum class LedState {
        INIT=0,
        PFST_FAIL,
        ARM_FAIL,
        ARM_OK
    };

    void set_state(LedState _state) {
        state = _state;
    }
    void update(void);

private:
    void init(void);
    bool done_init;
    uint32_t last_led_trig_ms;
    LedState state;

#ifdef WS2812_LED_PIN
    uint32_t last_led_strip_ms;
    Adafruit_NeoPixel ledStrip{1, WS2812_LED_PIN, NEO_GRB + NEO_KHZ800};
#endif
};

extern Led led;
