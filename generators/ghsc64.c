// https://github.com/wprsns/ghostscramble/blob/master/ghostscramble.c
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
} GhSc64State;


static inline uint64_t get_bits_raw(GhSc64State *s)
{
    s->a = rotl64(s->a, 29) ^ s->b;
    s->b += 1111111111111111;
    s->c = rotl64(s->c, 41) + s->a;
    return s->c;
}


static void *create(const CallerAPI *intf)
{
    GhSc64State *obj = intf->malloc(sizeof(GhSc64State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64();
    for (int i = 0; i < 16; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT64_PRNG("GhostScramble64", NULL)
