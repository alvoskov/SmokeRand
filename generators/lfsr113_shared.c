/**
 * @file lfsr113_shared.c
 * @brief Combined 32-bit LFSR generator. Fails linear complexity
 * and matrix rank tests.
 *
 * References:
 * 1. L'Ecuyer P. Tables of Maximally Equidistributed Combined LFSR
 *    Generators // Mathematics of Computation. 1999. V. 68. N 225.
 *    P.261-269
 * 2. https://www-labs.iro.umontreal.ca/~simul/rng/lfsr113.c
*/
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief LFSR113 PRNG state.
 */
typedef struct {
    uint32_t y[4];
} Lfsr113State;

/**
 * @brief Creates LFSR example with taking into account
 * limitations on seeds (initial state).
 */
void *create(const CallerAPI *intf)
{
    uint32_t seeds_lb[] = {0x1, 0x7, 0xF, 0x7F};
    Lfsr113State *obj = intf->malloc(sizeof(Lfsr113State));
    for (int i = 0; i < 4; i++) {
        do {
            obj->y[i] = intf->get_seed32();
        } while (obj->y[i] <= seeds_lb[i]);
    }
    return obj;
}


#define ROUND(i, shl1, shr, mask, shl2) \
    b = ((y[(i)] << (shl1)) ^ y[(i)]) >> (shr); \
    y[(i)] = ((y[(i)] & mask) << (shl2)) ^ b;

static inline uint64_t get_bits_raw(void *state)
{
    Lfsr113State *obj = state;    
    uint32_t *y = obj->y, b;
    ROUND(0,  6, 13, 0xFFFFFFFE, 18)
    ROUND(1,  2, 27, 0xFFFFFFF8, 2)
    ROUND(2, 13, 21, 0xFFFFFFF0, 7)
    ROUND(3,  3, 12, 0xFFFFFF80, 13)
    return y[0] ^ y[1] ^ y[2] ^ y[3];
}

/**
 * @brief Internal self-test based on the original code 
 * by P.L'Ecuyer.
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint32_t seed = 987654321u;
    Lfsr113State obj;
    uint32_t u_ref[] = {
        0xFFC82E32, 0x36428E7D, 0x87B8571B, 0xFF169F0F,
        0x930EDB4F, 0xA10D951E, 0xF28102A2, 0x4FC27B17
    };
    for (int i = 0; i < 4; i++) {
        obj.y[i] = seed;
    }
    int is_ok = 1;
    for (int i = 0; i < 10000; i++) {
        get_bits_raw(&obj);
    }
    intf->printf("%8s %s\n", "Output", "Reference");
    for (int i = 0; i < 8; i++) {
        uint64_t u = get_bits_raw(&obj);
        intf->printf("0x%.8lX 0x%.8lX\n",
            (unsigned long) u, (unsigned long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT32_PRNG("LFSR113", run_self_test)
