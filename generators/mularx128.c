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
    uint64_t out[2];
    int pos;
} Mularx128State;


static inline void mulbox128(uint64_t *v, int i, int j, uint64_t mul, int r1, int r2)
{
    uint64_t mul0_high;
    v[i] = (v[i] ^ v[j]);
    v[i] = unsigned_mul128(mul, v[i], &mul0_high);
    v[j] ^= mul0_high;

    v[i] = v[i] + rotl64(v[j], r1);
    v[j] = v[j] + rotl64(v[i], r2);
}


static inline uint64_t get_bits_raw(void *state)
{
    Mularx128State *obj = state;
    if (obj->pos == 2) {
        obj->pos = 0;
        for (int i = 0; i < 2; i++) {
            obj->out[i] = obj->x[i];
        }
        mulbox128(obj->out, 0, 1, 0xA3E354DF79256E37, 40, 36); // Round 1
        mulbox128(obj->out, 1, 0, 0xB3F67E79490FFABB, 25, 9);
        mulbox128(obj->out, 0, 1, 0xA3E354DF79256E37, 40, 36); // Round 2
        mulbox128(obj->out, 1, 0, 0xB3F67E79490FFABB, 25, 9);
        obj->x[0]++;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    Mularx128State *obj = intf->malloc(sizeof(Mularx128State));
    obj->pos = 2;
    obj->x[0] = 0;
    obj->x[1] = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT64_PRNG("Mularx128", NULL)
