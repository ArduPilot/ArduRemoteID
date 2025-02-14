#pragma once

#include <stdint.h>
#include "board_config.h"

#define MAX_PUBLIC_KEYS 5
#define PUBLIC_KEY_LEN 32
#define PARAM_NAME_MAX_LEN 16

#define PARAM_FLAG_NONE 0
#define PARAM_FLAG_PASSWORD (1U<<0)
#define PARAM_FLAG_HIDDEN (1U<<1)

class Parameters {
public:
#if defined(PIN_CAN_TERM)
    uint8_t can_term = !CAN_TERM_EN;
#endif
    int8_t lock_level;
    uint8_t can_node;
    uint8_t bcast_powerup;
    uint32_t baudrate = 57600;
    uint8_t ua_type;
    uint8_t id_type;
    char uas_id[21] = "ABCD123456789";
    uint8_t ua_type_2;
    uint8_t id_type_2;
    char uas_id_2[21] = "ABCD123456789";
    float wifi_nan_rate;
    float wifi_beacon_rate;
    float wifi_power;
    float bt4_rate;
    float bt4_power;
    float bt5_rate;
    float bt5_power;
    uint8_t done_init;
    uint8_t webserver_enable;
    uint8_t mavlink_sysid;
    char wifi_ssid[21] = "";
    char wifi_password[21] = "ArduRemoteID";
    uint8_t wifi_channel = 6;
    uint8_t to_factory_defaults = 0;
    uint8_t options;
    struct {
        char b64_key[64];
    } public_keys[MAX_PUBLIC_KEYS];

    enum class ParamType {
        NONE=0,
        UINT8=1,
        UINT32=2,
        FLOAT=3,
        CHAR20=4,
        CHAR64=5,
        INT8=6,
    };

    struct Param {
        char name[PARAM_NAME_MAX_LEN+1];
        ParamType ptype;
        const void *ptr;
        float default_value;
        float min_value;
        float max_value;
        uint16_t flags;
        uint8_t min_len;
        void set_float(float v) const;
        void set_uint8(uint8_t v) const;
        void set_int8(int8_t v) const;
        void set_uint32(uint32_t v) const;
        void set_char20(const char *v) const;
        void set_char64(const char *v) const;
        uint8_t get_uint8() const;
        int8_t get_int8() const;
        uint32_t get_uint32() const;
        float get_float() const;
        const char *get_char20() const;
        const char *get_char64() const;
        bool get_as_float(float &v) const;
        void set_as_float(float v) const;
    };
    static const struct Param params[];

    static const Param *find(const char *name);
    static const Param *find_by_index(uint16_t idx);
    static const Param *find_by_index_float(uint16_t idx);

    void init(void);

    bool have_basic_id_info(void) const;
    bool have_basic_id_2_info(void) const;

    bool set_by_name_uint8(const char *name, uint8_t v);
    bool set_by_name_int8(const char *name, int8_t v);
    bool set_by_name_char64(const char *name, const char *s);
    bool set_by_name_string(const char *name, const char *s);

    /*
      return a public key
    */
    bool get_public_key(uint8_t i, uint8_t key[32]) const;
    bool set_public_key(uint8_t i, const uint8_t key[32]);
    bool remove_public_key(uint8_t i);
    bool no_public_keys(void) const;

    static uint16_t param_count_float(void);
    static int16_t param_index_float(const Param *p);

private:
    void load_defaults(void);
};

// bits for OPTIONS parameter
#define OPTIONS_FORCE_ARM_OK (1U<<0)
#define OPTIONS_DONT_SAVE_BASIC_ID_TO_PARAMETERS (1U<<1)
#define OPTIONS_PRINT_RID_MAVLINK (1U<<2)

extern Parameters g;
