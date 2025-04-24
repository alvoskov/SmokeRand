/**
 * @file mularx512.c
 * @brief A simple counter-based generator.
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
 * @brief SplitMix PRNG state.
 */
typedef struct {
    uint64_t x[8];
    uint64_t out[8];
    int pos;
} Mularx512State;


static inline void mulbox128(uint64_t *v, int i, int j)
{
    uint64_t a = 0xfc0072fa0b15f4fd;
    uint64_t mul0_high;
    v[i] = unsigned_mul128(a, v[i] ^ v[j], &mul0_high);
    v[j] ^= mul0_high;
    v[j] = v[j] + rotl64(v[i], 46);
    v[i] = v[i] ^ rotl64(v[j], 13);
}

static inline uint64_t get_bits_raw(void *state)
{
    Mularx512State *obj = state;
    if (obj->pos == 8) {
        obj->pos = 0;
        uint64_t g = 0;
        for (int i = 0; i < 8; i++) {
            obj->out[i] = obj->x[i] ^ (g += 0x9E3779B97F4A7C15);
        }
        for (int i = 0; i < 7; i++) {
            mulbox128(obj->out, i, i + 1);
        }
        mulbox128(obj->out, 7, 0);

        for (int i = 7; i >= 0; i--) {
            mulbox128(obj->out, i, i - 1);
        }
        mulbox128(obj->out, 0, 7);


/*
        //-----------------------------
        mulbox128(obj->out, 1, 0);
        mulbox128(obj->out, 3, 2);
        mulbox128(obj->out, 5, 4);
        mulbox128(obj->out, 7, 6);

        mulbox128(obj->out, 2, 1);
        mulbox128(obj->out, 4, 3);
        mulbox128(obj->out, 6, 5);
        mulbox128(obj->out, 0, 7);
*/


/*
        * * | * * | * * | * *
        *|*   *|*   *|*   *|* 
*/


/*
        // Round 1
        for (int i = 0; i < 7; i++) {
            mulbox128(obj->out, i, i + 1);
        }
        mulbox128(obj->out, 7, 0);
        // Round 2
        mulbox128(obj->out, 1, 0);
        mulbox128(obj->out, 3, 2);
        mulbox128(obj->out, 5, 4);
        mulbox128(obj->out, 7, 6);
*/

        obj->x[7]++;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    Mularx512State *obj = intf->malloc(sizeof(Mularx512State));
    obj->pos = 8;
    for (int i = 0; i < 7; i++) {
        obj->x[i] = 0;
    }
    obj->x[7] = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT64_PRNG("Mularx512", NULL)
