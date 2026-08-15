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
#include "smokerand/threads_intf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int get_default_nthreads(void)
{
    unsigned int nthreads = get_cpu_numcores();
//    if (sizeof(size_t) == 4 * sizeof(char) && nthreads > 2) {
  //      nthreads = 2;
    //}
    if (nthreads > 4)
        nthreads--;
    return nthreads;
}


static void gen_free(void *state, const GeneratorInfo *info, const CallerAPI *intf)
{
    (void) info;
    intf->free(state);
}


int printf_null(const char *format, ...)
{
    (void) format;
    return 0;
}


typedef struct {
    unsigned int a;
    unsigned int b;
    unsigned int c;
    unsigned int thrd_ord;
    int is_good;
    double pvalue;
} ShiftsTriple;


ShiftsTriple *make_triples_array(unsigned int max_value,
    int (*is_triple_valid)(unsigned int, unsigned int, unsigned int),
    unsigned int nthreads)
{
    size_t len = 0;
    for (unsigned int ai = 1; ai < max_value; ai++) {
        for (unsigned int bi = 1; bi < max_value; bi++) {
            for (unsigned int ci = 1; ci < max_value; ci++) {
                if (is_triple_valid(ai, bi, ci)) {
                    len++;
                }
            }
        }
    }
    ShiftsTriple *triples = calloc(len + 1, sizeof(ShiftsTriple));
    size_t pos = 0;
    for (unsigned int ai = 1; ai < max_value; ai++) {
        for (unsigned int bi = 1; bi < max_value; bi++) {
            for (unsigned int ci = 1; ci < max_value; ci++) {
                if (is_triple_valid(ai, bi, ci)) {
                    triples[pos].a = ai;
                    triples[pos].b = bi;
                    triples[pos].c = ci;
                    triples[pos].thrd_ord = (unsigned int) (pos % nthreads);
                    triples[pos].is_good = 0;
                    triples[pos].pvalue = 0.0;
                    pos++;
                }
            }
        }
    }
    return triples;
}


typedef struct {
    unsigned int max_value; ///< Max shift value
    size_t nbytes; ///< State size, bytes
    int (*is_triple_valid)(unsigned int, unsigned int, unsigned int);
    void (*set_triple)(void *state, unsigned int, unsigned int, unsigned int);
} XorshiftProps;


typedef struct {
    const GeneratorInfo *gi;
    const CallerAPI *intf;
    const XorshiftProps *gen_props;
    ShiftsTriple *triples;
} XsThreadData;


ThreadRetVal THREADFUNC_SPEC xorshift_thread(void *data)
{
    LfsrPeriodOptions opts;
    opts.check_validity = 1;

    XsThreadData *obj = data;
    ThreadObj thrd = ThreadObj_current();
    GeneratorStateExt ext = GeneratorStateExt_create_sized(obj->gi, obj->intf, obj->gen_props->nbytes);
    for (ShiftsTriple *t = obj->triples; t->a != 0; t++) {
        if (t->thrd_ord == thrd.ord) {
            if (t->b == 1 && t->c == 1) {
                printf("_%u", t->a);
            } else {
                printf(".");
                fflush(stdout);
            }
            obj->gen_props->set_triple(ext.state.state, t->a, t->b, t->c);
            if (lfsr_period_test(&ext, obj->intf, &opts) == LFSR_PERIOD_MAX) {
                static const HammingDistrOptions
                    hw_distr = {.nvalues = 1ull << 33, .nlevels = 10}; // or << 30 for faster screening
                static const HammingDistrOptions
                    hw_distr_sm = {.nvalues = 1ull << 28, .nlevels = 10};
                const TestResults hw_res = hamming_distr_test(
                    &ext.state,
                    (ext.nbytes > 8) ? &hw_distr : &hw_distr_sm);
                printf("<%u>[%u %u %u]:%g", thrd.ord, t->a, t->b, t->c, hw_res.p);
                t->is_good = 1;
                t->pvalue = hw_res.p;
            }
        }
    }
    GeneratorStateExt_destruct(&ext);
    return 0;
}


