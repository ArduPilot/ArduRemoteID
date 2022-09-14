/*
  implement OpenDroneID MAVLink and DroneCAN support
 */
/*
  released under GNU GPL v2 or later
 */

#include "options.h"
#include <Arduino.h>
#include "version.h"
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <opendroneid.h>
#include "mavlink.h"
#include "DroneCAN.h"
#include "WiFi_TX.h"
#include "BLE_TX.h"
#include "parameters.h"
#include "webinterface.h"
#include "check_firmware.h"
#include <esp_ota_ops.h>

#if AP_DRONECAN_ENABLED
static DroneCAN dronecan;
#endif

#if AP_MAVLINK_ENABLED
static MAVLinkSerial mavlink1{Serial1, MAVLINK_COMM_0};
static MAVLinkSerial mavlink2{Serial,  MAVLINK_COMM_1};
#endif

static WiFi_NAN wifi;
static BLE_TX ble;

#define DEBUG_BAUDRATE 57600

// OpenDroneID output data structure
static ODID_UAS_Data UAS_data;
static uint32_t last_location_ms;
static WebInterface webif;

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

/*
  setup serial ports
 */
void setup()
{
    // disable brownout checking
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    g.init();

    // Serial for debug printf
    Serial.begin(g.baudrate);

    // Serial1 for MAVLink
    Serial1.begin(g.baudrate, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);

    // set all fields to invalid/initial values
    odid_initUasData(&UAS_data);

#if AP_MAVLINK_ENABLED
    mavlink1.init();
    mavlink2.init();
#endif
#if AP_DRONECAN_ENABLED
    dronecan.init();
#endif

    CheckFirmware::check_OTA_running();

#if defined(PIN_CAN_EN)
    // optional CAN enable pin
    pinMode(PIN_CAN_EN, OUTPUT);
    digitalWrite(PIN_CAN_EN, HIGH);
#endif

#if defined(PIN_CAN_nSILENT)
    // disable silent pin
    pinMode(PIN_CAN_nSILENT, OUTPUT);
    digitalWrite(PIN_CAN_nSILENT, HIGH);
#endif

#if defined(PIN_CAN_TERM)
    // optional CAN termination control
    pinMode(PIN_CAN_TERM, OUTPUT);
    digitalWrite(PIN_CAN_TERM, HIGH);
#endif

    esp_log_level_set("*", ESP_LOG_DEBUG);

    esp_ota_mark_app_valid_cancel_rollback();
}

#define IMIN(x,y) ((x)<(y)?(x):(y))
#define ODID_COPY_STR(to, from) strncpy(to, (const char*)from, IMIN(sizeof(to), sizeof(from)))

/*
  check parsing of UAS_data, this checks ranges of values to ensure we
  will produce a valid pack
 */
static const char *check_parse(void)
{
    {
        ODID_Location_encoded encoded {};
        if (encodeLocationMessage(&encoded, &UAS_data.Location) != ODID_SUCCESS) {
            return "bad LOCATION data";
        }
    }
    {
        ODID_System_encoded encoded {};
        if (encodeSystemMessage(&encoded, &UAS_data.System) != ODID_SUCCESS) {
            return "bad SYSTEM data";
        }
    }
    {
        ODID_BasicID_encoded encoded {};
        if (encodeBasicIDMessage(&encoded, &UAS_data.BasicID[0]) != ODID_SUCCESS) {
            return "bad BASIC_ID data";
        }
    }
    {
        ODID_SelfID_encoded encoded {};
        if (encodeSelfIDMessage(&encoded, &UAS_data.SelfID) != ODID_SUCCESS) {
            return "bad SELF_ID data";
        }
    }
    {
        ODID_OperatorID_encoded encoded {};
        if (encodeOperatorIDMessage(&encoded, &UAS_data.OperatorID) != ODID_SUCCESS) {
            return "bad OPERATOR_ID data";
        }
    }
    return nullptr;
}

/*
  fill in UAS_data from MAVLink packets
 */
