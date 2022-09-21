/*
  handle status web page
 */
#include "options.h"
#include <Arduino.h>
#include "version.h"
#include <opendroneid.h>
#include "status.h"
#include "util.h"

extern ODID_UAS_Data UAS_data;

typedef struct {
    String name;
    String value;
} json_table_t;

static String escape_string(String s)
{
    s.replace("\"", "");
    return s;
}

/*
  create a json string from a table
 */
static String json_format(const json_table_t *table, uint8_t n)
{
    String s = "{";
    for (uint8_t i=0; i<n; i++) {
        const auto &t = table[i];
        s += "\"" + t.name + "\" : ";
        s += "\"" + escape_string(t.value) + "\"";
        if (i != n-1) {
            s += ",";
        }
    }
    s += "}";
    return s;
}

typedef struct {
    int v;
    String s;
} enum_map_t;

static const enum_map_t enum_uatype[] = {
    { ODID_UATYPE_NONE , "NONE" },
    { ODID_UATYPE_AEROPLANE , "AEROPLANE" },
    { ODID_UATYPE_HELICOPTER_OR_MULTIROTOR , "HELICOPTER_OR_MULTIROTOR" },
    { ODID_UATYPE_GYROPLANE , "GYROPLANE" },
    { ODID_UATYPE_HYBRID_LIFT , "HYBRID_LIFT" },
    { ODID_UATYPE_ORNITHOPTER , "ORNITHOPTER" },
    { ODID_UATYPE_GLIDER , "GLIDER" },
    { ODID_UATYPE_KITE , "KITE" },
    { ODID_UATYPE_FREE_BALLOON , "FREE_BALLOON" },
    { ODID_UATYPE_CAPTIVE_BALLOON , "CAPTIVE_BALLOON" },
    { ODID_UATYPE_AIRSHIP , "AIRSHIP" },
    { ODID_UATYPE_FREE_FALL_PARACHUTE , "FREE_FALL_PARACHUTE" },
    { ODID_UATYPE_ROCKET , "ROCKET" },
    { ODID_UATYPE_TETHERED_POWERED_AIRCRAFT , "TETHERED_POWERED_AIRCRAFT" },
    { ODID_UATYPE_GROUND_OBSTACLE , "GROUND_OBSTACLE" },
    { ODID_UATYPE_OTHER , "OTHER" },
};

static const enum_map_t enum_idtype[] = {
    { ODID_IDTYPE_NONE , "NONE" },
    { ODID_IDTYPE_SERIAL_NUMBER , "SERIAL_NUMBER" },
    { ODID_IDTYPE_CAA_REGISTRATION_ID , "CAA_REGISTRATION_ID" },
    { ODID_IDTYPE_UTM_ASSIGNED_UUID , "UTM_ASSIGNED_UUID" },
    { ODID_IDTYPE_SPECIFIC_SESSION_ID , "SPECIFIC_SESSION_ID" },
};

static const enum_map_t enum_loctype[] = {
    { ODID_OPERATOR_LOCATION_TYPE_TAKEOFF , "TAKEOFF" },
    { ODID_OPERATOR_LOCATION_TYPE_LIVE_GNSS , "LIVE_GNSS" },
    { ODID_OPERATOR_LOCATION_TYPE_FIXED , "FIXED" },
};

static const enum_map_t enum_classif[] = {
    { ODID_CLASSIFICATION_TYPE_UNDECLARED , "UNDECLARED" },
    { ODID_CLASSIFICATION_TYPE_EU , "EU" },
};

static const enum_map_t enum_status[] = {
    { ODID_STATUS_UNDECLARED , "UNDECLARED" },
    { ODID_STATUS_GROUND , "GROUND" },
    { ODID_STATUS_AIRBORNE , "AIRBORNE" },
    { ODID_STATUS_EMERGENCY , "EMERGENCY" },
    { ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE , "REMOTE_ID_SYSTEM_FAILURE" },
};

static const enum_map_t enum_height[] = {
    { ODID_HEIGHT_REF_OVER_TAKEOFF , "OVER_TAKEOFF" },
    { ODID_HEIGHT_REF_OVER_GROUND , "OVER_GROUND" },
};

