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
 * 4. Alain Schumacher and Takuji Nishimura and Makoto Matsumoto.
 *    Some Patterns of Duplications in the outputs of Mersenne Twister
 *    Pseudorandom Number Generator MT19937. 2026.
 *    https://arxiv.org/abs/2512.21678.
 * 5. Shin Harase. On the F2-linear relations of Mersenne Twister pseudorandom
 *    number generators // Mathematics and Computers in Simulation. 2014.
 *    V. 100. P. 101-113. https://doi.org/10.1016/j.matcom.2014.02.002
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define MTWIST_N 624
#define MTWIST_M 397

typedef struct {
    uint32_t state[MTWIST_N];
    size_t pos;
} MT19937State;

void MT19937State_init(MT19937State *obj, uint32_t seed)
{
    obj->pos = MTWIST_N;
    for (unsigned int i = 0; i < MTWIST_N; i++) {
        obj->state[i] = seed;
        seed = 1812433253U * (seed ^ (seed >> 30)) + (i + 1);
    }
}

static inline uint32_t mtwist_twist(uint32_t u, uint32_t v)
{
    static const uint32_t UMASK = 0x80000000U, LMASK = 0x7FFFFFFFU;
    static const uint32_t MATRIX_A  = 0x9908B0DFU;
    uint32_t x = (u & UMASK) | (v & LMASK);
    return (x >> 1) ^ (v & 1 ? MATRIX_A : 0);
}

static inline uint32_t MT19937State_next(MT19937State *mt)
{
    if (mt->pos == MTWIST_N) {
        uint32_t *p = mt->state;
        for (int i = 0; i < MTWIST_N - MTWIST_M; i++)
            p[i] = p[i + MTWIST_M] ^ mtwist_twist(p[i], p[i + 1]);
        for (int i = MTWIST_N - MTWIST_M; i < MTWIST_N - 1; i++)
            p[i] = p[i + MTWIST_M - MTWIST_N] ^ mtwist_twist(p[i], p[i + 1]);
        p[MTWIST_N - 1] = p[MTWIST_M - 1] ^ mtwist_twist(p[MTWIST_N - 1], p[0]);
        mt->pos = 0;
    }  
    uint32_t r = mt->state[mt->pos++];
    // Tempering
    r ^= (r >> 11);
    r ^= (r << 7) & 0x9D2C5680U;
    r ^= (r << 15) & 0xEFC60000U;
    r ^= (r >> 18);
    return r;
}

static inline uint64_t get_bits_raw(void *state)
{
    return MT19937State_next(state);
}

void *create(const CallerAPI *intf)
{
    MT19937State *mt = intf->malloc(sizeof(MT19937State));
    MT19937State_init(mt, intf->get_seed32());
    return mt;
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t x_ref[] = {
        1179087213U,  643050027U, 877912121U, 1390209599U, 4231655160U,
        1714989237U, 1575447228U, 698285346U, 2593289829U, 1420374026U
    };
    int is_ok = 1;
    MT19937State *mt = intf->malloc(sizeof(MT19937State));
    MT19937State_init(mt, 0x12345678);
    for (int i = 0; i < 990; i++) {
        (void) get_bits_raw(mt);
    }
    for (int i = 0; i < 10; i++) {
        uint32_t x = MT19937State_next(mt);
        intf->printf("%12lu %12lu\n",
            (unsigned long) x, (unsigned long) x_ref[i]);
        if (x != x_ref[i])
            is_ok = 0;
    }    
    intf->free(mt);
    return is_ok;
}

MAKE_UINT32_PRNG("MT19937", run_self_test)
