// PractaRand: 2 MiB
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief arxfw8ex PRNG state.
 */
typedef struct {
    uint8_t a;
    uint8_t b;
    uint16_t w;
} Arxfw8ExState;

static inline uint8_t get_bits8(Arxfw8ExState *obj)
{
    static const uint16_t inc = 0x9D;
    uint8_t a = obj->a, b = obj->b;
    const uint16_t w = obj->w;
    b = (uint8_t) (b + w);
    a = (uint8_t) (a + (rotl8(b, 1) ^ rotl8(b, 4) ^ b));
    a = (uint8_t) (a ^ (w >> 8));
    b = (uint8_t) (b ^ (rotl8(a, 7) + rotl8(a, 4) + a));
    obj->a = b;
    obj->b = a;
    obj->w = (uint16_t) (w + inc);
    return obj->a ^ obj->b;
}

static inline uint64_t get_bits_raw(Arxfw8ExState *state)
{
    const uint32_t a = get_bits8(state);
    const uint32_t b = get_bits8(state);
    const uint32_t c = get_bits8(state);
    const uint32_t d = get_bits8(state);
    return a | (b << 8) | (c << 16) | (d << 24);
}


static void *create(const CallerAPI *intf)
{
    Arxfw8ExState *obj = intf->malloc(sizeof(Arxfw8ExState));
    uint64_t seed = intf->get_seed64();
    obj->a = (uint8_t) seed;
    obj->b = (uint8_t) (seed >> 16);
    obj->w = (uint16_t) (seed >> 32);
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("arxfw8ex", NULL)
