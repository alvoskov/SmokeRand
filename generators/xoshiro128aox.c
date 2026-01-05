/**
 * @file xoshiro128aox.c
 * @brief xoshiro128aoox pseurorandom number generator.
 * @details The implementation is based on public domain code by D.Blackman
 * and S.Vigna (vigna@acm.org). An output AOX (addition-or-xor) scrambler
 * is developed by J. Hanlon and S. Felix to be friendly to hardware
 * implementation. It removes low linear complexity even from the lowest bit
 * but is weaker than ++ or ** scramblers in Hamming weights dependencies
 * removal.
 *
 * Only xoroshiro128aox with the 64-bit output was suggested in [1]. The
 * xoshiro128aox modification with the 32-bit output was developed
 * by A. L. Voskov.
 *
 * References:
 * 1. Hanlon J., Felix S. A Fast Hardware Pseudorandom Number Generator Based on
 *    xoroshiro128 // IEEE Transactions on Computers. 2023. V. 72. N 5.
 *    P.1518-1528. https://doi.org/10.1109/TC.2022.3204226
 * 2. D. Blackman, S. Vigna. Scrambled Linear Pseudorandom Number Generators
 *    // ACM Transactions on Mathematical Software (TOMS). 2021. V. 47. N 4.
 *    Article No.: 36, P.1-32. https://doi.org/10.1145/3460772
 * 3. D. Lemire, M. E. O'Neill. Xorshift1024*, xorshift1024+, xorshift128+
 *    and xoroshiro128+ fail statistical tests for linearity // Journal of
 *    Computational and Applied Mathematics. 2019. V.350. P.139-142.
 *    https://doi.org/10.1016/j.cam.2018.10.019.
 * 4. xoshiro / xoroshiro generators and the PRNG shootout
 *    https://prng.di.unimi.it/
 *
 * @copyright xoshiro128 algorithmû was suggested by D. Blackman
 * and S. Vigna, the AOX output scrambler was developed by J. Hanlon
 * and S. Felix. Adaptation of the scrambler to the 32-bit output and
 * reentrant version for SmokeRand:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t s[4];
} Xoshiro128AoxState;


uint64_t get_bits_raw(Xoshiro128AoxState *obj)
{
    const uint32_t sx = obj->s[0] ^ obj->s[1], sa = obj->s[0] & obj->s[1];
    const uint32_t result = sx ^ (rotl32(sa, 1) | rotl32(sa, 2));
    const uint32_t t = obj->s[1] << 9;
    obj->s[2] ^= obj->s[0];
    obj->s[3] ^= obj->s[1];
    obj->s[1] ^= obj->s[2];
    obj->s[0] ^= obj->s[3];
    obj->s[2] ^= t;
    obj->s[3] = rotl32(obj->s[3], 11);
    return result;
}

void *create(const CallerAPI *intf)
{
    Xoshiro128AoxState *obj = intf->malloc(sizeof(Xoshiro128AoxState));
    uint64_t seed0, seed1;
    do {
        seed0 = intf->get_seed64();
        seed1 = intf->get_seed64();
    } while (seed0 == 0 && seed1 == 0);
    obj->s[0] = (uint32_t) seed0;
    obj->s[1] = (uint32_t) (seed0 >> 32);
    obj->s[2] = (uint32_t) seed1;
    obj->s[3] = (uint32_t) (seed0 >> 32);
    return obj;
}

int run_self_test(const CallerAPI *intf)
{
    Xoshiro128AoxState obj = {{12345678, 87654321, 2, 5}};
    static const uint32_t u_ref = 0x648D78B0;
    uint32_t u;
    for (long i = 0; i < 100000; i++) {
        u = (uint32_t) get_bits_raw(&obj);
    }
    intf->printf("Output: 0x%lX; reference value: 0x%lX\n",
        (unsigned long) u, (unsigned long) u_ref);
    return u == u_ref;
}

MAKE_UINT32_PRNG("xoshiro128aox", run_self_test)