static const enum_map_t enum_hacc[] = {
    { ODID_HOR_ACC_UNKNOWN , "UNKNOWN" },
    { ODID_HOR_ACC_10NM , "10 nm" },
    { ODID_HOR_ACC_4NM , "4 nm" },
    { ODID_HOR_ACC_2NM , "2 nm" },
    { ODID_HOR_ACC_1NM , "1 nm" },
    { ODID_HOR_ACC_0_5NM , "0.5 nm" },
    { ODID_HOR_ACC_0_3NM , "0.3 nm" },
    { ODID_HOR_ACC_0_1NM , "0.1 nm" },
    { ODID_HOR_ACC_0_05NM , "0.05 nm" },
    { ODID_HOR_ACC_30_METER , "30 m" },
    { ODID_HOR_ACC_10_METER , "10 m" },
    { ODID_HOR_ACC_3_METER , "3 m" },
    { ODID_HOR_ACC_1_METER , "1 m" },
};

static const enum_map_t enum_vacc[] = {
    { ODID_VER_ACC_UNKNOWN , "UNKNOWN" },
    { ODID_VER_ACC_150_METER , "150 m" },
    { ODID_VER_ACC_45_METER , "45 m" },
    { ODID_VER_ACC_25_METER , "25 m" },
    { ODID_VER_ACC_10_METER , "10 m" },
    { ODID_VER_ACC_3_METER , "3 m" },
    { ODID_VER_ACC_1_METER , "1 m" },
};

static const enum_map_t enum_sacc[] = {
    { ODID_SPEED_ACC_UNKNOWN , "UNKNOWN" },
    { ODID_SPEED_ACC_10_METERS_PER_SECOND , "10 m/s" },
    { ODID_SPEED_ACC_3_METERS_PER_SECOND , "3 m/s" },
    { ODID_SPEED_ACC_1_METERS_PER_SECOND , "1 m/s" },
    { ODID_SPEED_ACC_0_3_METERS_PER_SECOND , "0.3 m/s" },
};

static const enum_map_t enum_desctype[] = {
    { ODID_DESC_TYPE_TEXT , "TEXT" },
    { ODID_DESC_TYPE_EMERGENCY , "EMERGENCY" },
    { ODID_DESC_TYPE_EXTENDED_STATUS , "EXTENDED_STATUS" },
};

static const enum_map_t enum_classeu[] = {
    { ODID_CLASS_EU_UNDECLARED , "UNDECLARED" },
    { ODID_CLASS_EU_CLASS_0 , "CLASS_0" },
    { ODID_CLASS_EU_CLASS_1 , "CLASS_1" },
    { ODID_CLASS_EU_CLASS_2 , "CLASS_2" },
    { ODID_CLASS_EU_CLASS_3 , "CLASS_3" },
    { ODID_CLASS_EU_CLASS_4 , "CLASS_4" },
    { ODID_CLASS_EU_CLASS_5 , "CLASS_5" },
    { ODID_CLASS_EU_CLASS_6 , "CLASS_6" },
};

static const enum_map_t enum_cateu[] = {
    { ODID_CATEGORY_EU_UNDECLARED , "UNDECLARED" },
    { ODID_CATEGORY_EU_OPEN , "OPEN" },
    { ODID_CATEGORY_EU_SPECIFIC , "SPECIFIC" },
    { ODID_CATEGORY_EU_CERTIFIED , "CERTIFIED" },
};

static const enum_map_t enum_tsacc[] = {
    { ODID_TIME_ACC_UNKNOWN , "UNKNOWN" },
    { ODID_TIME_ACC_0_1_SECOND , "0.1 s" },
    { ODID_TIME_ACC_0_2_SECOND , "0.2 s" },
    { ODID_TIME_ACC_0_3_SECOND , "0.3 s" },
    { ODID_TIME_ACC_0_4_SECOND , "0.4 s" },
    { ODID_TIME_ACC_0_5_SECOND , "0.5 s" },
    { ODID_TIME_ACC_0_6_SECOND , "0.6 s" },
    { ODID_TIME_ACC_0_7_SECOND , "0.7 s" },
    { ODID_TIME_ACC_0_8_SECOND , "0.8 s" },
    { ODID_TIME_ACC_0_9_SECOND , "0.9 s" },
    { ODID_TIME_ACC_1_0_SECOND , "1.0 s" },
    { ODID_TIME_ACC_1_1_SECOND , "1.1 s" },
    { ODID_TIME_ACC_1_2_SECOND , "1.2 s" },
    { ODID_TIME_ACC_1_3_SECOND , "1.3 s" },
    { ODID_TIME_ACC_1_4_SECOND , "1.4 s" },
    { ODID_TIME_ACC_1_5_SECOND , "1.5 s" },
};

static String enum_string(const enum_map_t *m, uint8_t n, int v)
{
    for (uint8_t i=0; i<n; i++) {
        if (m[i].v == v) {
            return m[i].s;
        }
    }
    return String(v);
}

