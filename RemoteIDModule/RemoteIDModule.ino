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

#include "mavlink.h"
#include "DroneCAN.h"
#include "WiFi_TX.h"
#include "BLE_TX.h"

static DroneCAN dronecan;
static MAVLinkSerial mavlink{Serial1, MAVLINK_COMM_0};
static WiFi_NAN wifi;
static BLE_TX ble;

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

    mavlink.init();
    dronecan.init();
    wifi.init();
    ble.init();
}

#define MIN(x,y) ((x)<(y)?(x):(y))
#define ODID_COPY_STR(to, from) strncpy(to, (const char*)from, MIN(sizeof(to), sizeof(from)))

static void set_data_mavlink(void)
{
    const auto &operator_id = mavlink.get_operator_id();
    const auto &basic_id = mavlink.get_basic_id();
    const auto &system = mavlink.get_system();
    const auto &self_id = mavlink.get_self_id();
    const auto &location = mavlink.get_location();

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

    // Location
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

#undef ODID_COPY_STR
#define ODID_COPY_STR(to, from) memcpy(to, from.data, MIN(from.len, sizeof(to)))

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

    // Location
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

static uint8_t beacon_payload[256];

void loop()
{
    static uint32_t last_update;

    mavlink.update();
    dronecan.update();

    const uint32_t now_ms = millis();

    if (now_ms - last_update < 1000/OUTPUT_RATE_HZ) {
        // not ready for a new frame yet
        return;
    }

    const bool mavlink_ok = mavlink.location_valid() && mavlink.system_valid();
    const bool dronecan_ok = dronecan.location_valid() && dronecan.system_valid();
    if (!mavlink_ok && !dronecan_ok) {
        return;
    }

    if (mavlink_ok) {
        set_data_mavlink();
    }
    if (dronecan_ok) {
        set_data_dronecan();
    }

    last_update = now_ms;

    const auto length = odid_message_build_pack(&UAS_data,beacon_payload,255);

    wifi.transmit(UAS_data);
    ble.transmit(UAS_data);
}
