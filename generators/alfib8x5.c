/**
 * @file alfib8x5.c 
 * @brief A 4-tap additive lagged Fibonacci generator with output scrambler;
 * works only with bytes, doesn't use multiplication, may be used for 8-bit CPUs.
 * @details 
 *
 * References:
 *
 * 1. 10.1145/508366.508368
 * 2. https://eternityforest.com/doku/doku.php?id=tech:the_xabc_random_number_generator
 * 3. https://citeseerx.ist.psu.edu/document?repid=rep1&type=pdf&doi=3532b8a75efb3fe454c0d4dd68c1b09309d8288c
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

enum {
    LF8X3_BUFSIZE = 64,
    LF8X3_MASK = 0x3F
};

typedef struct {
    uint8_t x[LF8X3_BUFSIZE];
    uint8_t pos;
} Alfib8State;

static inline uint64_t get_bits8(Alfib8State *obj)
{
    uint8_t *x = obj->x;
    obj->pos++;
    uint8_t u = x[(obj->pos - 61) & LF8X3_MASK]
        + x[(obj->pos - 60) & LF8X3_MASK]
        + x[(obj->pos - 46) & LF8X3_MASK]
        + x[(obj->pos - 45) & LF8X3_MASK];
    x[obj->pos & LF8X3_MASK] = u;
    // Output scrambler
    u = u ^ (u >> 5);
    u = u + (u << 1);// + (u << 3) + (u << 6);
    return u;
}


static inline uint64_t get_bits_raw(void *state)
{
    union {
        uint8_t  u8[4];
        uint32_t u32;
    } out;
    for (int i = 0; i < 4; i++) {
        out.u8[i] = get_bits8(state);
    }
    return out.u32;    
}

static void Alfib8State_init(Alfib8State *obj, uint32_t seed)
{
    uint8_t x = seed & 0xFF;
    uint8_t a = (seed >> 8) & 0xFF;
    uint8_t b = (seed >> 16) & 0xFF;
    uint8_t c = (seed >> 24) & 0xFF;
    for (int i = 0; i < 32; i++) {
        a ^= c ^ (x += 151);
        b += a;
        c = (c + ((b << 7) | (b >> 1))) ^ a;
    }
    for (int i = 0; i < LF8X3_BUFSIZE; i++) {
        a ^= c ^ (x += 151);
        b += a;
        c = (c + ((b << 7) | (b >> 1))) ^ a;
        obj->x[i] = c ^ b;
    }
    obj->pos = 0;
}

static void *create(const CallerAPI *intf)
{
    Alfib8State *obj = intf->malloc(sizeof(Alfib8State));
    Alfib8State_init(obj, intf->get_seed32());
    return obj;
}

MAKE_UINT32_PRNG("Alfib8x5", NULL)
