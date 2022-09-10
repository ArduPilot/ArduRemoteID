#include <Arduino.h>
#include "romfs.h"
#include "romfs_files.h"
#include <string.h>

const ROMFS::embedded_file *ROMFS::find(const char *fname)
{
    for (const auto &f : files) {
        if (strcmp(fname, f.filename) == 0) {
            Serial.printf("ROMFS Returning '%s' size=%u len=%u\n",
                          fname, f.size, strlen((const char *)f.contents));
            return &f;
        }
    }
    Serial.printf("ROMFS not found '%s'\n", fname);
    return nullptr;
}

bool ROMFS::exists(const char *fname)
{
    return find(fname) != nullptr;
}

ROMFS_Stream *ROMFS::find_stream(const char *fname)
{
    const auto *f = find(fname);
    if (!f) {
        return nullptr;
    }
    return new ROMFS_Stream(*f);
}

size_t ROMFS_Stream::size(void) const
{
    return f.size;
}

const char *ROMFS_Stream::name(void) const
{
    return f.filename;
}

int ROMFS_Stream::available(void)
{
    return f.size - offset;
}

size_t ROMFS_Stream::read(uint8_t* buf, size_t size)
{
    const auto avail = available();
    if (size > avail) {
        size = avail;
    }
    memcpy(buf, &f.contents[offset], size);
    offset += size;
    return size;
}

int ROMFS_Stream::peek(void)
{
    if (offset >= f.size) {
        return -1;
    }
    return f.contents[offset];
}

int ROMFS_Stream::read(void)
{
    if (offset >= f.size) {
        return -1;
    }
    return f.contents[offset++];
}
