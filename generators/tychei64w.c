/**
 */
#include "smokerand/cinterface.h"
#include <stdint.h>

PRNG_CMODULE_PROLOG


typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
    uint64_t ctr;
} Tychei64WeylState;



static uint64_t get_bits_raw(Tychei64WeylState *obj)
{
    obj->c += obj->ctr++;
    obj->b = rotl64(obj->b, 63) ^ obj->c; obj->c -= obj->d;
    obj->d = rotl64(obj->d, 16) ^ obj->a; obj->a -= obj->b;
    obj->b = rotl64(obj->b, 24) ^ obj->c; obj->c -= obj->d;
    obj->d = rotl64(obj->d, 32) ^ obj->a; obj->a -= obj->b;
    return obj->a;
}

static void *create(const CallerAPI *intf)
{
    Tychei64WeylState *obj = intf->malloc(sizeof(Tychei64WeylState));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = 0x517CC1B727220A95;
    obj->d = 0x9E3779B97F4A7C15;
    obj->ctr = 0;
    for (int i = 0; i < 20; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT64_PRNG("Tychei64-Weyl", NULL)
