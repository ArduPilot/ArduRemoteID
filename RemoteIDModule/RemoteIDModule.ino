/*
  implement OpenDroneID MAVLink support, populating squitter for sending bluetooth RemoteID messages

  based on example code from https://github.com/sxjack/uav_electronic_ids
 */
/*
  released under GNU GPL v3 or later
 */

#include <Arduino.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include <id_open.h>
#include "mavlink.h"

static ID_OpenDrone          squitter;

static MAVLinkSerial mavlink{Serial1, MAVLINK_COMM_0};

static bool squitter_initialised;

#define OUTPUT_RATE_HZ 5

/*
  assume ESP32-s3 for now, using pins 17 and 18 for uart
 */
#define RX_PIN 17
#define TX_PIN 18
#define DEBUG_BAUDRATE 57600
#define MAVLINK_BAUDRATE 57600

/*
  setup serial ports
 */
void setup()
{
    // Serial for debug printf
    Serial.begin(DEBUG_BAUDRATE);

    // Serial1 for MAVLink
    Serial1.begin(MAVLINK_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);

    mavlink.init();
}

static void init_squitter(void)
{
    struct UTM_parameters utm_parameters {};

    const auto &operator_id = mavlink.get_operator_id();
    const auto &basic_id = mavlink.get_basic_id();
    const auto &system = mavlink.get_system();
    const auto &self_id = mavlink.get_self_id();

    strncpy((char*)utm_parameters.UAS_operator, (char *)operator_id.operator_id, sizeof(utm_parameters.UAS_operator));
    strncpy((char*)utm_parameters.UAV_id, (char *)basic_id.uas_id, sizeof(utm_parameters.UAV_id));
    strncpy((char*)utm_parameters.flight_desc, (char *)self_id.description, sizeof(utm_parameters.flight_desc));
    utm_parameters.UA_type = basic_id.ua_type;
    utm_parameters.ID_type = basic_id.id_type;
    utm_parameters.region = 1; // ??
    utm_parameters.EU_category = system.category_eu;
    utm_parameters.EU_class = system.class_eu;
    // char    UTM_id[ID_SIZE * 2] ??

    squitter.init(&utm_parameters);
}

void loop()
{
    static uint32_t last_update;

    mavlink.update();

    if (!mavlink.initialised()) {
        return;
    }

    if (!squitter_initialised) {
        squitter_initialised = true;
        init_squitter();
    }

    const uint32_t msecs = millis();

    if (msecs - last_update < 1000/OUTPUT_RATE_HZ) {
        // not ready for a new frame yet
        return;
    }
    last_update = msecs;

    const auto &location = mavlink.get_location();
    const auto &operator_id = mavlink.get_operator_id();
    const auto &system = mavlink.get_system();

    struct UTM_data utm_data {};

    const float M_PER_SEC_TO_KNOTS = 1.94384449;

    utm_data.heading     = location.direction * 0.01;
    utm_data.latitude_d  = location.latitude * 1.0e-7;
    utm_data.longitude_d = location.longitude * 1.0e-7;
    utm_data.base_latitude = system.operator_latitude * 1.0e-7;
    utm_data.base_longitude = system.operator_longitude * 1.0e-7;
    utm_data.base_alt_m     = system.operator_altitude_geo;
    utm_data.alt_msl_m = location.altitude_geodetic;
    utm_data.alt_agl_m = location.height;
    utm_data.speed_kn  = location.speed_horizontal * 0.01 * M_PER_SEC_TO_KNOTS;
    utm_data.base_valid = (system.operator_latitude != 0 && system.operator_longitude != 0);

    const float groundspeed = location.speed_horizontal * 0.01;
    const float vel_N = cos(radians(utm_data.heading)) * groundspeed;
    const float vel_E = sin(radians(utm_data.heading)) * groundspeed;

    utm_data.vel_N_cm = vel_N * 100;
    utm_data.vel_E_cm = vel_E * 100;
    utm_data.vel_D_cm = location.speed_vertical * -0.01;

    const uint32_t jan_1_2019_s = 1546261200UL;
    const time_t unix_s = time_t(system.timestamp) + jan_1_2019_s;
    const auto *tm = gmtime(&unix_s);

    utm_data.years = tm->tm_year;
    utm_data.months = tm->tm_mon;
    utm_data.days = tm->tm_mday;
    utm_data.hours = tm->tm_hour;
    utm_data.minutes = tm->tm_min;
    utm_data.seconds = tm->tm_sec;

    utm_data.satellites = 8;

    //char  *hdop_s;
    //char  *vdop_s;

    squitter.transmit(&utm_data);
}
