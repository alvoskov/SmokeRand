/**
 * @file test_lfsr_period.c
 * @brief Tests for LFSR period analyzer subroutines including automated
 * charateristic and jump polynomials derivations.
 * @details It includes the next tests:
 *
 * - 64-bit counter (must give an error)
 * - 64-bit Klimov-Shamir T-function (must give an error)
 * - xorrot64 xorshift-like LFSR with three shifts sets: a good one
 *   and two bad ones (in slightly different ways)
 * - xoroshiro128++: charateristic and jump polynomials are compared to the
 *   reference ones given by D. Blackman and S. Vigna. Jump polynomials
 *   generation is also tested here.
 *
 * References:
 *
 * - https://prng.di.unimi.it/xoroshiro128plusplus.c
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand_core.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

static void gen_free(void *state, const GeneratorInfo *info, const CallerAPI *intf)
{
    (void) info;
    intf->free(state);
}

///////////////////////////////
///// Counter based tests /////
///////////////////////////////

typedef struct {
    uint64_t x;
} Ctr64State;

static uint64_t get_bits_ctr64(void *state)
{
    Ctr64State *obj = state;
    obj->x++;
    return obj->x >> 32;
}

static void *gen_create_ctr64(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Ctr64State *obj = intf->malloc(sizeof(Ctr64State));
    obj->x = 0xDEADBEEFCAFEBABE;
    return obj;
}

/**
 * @brief Tests that is based on the 64-bit counter as a PRNG.
 * It tests if sanity checks inside the lfsr_period_test function
 * are working.
 */
static int test_ctr64(const CallerAPI *intf)
{
    static const GeneratorInfo gen = {
        .name = "ctr64",
        .description = "",
        .nbits = 32,
        .create = gen_create_ctr64,
        .free = gen_free,
        .get_bits = get_bits_ctr64,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };
    static const LfsrPeriodOptions opts = {.check_validity = 1};
    int is_ok = 1;

    intf->printf("----- ctr64 based test-----\n");
    GeneratorStateExt ext = GeneratorStateExt_create(&gen, intf);
    if (lfsr_period_test(&ext, intf, &opts) != LFSR_PERIOD_ERROR) {
        is_ok = 0;
    }
    GeneratorStateExt_destruct(&ext);
    return is_ok;
}


/////////////////////////////////////////////////////////
///// tf0_64 (Klimov-Shamir T-function) based tests /////
/////////////////////////////////////////////////////////

typedef struct {
    uint64_t x;
} Tf064State;

static uint64_t get_bits_tf64(void *state)
{
    Tf064State *obj = state;
    obj->x += obj->x * obj->x | 0x5;
    return obj->x >> 32;
}

static void *gen_create_tf64(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Tf064State *obj = intf->malloc(sizeof(Tf064State));
    obj->x = intf->get_seed64();
    return obj;
}

/**
 * @brief Tests that is based on the 64-bit TF0 function (so called
 * "crazy" Klimov-Shamir function) as a PRNG. It tests if sanity
 * checks inside the lfsr_period_test function are working.
 */
static int test_tf0_64(const CallerAPI *intf)
{
    static const GeneratorInfo gen = {
        .name = "tf0_64",
        .description = "",
        .nbits = 32,
        .create = gen_create_tf64,
        .free = gen_free,
        .get_bits = get_bits_tf64,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };
    static const LfsrPeriodOptions opts = {.check_validity = 1};
    int is_ok = 1;

    intf->printf("----- tf0_64 based test-----\n");
    GeneratorStateExt ext = GeneratorStateExt_create(&gen, intf);
    if (lfsr_period_test(&ext, intf, &opts) != LFSR_PERIOD_ERROR) {
        is_ok = 0;
    }
    GeneratorStateExt_destruct(&ext);
    return is_ok;
}


////////////////////////////////
///// xorrot32 based tests /////
////////////////////////////////

typedef struct {
    uint32_t x;
    int a;
    int b;
    int c;
} Xorrot32VarShiftsState;

