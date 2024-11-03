/**
 * @file crand_shared.c
 * @brief PRNG based on C rand() function. DON'T USE IN A MULTITHREADING
 * ENVIRONMENT! FOR EXPERIMENTAL PURPOSES ONLY!
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include <stdio.h>
#include <stdlib.h>

static inline uint64_t get_bits_raw(void *state)
{
    uint32_t x = 0;
    for (int i = 0; i < 4; i++) {
        x = (x << 8) | ((rand() >> 7) & 0xFF);
    }
    (void) state;
    return x;
}


static void *create(const CallerAPI *intf)
{
    srand(intf->get_seed64());
    return NULL;
}

MAKE_UINT32_PRNG("crand", NULL)
