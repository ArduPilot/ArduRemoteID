/*
  mavlink class for handling SECURE_COMMAND messages
 */
#include <Arduino.h>
#include "mavlink.h"
#include "board_config.h"
#include "version.h"
#include "parameters.h"

/*
  handle a SECURE_COMMAND
 */
void MAVLinkSerial::handle_secure_command(const mavlink_secure_command_t &pkt)
{
    mavlink_secure_command_reply_t reply {};
    reply.result = MAV_RESULT_UNSUPPORTED;
    reply.sequence = pkt.sequence;
    reply.operation = pkt.operation;

    if (uint16_t(pkt.data_length) + uint16_t(pkt.sig_length) > sizeof(pkt.data)) {
        reply.result = MAV_RESULT_DENIED;
        goto send_reply;
    }
    if (!check_signature(pkt.sig_length, pkt.data_length, pkt.sequence, pkt.operation, pkt.data)) {
        reply.result = MAV_RESULT_DENIED;
        goto send_reply;
    }

    switch (pkt.operation) {

    case SECURE_COMMAND_GET_SESSION_KEY:
    case SECURE_COMMAND_GET_REMOTEID_SESSION_KEY: {
        make_session_key(session_key);
        reply.data_length = sizeof(session_key);
        memcpy(reply.data, session_key, reply.data_length);
        reply.result = MAV_RESULT_ACCEPTED;
        break;
    }

    case SECURE_COMMAND_GET_PUBLIC_KEYS: {
        if (pkt.data_length != 2) {
            reply.result = MAV_RESULT_UNSUPPORTED;
            goto send_reply;
        }
        const uint8_t key_idx = pkt.data[0];
        uint8_t num_keys = pkt.data[1];
        const uint8_t max_fetch = (sizeof(reply.data)-1) / PUBLIC_KEY_LEN;
        if (key_idx >= MAX_PUBLIC_KEYS ||
            num_keys > max_fetch ||
            key_idx+num_keys > MAX_PUBLIC_KEYS ||
            g.no_public_keys()) {
            reply.result = MAV_RESULT_FAILED;
            goto send_reply;
        }

        for (uint8_t i=0;i<num_keys;i++) {
            g.get_public_key(i+key_idx, &reply.data[1+i*PUBLIC_KEY_LEN]);
        }

        reply.data_length = 1+num_keys*PUBLIC_KEY_LEN;
        reply.data[0] = key_idx;
        reply.result = MAV_RESULT_ACCEPTED;
        break;
    }

    case SECURE_COMMAND_SET_PUBLIC_KEYS: {
        if (pkt.data_length < PUBLIC_KEY_LEN+1) {
            reply.result = MAV_RESULT_FAILED;
            goto send_reply;
        }
        const uint8_t key_idx = pkt.data[0];
        const uint8_t num_keys = (pkt.data_length-1) / PUBLIC_KEY_LEN;
        if (num_keys == 0) {
            reply.result = MAV_RESULT_FAILED;
            goto send_reply;
        }
        if (key_idx >= MAX_PUBLIC_KEYS ||
            key_idx+num_keys > MAX_PUBLIC_KEYS) {
            reply.result = MAV_RESULT_FAILED;
            goto send_reply;
        }
        bool failed = false;
        for (uint8_t i=0; i<num_keys; i++) {
            failed |= !g.set_public_key(key_idx+i, &pkt.data[1+i*PUBLIC_KEY_LEN]);
        }
        reply.result = failed? MAV_RESULT_FAILED : MAV_RESULT_ACCEPTED;
        break;
    }

    case SECURE_COMMAND_REMOVE_PUBLIC_KEYS: {
        if (pkt.data_length != 2) {
            reply.result = MAV_RESULT_FAILED;
            goto send_reply;
        }
        const uint8_t key_idx = pkt.data[0];
        const uint8_t num_keys = pkt.data[1];
        if (num_keys == 0) {
            reply.result = MAV_RESULT_FAILED;
            goto send_reply;
        }
        if (key_idx >= MAX_PUBLIC_KEYS ||
            key_idx+num_keys > MAX_PUBLIC_KEYS) {
            reply.result = MAV_RESULT_FAILED;
            goto send_reply;
        }
        for (uint8_t i=0; i<num_keys; i++) {
            g.remove_public_key(key_idx+i);
        }
        reply.result = MAV_RESULT_ACCEPTED;
        break;
    }
    case SECURE_COMMAND_SET_REMOTEID_CONFIG: {
        int16_t data_len = pkt.data_length;
        char data[pkt.data_length+1];
        memcpy(data, pkt.data, pkt.data_length);
        data[pkt.data_length] = 0;
        /*
          command buffer is nul separated set of NAME=VALUE pairs
         */
        reply.result = MAV_RESULT_ACCEPTED;
        char *command = (char *)data;
        while (data_len > 0) {
            uint8_t cmdlen = strlen(command);
            char *eq = strchr(command, '=');
            if (eq != nullptr) {
                *eq = 0;
                if (!g.set_by_name_string(command, eq+1)) {
                    mav_printf(MAV_SEVERITY_INFO, "set %s failed", command);
                    reply.result = MAV_RESULT_FAILED;
                } else {
                    mav_printf(MAV_SEVERITY_INFO, "set %s OK", command);
                }
            }
            command += cmdlen+1;
            data_len -= cmdlen+1;
        }
        break;
    }
    }

send_reply:
    // send reply
    mavlink_msg_secure_command_reply_send_struct(chan, &reply);
}