static uint64_t get_bits_xr32(void *state)
{
    Xorrot32VarShiftsState *obj = state;
    obj->x ^= obj->x << obj->a;
    obj->x ^= rotl32(obj->x, obj->b) ^ rotl32(obj->x, obj->c);
    return obj->x;
}

static void *gen_create_xr32(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorrot32VarShiftsState *obj = intf->malloc(sizeof(Xorrot32VarShiftsState));
    obj->x = intf->get_seed32();
    obj->a = 1; // That shifts give a full period (2**32 - 1)
    obj->b = 9;
    obj->c = 27;
    return obj;
}

/**
 * @brief Tests based on the xorrot32 custom PRNG. It tests not only
 * the version with the maximal 2**32 - 1 period but also two versions
 * with smaller period (to test if the lfsr_period_test function fails
 * when required)
 */
static int test_xorrot32(const CallerAPI *intf)
{
    static const GeneratorInfo gen = {
        .name = "xorrot32:varshifts",
        .description = "",
        .nbits = 32,
        .create = gen_create_xr32,
        .free = gen_free,
        .get_bits = get_bits_xr32,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };
    static const LfsrPeriodOptions opts = {.check_validity = 1};
    int is_ok = 1;

    intf->printf("----- xorrot32 based test-----\n");
    GeneratorStateExt ext = GeneratorStateExt_create_sized(&gen, intf, 4);
    Xorrot32VarShiftsState *st = ext.state.state;

    intf->printf("--- a) maximal period version ---\n");
    if (lfsr_period_test(&ext, intf, &opts) != LFSR_PERIOD_MAX) {
        is_ok = 0;
    }

    intf->printf("--- b) bad version 1 ---\n");
    st->a = 2; st->b = 9; st->c = 27;
    if (lfsr_period_test(&ext, intf, &opts) != LFSR_PERIOD_NOT_MAX) {
        is_ok = 0;
    }

    intf->printf("--- c) bad version 2 ---\n");
    st->a = 1; st->b = 6; st->c = 23;
    if (lfsr_period_test(&ext, intf, &opts) != LFSR_PERIOD_NOT_MAX) {
        is_ok = 0;
    }
    GeneratorStateExt_destruct(&ext);
    return is_ok;
}



//////////////////////////////////////
///// xoroshiro128++ based tests /////
//////////////////////////////////////

/**
 * @brief xoroshiro128++ PRNG state. Mustn't be initialized as (0, 0).
 */
typedef struct {
    uint64_t s[2];
} Xoroshiro128PPState;


static uint64_t get_bits_xs128(void *state)
{
    Xoroshiro128PPState *obj = state;
    const uint64_t s0 = obj->s[0];
    uint64_t s1 = obj->s[1];
    const uint64_t result = rotl64(s0 + s1, 17) + s0;
    s1 ^= s0;
    obj->s[0] = rotl64(s0, 49) ^ s1 ^ (s1 << 21); // a, b
    obj->s[1] = rotl64(s1, 28); // c
    return result;
}


static void *gen_create_xs128(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xoroshiro128PPState *obj = intf->malloc(sizeof(Xoroshiro128PPState));
    obj->s[0] = intf->get_seed64();
    obj->s[1] = intf->get_seed64() | 0x1;
    (void) gi;
    return obj;
}

/**
 * @brief Tests that are based on the xoroshiro128++ generator.
 * @details It contains the next subtests
 *
 * - Checks if the period is 2**128 - 1
 * - Compares the characteristic and jump polynomials with reference
 *   ones obtained by Blackman and Vigna
 * - Tests the jump polynomial usage (and compares with the direct
 *   calls of the PRNG)
 *
 * References:
 *
 * 1. https://prng.di.unimi.it/xoroshiro128plusplus.c
 */
