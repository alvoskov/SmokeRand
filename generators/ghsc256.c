// https://github.com/wprsns/ghostscramble/blob/master/ghostscramble.c
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t output[4];
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
    int pos;
} GhSc256State;


static inline uint64_t get_bits_raw(GhSc256State *s)
{
    if (s->pos == 4) {
        s->a = rotl64(s->a, 29) ^ s->b;
        s->b += 111111111111111111;
        s->output[0] = s->a + s->c;
        s->c = rotl64(s->c, 47) + s->a;
        s->output[1] = (s->a + s->b) ^ s->c;
        s->output[2] = s->a ^ s->d;
        s->d = rotl64(s->d, 25) + s->a;
        s->output[3] = s->a + rotl64(s->d, 21);
        s->pos = 0;
    }
    return s->output[s->pos++];
}


static void *create(const CallerAPI *intf)
{
    GhSc256State *obj = intf->malloc(sizeof(GhSc256State));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = intf->get_seed64();
    obj->d = intf->get_seed64();
    for (int i = 0; i < 16; i++) {
        (void) get_bits_raw(obj);
    }
    obj->pos = 4;    
    return obj;
}

MAKE_UINT64_PRNG("GhostScramble256", NULL)
