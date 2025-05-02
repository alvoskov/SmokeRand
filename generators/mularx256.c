/**
 * @file mularx256.c
 * @brief A simple counter-based generator that passes `full` battery and
 * 64-bit birthday paradox test.
 *
 * @details PractRand 0.94: >= 32 TiB
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
 * @brief Mularx256 PRNG state.
 */
typedef struct {
    uint64_t x[4];
    uint64_t out[4];
    int pos;
} Mularx256State;


static inline void mulbox128(uint64_t *v, int i, int j, uint64_t mul, int r1, int r2)
{
    uint64_t mul0_high;
    v[i] = unsigned_mul128(mul, (v[i] ^ v[j]), &mul0_high);
    v[j] ^= mul0_high;
    v[j] = v[j] + rotl64(v[i], r1); // Slightly different from mulbox for
    v[i] = v[i] + rotl64(v[j], r2); // mularx128
}


static inline uint64_t get_bits_raw(void *state)
{
    Mularx256State *obj = state;
    if (obj->pos == 4) {
        obj->pos = 0;
        for (int i = 0; i < 4; i++) {
            obj->out[i] = obj->x[i];
        }
        // Round 1 (fails PractRand 0.94 at 4 GiB)
        mulbox128(obj->out, 0, 1, 0x8A86E64ACEA02AFB,  6, 43);
        mulbox128(obj->out, 2, 3, 0x8A86E64ACEA02AFB,  6, 43);
        mulbox128(obj->out, 1, 2, 0x8A86E64ACEA02AFB,  6, 43);
        mulbox128(obj->out, 3, 0, 0x8A86E64ACEA02AFB,  6, 43);
        // Round 2 (fails PractRand 0.94 at 4 GiB)
        mulbox128(obj->out, 0, 1, 0x43703AACE826543B, 28, 15);
        mulbox128(obj->out, 2, 3, 0x43703AACE826543B, 28, 15);
        mulbox128(obj->out, 1, 2, 0x43703AACE826543B, 28, 15);
        mulbox128(obj->out, 3, 0, 0x43703AACE826543B, 28, 15);
        obj->x[0]++;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    Mularx256State *obj = intf->malloc(sizeof(Mularx256State));
    obj->pos = 4;
    obj->x[0] = obj->x[1] = obj->x[2];
    obj->x[3] = 0;//intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT64_PRNG("Mularx256", NULL)
