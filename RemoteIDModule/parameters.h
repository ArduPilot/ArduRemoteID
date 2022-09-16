#pragma once

#include <stdint.h>

#define MAX_PUBLIC_KEYS 5
#define PUBLIC_KEY_LEN 32
#define PARAM_NAME_MAX_LEN 16

#define PARAM_FLAG_NONE 0
#define PARAM_FLAG_PASSWORD (1U<<0)
#define PARAM_FLAG_HIDDEN (1U<<1)

class Parameters {
public:
    uint8_t lock_level;
    uint8_t can_node;
    uint8_t bcast_powerup;
    uint32_t baudrate = 57600;
    uint8_t ua_type;
    uint8_t id_type;
    char uas_id[21] = "ABCD123456789";
    float wifi_nan_rate;
    float wifi_power;
    float bt4_rate;
    float bt4_power;
    float bt5_rate;
    float bt5_power;
    uint8_t done_init;
    uint8_t webserver_enable;
    char wifi_ssid[21] = "RID_123456";
    char wifi_password[21] = "penguin1234";
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
        void set_uint32(uint32_t v) const;
        void set_char20(const char *v) const;
        void set_char64(const char *v) const;
        uint8_t get_uint8() const;
        uint32_t get_uint32() const;
        float get_float() const;
        const char *get_char20() const;
        const char *get_char64() const;
    };
    static const struct Param params[];

    static const Param *find(const char *name);
    static const Param *find_by_index(uint16_t idx);

    void init(void);

    bool have_basic_id_info(void) const;

    bool set_by_name_uint8(const char *name, uint8_t v);
    bool set_by_name_char64(const char *name, const char *s);
    bool set_by_name_string(const char *name, const char *s);

    /*
      return a public key
    */
    bool get_public_key(uint8_t i, uint8_t key[32]) const;
    bool no_public_keys(void) const;

private:
    void load_defaults(void);
};

extern Parameters g;
