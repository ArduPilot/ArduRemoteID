#include <Arduino.h>
#include "romfs.h"
#include "romfs_files.h"
#include <string.h>
#include "tinf.h"

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

/*
  decompress and return a string
*/
const char *ROMFS::find_string(const char *name)
{
    const auto *f = find(name);
    if (!f) {
        return nullptr;
    }

    // last 4 bytes of gzip file are length of decompressed data
    const uint8_t *p = &f->contents[f->size-4];
    uint32_t decompressed_size = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
    
    uint8_t *decompressed_data = (uint8_t *)malloc(decompressed_size + 1);
    if (!decompressed_data) {
        return nullptr;
    }

    // explicitly null terimnate the data
    decompressed_data[decompressed_size] = 0;

    TINF_DATA *d = (TINF_DATA *)malloc(sizeof(TINF_DATA));
    if (!d) {
        ::free(decompressed_data);
        return nullptr;
    }
    uzlib_uncompress_init(d, NULL, 0);

    d->source = f->contents;
    d->source_limit = f->contents + f->size - 4;

    // assume gzip format
    int res = uzlib_gzip_parse_header(d);
    if (res != TINF_OK) {
        ::free(decompressed_data);
        ::free(d);
        return nullptr;
    }

    d->dest = decompressed_data;
    d->destSize = decompressed_size;

    // we don't check CRC, as it just wastes flash space for constant
    // ROMFS data
    res = uzlib_uncompress(d);

    ::free(d);
    
    if (res != TINF_OK) {
        ::free(decompressed_data);
        return nullptr;
    }

    return (const char *)decompressed_data;
}
