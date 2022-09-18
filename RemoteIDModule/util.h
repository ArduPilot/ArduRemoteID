#pragma once

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#include <stdint.h>

/*
  64 bit crc from ArduPilot
*/
uint64_t crc_crc64(const uint32_t *data, uint16_t num_words);

/*
  decode a base64 string
 */
int32_t base64_decode(const char *s, uint8_t *out, const uint32_t max_len);

/*
  encode as base64, returning allocated string
*/
char *base64_encode(const uint8_t *buf, int len);