#define ENUM_MAP(ename, v) enum_string(enum_ ## ename, ARRAY_SIZE(enum_ ## ename), int(v))

String status_json(void)
{
    const uint32_t now_s = millis() / 1000;
    const uint32_t sec = now_s % 60;
    const uint32_t min = (now_s / 60) % 60;
    const uint32_t hr = (now_s / 3600) % 24;
    char minsec_str[6] {};  // HOUR does not include. Because wired powered drones allow for longer flight times.
    snprintf(minsec_str, sizeof(minsec_str), "%02d:%02d", min, sec);
    char githash[20];
    snprintf(githash, sizeof(githash), "(%08x)", GIT_VERSION);
    const json_table_t table[] = {
        { "STATUS:VERSION", String(FW_VERSION_MAJOR) + "." + String(FW_VERSION_MINOR) + " " + githash},
        { "STATUS:UPTIME", String(hr) + ":" + String(minsec_str) },
        { "STATUS:FREEMEM", String(ESP.getFreeHeap()) },
        { "BASICID:UAType", ENUM_MAP(uatype, UAS_data.BasicID[0].UAType) },
        { "BASICID:IDType", ENUM_MAP(idtype, UAS_data.BasicID[0].IDType) },
        { "BASICID:UASID", String(UAS_data.BasicID[0].UASID) },
        { "OPERATORID:IDType", String(UAS_data.OperatorID.OperatorIdType) },
        { "OPERATORID:ID", String(UAS_data.OperatorID.OperatorId) },
        { "SELFID:DescType", ENUM_MAP(desctype, UAS_data.SelfID.DescType) },
        { "SELFID:Desc", String(UAS_data.SelfID.Desc) },
        { "SYSTEM:OperatorLocationType", ENUM_MAP(loctype, UAS_data.System.OperatorLocationType) },
        { "SYSTEM:ClassificationType", ENUM_MAP(classif, UAS_data.System.ClassificationType) },
        { "SYSTEM:OperatorLatitude", String(UAS_data.System.OperatorLatitude, 8) },
        { "SYSTEM:OperatorLongitude", String(UAS_data.System.OperatorLongitude, 8) },
        { "SYSTEM:AreaCount", String(UAS_data.System.AreaCount) },
        { "SYSTEM:AreaRadius", String(UAS_data.System.AreaRadius) },
        { "SYSTEM:AreaCeiling", String(UAS_data.System.AreaCeiling) },
        { "SYSTEM:AreaFloor", String(UAS_data.System.AreaFloor) },
        { "SYSTEM:CategoryEU", ENUM_MAP(cateu, UAS_data.System.CategoryEU) },
        { "SYSTEM:ClassEU", ENUM_MAP(classeu, UAS_data.System.ClassEU) },
        { "SYSTEM:OperatorAltitudeGeo", String(UAS_data.System.OperatorAltitudeGeo) },
        { "SYSTEM:Timestamp", String(UAS_data.System.Timestamp) },
        { "LOCATION:Status", ENUM_MAP(status, UAS_data.Location.Status) },
        { "LOCATION:Direction", String(UAS_data.Location.Direction) },
        { "LOCATION:SpeedHorizontal", String(UAS_data.Location.SpeedHorizontal) },
        { "LOCATION:SpeedVertical", String(UAS_data.Location.SpeedVertical) },
        { "LOCATION:Latitude", String(UAS_data.Location.Latitude, 8) },
        { "LOCATION:Longitude", String(UAS_data.Location.Longitude, 8) },
        { "LOCATION:AltitudeBaro", String(UAS_data.Location.AltitudeBaro) },
        { "LOCATION:AltitudeGeo", String(UAS_data.Location.AltitudeGeo) },
        { "LOCATION:HeightType", ENUM_MAP(height, UAS_data.Location.HeightType) },
        { "LOCATION:Height", String(UAS_data.Location.Height) },
        { "LOCATION:HorizAccuracy", ENUM_MAP(hacc, UAS_data.Location.HorizAccuracy) },
        { "LOCATION:VertAccuracy", ENUM_MAP(vacc, UAS_data.Location.VertAccuracy) },
        { "LOCATION:BaroAccuracy", ENUM_MAP(vacc, UAS_data.Location.BaroAccuracy) },
        { "LOCATION:SpeedAccuracy", ENUM_MAP(sacc, UAS_data.Location.SpeedAccuracy) },
        { "LOCATION:TSAccuracy", ENUM_MAP(tsacc, UAS_data.Location.TSAccuracy) },
        { "LOCATION:TimeStamp", String(UAS_data.Location.TimeStamp) },
    };
    return json_format(table, ARRAY_SIZE(table));
}

