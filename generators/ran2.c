/**
 * @file ran2.c
 * @brief ran2 is ranecu (or CombLec88) generator improved with Bays-Durham
 * shuffle.
 * @details It is a 31-bit generator that should be tested with the
 * `--filter=uint31` key. It passes the `full` battery.
 *
 * References:
 * 
 * 1. P. L'Ecuyer Efficient and portable combined random number generators //
 *    Communications of the ACM. 1998. V. 31. N 6. P.742-751
 *    https://doi.org/10.1145/62959.62969
 * 2. C. Bays, S. D. Durham. Improving a Poor Random Number Generator //
 *    ACM Transactions on Mathematical Software (TOMS). 1976. V. 2. N 1.
 *    P. 59-64. https://doi.org/10.1145/355666.355670
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define TBL_SIZE 32
#define TBL_INDMASK 0x1F
#define MOD0 2147483563
#define MOD1 2147483399

typedef struct {
    int32_t s[2];
    int32_t t[TBL_SIZE];
    int32_t z;
} Ran2State;


static inline void lcg31(int32_t *s, int32_t a, int32_t m, int32_t r, int32_t q)
{
    const int32_t k0 = *s / q;
    *s = a * (*s - k0 * q) - k0 * r;
    if (*s < 0) *s += m;
}


static inline int32_t ranecu_next(Ran2State *obj)
{
    lcg31(&obj->s[0], 40014, MOD0, 12211, 53668);
    lcg31(&obj->s[1], 40692, MOD1, 3791,  52774);
    int32_t z = obj->s[0] - obj->s[1];
    if (z < 1) z += 2147483562; // 2^31 - 86
    return z;
}


static inline uint64_t get_bits_raw(Ran2State *obj)
{
    const int j = obj->z & TBL_INDMASK;
    obj->z = obj->t[j];
    obj->t[j] = ranecu_next(obj);
    return (uint32_t)obj->z << 1;
}


static void *create(const CallerAPI *intf)
{                                             
    Ran2State *obj = intf->malloc(sizeof(Ran2State));
    obj->s[0] = (int32_t) (intf->get_seed64() % MOD0);
    obj->s[1] = (int32_t) (intf->get_seed64() % MOD1);
    if (obj->s[0] == 0) obj->s[0] = 1234567;
    if (obj->s[1] == 0) obj->s[1] = 7654321;
    for (int i = 0; i < TBL_SIZE; i++) {
        obj->t[i] = ranecu_next(obj);
    }
    obj->z = ranecu_next(obj);
    return obj;
}

MAKE_UINT32_PRNG("Ran2", NULL)
