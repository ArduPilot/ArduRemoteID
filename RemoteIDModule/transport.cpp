/*
  generic transport class for handling OpenDroneID messages
 */
#include <Arduino.h>
#include "transport.h"
#include "parameters.h"

const char *Transport::parse_fail = "uninitialised";

uint32_t Transport::last_location_ms;
uint32_t Transport::last_basic_id_ms;
uint32_t Transport::last_self_id_ms;
uint32_t Transport::last_operator_id_ms;
uint32_t Transport::last_system_ms;

mavlink_open_drone_id_location_t Transport::location;
mavlink_open_drone_id_basic_id_t Transport::basic_id;
mavlink_open_drone_id_authentication_t Transport::authentication;
mavlink_open_drone_id_self_id_t Transport::self_id;
mavlink_open_drone_id_system_t Transport::system;
mavlink_open_drone_id_operator_id_t Transport::operator_id;

Transport::Transport()
{
}

/*
  check we are OK to arm
 */
uint8_t Transport::arm_status_check(const char *&reason)
{
    const uint32_t max_age_location_ms = 3000;
    const uint32_t max_age_other_ms = 22000;
    const uint32_t now_ms = millis();

    uint8_t status = MAV_ODID_PRE_ARM_FAIL_GENERIC;

    if (last_location_ms == 0 || now_ms - last_location_ms > max_age_location_ms) {
        reason = "missing location message";
    } else if (!g.have_basic_id_info() && (last_basic_id_ms == 0 || now_ms - last_basic_id_ms > max_age_other_ms)) {
        reason = "missing basic_id message";
    } else if (last_self_id_ms == 0  || now_ms - last_self_id_ms > max_age_other_ms) {
        reason = "missing self_id message";
    } else if (last_operator_id_ms == 0 || now_ms - last_operator_id_ms > max_age_other_ms) {
        reason = "missing operator_id message";
    } else if (last_system_ms == 0 || now_ms - last_system_ms > max_age_location_ms) {
        // we use location age limit for system as the operator location needs to come in as fast
        // as the vehicle location for FAA standard
        reason = "missing system message";
    } else if (location.latitude == 0 && location.longitude == 0) {
        reason = "Bad location";
    } else if (system.operator_latitude == 0 && system.operator_longitude == 0) {
        reason = "Bad operator location";
    } else if (reason == nullptr) {
        status = MAV_ODID_GOOD_TO_ARM;
    }

    return status;
}
