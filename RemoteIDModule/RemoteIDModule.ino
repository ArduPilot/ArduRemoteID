/*
  implement OpenDroneID MAVLink and DroneCAN support
 */
/*
  released under GNU GPL v3 or later
 */

#include <Arduino.h>
#include "version.h"
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <opendroneid.h>

#include "options.h"
#include "mavlink.h"
#include "DroneCAN.h"
#include "WiFi_TX.h"
#include "BLE_TX.h"

#if AP_DRONECAN_ENABLED
static DroneCAN dronecan;
#endif

#if AP_MAVLINK_ENABLED
static MAVLinkSerial mavlink1{Serial1, MAVLINK_COMM_0};
static MAVLinkSerial mavlink2{Serial, MAVLINK_COMM_1};
#endif

#if AP_WIFI_NAN_ENABLED
static WiFi_NAN wifi;
#endif

#if AP_BLE_ENABLED
static BLE_TX ble;
#endif

#define OUTPUT_RATE_HZ 5

/*
  assume ESP32-s3 for now, using pins 17 and 18 for uart
 */
#define RX_PIN 17
#define TX_PIN 18
#define DEBUG_BAUDRATE 57600
#define MAVLINK_BAUDRATE 57600

// OpenDroneID output data structure
static ODID_UAS_Data UAS_data;
static uint32_t last_location_ms;

/*
  setup serial ports
 */
void setup()
{
    // Serial for debug printf
    Serial.begin(DEBUG_BAUDRATE);

    Serial.printf("ArduRemoteID version %u.%u %08x\n",
                  FW_VERSION_MAJOR, FW_VERSION_MINOR, GIT_VERSION);

    // Serial1 for MAVLink
    Serial1.begin(MAVLINK_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);

    // set all fields to invalid/initial values
    odid_initUasData(&UAS_data);

#if AP_MAVLINK_ENABLED
    mavlink1.init();
    mavlink2.init();
#endif
#if AP_DRONECAN_ENABLED
    dronecan.init();
#endif
#if AP_WIFI_NAN_ENABLED
    wifi.init();
#endif
#if AP_BLE_ENABLED
    ble.init();
#endif
}

#define MIN(x,y) ((x)<(y)?(x):(y))
#define ODID_COPY_STR(to, from) strncpy(to, (const char*)from, MIN(sizeof(to), sizeof(from)))

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

static void set_data_mavlink(MAVLinkSerial &m)
{
    const auto &operator_id = m.get_operator_id();
    const auto &basic_id = m.get_basic_id();
    const auto &system = m.get_system();
    const auto &self_id = m.get_self_id();
    const auto &location = m.get_location();

    // BasicID
    UAS_data.BasicID[0].UAType = (ODID_uatype_t)basic_id.ua_type;
    UAS_data.BasicID[0].IDType = (ODID_idtype_t)basic_id.id_type;
    ODID_COPY_STR(UAS_data.BasicID[0].UASID, basic_id.uas_id);
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

    m.set_parse_fail(check_parse());

    uint32_t now_ms = millis();
    uint32_t location_age_ms = now_ms - m.get_last_location_ms();
    uint32_t last_location_age_ms = now_ms - last_location_ms;
    if (location_age_ms < last_location_age_ms) {
        last_location_ms = m.get_last_location_ms();
    }
}

#undef ODID_COPY_STR
#define ODID_COPY_STR(to, from) memcpy(to, from.data, MIN(from.len, sizeof(to)))

#if AP_DRONECAN_ENABLED
static void set_data_dronecan(void)
{
    const auto &operator_id = dronecan.get_operator_id();
    const auto &basic_id = dronecan.get_basic_id();
    const auto &system = dronecan.get_system();
    const auto &self_id = dronecan.get_self_id();
    const auto &location = dronecan.get_location();

    // BasicID
    UAS_data.BasicID[0].UAType = (ODID_uatype_t)basic_id.ua_type;
    UAS_data.BasicID[0].IDType = (ODID_idtype_t)basic_id.id_type;
    ODID_COPY_STR(UAS_data.BasicID[0].UASID, basic_id.uas_id);
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

    dronecan.set_parse_fail(check_parse());

    uint32_t now_ms = millis();
    uint32_t location_age_ms = now_ms - dronecan.get_last_location_ms();
    uint32_t last_location_age_ms = now_ms - last_location_ms;
    if (location_age_ms < last_location_age_ms) {
        last_location_ms = dronecan.get_last_location_ms();
    }
}
#endif // AP_DRONECAN_ENABLED

void loop()
{
    static uint32_t last_update;

    mavlink1.update();
    mavlink2.update();
#if AP_DRONECAN_ENABLED
    dronecan.update();
#endif

    const uint32_t now_ms = millis();

    if (now_ms - last_update < 1000/OUTPUT_RATE_HZ) {
        // not ready for a new frame yet
        return;
    }

    bool have_location = false;

#if AP_MAVLINK_ENABLED
    const bool mavlink1_ok = mavlink1.get_last_location_ms() != 0;
    const bool mavlink2_ok = mavlink2.get_last_location_ms() != 0;
    have_location |= mavlink1_ok || mavlink2_ok;
#endif

#if AP_DRONECAN_ENABLED
    const bool dronecan_ok = dronecan.get_last_location_ms() != 0;
    have_location |= dronecan_ok;
#endif

#if AP_BROADCAST_ON_POWER_UP
    // if we are broadcasting on powerup we always mark location valid
    // so the location with default data is sent
    if (!UAS_data.LocationValid) {
        UAS_data.Location.Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE;
        UAS_data.LocationValid = 1;
    }
#else
    // only broadcast if we have received a location at least once
    if (!have_location) {
        return;
    }
#endif

    if (now_ms - last_location_ms > 5000) {
        UAS_data.Location.Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE;
    }

#if AP_MAVLINK_ENABLED
    if (mavlink1_ok) {
        set_data_mavlink(mavlink1);
    }
    if (mavlink2_ok) {
        set_data_mavlink(mavlink2);
    }
#endif

#if AP_DRONECAN_ENABLED
    if (dronecan_ok) {
        set_data_dronecan();
    }
#endif

    last_update = now_ms;

#if AP_WIFI_NAN_ENABLED
    wifi.transmit(UAS_data);
#endif
#if AP_BLE_ENABLED
    ble.transmit(UAS_data);
#endif
}
