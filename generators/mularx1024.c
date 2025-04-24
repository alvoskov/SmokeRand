/**
 * @file mularx1024.c
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
    uint64_t x[16];
    uint64_t out[16];
    int pos;
} Mularx1024State;


static inline void mulbox128(uint64_t *v, int i, int j)
{
    uint64_t a = 0xfc0072fa0b15f4fd;
    uint64_t mul0_high;
    v[i] = unsigned_mul128(a, v[i] ^ v[j], &mul0_high);
    v[j] ^= mul0_high;
    v[j] = v[j] + rotl64(v[i], 46);
    v[i] = v[i] ^ rotl64(v[j], 13);
}

static inline void arxbox128(uint64_t *v, int i, int j)
{
    v[j] = v[j] + rotl64(v[i], 46);
    v[i] = v[i] ^ rotl64(v[j], 13);
}

static inline uint64_t get_bits_raw(void *state)
{
    Mularx1024State *obj = state;
    if (obj->pos == 16) {
        uint64_t g = 0;
        obj->pos = 0;
        for (int i = 0; i < 16; i++) {
            obj->out[i] = obj->x[i] ^ (g += 0x9E3779B97F4A7C15);
        }
        for (int i = 0; i < 15; i++) {
            arxbox128(obj->out, i, i + 1);
        }
        arxbox128(obj->out, 15, 0);

        for (int i = 15; i >= 0; i--) {
            mulbox128(obj->out, i, i - 1);
        }
        mulbox128(obj->out, 0, 15);


/*
        for (int i = 1, j = 0; i <= 15; i += 2, j += 2) {
            mulbox128(obj->out, i, j);
        }
*/
/*
        arxbox128(obj->out, 1, 0);
        arxbox128(obj->out, 3, 2);
        arxbox128(obj->out, 5, 4);
        arxbox128(obj->out, 7, 6);
        arxbox128(obj->out, 9, 8);
        arxbox128(obj->out, 11, 10);
        arxbox128(obj->out, 13, 12);
        arxbox128(obj->out, 15, 14);
*/

        obj->x[7]++;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    Mularx1024State *obj = intf->malloc(sizeof(Mularx1024State));
    obj->pos = 16;
    for (int i = 0; i < 15; i++) {
        obj->x[i] = 0;
    }
    obj->x[15] = intf->get_seed64();
    return (void *) obj;
}

MAKE_UINT64_PRNG("Mularx1024", NULL)
