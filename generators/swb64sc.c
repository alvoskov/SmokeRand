#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_R 9
#define SWB_S 8

typedef struct {    
    uint64_t x[SWB_R];
    uint64_t c;
    size_t pos;
} Swb64State;


static inline uint64_t u64_swb(uint64_t x, uint64_t y, uint64_t b_in, uint64_t *b_out)
{
//    return __builtin_subcll(x, y, b_in, b_out);
/*
    uint64_t diff1 = x - y;
    uint8_t borrow1 = (x < y) ? 1 : 0;
    
    uint64_t diff2 = diff1 - b_in;
    uint8_t borrow2 = (diff1 < b_in) ? 1 : 0;
    
    *b_out = borrow1 | borrow2;
    return diff2;
*/

    uint64_t ans = x - y - b_in;
    *b_out = (ans > x) ? 1 : 0;
    return ans;
}

static inline uint64_t get_bits_raw(Swb64State *obj)
{
    if (obj->pos == SWB_R) {
        for (int i = 0; i < SWB_S; i++) {
            obj->x[i] = u64_swb(obj->x[i + (SWB_R - SWB_S)], obj->x[i], obj->c, &obj->c);
        }
        for (int i = SWB_S; i < SWB_R; i++) {
            obj->x[i] = u64_swb(obj->x[i - SWB_S], obj->x[i], obj->c, &obj->c);
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
