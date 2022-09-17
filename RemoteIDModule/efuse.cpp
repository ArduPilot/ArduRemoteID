#include <Arduino.h>
#include "efuse.h"
#include <soc/efuse_reg.h>
#include <esp_efuse.h>
#include <esp_efuse_table.h>
#include "parameters.h"

static const struct {
    const esp_efuse_desc_t **desc;
    const char *name;
} fuses[] = {
    { ESP_EFUSE_DIS_DOWNLOAD_MODE, "DIS_DOWNLOAD_MODE" },
    { ESP_EFUSE_DIS_USB_JTAG, "DIS_USB_JTAG" },
#ifdef ESP_EFUSE_DIS_PAD_JTAG
    { ESP_EFUSE_DIS_PAD_JTAG, "DIS_PAD_JTAG" },
#endif
#ifdef ESP_EFUSE_DIS_USB_SERIAL_JTAG
    { ESP_EFUSE_DIS_USB_SERIAL_JTAG, "DIS_USB_SERIAL_JTAG" },
#endif
#ifdef ESP_EFUSE_DIS_USB_DOWNLOAD_MODE
    { ESP_EFUSE_DIS_USB_DOWNLOAD_MODE, "DIS_USB_DOWNLOAD_MODE" },
#endif
#ifdef ESP_EFUSE_DIS_FORCE_DOWNLOAD
    { ESP_EFUSE_DIS_FORCE_DOWNLOAD, "DIS_FORCE_DOWNLOAD" },
#endif
#ifdef ESP_EFUSE_DIS_LEGACY_SPI_BOOT
    { ESP_EFUSE_DIS_LEGACY_SPI_BOOT, "DIS_LEGACY_SPI_BOOT" },
#endif
};

/*
  set efuses to prevent firmware upload except via signed web
  interface
 */
void set_efuses(void)
{
    bool some_unset = false;
    for (const auto &f : fuses) {
        const bool v = esp_efuse_read_field_bit(f.desc);
        Serial.printf("%s = %u\n", f.name, unsigned(v));
        some_unset |= !v;
    }
    if (g.lock_level >= 2 && some_unset) {
        Serial.printf("Burning efuses\n");
        esp_efuse_batch_write_begin();
        for (const auto &f : fuses) {
            const bool v = esp_efuse_read_field_bit(f.desc);
            if (!v) {
                Serial.printf("%s -> 1\n", f.name);
                auto ret = esp_efuse_write_field_bit(f.desc);
                if (ret != ESP_OK) {
                    Serial.printf("%s change failed\n", f.name);
                }
            }
        }
        esp_efuse_batch_write_commit();
    }
}

