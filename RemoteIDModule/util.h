#pragma once

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#include <stdint.h>

/*
  64 bit crc from ArduPilot
*/
uint64_t crc_crc64(const uint32_t *data, uint16_t num_words);

/*
  decode a base64 string
 */
int32_t base64_decode(const char *s, uint8_t *out, const uint32_t max_len);
