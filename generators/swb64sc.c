/*
(9,8)>=2 TiB; smokerand full
def rotl64(x, r):
    return ((x << r) | (x >> (64 - r))) % 2**64

class Swb64:
    def __init__(self):
        self.r, self.s = 13, 7
        self.x = [x + 1000 for x in range(0, self.r)]
        self.x[0] = 2**64 - 1
        self.x[self.r - self.s] = 0
        self.c = 1

    @staticmethod
    def scramble(x):
        t = (x + (x * x | 0x40000005)) % 2**64
        return t ^ rotl64(t, 13) ^ rotl64(t, 47)
        
    def next(self):
        xj, xi = self.x[self.r - self.s], self.x[0]
        d = xj - xi - self.c
        xn = d % 2**64
        self.c = 1 if d < 0 else 0
        self.x = self.x[1:] + [xn]
        return self.scramble(xn)

swb = Swb64()

for i in range(1_000_000):
    swb.next()

for i in range(16):
    print(hex(swb.next()))
*/

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_R 9
#define SWB_S 8

typedef struct {    
    uint64_t x[SWB_R];
    uint64_t c;
    size_t pos;
} Swb64State;


static inline uint64_t get_bits_raw(Swb64State *obj)
{
    if (obj->pos == SWB_R) {
        for (int i = 0; i < SWB_S; i++) {
            obj->x[i] = swb_u64(obj->x[i + (SWB_R - SWB_S)], obj->x[i], obj->c, &obj->c);
        }
        for (int i = SWB_S; i < SWB_R; i++) {
            obj->x[i] = swb_u64(obj->x[i - SWB_S], obj->x[i], obj->c, &obj->c);
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


static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[16] = { // For r=13, s=7
        0xd4076b83f48cb3df, 0xf333cb436a9ec32,
        0xace36c03140c2397, 0xed3a1bf9d97c9a39,
        0x491fee38d8d58df0, 0xaddb1325a17b8f95,
        0x4369d4b71fde91b4, 0x422fd412aeede7b,
        0x503ad2a192ecbec2, 0x1c80c03efd4bc81b,
        0x69940b6b875d05ab, 0x841f711f72fa7bb8,
        0x9c84ca32fabb8ec0, 0x5b4856e5b0539486,
        0xc2ff6d108c16cd47, 0x174838608b788585
    };
    Swb64State *obj = create(intf);
    for (size_t i = 0; i < SWB_R; i++) {
        obj->x[i] = 1000U + i;
    }
    obj->x[0] = 0xFFFFFFFFFFFFFFFFU;
    obj->x[SWB_R - SWB_S] = 0;
    obj->c = 1;

    for (long i = 0; i < 1000000; i++) {
        (void) get_bits_raw(obj);
    }
    int is_ok = 1;
    for (size_t i = 0; i < 16; i++) {
        const uint64_t u = get_bits_raw(obj);
        intf->printf("%16.16llX %16.16llX\n",
            (unsigned long long) u, (unsigned long long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}

MAKE_UINT64_PRNG("SWB64SC", run_self_test)
