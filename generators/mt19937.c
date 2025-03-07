/**
 * @file mt19937.c
 * @brief Mersenne twister (MT19937) implementation.
 * @details The MT19937 algorithm is developed by M. Matsumoto and
 * T. Nishimura. This implementation is based on public domain code
 * by dajobe [1].
 *
 * 1. https://github.com/dajobe/libmtwist
 * 2. http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
 * 3. M. Matsumoto and T. Nishimura, "Mersenne Twister: A 623-dimensionally
 *    equidistributed uniform pseudorandom number generator" // ACM Trans.
 *    on Modeling and Computer Simulation. 1998. V. 8. N 1. P.3-30.
 *    https://doi.org/10.1145/272991.272995
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define MTWIST_N           624
#define MTWIST_M           397
#define MTWIST_UPPER_MASK  0x80000000
#define MTWIST_LOWER_MASK  0x7FFFFFFF
#define MTWIST_MATRIX_A    0x9908B0DF
#define MTWIST_MIXBITS(u, v) \
    ( ( (u) & MTWIST_UPPER_MASK) | ( (v) & MTWIST_LOWER_MASK) )
#define MTWIST_TWIST(u, v) \
    ( (MTWIST_MIXBITS(u, v) >> 1) ^ ( (v) & 1 ? MTWIST_MATRIX_A : 0) )

typedef struct {
    uint32_t state[MTWIST_N];
    size_t pos;
} MT19937State;

static inline uint64_t get_bits_raw(void *state)
{
    MT19937State *mt = state;
    if (mt->pos == MTWIST_N) {
        uint32_t *p = mt->state;
        for (int count = (MTWIST_N - MTWIST_M + 1); --count; p++)
            *p = p[MTWIST_M] ^ MTWIST_TWIST(p[0], p[1]);
        for (int count = MTWIST_M; --count; p++)
            *p = p[MTWIST_M - MTWIST_N] ^ MTWIST_TWIST(p[0], p[1]);
        *p = p[MTWIST_M - MTWIST_N] ^ MTWIST_TWIST(p[0], mt->state[0]);
        mt->pos = 0;
    }  
    uint32_t r = mt->state[mt->pos++];
    // Tempering
    r ^= (r >> 11);
    r ^= (r << 7) & 0x9D2C5680;
    r ^= (r << 15) & 0xEFC60000;
    r ^= (r >> 18);
    return r;
}

void *create(const CallerAPI *intf)
{
    MT19937State *mt = intf->malloc(sizeof(MT19937State));
    mt->pos = MTWIST_N;
    uint32_t *st = mt->state;
    st[0] = intf->get_seed32();
    for (int i = 1; i < MTWIST_N; i++) {
        st[i] = (1812433253u * (st[i - 1] ^ (st[i - 1] >> 30)) + i);
    }
    return (void *) mt;
}

MAKE_UINT32_PRNG("MT19937", NULL)
