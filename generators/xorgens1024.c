// https://maths-people.anu.edu.au/~brent/pd/rpb224.pdf
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift256 PRNG state
 */
typedef struct {
    uint64_t x[16];
} Xorgens1024State;

// n 1024 w 64 r 16  s 7  a 34 b 29 c 25 d 31  Wt 439 delta 25
static inline uint64_t get_bits_raw(Xorgens1024State *obj)
{
    uint64_t a = obj->x[0] ^ (obj->x[0] << 34); a ^= a >> 29; // shifts (a,b)
    uint64_t b = obj->x[9] ^ (obj->x[9] << 25); b ^= b >> 31; // shifts (c,d)
    for (int i = 0; i < 15; i++) {
        obj->x[i] = obj->x[i + 1];
    }
    obj->x[15] = a ^ b;
    return obj->x[15];
}


static void *create(const CallerAPI *intf)
{
    Xorgens1024State *obj = intf->malloc(sizeof(Xorgens1024State));
    seeds_to_array_u64(intf, obj->x, 16);
    if (obj->x[15] == 0) { // State mustn't be all zeros
        obj->x[15] = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("Xorgens1024", NULL)
