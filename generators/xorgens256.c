// https://maths-people.anu.edu.au/~brent/pd/rpb224.pdf
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift256 PRNG state
 */
typedef struct {
    uint64_t x;
    uint64_t y; 
    uint64_t z;
    uint64_t w;
} Xorgens256State;

// r = 4, s = 3, a = 37, b = 27, c = 29, d = 33
static inline uint64_t get_bits_raw(Xorgens256State *obj)
{
    uint64_t a = obj->x ^ (obj->x << 37); a ^= a >> 27; // shifts (a,b)
    uint64_t b = obj->y ^ (obj->y << 29); b ^= b >> 33; // shifts (c,d)
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = a ^ b;
    return obj->w;
}


static void *create(const CallerAPI *intf)
{
    Xorgens256State *obj = intf->malloc(sizeof(Xorgens256State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    if (obj->w == 0) { // State mustn't be all zeros
        obj->w = 0xDEADBEEF;
    }
    return obj;
}

MAKE_UINT64_PRNG("Xorgens256", NULL)