static void set_data(Transport &t)
{
    const auto &operator_id = t.get_operator_id();
    const auto &basic_id = t.get_basic_id();
    const auto &system = t.get_system();
    const auto &self_id = t.get_self_id();
    const auto &location = t.get_location();

    // BasicID
    if (g.have_basic_id_info()) {
        // from parameters
        UAS_data.BasicID[0].UAType = (ODID_uatype_t)g.ua_type;
        UAS_data.BasicID[0].IDType = (ODID_idtype_t)g.id_type;
        ODID_COPY_STR(UAS_data.BasicID[0].UASID, g.uas_id);
    } else {
        // from transport
        UAS_data.BasicID[0].UAType = (ODID_uatype_t)basic_id.ua_type;
        UAS_data.BasicID[0].IDType = (ODID_idtype_t)basic_id.id_type;
        ODID_COPY_STR(UAS_data.BasicID[0].UASID, basic_id.uas_id);
    }
    UAS_data.BasicIDValid[0] = 1;

    // OperatorID
    UAS_data.OperatorID.OperatorIdType = (ODID_operatorIdType_t)operator_id.operator_id_type;
    ODID_COPY_STR(UAS_data.OperatorID.OperatorId, operator_id.operator_id);
    UAS_data.OperatorIDValid = 1;

    // SelfID
    UAS_data.SelfID.DescType = (ODID_desctype_t)self_id.description_type;
    ODID_COPY_STR(UAS_data.SelfID.Desc, self_id.description);
    UAS_data.SelfIDValid = 1;

    // System
    if (system.timestamp != 0) {
        UAS_data.System.OperatorLocationType = (ODID_operator_location_type_t)system.operator_location_type;
        UAS_data.System.ClassificationType = (ODID_classification_type_t)system.classification_type;
        UAS_data.System.OperatorLatitude = system.operator_latitude * 1.0e-7;
        UAS_data.System.OperatorLongitude = system.operator_longitude * 1.0e-7;
        UAS_data.System.AreaCount = system.area_count;
        UAS_data.System.AreaRadius = system.area_radius;
        UAS_data.System.AreaCeiling = system.area_ceiling;
        UAS_data.System.AreaFloor = system.area_floor;
        UAS_data.System.CategoryEU = (ODID_category_EU_t)system.category_eu;
        UAS_data.System.ClassEU = (ODID_class_EU_t)system.class_eu;
        UAS_data.System.OperatorAltitudeGeo = system.operator_altitude_geo;
        UAS_data.System.Timestamp = system.timestamp;
        UAS_data.SystemValid = 1;
    }

    // Location
    if (location.timestamp != 0) {
        UAS_data.Location.Status = (ODID_status_t)location.status;
        UAS_data.Location.Direction = location.direction*0.01;
        UAS_data.Location.SpeedHorizontal = location.speed_horizontal*0.01;
        UAS_data.Location.SpeedVertical = location.speed_vertical*0.01;
        UAS_data.Location.Latitude = location.latitude*1.0e-7;
        UAS_data.Location.Longitude = location.longitude*1.0e-7;
        UAS_data.Location.AltitudeBaro = location.altitude_barometric;
        UAS_data.Location.AltitudeGeo = location.altitude_geodetic;
        UAS_data.Location.HeightType = (ODID_Height_reference_t)location.height_reference;
        UAS_data.Location.Height = location.height;
        UAS_data.Location.HorizAccuracy = (ODID_Horizontal_accuracy_t)location.horizontal_accuracy;
        UAS_data.Location.VertAccuracy = (ODID_Vertical_accuracy_t)location.vertical_accuracy;
        UAS_data.Location.BaroAccuracy = (ODID_Vertical_accuracy_t)location.barometer_accuracy;
        UAS_data.Location.SpeedAccuracy = (ODID_Speed_accuracy_t)location.speed_accuracy;
        UAS_data.Location.TSAccuracy = (ODID_Timestamp_accuracy_t)location.timestamp_accuracy;
        UAS_data.Location.TimeStamp = location.timestamp;
        UAS_data.LocationValid = 1;
    }

    const char *reason = check_parse();
    if (reason == nullptr) {
        t.arm_status_check(reason);
    }
    t.set_parse_fail(reason);

#ifdef PIN_STATUS_LED
    // LED off if good to arm
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, reason==nullptr?!STATUS_LED_ON:STATUS_LED_ON);
#endif

    uint32_t now_ms = millis();
    uint32_t location_age_ms = now_ms - t.get_last_location_ms();
    uint32_t last_location_age_ms = now_ms - last_location_ms;
    if (location_age_ms < last_location_age_ms) {
        last_location_ms = t.get_last_location_ms();
    }
}

static uint8_t loop_counter = 0;

void loop()
{
#if AP_MAVLINK_ENABLED
    mavlink1.update();
    mavlink2.update();
#endif
#if AP_DRONECAN_ENABLED
    dronecan.update();
#endif

    const uint32_t now_ms = millis();

    if (g.webserver_enable) {
        webif.update();
    }

    // the transports have common static data, so we can just use the
    // first for status
#if AP_MAVLINK_ENABLED
    auto &transport = mavlink1;
#elif AP_DRONECAN_ENABLED
    auto &transport = dronecan;
#else
    #error "Must enable DroneCAN or MAVLink"
#endif

    bool have_location = false;
    const uint32_t last_location_ms = transport.get_last_location_ms();
    const uint32_t last_system_ms = transport.get_last_system_ms();

    if (g.bcast_powerup) {
        // if we are broadcasting on powerup we always mark location valid
        // so the location with default data is sent
        if (!UAS_data.LocationValid) {
            UAS_data.Location.Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE;
            UAS_data.LocationValid = 1;
        }
    } else {
        // only broadcast if we have received a location at least once
        if (last_location_ms == 0) {
            delay(1);
            return;
        }
    }

    if (last_location_ms == 0 ||
        now_ms - last_location_ms > 5000) {
        UAS_data.Location.Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE;
    }

    if (last_system_ms == 0 ||
        now_ms - last_system_ms > 5000) {
        UAS_data.Location.Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE;
    }

    if (transport.get_parse_fail() != nullptr) {
        UAS_data.Location.Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE;
    }
    
    set_data(transport);

    static uint32_t last_update_wifi_ms;
    if (g.wifi_nan_rate > 0 &&
        now_ms - last_update_wifi_ms > 1000/g.wifi_nan_rate) {
        last_update_wifi_ms = now_ms;
        wifi.transmit(UAS_data);
    }

    static uint32_t last_update_bt5_ms;
    if (g.bt5_rate > 0 &&
        now_ms - last_update_bt5_ms > 1000/g.bt5_rate) {
        last_update_bt5_ms = now_ms;
        ble.transmit_longrange(UAS_data);
    }

    static uint32_t last_update_bt4_ms;
    if (g.bt4_rate > 0 &&
        now_ms - last_update_bt4_ms > 200/g.bt4_rate) {
        last_update_bt4_ms = now_ms;
        ble.transmit_legacy(UAS_data);
        ble.transmit_legacy_name(UAS_data);
    }

    // sleep for a bit for power saving
    delay(1);
}
