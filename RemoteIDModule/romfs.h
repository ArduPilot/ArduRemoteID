#pragma once

#include <stdint.h>

class ROMFS_Stream;

class ROMFS {
public:
    static bool exists(const char *fname);
    static ROMFS_Stream *find_stream(const char *fname);
    static const char *find_string(const char *name);

    struct embedded_file {
        const char *filename;
        uint32_t size;
        const uint8_t *contents;
    };

private:
    static const struct embedded_file *find(const char *fname);
    static const struct embedded_file files[];
};

class ROMFS_Stream : public Stream
{
public:
    ROMFS_Stream(const ROMFS::embedded_file &_f) :
        f(_f) {}

    size_t write(const uint8_t *buffer, size_t size) override { return 0; }
    size_t write(uint8_t data) override { return 0; }
    size_t size(void) const;
    const char *name(void) const;

    int available() override;
    int read() override;
    int peek() override;
    void flush() override {}
    size_t readBytes(char *buffer, size_t length) override {
        return read((uint8_t*)buffer, length);
    }

private:
    size_t read(uint8_t* buf, size_t size);
    const ROMFS::embedded_file &f;
    uint32_t offset = 0;
};

