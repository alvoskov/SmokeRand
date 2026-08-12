/**
 * @file dandelion128.c
 * @brief dandelion128 is a xorshift-like LFSR that uses arithmetic right shift
 * and a non-bijective output scrambler.
 * @details This PRNG was designed by Aaron Pribadi. It uses the next
 * transition function and the next output function `F(x, y)`:
 *
 * - `T(x, y) = (y ^ lsl(x, sh1), x ^ asr(y, sh2)`
 * - `F(x, y) = ((x*x) % 2**64) + y) ^ ((x*x) // 2**64)`
 *
 * It has a proven period of \f$ 2^{128} - 1 \f$. Note: it also uses
 * an implementation defined behaviour of the `>>` operator for the signed
 * integers, but it is almost always implemented as an arithmetic shift.
 *
 * References:
 *
 * - https://github.com/apribadi/dandelion
 *
 * @copyright The dandelion64 PRNG was developed by Aaron Pribadi.
 *
 * Reentrant portable C99 implementation:
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x;
    uint64_t y;
} Dandelion128State;


static inline uint64_t get_bits_raw(Dandelion128State *obj)
{
    const uint64_t x = obj->x, y = obj->y;
    obj->x = y ^ (x << 7);
    obj->y = x ^ (uint64_t) ((int64_t) y >> 4);
    uint64_t hi, lo;
    lo = unsigned_mul128(x, x, &hi);
    return (lo + y) ^ hi;
}

static void *create(const CallerAPI *intf)
{
    Dandelion128State *obj = intf->malloc(sizeof(Dandelion128State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    if (obj->x == 0 && obj->y == 0) {
        obj->x = 0xDEADBEEF12345678;
        obj->y = 0x12345678DEADBEEF;
    }
    return obj;    
}

/**
 * @brief An internal self-test based on the values from the tests
 * of the existing reference implementation.
 * @details References:
 *
 * - https://github.com/apribadi/dandelion/blob/master/tests/unified.rs
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t x_ref[10] = {
        0x0000000000000001, 0x0000000000004001,
        0x0000000010008081, 0x0000040000004009,
        0x0100080130248451, 0x0000080010028009,
        0x11028815223439d5, 0x0010040490120211,
        0xe317498d281508a5, 0x41122e4dd5f25bf7
    };

    Dandelion128State obj = {.x = 1, .y = 0};
    int is_ok = 1;
    for (int i = 0; i < 10; i++) {
        const uint64_t x = get_bits_raw(&obj);
        intf->printf("%llX %llX\n",
            (unsigned long long) x, (unsigned long long) x_ref[i]);
        if (x != x_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT64_PRNG("dandelion128", run_self_test)
