/**
 * @file bat_base64.h
 * @brief Functions for conversion between base64 and arrays of big-endian
 * unsigned 32-bit words. Used for serialization/deserialization of the seeder
 * based on ChaCha20 stream cipher.
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_BASE64_H
#define __SMOKERAND_BASE64_H
#include <stdint.h>

char *sr_u32_bigendian_to_base64(const uint32_t *in, size_t len);
uint32_t *sr_base64_to_u32_bigendian(const char *in, size_t *u32_len);

#endif
