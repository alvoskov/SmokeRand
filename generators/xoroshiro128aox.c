/**
 * @file xoroshiro128aox.c
 * @brief xoroshiro128-aox pseurorandom number generator.
 * @details It is based on the xoroshiro128 generator developed by D.Blackman
 * and and S.Vigna (vigna@acm.org). An output AOX (addition-or-xor) scrambler
 * is developed by J. Hanlon and S. Felix to be friendly to hardware
 * implementation. It removes low linear complexity even from the lowest bit
 * but is weaker than ++ or ** scramblers in Hamming weights dependencies
 * removal.
 *
 * References:
 * 1. Hanlon J., Felix S. A Fast Hardware Pseudorandom Number Generator Based on
 *    xoroshiro128 // IEEE Transactions on Computers. 2023. V. 72. N 5.
*     P.1518-1528. https://doi.org/10.1109/TC.2022.3204226
 * 2. https://www.jameswhanlon.com/the-hardware-pseudorandom-number-generator-of-the-graphcore-ipu.html
 * 3. D. Blackman, S. Vigna. Scrambled Linear Pseudorandom Number Generators
 *    // ACM Transactions on Mathematical Software (TOMS). 2021. V. 47. N 4.
 *    Article No.: 36, P.1-32. https://doi.org/10.1145/3460772
 * 4. xoshiro / xoroshiro generators and the PRNG shootout
 *    https://prng.di.unimi.it/
 * @copyright The xoroshiro128 algorithm was suggested by D. Blackman
 * and S. Vigna, the AOX output scrambler was developed by J. Hanlon
 * and S. Felix.
 *
 * Reentrant implementation for SmokeRand:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief xoroshiro128 PRNG state. Mustn't be initialized as (0, 0).
 */
typedef struct {
    uint64_t s[2];
} Xoroshiro128AoxState;


static inline uint64_t get_bits_raw(void *state)
{
    Xoroshiro128AoxState *obj = state;
	uint64_t s0 = obj->s[0], s1 = obj->s[1];
    uint64_t sx = s0 ^ s1, sa = s0 & s1;
	obj->s[0] = rotl64(s0, 24) ^ sx ^ (sx << 16); // a, b
	obj->s[1] = rotl64(sx, 37); // c
	return sx ^ (rotl64(sa, 1) | rotl64(sa, 2));
}


static void *create(const CallerAPI *intf)
{
    Xoroshiro128AoxState *obj = intf->malloc(sizeof(Xoroshiro128AoxState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64() | 0x1;
    return (void *) obj;    
}

MAKE_UINT64_PRNG("xoroshiro128aox", NULL)
