/**
 * @file swb_shared.c
 * @brief Subtract with borrow PRNG with prime modulus.
 * @details Fails birthday spacings test and gap test. Also causes biases
 * in Wolff algorithm for 2D Ising model by Monte-Carlo method, see the
 * `ising16_wolff` test in the `ising` battery. It means that SWB generators
 * mustn't be used as a general purpose PRNGs. Their quality may be improved
 * by the decimation/luxury level but it makes them 10 and more times slower
 * than modern CSPRNGs.
 *
 * 1. https://doi.org/10.1103/PhysRevLett.69.3382
 * 2. https://doi.org/10.1016/0010-4655(90)90033-W
 * 3. https://doi.org/10.1214/aoap/1177005878
 *
 * @copyright The SWB algorithm was suggested by G.Marsaglia and A.Zaman.
 *
 * Implementation for SmokeRand:
 *
 * (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_A 43
#define SWB_B 22

/**
 * @brief 32-bit LCG state.
 */
typedef struct {    
    uint32_t x[SWB_A + 1];
    uint32_t c;
    int i;
    int j;
} SwbState;


static inline uint64_t get_bits_raw(void *state)
{
    SwbState *obj = state;
    uint32_t x;
    int64_t xj = obj->x[obj->j], xi = obj->x[obj->i];
    int64_t t = xj - xi - (int64_t) obj->c;
    if (t >= 0) {
        x = (uint32_t) t;
        obj->c = 0;
    } else {
        x = (uint32_t) (t + 0xFFFFFFFB);
        obj->c = 1;
    }
    obj->x[obj->i] = x;
    if (--obj->i == 0) obj->i = SWB_A;
	if (--obj->j == 0) obj->j = SWB_A;
    return x;
}


static void *create(const CallerAPI *intf)
{
    SwbState *obj = intf->malloc(sizeof(SwbState));
    for (size_t i = 1; i <= SWB_A; i++) {
        obj->x[i] = intf->get_seed32();
    }
    obj->c = 1;
    obj->x[1] |= 1;
    obj->x[2] = (obj->x[2] >> 1) << 1;
    obj->i = SWB_A; obj->j = SWB_B;
    return (void *) obj;
}


MAKE_UINT32_PRNG("SWB", NULL)
