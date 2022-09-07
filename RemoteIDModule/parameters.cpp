#include "options.h"
#include <Arduino.h>
#include "parameters.h"
#include <nvs_flash.h>
#include <string.h>

Parameters g;
static nvs_handle handle;

const Parameters::Param Parameters::params[] = {
    { "LOCK_LEVEL",        Parameters::ParamType::UINT8,  (const void*)&g.lock_level,       0, 0, 2 },
    { "CAN_NODE",          Parameters::ParamType::UINT8,  (const void*)&g.can_node,         0, 0, 127 },
    { "UAS_ID",            Parameters::ParamType::CHAR20, (const void*)&g.uas_id[0],        0, 0, 0 },
    { "WIFI_NAN_RATE",     Parameters::ParamType::FLOAT,  (const void*)&g.wifi_nan_rate,    1, 0, 5 },
    { "BT4_RATE",          Parameters::ParamType::FLOAT,  (const void*)&g.bt4_rate,         1, 0, 5 },
    { "BT5_RATE",          Parameters::ParamType::FLOAT,  (const void*)&g.bt5_rate,         1, 0, 5 },
    { "WEBSERVER_ENABLE",  Parameters::ParamType::UINT8,  (const void*)&g.webserver_enable, 1, 0, 1 },
    { "WIFI_SSID",         Parameters::ParamType::CHAR20, (const void*)&g.wifi_ssid, },
    { "WIFI_PASSWORD",     Parameters::ParamType::CHAR20, (const void*)&g.wifi_password, },
    { "BCAST_POWERUP",     Parameters::ParamType::UINT8,  (const void*)&g.bcast_powerup,    1, 0, 1 },
    { "",                  Parameters::ParamType::NONE,   nullptr,  },
};

/*
  find by name
 */
const Parameters::Param *Parameters::find(const char *name)
{
    for (const auto &p : params) {
        if (strcmp(name, p.name) == 0) {
            return &p;
        }
    }
    return nullptr;
}

/*
  find by index
 */
const Parameters::Param *Parameters::find_by_index(uint16_t index)
{
    if (index >= (sizeof(params)/sizeof(params[0])-1)) {
        return nullptr;
    }
    return &params[index];
}

void Parameters::Param::set(uint8_t v) const
{
    auto *p = (uint8_t *)ptr;
    *p = v;
    nvs_set_u8(handle, name, *p);
}

void Parameters::Param::set(float v) const
{
    auto *p = (float *)ptr;
    *p = v;
    union {
        float f;
        uint32_t u32;
    } u;
    u.f = v;
    nvs_set_u32(handle, name, u.u32);
}

void Parameters::Param::set(const char *v) const
{
    strncpy((char *)ptr, v, PARAM_NAME_MAX_LEN);
    nvs_set_str(handle, name, v);
}

uint8_t Parameters::Param::get_uint8() const
{
    const auto *p = (const uint8_t *)ptr;
    return *p;
}

float Parameters::Param::get_float() const
{
    const auto *p = (const float *)ptr;
    return *p;
}

const char *Parameters::Param::get_char20() const
{
    const char *p = (const char *)ptr;
    return p;
}


/*
  load defaults from parameter table
 */
void Parameters::load_defaults(void)
{
    for (const auto &p : params) {
        switch (p.ptype) {
        case ParamType::UINT8:
            *(uint8_t *)p.ptr = uint8_t(p.default_value);
            break;
        case ParamType::FLOAT:
            *(float *)p.ptr = p.default_value;
            break;
        }
    }
}

void Parameters::init(void)
{
    load_defaults();

    if (nvs_flash_init() != ESP_OK ||
        nvs_open("storage", NVS_READWRITE, &handle) != ESP_OK) {
        Serial.printf("NVS init failed\n");
    }
    // load values from NVS
    for (const auto &p : params) {
        switch (p.ptype) {
        case ParamType::UINT8:
            nvs_get_u8(handle, p.name, (uint8_t *)p.ptr);
            break;
        case ParamType::FLOAT:
            nvs_get_u32(handle, p.name, (uint32_t *)p.ptr);
            break;
        case ParamType::CHAR20: {
            size_t len = 21;
            nvs_get_str(handle, p.name, (char *)p.ptr, &len);
            break;
        }
        }
    }
}
