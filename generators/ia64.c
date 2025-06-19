// https://burtleburtle.net/bob/rand/isaac.html#IAcode
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


#define ALPHA      (8)
#define SIZE       (1<<ALPHA)
#define ind(x)     (x&(SIZE-1))

typedef struct {
    uint64_t m[SIZE]; ///< Memory: array of SIZE ALPHA-bit terms
    uint64_t bb; ///< Previous result
    int i;
} IaState;


static inline uint64_t get_bits_raw(void *state)
{
    IaState *obj = state;
    uint64_t x = obj->m[obj->i];
    uint64_t y = obj->m[ind(x)] + obj->bb;
    obj->m[obj->i] = y;                     // set m
    uint64_t r = obj->m[ind(y>>ALPHA)] + x; // set r
    obj->bb = r;
    obj->i++;
    obj->i = ind(obj->i);
    return r;
}


static void *create(const CallerAPI *intf)
{
    IaState *obj = intf->malloc(sizeof(IaState));
    uint64_t seed = intf->get_seed64();
    obj->i = 0;
    obj->bb = 0;
    for (int i = 0; i < SIZE; i++) {
        obj->m[i] = (uint32_t) pcg_bits64(&seed);
    }
    return obj;
}


MAKE_UINT64_PRNG("IA64", NULL)
