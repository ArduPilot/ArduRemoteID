#pragma once

#include <stdint.h>

class ROMFS {
public:
    static const char *find_string(const char *fname);
private:

    struct embedded_file {
        const char *filename;
        uint32_t size;
        const uint8_t *contents;
    };
    static const struct embedded_file files[];
};