int run_triples_search(const GeneratorInfo *gen, const XorshiftProps *props)
{
    LfsrPeriodOptions opts;
    opts.check_validity = 1;

    const unsigned int nthreads = get_default_nthreads();
    CallerAPI intf = CallerAPI_init_mthr();
    intf.printf = printf_null;
    printf("Number of threads: %u\n", nthreads);

    GeneratorStateExt ext = GeneratorStateExt_create_sized(gen, &intf, props->nbytes);

    if (lfsr_period_test(&ext, &intf, &opts) == LFSR_PERIOD_ERROR) {
        printf("The xorshift implementation is damaged\n");
        GeneratorStateExt_destruct(&ext);
        CallerAPI_free();
        return 1;
    }
    opts.check_validity = 0;

    ShiftsTriple *triples = make_triples_array(props->max_value, props->is_triple_valid, nthreads);

    XsThreadData thread_data;
    thread_data.gi = gen;
    thread_data.intf = &intf;
    thread_data.gen_props = props;
    thread_data.triples = triples;


    // Run threads
    ThreadObj *thrd = calloc(nthreads, sizeof(ThreadObj));
    for (unsigned int ord = 0; ord < nthreads; ord++) {
        thrd[ord] = ThreadObj_create(xorshift_thread, &thread_data, ord);
    }
    // Get data from threads
    for (unsigned int i = 0; i < nthreads; i++) {
        ThreadObj_wait(&thrd[i]);
    }

    unsigned int ntriples = 0;
    printf("\n\n");
    for (ShiftsTriple *t = triples; t->a != 0; t++) {
        if (t->is_good) {
            printf("[%2u %2u %2u]:%.3g ", t->a, t->b, t->c, t->pvalue);
            if (++ntriples % 4 == 0) {
                printf("\n");
            }
        }
    }

    printf("\nTotal number of triples: %u", ntriples);


    free(triples);
    free(thrd);
    GeneratorStateExt_destruct(&ext);
    CallerAPI_free();
    return 0;
}


static int is_triple_valid_generic(unsigned int ai, unsigned int bi, unsigned int ci)
{
    (void) ai; (void) bi; (void) ci;
    return 1;
}



//////////////////////////////
///// xorshift32 testing /////
//////////////////////////////

typedef struct {
    uint32_t x;
    unsigned int a;
    unsigned int b;
    unsigned int c;
} Xorshift32VarShiftsState;

static uint64_t get_bits_xs32(void *state)
{
    Xorshift32VarShiftsState *obj = state;
    obj->x ^= obj->x >> obj->a;
    obj->x ^= obj->x << obj->b;
    obj->x ^= obj->x >> obj->c;
    return obj->x;
}

static void *gen_create_xs32(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorshift32VarShiftsState *obj = intf->malloc(sizeof(Xorshift32VarShiftsState));
    obj->x = intf->get_seed32();
    obj->a = 13;
    obj->b = 17;
    obj->c = 5;
    return obj;
}


static int is_triple_valid_xs32(unsigned int ai, unsigned int bi, unsigned int ci)
{
    (void) bi;
    return ai <= ci;
}

static void set_triple_xs32(void *state, unsigned int ai, unsigned int bi, unsigned int ci)
{
    Xorshift32VarShiftsState *obj = state;
    obj->a = ai; obj->b = bi; obj->c = ci;
}


