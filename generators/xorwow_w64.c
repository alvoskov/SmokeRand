// >= 2 TiB
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief Xorshift320 PRNG state
 */
typedef struct {
    uint64_t x;
    uint64_t y; 
    uint64_t z;
    uint64_t w;
    uint64_t v;
    uint64_t d1;
    uint64_t d2;
} XorwowW64State;


static inline uint64_t get_bits_raw(XorwowW64State *obj)
{
    const int a = 31, b = 1, c = 28;
    const uint64_t x0 = obj->x;
    uint64_t t = obj->x ^ (obj->x << a); // a
    t ^= t >> b; // b
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = obj->v;
    obj->v = (obj->v ^ (obj->v >> c)) ^ t; // c

    const uint64_t d1 = obj->d1;
    obj->d1 = d1 + obj->d2;
    obj->d2 = d1;

    return x0 + rotl64(d1, 1);
}


static void *create(const CallerAPI *intf)
{
    XorwowW64State *obj = intf->malloc(sizeof(XorwowW64State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->w = intf->get_seed64();
    obj->v = intf->get_seed64();
    obj->d1 = intf->get_seed64();
    obj->d2 = intf->get_seed64() | 0x1;
    if (obj->v == 0) { // State mustn't be all zeros
        obj->v = 0xDEADBEEF;
    }        
    return obj;
}

MAKE_UINT64_PRNG("Xorwow_w64", NULL)
