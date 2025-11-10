/**
 * @file blake2s.h
 * @brief A simple Blake2s implementation based on the reference implementation
 * from RFC 7693. It also includes internal self-tests.
 *
 * @details The original reference implementation and licensing information
 * can be found here:
 *
 * 1. https://datatracker.ietf.org/doc/html/rfc7693.html
 * 2. https://trustee.ietf.org/documents/trust-legal-provisions/tlp-5/
 *
 * @copyright
 * The original code of Blake2s reference implementation was taken from
 * RFC 7693, the authors are Markku-Juhani O. Saarinen and Jean-Philippe
 * Aumasson. Some code refactoring and integration of self-tests were
 * made by A.L. Voskov. According to TLP 5.0 it is licensed under the
 * 3-Clause BSD License (or "Revised BSD License" according to IETF
 * TLP 5.0).
 *
 *
 * Copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * All rights reserved.
 *
 * Copyright (c) 2015 IETF Trust and the persons identified as authors of
 * the code. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - Neither the name of Internet Society, IETF or IETF Trust, nor the
 *   names of specific contributors, may be used to endorse or promote
 *   products  derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "smokerand/blake2s.h"
#include "smokerand/coredefs.h"
#include <stdio.h>

/**
 * @brief Little-endian byte access.
 */
static uint32_t b2s_get32(const void *src)
{
    const uint8_t *p = (const uint8_t *) src;
    return ((uint32_t) (p[0]) ^
           ((uint32_t) (p[1]) << 8) ^
           ((uint32_t) (p[2]) << 16) ^
           ((uint32_t) (p[3]) << 24));
}


static inline void b2s_g(uint32_t *v,
    int a, int b, int c, int d,
    uint32_t x, uint32_t y)
{
    v[a] = v[a] + v[b] + x; v[d] = rotr32(v[d] ^ v[a], 16);
    v[c] = v[c] + v[d];     v[b] = rotr32(v[b] ^ v[c], 12);
    v[a] = v[a] + v[b] + y; v[d] = rotr32(v[d] ^ v[a], 8);
    v[c] = v[c] + v[d];     v[b] = rotr32(v[b] ^ v[c], 7);
}


