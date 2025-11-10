/**
 * @file bat_base64.c
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
#include "smokerand/base64.h"
#include <stdlib.h>
#include <stdio.h>
/**
 * @brief Calculate length of base64 string for binary data.
 * @param nbytes  Binary data size, bytes.
 */
static size_t calc_base64_len(size_t nbytes)
{
    return (nbytes == 0) ? 0 : (1 + (nbytes - 1) / 3) * 4;
}

/**
 * @brief Convert array of big-endian unsigned 32-bit words to the base64
 * string with padding.
 * @param in   Input array of unsigned 32-bit words.
 * @param len  Input array size (number of 32-bit words in the array).
 * @return Pointer to the buffer with ASCIIZ base64 string, must be
 * deallocated by `free` function by the caller.
 */
char *sr_u32_bigendian_to_base64(const uint32_t *in, size_t len)
{
    static const char b64syms[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    // Initialize buffers
    uint64_t queue = 0;
    int queue_nbits = 0;   
    const size_t base64_len = calc_base64_len(len * 4);
    size_t pos = 0;
    char *base64_txt = calloc(base64_len + 1, sizeof(char));
    // Encode to base64
    for (size_t i = 0; i < len; i++) {
        queue |= ((uint64_t) in[i]) << (32 - queue_nbits);
        queue_nbits += 32;
        while (queue_nbits >= 6) {
            const size_t ord = (size_t) (queue >> 58);
            queue <<= 6;
            queue_nbits -= 6;
            base64_txt[pos++] = b64syms[ord];
        }
    }
    // Dump the rest of bytes from the queue
    if (queue_nbits > 0) {
        size_t ord = (size_t) (queue >> 58);
        base64_txt[pos++] = b64syms[ord];
    }
    // Add padding
    while (pos < base64_len) {
        base64_txt[pos++] = '=';
    }
    return base64_txt;
}

/**
 * @brief Converts ASCII char to the base64 code (6-bit chunk)
 * @return Either 6-bit chunk in the case of success or 255 in the
 *         case of failure.
 */
static inline unsigned int base64_char_to_ord(char c)
{
    static const uint8_t codes[] = {
       255,255,255,255,255,255,255,255,  // Codes 0-8
       255,255,255,255,255,255,255,255,  // Codes 9-15
       255,255,255,255,255,255,255,255,  // Codes 16-23
       255,255,255,255,255,255,255,255,  // Codes 24-31
       255,255,255,255,255,255,255,255,  //  !"#$%&'
       255,255,255, 62,255,255,255, 63,  // ()*+,-./
        52, 53, 54, 55, 56, 57, 58, 59,  // 01234567
        60, 61,255,255,255,255,255,255,  // 89:;<=>?
       255,  0,  1,  2,  3,  4,  5,  6,  // @ABCDEFG
         7,  8,  9, 10, 11, 12, 13, 14,  // HIJKLMNO
        15, 16, 17, 18, 19, 20, 21, 22,  // PQRSTUVW
        23, 24, 25,255,255,255,255,255,  // XYZ[\]^_
       255, 26, 27, 28, 29, 30, 31, 32,  // `abcdefg
        33, 34, 35, 36, 37, 38, 39, 40,  // hijklmno
        41, 42, 43, 44, 45, 46, 47, 48,  // pqrstuvw
        49, 50, 51,255,255,255,255,255   // xyz{|}~
    };
    const unsigned char u = (unsigned char) c;
    return (u > 127u) ? 255u : codes[u];
}


/**
 * @brief Convert base64 string that encodes an array of unsigned big-endian
 * 32-bit words to its original binary form.
 * @param in   Input array of unsigned 32-bit words.
 * @param len  Input array size (number of 32-bit words in the array).
 * @return Pointer to the buffer with ASCIIZ base64 string, must be
 * deallocated by `free` function by the caller.
 */
uint32_t *sr_base64_to_u32_bigendian(const char *in, size_t *u32_len)
{
    static const unsigned int nextrabits_ok[3] = {4, 2, 0};
    size_t len, nsyms = 0, pos = 0;
    if (in == NULL || u32_len == NULL) {
        return NULL;
    }
    // Find string length and calculate number of bytes
    // present in base64
    for (len = 0; in[len] != '\0'; len++) {
        if (base64_char_to_ord(in[len]) != 255u) {
            nsyms++;
        }
    }
    *u32_len = (6 * nsyms) / 32;
    if (*u32_len == 0 ||
        nextrabits_ok[(*u32_len - 1) % 3] != 6 * nsyms - 32 * (*u32_len)) {
        return NULL;
    }
    uint32_t *u32_out = calloc(*u32_len, sizeof(uint32_t));    
    uint64_t queue = 0;
    int queue_nbits = 0;
    for (size_t i = 0; i < len; i++) {
        const unsigned int ord = base64_char_to_ord(in[i]);
        if (ord != 255u) {
            queue |= ((uint64_t) ord) << (58 - queue_nbits);
            queue_nbits += 6;
            if (queue_nbits >= 32) {
                const uint32_t u = (uint32_t) (queue >> 32);
                queue <<= 32;
                queue_nbits -= 32;
                u32_out[pos++] = u;
            }
        }
    }
    // If 32-bit words were encoded - they queue must contain only
    // zeros. 
    if (queue != 0) {
        free(u32_out);
        return NULL;
    }
    return u32_out;
}
