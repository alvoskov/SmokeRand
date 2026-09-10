#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_R 9
#define SWB_S 8

typedef struct {    
    uint64_t x[SWB_R];
    unsigned int c;
    size_t pos;
} Swb64State;


static inline uint64_t get_bits_raw(Swb64State *obj)
{
    if (obj->pos == SWB_R) {
        for (int i = 0; i < SWB_S; i++) {
            const uint64_t xr = obj->x[i], xs = obj->x[i + (SWB_R - SWB_S)];
            obj->x[i] = xs - xr - obj->c;
            obj->c = (xs < obj->x[i]) ? 1 : 0;
        }
        for (int i = SWB_S; i < SWB_R; i++) {
            const uint64_t xr = obj->x[i], xs = obj->x[i - SWB_S];
            obj->x[i] = xs - xr - obj->c;
            obj->c = (xs < obj->x[i]) ? 1 : 0;
        }
        obj->pos = 0;
    }
    uint64_t out = obj->x[obj->pos++];
    out += out * out | 0x40000005;
    out ^= rotl64(out, 13) ^ rotl64(out, 47);
    return out;
}


static void *create(const CallerAPI *intf)
{
    Swb64State *obj = intf->malloc(sizeof(Swb64State));
    expand_seed64_to_u64(obj->x, SWB_R, intf->get_seed64());
    obj->c = (obj->x[0] == 0) ? 1 : 0;
    obj->pos = SWB_R;
    return obj;
}

MAKE_UINT64_PRNG("SWB64SC", NULL)
