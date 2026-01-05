/**
 * @file tinymt32.c
 * @brief Tiny Mersenne Twister only 127 bit internal state (32-bit version)
 * @copyright Copyright (C) 2011 Mutsuo Saito (Hiroshima University),
 * Makoto Matsumoto (The University of Tokyo).
 * (C) 2024-2025 Alexey L. Voskov (Lomonosov Moscow State University).
 * 
 * The TinyMT64 algorithm was suggested by Mutsuo Saito and Makoto Matsumoto.
 * Refactored to SmokeRand module by Alexey L. Voskov.
 *
 * The 3-clause BSD License is applied to this software.
 *
 * Copyright (c) 2011, 2013 Mutsuo Saito, Makoto Matsumoto,
 * Hiroshima University and The University of Tokyo.
 * Copyright (c) 2024-2026 Alexey L. Voskov (Lomonosov Moscow State University)
 *
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of the Hiroshima University nor the names of
 *       its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
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
#include "smokerand/cinterface.h"
#include <stdint.h>

PRNG_CMODULE_PROLOG


#define TINYMT32_SH0 1
#define TINYMT32_SH1 10
#define TINYMT32_SH8 8
#define TINYMT32_MASK UINT32_C(0x7fffffff)

#define TINYMT32_MAT1  0x8f7011ee
#define TINYMT32_MAT2  0xfc78ff1f
#define TINYMT32_TMAT  0x3793fdff

#define MIN_LOOP 8
#define PRE_LOOP 8

/**
 * tinymt32 internal state vector and parameters
 */
typedef struct {
    uint32_t status[4];
} tinymt32_t;

/**
 * This function certificate the period of 2^127-1.
 * @param random tinymt state vector.
 */
static void period_certification(tinymt32_t * random)
{
    if ((random->status[0] & TINYMT32_MASK) == 0 &&
        random->status[1] == 0 &&
        random->status[2] == 0 &&
        random->status[3] == 0) {
        random->status[0] = 'T';
        random->status[1] = 'I';
        random->status[2] = 'N';
        random->status[3] = 'Y';
    }
}


/**
 * This function changes internal state of tinymt32.
 * Users should not call this function directly.
 * @param random tinymt internal status
 */
inline static void tinymt32_next_state(tinymt32_t * random)
{
    uint32_t x;
    uint32_t y;

    y = random->status[3];
    x = (random->status[0] & TINYMT32_MASK)
        ^ random->status[1]
        ^ random->status[2];
    x ^= (x << TINYMT32_SH0);
    y ^= (y >> TINYMT32_SH0) ^ x;
    random->status[0] = random->status[1];
    random->status[1] = random->status[2];
    random->status[2] = x ^ (y << TINYMT32_SH1);
    random->status[3] = y;
    int32_t const a = -((int32_t)(y & 1)) & (int32_t) TINYMT32_MAT1;
    int32_t const b = -((int32_t)(y & 1)) & (int32_t) TINYMT32_MAT2;
    random->status[1] ^= (uint32_t)a;
    random->status[2] ^= (uint32_t)b;
}

/**
 * This function initializes the internal state array with a 32-bit
 * unsigned integer seed.
 * @param random tinymt state vector.
 * @param seed a 32-bit unsigned integer used as a seed.
 */
void tinymt32_init(tinymt32_t * random, uint32_t seed)
{
    random->status[0] = seed;
    random->status[1] = TINYMT32_MAT1;
    random->status[2] = TINYMT32_MAT2;
    random->status[3] = TINYMT32_TMAT;
    for (unsigned int i = 1; i < MIN_LOOP; i++) {
        random->status[i & 3] ^= i + UINT32_C(1812433253)
            * (random->status[(i - 1) & 3]
               ^ (random->status[(i - 1) & 3] >> 30));
    }
    period_certification(random);
    for (unsigned int i = 0; i < PRE_LOOP; i++) {
        tinymt32_next_state(random);
    }
}

/**
 * This function outputs 32-bit unsigned integer from internal state.
 * @param random tinymt internal status
 * @return 32-bit unsigned integer r (0 <= r < 2^32)
 */
static inline uint64_t get_bits_raw(tinymt32_t *random)
{
    tinymt32_next_state(random);
    uint32_t t0, t1;
    t0 = random->status[3];
    t1 = random->status[0]
        + (random->status[2] >> TINYMT32_SH8);
    t0 ^= t1;
    if ((t1 & 1) != 0) {
        t0 ^= TINYMT32_TMAT;
    }
    return t0;
}


static void *create(const CallerAPI *intf)
{
    tinymt32_t *obj = intf->malloc(sizeof(tinymt32_t));
    tinymt32_init(obj, intf->get_seed32());
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t refval[] = {
        2545341989,  981918433, 3715302833, 2387538352, 3591001365, 
        3820442102, 2114400566, 2196103051, 2783359912,  764534509, 
         643179475, 1822416315,  881558334, 4207026366, 3690273640, 
        3240535687, 2921447122, 3984931427, 4092394160,   44209675, 
        2188315343, 2908663843, 1834519336, 3774670961, 3019990707, 
        4065554902, 1239765502, 4035716197, 3412127188,  552822483, 
         161364450,  353727785,  140085994,  149132008, 2547770827, 
        4064042525, 4078297538, 2057335507,  622384752, 2041665899, 
        2193913817, 1080849512,   33160901,  662956935,  642999063, 
        3384709977, 1723175122, 3866752252,  521822317, 2292524454
    };

    tinymt32_t *obj = intf->malloc(sizeof(tinymt32_t));
    tinymt32_init(obj, 1);
    int is_ok = 1;
    for (size_t i = 0; i < 50; i++) {
        uint32_t x = (uint32_t) get_bits_raw(obj);
        if (x != refval[i]) {
            is_ok = 0;
            break;
        }
    }
    intf->free(obj);
    return is_ok;
}

MAKE_UINT32_PRNG("TinyMT32", run_self_test)
