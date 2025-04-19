/**
 * @file test_crand.c
 * @brief PRNG based on C rand() function. DON'T USE IN A MULTITHREADING
 * ENVIRONMENT! FOR EXPERIMENTAL PURPOSES ONLY!
 * @details The quality of this generator is entirely dependent on the
 * implementation of rand() function. Only one byte is taken from each
 * rand() output (the higher bits). At least two variants are possible:
 *
 * 1. MinGW and MSVC: some 32-bit modulo 2^{32} that returns the higher
 *    15 bits (bits 30..16). Fails almost everything.
 * 2. GCC (glibc): lagged Fibonacci PRNG with short lags initialized
 *    by minstd algorithm. Fails some modifications of `hamming_ot` test.
 *
 * This PRNG is also VERY SLOW and NOT THREAD SAFE!
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand_core.h"
#include "smokerand_bat.h"
#include <stdio.h>
#include <stdlib.h>

static uint64_t get_bits(void *state)
{
    uint32_t x = 0;
    for (int i = 0; i < 4; i++) {
        x = (x << 8) | ((rand() >> 7) & 0xFF);
    }
    (void) state;
    return x;
}

/**
 * @brief Just seeds PRNG from C standard library. So this PRNG has
 * no local state and IS NOT THREAD SAFE!
 * @details Even if some implementation of rand() includes mutexes -
 * it will distort the results of statistical tests due to effect similar
 * to decimation in ranlux.
 */
static void *gen_create(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    srand((unsigned int) intf->get_seed64());
    return intf->malloc(sizeof(uint64_t));
}

static void gen_free(void *state, const GeneratorInfo *info, const CallerAPI *intf)
{
    (void) info;
    intf->free(state);
}

int main()
{
    static const GeneratorInfo gen = {
        .name = "crand",
        .description = "rand() function test",
        .nbits = 32,
        .create = gen_create,
        .free = gen_free,
        .get_bits = get_bits,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    CallerAPI intf = CallerAPI_init_mthr();
    battery_speed(&gen, &intf);
    battery_default(&gen, &intf, TESTS_ALL, 4, REPORT_FULL);
    CallerAPI_free();
    return 0;
}
