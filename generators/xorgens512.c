// https://maths-people.anu.edu.au/~brent/pd/rpb224.pdf
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift256 PRNG state
 */
typedef struct {
    uint64_t x[8];
} Xorgens512State;

// r = 8, s = 1, a = 37, b = 26, c = 29, d = 34
static inline uint64_t get_bits_raw(Xorgens512State *obj)
{
    uint64_t a = obj->x[0] ^ (obj->x[0] << 37); a ^= a >> 26; // shifts (a,b)
    uint64_t b = obj->x[7] ^ (obj->x[7] << 29); b ^= b >> 34; // shifts (c,d)
    obj->x[0] = obj->x[1];
    obj->x[1] = obj->x[2];
    obj->x[2] = obj->x[3];
    obj->x[3] = obj->x[4];
    obj->x[4] = obj->x[5];
    obj->x[5] = obj->x[6];
    obj->x[6] = obj->x[7];
    obj->x[7] = a ^ b;
    return obj->x[7];
}


static void *create(const CallerAPI *intf)
{
    Xorgens512State *obj = intf->malloc(sizeof(Xorgens512State));
    seeds_to_array_u64(intf, obj->x, 8);
    if (obj->x[7] == 0) { // State mustn't be all zeros
        obj->x[7] = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("Xorgens512", NULL)
