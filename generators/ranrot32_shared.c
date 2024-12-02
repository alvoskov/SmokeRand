#include "smokerand/cinterface.h"

// Note: LAG1 > LAG2 > 0
#define LAG1 17
#define LAG2 9
#define ROT1 9
#define ROT2 13

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t x[LAG1];
    size_t pos;
} RanRot32State;


static inline uint32_t rotl32(uint32_t x, unsigned int r)
{
    return (x << r) | (x >> (32 - r));
}


static inline uint64_t get_bits_raw(void *state)
{
    RanRot32State *obj = state;
    uint32_t *x = obj->x;
    if (obj->pos != 0) {
        return x[--obj->pos];
    }
    for (size_t i = 0; i < LAG2; i++) {
        x[i] = rotl32(x[i + LAG1 - LAG1], ROT1) + rotl32(x[i + LAG1 - LAG2], ROT2);
    }
    for (size_t i = LAG2; i < LAG1; i++) {
        x[i] = rotl32(x[i - 0], ROT1) + rotl32(x[i - LAG2], ROT2);
    }
    obj->pos = LAG1;
    return x[--obj->pos];
}


static void *create(const CallerAPI *intf)
{
    RanRot32State *obj = intf->malloc(sizeof(RanRot32State));
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    for (int i = 0; i < LAG1; i++) {
        obj->x[i] = pcg_bits64(&state) >> 32;
    }
    obj->pos = 0; // Mark buffer uninitialized
    return (void *) obj;
}



MAKE_UINT32_PRNG("ranrot32", NULL)
