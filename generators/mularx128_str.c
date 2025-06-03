/**
 * @file mularx128.c
 * @brief A simple counter-based generator that passes `full` battery and
 * 64-bit birthday paradox test(?).
 *
 * References:
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Mularx128 PRNG state.
 */
typedef struct {
    uint64_t x[2];
    uint64_t ctr;
} Mularx128State;

/*
static inline void mulbox128(uint64_t *v, int i, int j, uint64_t mul, int r1, int r2)
{
    uint64_t mul0_high;
    uint64_t v0 = v[i], v1 = v[j];
    v0 = unsigned_mul128(mul, v0, &mul0_high);
    v1 ^= mul0_high;
    v0 = v0 + rotl64(v1, r1);
//    v1 = v1 + rotl64(v0, r2);
    (void) r2;
    v[i] = v1;
    v[j] = v0;
}
*/


static inline uint64_t get_bits_raw(void *state)
{
    Mularx128State *obj = state;

    uint64_t mul0_high;
    uint64_t v0 = obj->x[0], v1 = obj->x[1];
    v0 = unsigned_mul128(0xB3F67E79490FFABB, v0, &mul0_high);
    v1 ^= mul0_high;
    v0 = v0 + rotl64(v1, 25) + (obj->ctr += 0x9E3779B97F4A7C15);
    obj->x[0] = v1; obj->x[1] = v0;
    return v0 + v1;
}

static void *create(const CallerAPI *intf)
{
    Mularx128State *obj = intf->malloc(sizeof(Mularx128State));
    obj->x[0] = 0;
    obj->x[1] = intf->get_seed64();
    obj->ctr = 0;
    return (void *) obj;
}

MAKE_UINT64_PRNG("Mularx128_str", NULL)
