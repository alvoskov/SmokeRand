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
 * RFC7693, the authors are Markku-Juhani O. Saarinen and Jean-Philippe
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
#ifndef __SMOKERAND_BLAKE2S_H
#define __SMOKERAND_BLAKE2S_H

#include <stdint.h>
#include <stddef.h>

enum blake2s_constant
{
    BLAKE2S_BLOCKBYTES = 64,
    BLAKE2S_KEYBYTES   = 32,
    BLAKE2S_OUTBYTES   = 32,
};

/**
 * @brief Blake2s state (context)
 */
typedef struct {
    uint8_t buf[BLAKE2S_BLOCKBYTES]; ///< Input buffer
    uint32_t h[BLAKE2S_OUTBYTES / 4];  ///< Chained state
    uint32_t t[2];  ///< Total number of bytes
    size_t c;       ///< Pointer for b[]
    size_t outlen;  ///< Digest size
} blake2s_state;

int blake2s_init(blake2s_state *ctx, size_t outlen,
    const void *key, size_t keylen);
void blake2s_update(blake2s_state *ctx, const void *in, size_t inlen);
void blake2s_final(blake2s_state *ctx, void *out);
int blake2s(void *out, size_t outlen,
    const void *key, size_t keylen,
    const void *in, size_t inlen);

static inline int blake2s_256(void *out, const void *in, size_t inlen)
{
    return blake2s(out, 32, NULL, 0, in, inlen);
}

static inline int blake2s_128(void *out, const void *in, size_t inlen)
{
    return blake2s(out, 16, NULL, 0, in, inlen);
}

int blake2s_self_test(void);

#endif // __SMOKERAND_BLAKE2S_H
