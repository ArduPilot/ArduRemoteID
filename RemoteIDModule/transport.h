/*
  parent class for handling transports
 */
#pragma once

#include "mavlink_msgs.h"

/*
  abstraction for opendroneid transports
 */
class Transport {
public:
    Transport();
    virtual void init(void) = 0;
    virtual void update(void) = 0;
    uint8_t arm_status_check(const char *&reason);

    const mavlink_open_drone_id_location_t &get_location(void) const {
        return location;
    }

    const mavlink_open_drone_id_basic_id_t &get_basic_id(void) const {
        return basic_id;
    }

    const mavlink_open_drone_id_authentication_t &get_authentication(void) const {
        return authentication;
    }

    const mavlink_open_drone_id_self_id_t &get_self_id(void) const {
        return self_id;
    }

    const mavlink_open_drone_id_system_t &get_system(void) const {
        return system;
    }

    const mavlink_open_drone_id_operator_id_t &get_operator_id(void) const {
        return operator_id;
    }

    uint32_t get_last_location_ms(void) const {
        return last_location_ms;
    }

    uint32_t get_last_system_ms(void) const {
        return last_system_ms;
    }
    
    void set_parse_fail(const char *msg) {
        parse_fail = msg;
    }

    const char *get_parse_fail(void) {
        return parse_fail;
    }
    
protected:
    // common variables between transports. The last message of each
    // type, no matter what transport it was on, wins
    static const char *parse_fail;

    static uint32_t last_location_ms;
    static uint32_t last_basic_id_ms;
    static uint32_t last_self_id_ms;
    static uint32_t last_operator_id_ms;
    static uint32_t last_system_ms;
    static uint32_t last_system_timestamp;
    static float last_location_timestamp;

    static mavlink_open_drone_id_location_t location;
    static mavlink_open_drone_id_basic_id_t basic_id;
    static mavlink_open_drone_id_authentication_t authentication;
    static mavlink_open_drone_id_self_id_t self_id;
    static mavlink_open_drone_id_system_t system;
    static mavlink_open_drone_id_operator_id_t operator_id;

    void make_session_key(uint8_t key[8]) const;

    /*
      check signature in a command against public keys
    */
    bool check_signature(uint8_t sig_length, uint8_t data_len, uint32_t sequence, uint32_t operation,
                         const uint8_t *data);

    uint8_t session_key[8];
};
