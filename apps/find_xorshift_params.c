/**
 * @file find_xorshift_params.c
 * @brief Finds the `[a b c]` shifts triples for the xorshift64 PRNG.
 * @details It uses the `lfsr_test_period` engine that checks if the LFSR
 * period is maximal or not.
 *
 * References:
 *
 * - Marsaglia G. Xorshift RNGs // Journal of Statistical Software. 2003.
 *   V. 8. N. 14. P.1-6. https://doi.org/10.18637/jss.v008.i14
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand_core.h"
#include "smokerand_bat.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned int a = 12;
static unsigned int b = 25;
static unsigned int c = 27;


typedef struct {
    uint64_t x;    
} Xorshift64State;

static uint64_t get_bits(void *state)
{
    Xorshift64State *obj = state;
    obj->x ^= obj->x >> a;
    obj->x ^= obj->x << b;
    obj->x ^= obj->x >> c;
    return obj->x;
}

static void *gen_create(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorshift64State *obj = intf->malloc(sizeof(Xorshift64State));
    obj->x = intf->get_seed64();
    return obj;
}

static void gen_free(void *state, const GeneratorInfo *info, const CallerAPI *intf)
{
    (void) info;
    intf->free(state);
}


static int printf_null(const char *format, ...)
{
    (void) format;
    return 0;
}


int main()
{
    const LfsrPeriodOptions opts = {.check_validity = 0};
    static const GeneratorInfo gen = {
        .name = "xorshift64:dynshifts",
        .description = "xorshift64 with dynamic shifts",
        .nbits = 64,
        .create = gen_create,
        .free = gen_free,
        .get_bits = get_bits,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    CallerAPI intf = CallerAPI_init();
    intf.printf = printf_null;

    if (lfsr_period_test(&gen, &intf, &opts) == LFSR_PERIOD_ERROR) {
        printf("The xorshift implementation is damaged\n");
        CallerAPI_free();
        return 1;
    }

    unsigned int ntriples = 0;
    for (unsigned int ai = 1; ai < 64; ai++) {
        for (unsigned int bi = 1; bi < 64; bi++) {
            for (unsigned int ci = 1; ci < 64; ci++) {
                a = ai; b = bi; c = ci;
                if (a <= c && lfsr_period_test(&gen, &intf, &opts) == LFSR_PERIOD_MAX) {
                    printf("[%2u %2u %2u] ", a, b, c);
                    fflush(stdout);
                    if (++ntriples % 9 == 0) {
                        printf("\n");
                    }
                }
            }
        }
    }
    printf("\nTotal number of triples: %u", ntriples);

    CallerAPI_free();
    return 0;
}
