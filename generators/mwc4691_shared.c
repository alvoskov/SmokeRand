/**
 * @details https://www.thecodingforums.com/threads/kiss4691-a-potentially-top-ranked-rng.729111/
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC4691 state.
 */
typedef struct {
    uint32_t Q[4691];
    int c;
    int j;
} Mwc4691State;

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t t, x;
    Mwc4691State *obj = state;
    obj->j = (obj->j < 4690) ? obj->j + 1 : 0;
    x = obj->Q[obj->j];
    t = (x << 13) + obj-> c + x;
    obj->c = (t < x) + (x >> 19);
    obj->Q[obj->j] = t;
    return obj->Q[obj->j];
}

static void Mwc4691State_init(Mwc4691State *obj, uint32_t xcng, uint32_t xs)
{
    for (int i = 0; i < 4691; i++) {
        xcng = 69069 * xcng + 123;
        xs ^= (xs << 13);
        xs ^= (xs >> 17);
        xs ^= (xs << 5);
        obj->Q[i] = xcng + xs;
    }
    obj->c = 0;
    obj->j = 4691;
}


static void *create(const CallerAPI *intf)
{
    Mwc4691State *obj = intf->malloc(sizeof(Mwc4691State));
    uint64_t seed = intf->get_seed64();
    Mwc4691State_init(obj, seed >> 32, (seed & 0xFFFFFFFF) | 1);
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
    uint32_t x, x_ref = 3740121002;
    Mwc4691State *obj = intf->malloc(sizeof(Mwc4691State));
    Mwc4691State_init(obj, 521288629, 362436069); 
    for (unsigned long i = 0; i < 1000000000;i++) {
        x = get_bits_raw(obj);
    }
    intf->printf("x = %22u; x_ref = %22u\n", x, x_ref);
    intf->free(obj);
    return x == x_ref;
}


MAKE_UINT32_PRNG("Mwc4691", run_self_test)

/*
int main()
{unsigned long i,x;
for(i=0;i<4691;i++) Q=CNG+XS;
for(i=0;i<1000000000;i++) x=MWC();
printf(" MWC result=3740121002 ?\n%22u\n",x);
for(i=0;i<1000000000;i++) x=KISS;
printf("KISS result=2224631993 ?\n%22u\n",x);
}
*/