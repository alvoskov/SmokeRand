/**
 * @file tinymt64.c
 * @brief Tiny Mersenne Twister only 127 bit internal state (64-bit version)
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


#define TINYMT64_SH0 12
#define TINYMT64_SH1 11
#define TINYMT64_SH8 8
#define TINYMT64_MASK UINT64_C(0x7fffffffffffffff)


#define TINYMT64_MAT1  0xfa051f40
#define TINYMT64_MAT2  0xffd0fff4
#define TINYMT64_TMAT  0x58d02ffeffbfffbc

#define MIN_LOOP 8
//#define PRE_LOOP 8

/**
 * tinymt32 internal state vector and parameters
 */
typedef struct {
    uint64_t status[2];
} tinymt64_t;

/**
 * This function certificate the period of 2^127-1.
 * @param random tinymt state vector.
 */
static void period_certification(tinymt64_t *random)
{
    if ((random->status[0] & TINYMT64_MASK) == 0 &&
        random->status[1] == 0) {
        random->status[0] = 'T';
        random->status[1] = 'M';
    }
}


/**
 * This function changes internal state of tinymt32.
 * Users should not call this function directly.
 * @param random tinymt internal status
 */
inline static void tinymt64_next_state(tinymt64_t *random)
{
    uint64_t x;
    random->status[0] &= TINYMT64_MASK;
    x = random->status[0] ^ random->status[1];
    x ^= x << TINYMT64_SH0;
    x ^= x >> 32;
    x ^= x << 32;
    x ^= x << TINYMT64_SH1;
    random->status[0] = random->status[1];
    random->status[1] = x;
    if ((x & 1) != 0) {
        random->status[0] ^= TINYMT64_MAT1;
        random->status[1] ^= ((uint64_t)TINYMT64_MAT2 << 32);
    }
}

/**
 * This function initializes the internal state array with a 32-bit
 * unsigned integer seed.
 * @param random tinymt state vector.
 * @param seed a 32-bit unsigned integer used as a seed.
 */
void tinymt64_init(tinymt64_t *random, uint64_t seed)
{
    random->status[0] = seed ^ ((uint64_t) TINYMT64_MAT1 << 32);
    random->status[1] = TINYMT64_MAT2 ^ TINYMT64_TMAT;
    for (unsigned int i = 1; i < MIN_LOOP; i++) {
        random->status[i & 1] ^= i + UINT64_C(6364136223846793005)
            * (random->status[(i - 1) & 1]
               ^ (random->status[(i - 1) & 1] >> 62));
    }
    period_certification(random);
}

/**
 * This function outputs 32-bit unsigned integer from internal state.
 * @param random tinymt internal status
 * @return 32-bit unsigned integer r (0 <= r < 2^32)
 */
static inline uint64_t get_bits_raw(tinymt64_t *random)
{
    uint64_t x;
    tinymt64_next_state(random);
    x = random->status[0] ^ random->status[1];
    x = random->status[0] + random->status[1];
    x ^= random->status[0] >> TINYMT64_SH8;
    if ((x & 1) != 0) {
        x ^= TINYMT64_TMAT;
    }
    return x;
}


static void *create(const CallerAPI *intf)
{
    tinymt64_t *obj = intf->malloc(sizeof(tinymt64_t));
    tinymt64_init(obj, intf->get_seed32());
    return obj;
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t refval[] = {
        15503804787016557143ull, 17280942441431881838ull,  2177846447079362065ull,
        10087979609567186558ull,  8925138365609588954ull, 13030236470185662861ull, 
         4821755207395923002ull, 11414418928600017220ull, 18168456707151075513ull, 
         1749899882787913913ull,  2383809859898491614ull,  4819668342796295952ull, 
        11996915412652201592ull, 11312565842793520524ull,   995000466268691999ull, 
         6363016470553061398ull,  7460106683467501926ull,   981478760989475592ull, 
        11852898451934348777ull,  5976355772385089998ull, 16662491692959689977ull, 
         4997134580858653476ull, 11142084553658001518ull, 12405136656253403414ull, 
        10700258834832712655ull, 13440132573874649640ull, 15190104899818839732ull, 
        14179849157427519166ull, 10328306841423370385ull,  9266343271776906817ull
    };

    tinymt64_t *obj = intf->malloc(sizeof(tinymt64_t));
    tinymt64_init(obj, 1);
    int is_ok = 1;
    for (size_t i = 0; i < 30; i++) {
        uint64_t x = get_bits_raw(obj);
        if (x != refval[i]) {
            is_ok = 0;
            break;
        }
    }
    intf->free(obj);
    return is_ok;
}

MAKE_UINT64_PRNG("TinyMT64", run_self_test)
