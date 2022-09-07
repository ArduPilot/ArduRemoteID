#pragma once

#include <stdint.h>

#define MAX_PUBLIC_KEYS 10
#define PUBLIC_KEY_LEN 32
#define PARAM_NAME_MAX_LEN 16

class Parameters {
public:
    uint8_t lock_level;
    uint8_t can_node;
    uint8_t bcast_powerup;
    char uas_id[21] = "ABCD123456789";
    float wifi_nan_rate;
    float bt4_rate;
    float bt5_rate;
    uint8_t webserver_enable;
    char wifi_ssid[21] = "RemoteID_XXXXXXXX";
    char wifi_password[21] = "SecretPassword";

    /*
      header at the front of storage
     */
    struct header {
        struct public_key {
            uint8_t key[PUBLIC_KEY_LEN];
        } public_keys[MAX_PUBLIC_KEYS];
        uint32_t reserved[30];
    };

    enum class ParamType {
        NONE=0,
        UINT8=1,
        FLOAT=2,
        CHAR20=3,
    };

    struct Param {
        char name[PARAM_NAME_MAX_LEN+1];
        ParamType ptype;
        const void *ptr;
        float default_value;
        float min_value;
        float max_value;
        void set(float v) const;
        void set(uint8_t v) const;
        void set(const char *v) const;
        uint8_t get_uint8() const;
        float get_float() const;
        const char *get_char20() const;
    };
    static const struct Param params[];

    static const Param *find(const char *name);
    static const Param *find_by_index(uint16_t idx);

    void init(void);
private:
    void load_defaults(void);
};

extern Parameters g;