static int test_xoroshiro128(const CallerAPI *intf)
{
    static const GeneratorInfo gen = {
        .name = "xoroshiro128pp",
        .description = "",
        .nbits = 64,
        .create = gen_create_xs128,
        .free = gen_free,
        .get_bits = get_bits_xs128,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };
    static const LfsrPeriodOptions opts = {.check_validity = 1};
    static const uint64_t char_poly_ref[] = {0x8dae70779760b081, 0x0031bcf2f855d6e5};
    static const uint64_t jump_poly_ref[] = {0x2bd7a6a6e99c2ddc, 0x0992ccaf6a6fca05};
    int is_ok = 1;

    intf->printf("----- xoroshiro128++ based test-----\n");
    GeneratorStateExt ext = GeneratorStateExt_create(&gen, intf);
    if (lfsr_period_test(&ext, intf, &opts) != LFSR_PERIOD_MAX) {
        is_ok = 0;
    } else {
        // Compare jump poly to the reference one
        LfsrPoly char_poly = GeneratorStateExt_get_poly(&ext);
        LfsrPoly jump_poly = GeneratorStateExt_get_jump_poly_pow2(&ext, 64);
        intf->printf("Char.poly.:         ");
        LfsrPoly_print_hex(&char_poly, intf);
        intf->printf("\n");
        intf->printf("Jump poly.(j=2^64): ");
        LfsrPoly_print_hex(&jump_poly, intf);
        intf->printf("\n");
        if (char_poly.nwords != 2 || jump_poly.nwords != 2) {
            is_ok = 0;
        }
        for (size_t i = 0; i < 2; i++) {
            if (char_poly.w64[i] != char_poly_ref[i] ||
                jump_poly.w64[i] != jump_poly_ref[i]) {
                is_ok = 0;
            }
        }
        LfsrPoly_destruct(&char_poly);
        LfsrPoly_destruct(&jump_poly);
        // Check if jump polynomials are working
        const unsigned int jmp_pow = 20;
        intf->printf("2**%u jump\n", jmp_pow);
        Xoroshiro128PPState *obj = ext.state.state;
        // a) reference value
        obj->s[0] = 0xDEADBEEF; obj->s[1] = 0xCAFEBABE;
        for (size_t i = 0; i < (size_t) (1ULL << jmp_pow); i++) {
            (void) ext.state.gi->get_bits(ext.state.state);
        }
        const uint64_t u_ref = ext.state.gi->get_bits(ext.state.state);
        intf->printf("  u_ref = %llX\n", (unsigned long long) u_ref);

        // b) jump matrix value
        obj->s[0] = 0xDEADBEEF; obj->s[1] = 0xCAFEBABE;
        GeneratorStateExt_make_jump_pow2(&ext, jmp_pow);
        const uint64_t u_jmp = ext.state.gi->get_bits(ext.state.state);
        intf->printf("  u_jmp = %llX\n", (unsigned long long) u_jmp);

        if (u_jmp != u_ref) {
            is_ok = 0;
        }
    }

    GeneratorStateExt_destruct(&ext);
    return is_ok;
}


int main()
{
    CallerAPI intf = CallerAPI_init();
    const int is_ctr_ok    = test_ctr64(&intf);
    const int is_tf0_64_ok = test_tf0_64(&intf);
    const int is_xr32_ok   = test_xorrot32(&intf);
    const int is_xs128_ok  = test_xoroshiro128(&intf);
    CallerAPI_free();

    printf("ctr:            [%s]\n", is_ctr_ok    ? "PASSED" : "FAILED");
    printf("tf0_64:         [%s]\n", is_tf0_64_ok ? "PASSED" : "FAILED");
    printf("xorrot32:       [%s]\n", is_xr32_ok   ? "PASSED" : "FAILED");
    printf("xoroshiro128++: [%s]\n", is_xs128_ok  ? "PASSED" : "FAILED");
    const int is_ok = is_ctr_ok && is_tf0_64_ok && is_xr32_ok && is_xs128_ok;
    return is_ok ? 0 : 1;
}