static inline void b2s_round(uint32_t *v, const uint32_t *m, size_t i)
{
    static const uint8_t sigma[10][16] = {
        { 0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
        { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
        { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 },
        { 7,   9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 },
        { 9,   0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 },
        { 2,  12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 },
        { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 },
        { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 },
        { 6,  15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 },
        { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13,  0 }
    };
    b2s_g(v,  0, 4,  8, 12, m[sigma[i][ 0]], m[sigma[i][ 1]]);
    b2s_g(v,  1, 5,  9, 13, m[sigma[i][ 2]], m[sigma[i][ 3]]);
    b2s_g(v,  2, 6, 10, 14, m[sigma[i][ 4]], m[sigma[i][ 5]]);
    b2s_g(v,  3, 7, 11, 15, m[sigma[i][ 6]], m[sigma[i][ 7]]);
    b2s_g(v,  0, 5, 10, 15, m[sigma[i][ 8]], m[sigma[i][ 9]]);
    b2s_g(v,  1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
    b2s_g(v,  2, 7,  8, 13, m[sigma[i][12]], m[sigma[i][13]]);
    b2s_g(v,  3, 4,  9, 14, m[sigma[i][14]], m[sigma[i][15]]);
}

/**
 * @brief Initialization Vector.
 */
static const uint32_t blake2s_iv[8] =
{
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

/**
 * @brief Reset 64-bit counter
 */
static inline void blake2s_reset_counter(blake2s_state *obj)
{
    obj->t[0] = 0; // Lower 32 bits
    obj->t[1] = 0; // Higher 32 bits
}

/**
 * @brief Increment 64-bit counter
 */
static inline void blake2s_inc_counter(blake2s_state *obj, uint32_t inc)
{
    obj->t[0] += inc;
    if (obj->t[0] < inc)
        obj->t[1]++;
}


/**
 * @brief Compression function. "last" flag indicates last block.
 */
static void blake2s_compress(blake2s_state *obj, int last)
{
    uint32_t m[16], v[16];

    for (size_t i = 0; i < 8; i++) {    // init work variables
        v[i] = obj->h[i];
        v[i + 8] = blake2s_iv[i];
    }

    v[12] ^= obj->t[0]; // low 32 bits of offset
    v[13] ^= obj->t[1]; // high 32 bits
    if (last)           // last block flag set ?
        v[14] = ~v[14];


    for (size_t i = 0; i < 16; i++)     // get little-endian words
        m[i] = b2s_get32(&obj->buf[4 * i]);

    for (size_t i = 0; i < 10; i++)        // ten rounds
        b2s_round(v, m, i);
        
    for (size_t i = 0; i < 8; i++)
        obj->h[i] ^= v[i] ^ v[i + 8];
}

/**
 * @brief Initialize the Blake2s state.
 * @param outlen  The digest size in bytes
 * 1 <= outlen <= 32 gives the digest size in bytes.
 * Secret key (also <= 32 bytes) is optional (keylen = 0).
 */
int blake2s_init(blake2s_state *obj, size_t outlen,
    const void *key, size_t keylen)
{
    // Check input parameters
    if (outlen == 0 || outlen > BLAKE2S_OUTBYTES || keylen > BLAKE2S_KEYBYTES) {
        return BLAKE2S_FAILURE;
    }
    for (size_t i = 0; i < 8; i++)      // state, "param block"
        obj->h[i] = blake2s_iv[i];
    obj->h[0] ^= 0x01010000 ^ ((uint32_t) keylen << 8) ^ (uint32_t) outlen;
    blake2s_reset_counter(obj);
    obj->c = 0;                         // pointer within buffer
    obj->outlen = outlen;
    for (size_t i = keylen; i < BLAKE2S_BLOCKBYTES; i++) // zero input block
        obj->buf[i] = 0;
    if (keylen > 0) {
        blake2s_update(obj, key, keylen);
        obj->c = BLAKE2S_BLOCKBYTES;    // at the end
    }
    return BLAKE2S_SUCCESS;
}


/**
 * @brief Add "inlen" bytes from "in" into the hash.
 * @param ctx    Context
 * @param in     Pointer to data to be hashed
 * @param inlen  Data size to be hashed, bytes
 */
void blake2s_update(blake2s_state *obj, const void *in, size_t inlen)
{
    for (size_t i = 0; i < inlen; i++) {
        if (obj->c == BLAKE2S_BLOCKBYTES) { // buffer full ?
            blake2s_inc_counter(obj, (uint32_t) obj->c);
            blake2s_compress(obj, 0); // compress (not last)
            obj->c = 0;               // counter to zero
        }
        obj->buf[obj->c++] = ((const uint8_t *) in)[i];
    }
}


/**
 * @brief Generate the message digest (size given in init).
 * @param obj  State context
 * @param out  Output buffer (result will be placed in it)
 */
void blake2s_final(blake2s_state *obj, void *out)
{
    blake2s_inc_counter(obj, (uint32_t) obj->c);
    while (obj->c < BLAKE2S_BLOCKBYTES)
        obj->buf[obj->c++] = 0;
    blake2s_compress(obj, 1); // Final block flag = 1
    // Digest output (little endian)
    for (size_t i = 0; i < obj->outlen; i++) {
        ((uint8_t *) out)[i] =
            (obj->h[i >> 2] >> (8 * (i & 3))) & 0xFF;
    }
}

/**
 * @brief Convenience function for all-in-one computation.
 * @param out  Output buffer for the digest
 * @param key  Secret key
 * @param in   Data to be hashed
 */
int blake2s(void *out, size_t outlen,
    const void *key, size_t keylen,
    const void *in, size_t inlen)
{
    blake2s_state obj;
    if (blake2s_init(&obj, outlen, key, keylen)) {
        return BLAKE2S_FAILURE;
    }
    blake2s_update(&obj, in, inlen);
    blake2s_final(&obj, out);
    return BLAKE2S_SUCCESS;
}


/**
 * @brief Deterministic sequences (Fibonacci generator).
 */
static void selftest_seq(uint8_t *out, size_t len, uint32_t seed)
{
    uint32_t a = 0xDEAD4BAD * seed; // prime
    uint32_t b = 1;
    for (size_t i = 0; i < len; i++) { // fill the buf
        uint32_t t = a + b;
        a = b;
        b = t;
        out[i] = (uint8_t) ((t >> 24) & 0xFF);
    }
}


/**
 * @brief Blake2s test taken from RFC 7693. It is described by the authors
 * as: "This is a fairly exhaustive, yet compact and fast method for verifying
 * that the hashing module is functioning correctly".
 */
static int blake2s_selftest_prng()
{
    // Grand hash of hash results.
    static const uint8_t blake2s_res[32] = {
        0x6A, 0x41, 0x1F, 0x08, 0xCE, 0x25, 0xAD, 0xCD,
        0xFB, 0x02, 0xAB, 0xA6, 0x41, 0x45, 0x1C, 0xEC,
        0x53, 0xC5, 0x98, 0xB2, 0x4F, 0x4F, 0xC7, 0x87,
        0xFB, 0xDC, 0x88, 0x79, 0x7F, 0x4C, 0x1D, 0xFE
    };
    // Parameter sets.
    static const size_t b2s_md_len[4] = { 16, 20, 28, 32 };
    static const size_t b2s_in_len[6] = { 0,  3,  64, 65, 255, 1024 };
    uint8_t in[1024], md[32], key[32];
    blake2s_state obj;

    // 256-bit hash for testing.
    if (blake2s_init(&obj, 32, NULL, 0)) {
        return BLAKE2S_FAILURE;
    }

    for (size_t i = 0; i < 4; i++) {
        size_t outlen = b2s_md_len[i];
        for (size_t j = 0; j < 6; j++) {
            size_t inlen = b2s_in_len[j];

            selftest_seq(in, inlen, (uint32_t) inlen);     // unkeyed hash
            blake2s(md, outlen, NULL, 0, in, (uint32_t) inlen);
            blake2s_update(&obj, md, outlen);   // hash the hash

            selftest_seq(key, outlen, (uint32_t) outlen);  // keyed hash
            blake2s(md, outlen, key, outlen, in, (uint32_t) inlen);
            blake2s_update(&obj, md, outlen);   // hash the hash
        }
    }

    // Compute and compare the hash of hashes.
    blake2s_final(&obj, md);
    for (size_t i = 0; i < 32; i++) {
        if (md[i] != blake2s_res[i])
            return BLAKE2S_FAILURE;
    }
    return BLAKE2S_SUCCESS;
}

static int blake2s_selftest_string(const uint8_t *ref, const char *str)
{
    uint8_t out[32];
    size_t len = 0;
    while (str[len] != '\0') { len++; }
    blake2s_256(out, str, len);
    for (size_t i = 0; i < 32; i++) {
        if (out[i] != ref[i])
            return BLAKE2S_FAILURE;
    }
    return BLAKE2S_SUCCESS;
}


/**
 * @brief An internal self-test for Blake2s-256.
 */
int blake2s_selftest(void)
{
    static const char in_1[] = "The quick brown fox jumps over the lazy dog|"
        "The quick brown fox jumps over the lazy dog";
    static const uint8_t ref_1[32] = {
        0xad, 0xd6, 0x41, 0x6a,  0x0c, 0x13, 0xa8, 0x35,
        0xf0, 0xba, 0xe3, 0x53,  0x69, 0x8b, 0x1c, 0x02,
        0x52, 0x70, 0x34, 0xaa,  0xd4, 0x60, 0x08, 0x5d,
        0x02, 0x74, 0x5b, 0x78,  0xc9, 0x65, 0x92, 0x84
    };

    static const char in_2[] = "";
    static const uint8_t ref_2[32] = {
        0x69, 0x21, 0x7A, 0x30,  0x79, 0x90, 0x80, 0x94, 
        0xE1, 0x11, 0x21, 0xD0,  0x42, 0x35, 0x4A, 0x7C,
        0x1F, 0x55, 0xB6, 0x48,  0x2C, 0xA1, 0xA5, 0x1E,
        0x1B, 0x25, 0x0D, 0xFD,  0x1E, 0xD0, 0xEE, 0xF9
    };
    static const char in_3[] = "abc"; // From RFC 7693
    static const uint8_t ref_3[32] = {
        0x50, 0x8C, 0x5E, 0x8C,  0x32, 0x7C, 0x14, 0xE2,
        0xE1, 0xA7, 0x2B, 0xA3,  0x4E, 0xEB, 0x45, 0x2F,
        0x37, 0x45, 0x8B, 0x20,  0x9E, 0xD6, 0x3A, 0x29,
        0x4D, 0x99, 0x9B, 0x4C,  0x86, 0x67, 0x59, 0x82
    };

    if (blake2s_selftest_string(ref_1, in_1) == BLAKE2S_FAILURE ||
        blake2s_selftest_string(ref_2, in_2) == BLAKE2S_FAILURE ||
        blake2s_selftest_string(ref_3, in_3) == BLAKE2S_FAILURE ||
        blake2s_selftest_prng()) {
        return BLAKE2S_FAILURE;
    } else {
        return BLAKE2S_SUCCESS;
    }
}