int test_xorshift32(void)
{
    static const GeneratorInfo gen = {
        .name = "xorshift32:dynshifts",
        .description = "xorshift32 with dynamic shifts",
        .nbits = 32,
        .create = gen_create_xs32,
        .free = gen_free,
        .get_bits = get_bits_xs32,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    static const XorshiftProps props = {
        .max_value = 32,
        .nbytes = 4,
        .is_triple_valid = is_triple_valid_xs32,
        .set_triple = set_triple_xs32
    };

    return run_triples_search(&gen, &props);
}


//////////////////////////////
///// xorshift64 testing /////
//////////////////////////////

typedef struct {
    uint64_t x;
    unsigned int a;
    unsigned int b;
    unsigned int c;
} Xorshift64VarShiftsState;

static uint64_t get_bits_xs64(void *state)
{
    Xorshift64VarShiftsState *obj = state;
    obj->x ^= obj->x >> obj->a;
    obj->x ^= obj->x << obj->b;
    obj->x ^= obj->x >> obj->c;
    return obj->x;
}

static void *gen_create_xs64(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorshift64VarShiftsState *obj = intf->malloc(sizeof(Xorshift64VarShiftsState));
    obj->x = intf->get_seed64();
    obj->a = 12;
    obj->b = 25;
    obj->c = 27;
    return obj;
}


static int is_triple_valid_xs64(unsigned int ai, unsigned int bi, unsigned int ci)
{
    (void) bi;
    return ai <= ci;
}

static void set_triple_xs64(void *state, unsigned int ai, unsigned int bi, unsigned int ci)
{
    Xorshift64VarShiftsState *obj = state;
    obj->a = ai; obj->b = bi; obj->c = ci;
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

    static const XorshiftProps props = {
        .max_value = 64,
        .nbytes = 8,
        .is_triple_valid = is_triple_valid_xs64,
        .set_triple = set_triple_xs64
    };

    return run_triples_search(&gen, &props);
}



//////////////////////////////////
///// xoroshiro64w16 testing /////
//////////////////////////////////

typedef struct {
    uint16_t s[4];
    int a;
    int b;
    int c;
} Xoroshiro64w16VarShiftsState;


static uint16_t get_bits16_xo64w16(Xoroshiro64w16VarShiftsState *obj)
{
    const uint16_t s0 = obj->s[0];
    const uint16_t s0x3 = (uint16_t) (s0 ^ obj->s[3]);
    obj->s[0] = obj->s[1];
    obj->s[1] = obj->s[2];
    obj->s[2] = (uint16_t) (rotl16(s0, obj->a) ^ s0x3 ^ (s0x3 << obj->b));
    obj->s[3] = rotl16(s0x3, obj->c);
    return obj->s[3];
}


static uint64_t get_bits_xo64w16(void *obj)
{
    const uint32_t hi = get_bits16_xo64w16(obj);
    const uint32_t lo = get_bits16_xo64w16(obj);
    return (hi << 16) | lo;
}

static void *gen_create_xo64w16(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xoroshiro64w16VarShiftsState *obj = intf->malloc(sizeof(Xoroshiro64w16VarShiftsState));
    obj->s[0] = (uint16_t) intf->get_seed64();
    obj->s[1] = (uint16_t) intf->get_seed64();
    obj->s[2] = (uint16_t) intf->get_seed64();
    obj->s[3] = (uint16_t) intf->get_seed64();
    obj->a = 5;
    obj->b = 10;
    obj->c = 5;
    return obj;
}


static void set_triple_xo64w16(void *state, unsigned int ai, unsigned int bi, unsigned int ci)
{
    Xoroshiro64w16VarShiftsState *obj = state;
    obj->a = (int) ai; obj->b = (int) bi; obj->c = (int) ci;
}


int test_xoshiro64w16(void)
{
    static const GeneratorInfo gen = {
        .name = "xoroshiro64w16:dynshifts",
        .description = "xoroshiro64w16 with dynamic shifts",
        .nbits = 32,
        .create = gen_create_xo64w16,
        .free = gen_free,
        .get_bits = get_bits_xo64w16,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    static const XorshiftProps props = {
        .max_value = 16,
        .nbytes = 8,
        .is_triple_valid = is_triple_valid_generic,
        .set_triple = set_triple_xo64w16
    };

    return run_triples_search(&gen, &props);
}


/////////////////////////////////
///// xoroshiro48w8 testing /////
/////////////////////////////////

typedef struct {
    uint8_t s[6];
    int a;
    int b;
    int c;
} Xoroshiro48w8VarShiftsState;


static uint8_t get_bits8_xo48w8(Xoroshiro48w8VarShiftsState *obj)
{
    const uint8_t s0 = obj->s[0];
    const uint8_t s0x5 = (uint8_t) (s0 ^ obj->s[5]);
    obj->s[0] = obj->s[1];
    obj->s[1] = obj->s[2];
    obj->s[2] = obj->s[3];
    obj->s[3] = obj->s[4];
    obj->s[4] = (uint8_t) (rotl8(s0, obj->a) ^ s0x5 ^ (s0x5 << obj->b));
    obj->s[5] = rotl8(s0x5, obj->c);
    return obj->s[5];
}


static uint64_t get_bits_xo48w8(void *obj)
{
    uint32_t x = 0;
    for (int i = 0; i < 4; i++) {
        x <<= 8;
        x |= get_bits8_xo48w8(obj);
    }
    return x;
}

static void *gen_create_xo48w8(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xoroshiro48w8VarShiftsState *obj = intf->malloc(sizeof(Xoroshiro48w8VarShiftsState));
    for (int i = 0; i < 6; i++) {
        obj->s[i] = (uint8_t) intf->get_seed64();
    }
    obj->a = 1;
    obj->b = 2;
    obj->c = 3;
    return obj;
}


static void set_triple_xo48w8(void *state, unsigned int ai, unsigned int bi, unsigned int ci)
{
    Xoroshiro48w8VarShiftsState *obj = state;
    obj->a = (int) ai; obj->b = (int) bi; obj->c = (int) ci;
}


int test_xoshiro48w8(void)
{
    static const GeneratorInfo gen = {
        .name = "xoroshiro48w8:dynshifts",
        .description = "xoroshiro48w8 with dynamic shifts",
        .nbits = 32,
        .create = gen_create_xo48w8,
        .free = gen_free,
        .get_bits = get_bits_xo48w8,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    static const XorshiftProps props = {
        .max_value = 8,
        .nbytes = 6,
        .is_triple_valid = is_triple_valid_generic,
        .set_triple = set_triple_xo48w8
    };

    return run_triples_search(&gen, &props);
}



//////////////////////////////////
///// xoroshiro48w16 testing /////
//////////////////////////////////

typedef struct {
    uint16_t s[3];
    int a;
    int b;
    int c;
} Xoroshiro48w16VarShiftsState;


static uint16_t get_bits16_xo48w16(Xoroshiro64w16VarShiftsState *obj)
{
    const uint16_t s0 = obj->s[0];
    const uint16_t s0x2 = (uint16_t) (s0 ^ obj->s[2]);
    obj->s[0] = obj->s[1];
    obj->s[1] = (uint16_t) (rotl16(s0, obj->a) ^ s0x2 ^ (s0x2 << obj->b));
    obj->s[2] = rotl16(s0x2, obj->c);
    return obj->s[2];
}


static uint64_t get_bits_xo48w16(void *obj)
{
    const uint32_t hi = get_bits16_xo48w16(obj);
    const uint32_t lo = get_bits16_xo48w16(obj);
    return (hi << 16) | lo;
}

static void *gen_create_xo48w16(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xoroshiro48w16VarShiftsState *obj = intf->malloc(sizeof(Xoroshiro48w16VarShiftsState));
    obj->s[0] = (uint16_t) intf->get_seed64();
    obj->s[1] = (uint16_t) intf->get_seed64();
    obj->s[2] = (uint16_t) intf->get_seed64();
    obj->a = 5;
    obj->b = 10;
    obj->c = 5;
    return obj;
}


static void set_triple_xo48w16(void *state, unsigned int ai, unsigned int bi, unsigned int ci)
{
    Xoroshiro48w16VarShiftsState *obj = state;
    obj->a = (int) ai; obj->b = (int) bi; obj->c = (int) ci;
}


int test_xoshiro48w16(void)
{
    static const GeneratorInfo gen = {
        .name = "xoroshiro48w16:dynshifts",
        .description = "xoroshiro48w16 with dynamic shifts",
        .nbits = 32,
        .create = gen_create_xo48w16,
        .free = gen_free,
        .get_bits = get_bits_xo48w16,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    static const XorshiftProps props = {
        .max_value = 16,
        .nbytes = 6,
        .is_triple_valid = is_triple_valid_generic,
        .set_triple = set_triple_xo48w16
    };

    return run_triples_search(&gen, &props);
}



///////////////////////////////
///// xorshift128 testing /////
///////////////////////////////

typedef struct {
    uint32_t x;
    uint32_t y; 
    uint32_t z;
    uint32_t w;
    unsigned int a;
    unsigned int b;
    unsigned int c;
} Xorshift128VarShiftsState;


static uint64_t get_bits_xs128(void *state)
{
    Xorshift128VarShiftsState *obj = state;
    uint32_t t = obj->x ^ (obj->x << obj->a);
    t ^= t >> obj->b;
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = (obj->w ^ (obj->w >> obj->c)) ^ t;
    return obj->z;
}


static void *gen_create_xs128(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorshift128VarShiftsState *obj = intf->malloc(sizeof(Xorshift128VarShiftsState));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->w = intf->get_seed32() | 0x1; // State mustn't be all zeros
    obj->a = 12;
    obj->b = 25;
    obj->c = 27;
    return obj;
}


static void set_triple_xs128(void *state, unsigned int ai, unsigned int bi, unsigned int ci)
{
    Xorshift128VarShiftsState *obj = state;
    obj->a = ai; obj->b = bi; obj->c = ci;
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

    static const XorshiftProps props = {
        .max_value = 32,
        .nbytes = 16,
        .is_triple_valid = is_triple_valid_generic,
        .set_triple = set_triple_xs128
    };

    return run_triples_search(&gen, &props);
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
    unsigned int a;
    unsigned int b;
    unsigned int c;
} Xorshift160VarShiftsState;


static uint64_t get_bits_xs160(void *state)
{
    Xorshift160VarShiftsState *obj = state;
    uint32_t t = obj->x ^ (obj->x << obj->a);
    t ^= t >> obj->b;
    obj->x = obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = obj->v;
    obj->v = (obj->v ^ (obj->v >> obj->c)) ^ t;
    return obj->v;
}


static void *gen_create_xs160(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorshift160VarShiftsState *obj = intf->malloc(sizeof(Xorshift160VarShiftsState));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->w = intf->get_seed32();
    obj->v = intf->get_seed32() | 0x1; // State mustn't be all zeros
    obj->a = 12;
    obj->b = 25;
    obj->c = 27;
    return obj;
}


static void set_triple_xs160(void *state, unsigned int ai, unsigned int bi, unsigned int ci)
{
    Xorshift160VarShiftsState *obj = state;
    obj->a = ai; obj->b = bi; obj->c = ci;
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

    static const XorshiftProps props = {
        .max_value = 32,
        .nbytes = 20,
        .is_triple_valid = is_triple_valid_generic,
        .set_triple = set_triple_xs160
    };

    return run_triples_search(&gen, &props);
}


/////////////////////////////
///// xorrot256 testing /////
/////////////////////////////

typedef struct {
    uint64_t x;
    uint64_t y; 
    uint64_t z;
    uint64_t w;
    unsigned int a;
    unsigned int b;
    unsigned int c;
} Xorrot256VarShiftsState;


static uint64_t get_bits_xr256(void *state)
{
    Xorrot256VarShiftsState *obj = state;
    const uint64_t x0 = obj->x, w0 = obj->w;
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = x0 ^ w0;
    obj->w = (x0 << (int) obj->a) ^ obj->z ^ rotl64(w0, (int) obj->b) ^ rotl64(w0, (int) obj->c);
    return x0;
}


static int is_triple_valid_xr256(unsigned int ai, unsigned int bi, unsigned int ci)
{
    (void) bi;
    return ai < ci;
}


static void *gen_create_xr256(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorrot256VarShiftsState *obj = intf->malloc(sizeof(Xorrot256VarShiftsState));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->w = intf->get_seed64() | 0x1; // State mustn't be all zeros
    return obj;
}


static void set_triple_xr256(void *state, unsigned int ai, unsigned int bi, unsigned int ci)
{
    Xorrot256VarShiftsState *obj = state;
    obj->a = ai; obj->b = bi; obj->c = ci;
}


int test_xorrot256(void)
{
    static const GeneratorInfo gen = {
        .name = "xorrot256:dynshifts",
        .description = "xorrot256 with dynamic shifts",
        .nbits = 64,
        .create = gen_create_xr256,
        .free = gen_free,
        .get_bits = get_bits_xr256,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    static const XorshiftProps props = {
        .max_value = 64,
        .nbytes = 32,
        .is_triple_valid = is_triple_valid_xr256,
        .set_triple = set_triple_xr256
    };


    return run_triples_search(&gen, &props);
}


/////////////////////////////
///// xorrot160 testing /////
/////////////////////////////

typedef struct {
    uint32_t x;
    uint32_t y; 
    uint32_t z;
    uint32_t w;
    uint32_t v;
    unsigned int a;
    unsigned int b;
    unsigned int c;
} Xorrot160State;


static uint64_t get_bits_xr160(void *state)
{
    Xorrot160State *obj = state;
    const uint32_t x0 = obj->x, v0 = obj->v;
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = x0 ^ v0;
    obj->v = (x0 << (int) obj->a) ^ obj->w ^ rotl32(v0, (int) obj->b) ^ rotl32(v0, (int) obj->c);
    return x0;
}


static int is_triple_valid_xr160(unsigned int ai, unsigned int bi, unsigned int ci)
{
    (void) ai;
    return bi < ci;
}


static void *gen_create_xr160(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorrot160State *obj = intf->malloc(sizeof(Xorrot160State));
    obj->x = intf->get_seed32();
    obj->y = intf->get_seed32();
    obj->z = intf->get_seed32();
    obj->w = intf->get_seed32();
    obj->v = intf->get_seed32() | 0x1; // State mustn't be all zeros
    return obj;
}


static void set_triple_xr160(void *state, unsigned int ai, unsigned int bi, unsigned int ci)
{
    Xorrot160State *obj = state;
    obj->a = ai; obj->b = bi; obj->c = ci;
}


int test_xorrot160(void)
{
    static const GeneratorInfo gen = {
        .name = "xorrot160:dynshifts",
        .description = "xorrot160 with dynamic shifts",
        .nbits = 32,
        .create = gen_create_xr160,
        .free = gen_free,
        .get_bits = get_bits_xr160,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    static const XorshiftProps props = {
        .max_value = 32,
        .nbytes = 20,
        .is_triple_valid = is_triple_valid_xr160,
        .set_triple = set_triple_xr160
    };


    return run_triples_search(&gen, &props);
}



/////////////////////////////
///// xorrot320 testing /////
/////////////////////////////

typedef struct {
    uint64_t x;
    uint64_t y; 
    uint64_t z;
    uint64_t w;
    uint64_t v;
    unsigned int a;
    unsigned int b;
    unsigned int c;
} Xorrot320VarShiftsState;


static uint64_t get_bits_xr320(void *state)
{
    Xorrot320VarShiftsState *obj = state;
    const uint64_t x0 = obj->x, v0 = obj->v;
    obj->x = x0 ^ obj->y;
    obj->y = obj->z;
    obj->z = obj->w;
    obj->w = x0 ^ v0;
    obj->v = (x0 << (int) obj->a) ^ obj->w ^ rotl64(v0, (int) obj->b) ^ rotl64(v0, (int) obj->c);
    return x0;
}


static int is_triple_valid_xr320(unsigned int ai, unsigned int bi, unsigned int ci)
{
    (void) ai;
    return bi < ci;
}


static void *gen_create_xr320(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorrot320VarShiftsState *obj = intf->malloc(sizeof(Xorrot320VarShiftsState));
    obj->x = intf->get_seed64();
    obj->y = intf->get_seed64();
    obj->z = intf->get_seed64();
    obj->w = intf->get_seed64();
    obj->v = intf->get_seed64() | 0x1; // State mustn't be all zeros
    return obj;
}


static void set_triple_xr320(void *state, unsigned int ai, unsigned int bi, unsigned int ci)
{
    Xorrot320VarShiftsState *obj = state;
    obj->a = ai; obj->b = bi; obj->c = ci;
}


int test_xorrot320(void)
{
    static const GeneratorInfo gen = {
        .name = "xorrot320:dynshifts",
        .description = "xorrot320 with dynamic shifts",
        .nbits = 64,
        .create = gen_create_xr320,
        .free = gen_free,
        .get_bits = get_bits_xr320,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    static const XorshiftProps props = {
        .max_value = 64,
        .nbytes = 40,
        .is_triple_valid = is_triple_valid_xr320,
        .set_triple = set_triple_xr320
    };


    return run_triples_search(&gen, &props);
}


typedef struct {
    const char *testname;
    int (*testfunc)(void);
} TestEntry;


int main(int argc, char *argv[])
{
    static const TestEntry tests[] = {
        {"xoroshiro48w8", test_xoshiro48w8},
        {"xoroshiro48w16", test_xoshiro48w16},
        {"xoroshiro64w16", test_xoshiro64w16},
        {"xorshift32",     test_xorshift32},
        {"xorshift64",     test_xorshift64},
        {"xorshift128",    test_xorshift128},
        {"xorrot160",      test_xorrot160},
        {"xorrot320",      test_xorrot320},
        {NULL, NULL}
    };

    if (argc != 2) {
        printf("Usage: find_xorshift_params [name]\n");
        printf("Available names:\n");
        for (const TestEntry *t = tests; t->testname != NULL; t++) {
            printf("  %s\n", t->testname);
        }
        return 1;
    } else {
        for (const TestEntry *t = tests; t->testname != NULL; t++) {
            if (!strcmp(t->testname, argv[1])) {
                init_thread_dispatcher();
                return t->testfunc();
            }
        }
        printf("name %s not found\n", argv[1]);
        return 1;
    }
}
