#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint16_t m1[10]; ///< (c, x_{n-1}, x_{n-2}, ...)
    uint16_t m2[10]; ///< (c, x_{n-1}, x_{n-2}, ...)
} MAllState;


static uint64_t get_bits_raw(MAllState *obj)
{
    uint16_t *m1 = obj->m1, *m2 = obj->m2;
    // Move elements 1 to 8 to 2 to 9
    for (size_t i = 1; i < 9; i++) { m1[i + 1] = m1[i]; }
    for (size_t i = 1; i < 9; i++) { m2[i + 1] = m2[i]; }
    // PRNG iteration
    uint32_t num1 = (uint32_t) ((uint32_t)m1[0] + // Carry
        1941UL*m1[2] + 1860UL*m1[3] + 1812UL*m1[4] + 1776UL*m1[5] +
        1492UL*m1[6] + 1215UL*m1[7] + 1066UL*m1[8] + 12013UL*m1[9]);
    uint32_t num2 = (uint32_t) ((uint32_t)m2[0] + // Carry
        1111UL*m2[2] + 2222UL*m2[3] + 3333UL*m2[4] + 4444UL*m2[5] + 
        5555UL*m2[6] + 6666UL*m2[7] + 7777UL*m2[8] + 9272UL*m2[9]);
    // Calculate new carries/values pairs
    m1[0] = (uint16_t) (num1 >> 16); m1[1] = (uint16_t) (num1 & 0xFFFFU);
    m2[0] = (uint16_t) (num2 >> 16); m2[1] = (uint16_t) (num2 & 0xFFFFU);
    // Concatenate the 32-bit output
    return ((uint32_t)m1[1] << 16) | (uint32_t)m2[1];
}


static void *create(const CallerAPI *intf)
{
    MAllState *obj = intf->malloc(sizeof(MAllState));
    obj->m1[0] = 0xAB;
    obj->m2[0] = 0xCD;
    for (int i = 1; i < 10; i++) {    
        const uint32_t s = intf->get_seed32();
        obj->m1[i] = (uint16_t) s;
        obj->m1[i] = (uint16_t) (s >> 16);
    }
    return obj;
}

MAKE_UINT32_PRNG("Mall", NULL)
