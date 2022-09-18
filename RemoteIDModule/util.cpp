#include "util.h"
#include <string.h>

/*
  64 bit crc from ArduPilot
*/
uint64_t crc_crc64(const uint32_t *data, uint16_t num_words)
{
    const uint64_t poly = 0x42F0E1EBA9EA3693ULL;
    uint64_t crc = ~(0ULL);
    while (num_words--) {
        uint32_t value = *data++;
        for (uint8_t j = 0; j < 4; j++) {
            uint8_t byte = ((uint8_t *)&value)[j];
            crc ^= (uint64_t)byte << 56u;
            for (uint8_t i = 0; i < 8; i++) {
                if (crc & (1ull << 63u)) {
                    crc = (uint64_t)(crc << 1u) ^ poly;
                } else {
                    crc = (uint64_t)(crc << 1u);
                }
            }
        }
    }
    crc ^= ~(0ULL);

    return crc;
}

static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
  simple base64 decoder, not particularly efficient, but small
 */
int32_t base64_decode(const char *s, uint8_t *out, const uint32_t max_len)
{
    const char *p;
    uint32_t n = 0;
    uint32_t i = 0;
    while (*s && (p=strchr(b64,*s))) {
        const uint8_t idx = (p - b64);
        const uint32_t byte_offset = (i*6)/8;
        const uint32_t bit_offset = (i*6)%8;
        out[byte_offset] &= ~((1<<(8-bit_offset))-1);
        if (bit_offset < 3) {
            if (byte_offset >= max_len) {
                break;
            }
            out[byte_offset] |= (idx << (2-bit_offset));
            n = byte_offset+1;
        } else {
            if (byte_offset >= max_len) {
                break;
            }
            out[byte_offset] |= (idx >> (bit_offset-2));
            n = byte_offset+1;
            if (byte_offset+1 >= max_len) {
                break;
            }
            out[byte_offset+1] = (idx << (8-(bit_offset-2))) & 0xFF;
            n = byte_offset+2;
        }
        s++; i++;
    }

    if ((n > 0) && (*s == '=')) {
        n -= 1;
    }

    return n;
}

/*
  encode as base64, returning allocated string
*/
char *base64_encode(const uint8_t *d, int len)
{
    uint32_t bit_offset, byte_offset, idx, i;
    uint32_t bytes = (len*8 + 5)/6;
    uint32_t pad_bytes = (bytes % 4) ? 4 - (bytes % 4) : 0;

    char *out = new char[bytes+pad_bytes+1];
    if (!out) {
        return nullptr;
    }

    for (i=0;i<bytes;i++) {
        byte_offset = (i*6)/8;
        bit_offset = (i*6)%8;
        if (bit_offset < 3) {
            idx = (d[byte_offset] >> (2-bit_offset)) & 0x3FU;
        } else {
            idx = (d[byte_offset] << (bit_offset-2)) & 0x3FU;
            if (byte_offset+1 < len) {
                idx |= (d[byte_offset+1] >> (8-(bit_offset-2)));
            }
        }
        out[i] = b64[idx];
    }

    for (;i<bytes+pad_bytes;i++) {
        out[i] = '=';
    }
    out[i] = 0;

    return out;
}
