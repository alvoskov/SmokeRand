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


int run_triples_search(const GeneratorInfo *gen,
    unsigned int max_value,
    int (*is_triple_valid)(unsigned int, unsigned int, unsigned int))
{
    const LfsrPeriodOptions opts = {.check_validity = 0};
    CallerAPI intf = CallerAPI_init();
    intf.printf = printf_null;

    GeneratorStateExt ext = GeneratorStateExt_create(gen, &intf);

    if (lfsr_period_test(&ext, &intf, &opts) == LFSR_PERIOD_ERROR) {
        printf("The xorshift implementation is damaged\n");
        GeneratorStateExt_destruct(&ext);
        CallerAPI_free();
        return 1;
    }

    unsigned int ntriples = 0;
    for (unsigned int ai = 1; ai < max_value; ai++) {
        for (unsigned int bi = 1; bi < max_value; bi++) {
            for (unsigned int ci = 1; ci < max_value; ci++) {
                a = ai; b = bi; c = ci;
                if (is_triple_valid(a, b, c) && lfsr_period_test(&ext, &intf, &opts) == LFSR_PERIOD_MAX) {
                    printf("[%2u %2u %2u]", a, b, c);
                    static const HammingDistrOptions
                        hw_distr = {.nvalues = 1ull << 30, .nlevels = 10};
                    TestResults hw_res = hamming_distr_test(&ext.state, &hw_distr);
                    printf(":%.3g ", hw_res.p);
                    fflush(stdout);
                    if (++ntriples % 4 == 0) {
                        printf("\n");
                    }
                }
            }
        }
    }
    printf("\nTotal number of triples: %u", ntriples);
    GeneratorStateExt_destruct(&ext);
    CallerAPI_free();
    return 0;
}

//////////////////////////////
///// xorshift64 testing /////
//////////////////////////////

typedef struct {
    uint64_t x;    
} Xorshift64State;

static uint64_t get_bits_xs64(void *state)
{
    Xorshift64State *obj = state;
    obj->x ^= obj->x >> a;
    obj->x ^= obj->x << b;
    obj->x ^= obj->x >> c;
    return obj->x;
}

static void *gen_create_xs64(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorshift64State *obj = intf->malloc(sizeof(Xorshift64State));
    obj->x = intf->get_seed64();
    return obj;
}


static int is_triple_valid_xs64(unsigned int ai, unsigned int bi, unsigned int ci)
{
    (void) bi;
    return ai <= ci;
}


static int is_triple_valid_generic(unsigned int ai, unsigned int bi, unsigned int ci)
{
    (void) ai; (void) bi; (void) ci;
    return 1;
}


int test_xorshift64(void)
{
    static const GeneratorInfo gen = {
        .name = "xorshift64:dynshifts",
        .description = "xorshift64 with dynamic shifts",
        .nbits = 64,
        .create = gen_create_xs64,
        .free = gen_free,
        .get_bits = get_bits_xs64,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    return run_triples_search(&gen, 64, is_triple_valid_xs64);
}





///////////////////////////////
///// xorshift128 testing /////
///////////////////////////////

typedef struct {
    uint32_t x;
    uint32_t y; 
    uint32_t z;
    uint32_t w;
} Xorshift128State;


static uint64_t get_bits_xs128(void *state)
{
    Xorshift128State *obj = state;
    uint32_t t = obj->x ^ (obj->x << a);
    t ^= t >> b;
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = (obj->w ^ (obj->w >> c)) ^ t;
    return obj->z;
}


static void *gen_create_xs128(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorshift128State *obj = intf->malloc(sizeof(Xorshift128State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->w = intf->get_seed32() | 0x1; // State mustn't be all zeros
    return obj;
}



int test_xorshift128(void)
{
    static const GeneratorInfo gen = {
        .name = "xorshift128:dynshifts",
        .description = "xorshift128 with dynamic shifts",
        .nbits = 32,
        .create = gen_create_xs128,
        .free = gen_free,
        .get_bits = get_bits_xs128,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    return run_triples_search(&gen, 32, is_triple_valid_generic);
}


///////////////////////////////
///// xorshift160 testing /////
///////////////////////////////

typedef struct {
    uint32_t x;
    uint32_t y; 
    uint32_t z;
    uint32_t w;
    uint32_t v;
} Xorshift160State;


static uint64_t get_bits_xs160(void *state)
{
    Xorshift160State *obj = state;
    uint32_t t = obj->x ^ (obj->x << a);
    t ^= t >> b;
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = obj->v;
    obj->v = (obj->v ^ (obj->v >> c)) ^ t;
    return obj->v;
}


static void *gen_create_xs160(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorshift160State *obj = intf->malloc(sizeof(Xorshift160State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->w = intf->get_seed32();
    obj->v = intf->get_seed32() | 0x1; // State mustn't be all zeros
    return obj;
}



int test_xorshift160(void)
{
    static const GeneratorInfo gen = {
        .name = "xorshift160:dynshifts",
        .description = "xorshift160 with dynamic shifts",
        .nbits = 32,
        .create = gen_create_xs160,
        .free = gen_free,
        .get_bits = get_bits_xs160,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    return run_triples_search(&gen, 64, is_triple_valid_generic);
}


///////////////////////////////
///// xorshift320 testing /////
///////////////////////////////

typedef struct {
    uint64_t x;
    uint64_t y; 
    uint64_t z;
    uint64_t w;
    uint64_t v;
} Xorshift320State;


static uint64_t get_bits_xs320(void *state)
{
    Xorshift320State *obj = state;
    uint64_t t = obj->x ^ (obj->x << a);
    t ^= t >> b;
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = obj->v;
    obj->v = (obj->v ^ (obj->v >> c)) ^ t;
    return obj->v;
}


static void *gen_create_xs320(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorshift320State *obj = intf->malloc(sizeof(Xorshift320State));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->w = intf->get_seed64();
    obj->v = intf->get_seed64() | 0x1; // State mustn't be all zeros
    return obj;
}



int test_xorshift320(void)
{
    static const GeneratorInfo gen = {
        .name = "xorshift320:dynshifts",
        .description = "xorshift320 with dynamic shifts",
        .nbits = 64,
        .create = gen_create_xs320,
        .free = gen_free,
        .get_bits = get_bits_xs320,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    return run_triples_search(&gen, 64, is_triple_valid_generic);
}



int main()
{
    //test_xorshift64();
    test_xorshift128();
}
