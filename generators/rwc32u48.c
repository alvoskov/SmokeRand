/**
 * @file rwc32u48.c
 * @brief A modification of rwc32 for 48-bit integers, useful for MS Excel,
 * LibreOffice Calc etc.
 * @details Its period is around 2^109, may be implemented even in the case
 * when only double precision numbers are available. 48 bit limitation is
 * due to limitations of MS Excel, it also simplified porting to 16-bit
 * platforms.
 *
 * References:
 *
 * 1. https://www.stat.berkeley.edu/~spector/s243/mother.c
 * 2. M. Goresky, A. Klapper. Efficient multiply-with-carry random number
 *    generators with maximal period // ACM Trans. Model. Comput. Simul. 2003.
 *    V. 13. N 4. P. 310-321. https://doi.org/10.1145/945511.945514
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t c;
} Rwc32State;


static inline uint64_t get_bits_raw(Rwc32State *obj)
{
    const uint64_t new = 29386ULL*((uint64_t)obj->y + obj->z) + obj->c;
    obj->z = obj->y;
    obj->y = obj->x;
    obj->x = (uint32_t) new;
    obj->c = (uint32_t) (new >> 32);
    return obj->x;
}


static void *create(const CallerAPI *intf)
{
    Rwc32State *obj = intf->malloc(sizeof(Rwc32State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->c = 1;
    return obj;
}


static int run_self_test(const CallerAPI *intf)
{
    Rwc32State obj = {.x = 12345678, .y = 87654321, .z = 12345, .c = 12345};
    const uint32_t u_ref = 0x73A18CE2;
    for (long i = 0; i < 1000000; i++) {
        (void) get_bits_raw(&obj);
    }
    const uint32_t u = (uint32_t) get_bits_raw(&obj);
    intf->printf("c = 0x%lX; x = 0x%lX; y = 0x%lX; z = 0x%lX\n",
        (unsigned long) obj.c, (unsigned long) obj.x,
        (unsigned long) obj.y, (unsigned long) obj.z);
    intf->printf("Out=0x%lX; ref=0x%lX\n",
        (unsigned long) u, (unsigned long) u_ref);
    return u == u_ref ? 1 : 0;
    
}


MAKE_UINT32_PRNG("rwc32u48", run_self_test)
