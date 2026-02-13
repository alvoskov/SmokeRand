/**
 * https://doi.org/10.1007/978-3-642-31464-3_10
 */
#include "smokerand/cinterface.h"
#include <stdint.h>

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
} TycheiState;



static uint64_t get_bits_raw(TycheiState *obj)
{
    obj->b = rotl32(obj->b, 25) ^ obj->c; obj->c -= obj->d;
    obj->d = rotl32(obj->d, 24) ^ obj->a; obj->a -= obj->b;
    obj->b = rotl32(obj->b, 20) ^ obj->c; obj->c -= obj->d;
    obj->d = rotl32(obj->d, 16) ^ obj->a; obj->a -= obj->b;
    return obj->a;
}

static void *create(const CallerAPI *intf)
{
    TycheiState *obj = intf->malloc(sizeof(TycheiState));
    const uint64_t s = intf->get_seed64();
    obj->a = (uint32_t) s;
    obj->b = (uint32_t) (s >> 32);
    obj->c = 0x9B3779B9;
    obj->d = 0x517CC1B7;
    for (int i = 0; i < 20; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT32_PRNG("Tychei", NULL)
