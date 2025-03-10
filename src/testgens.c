/**
 * @file testgens.c
 * @brief A simplified variant of SmokeRand command line inteface for systems
 * without dynamic libraries support. Some pseudorandom number generators are
 * just embedded into the program.
 * @details It is essentially a hack to run SmokeRand under DOS using using
 * 32-bit extenders. Probably it will be useful as an example of SmokeRand
 * adaptation for constrained environments.
 *
 * Notes about tests in constrained environment:
 *
 * - `gap16_count0` may consume several MiB for gaps and frequencies tables.
 *   Probably RAM consumption may be significantly reduces but it may slow down
 *   and complicate a program for 64-bit environments.
 * - `bspace` uses more than 64 MiB of memory for 64-bit values.
 * - `collover` may use about 0.5 GiB of memory in most batteries. It may be
 *   significantly reduced but the test will be slower and less sensitive
 *   for 64-bit systems.
 *
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#define NO_CUSTOM_DLLENTRY
#define __SMOKERAND_CINTERFACE_H
#define PRNG_CMODULE_PROLOG
#include "smokerand_core.h"
#include "smokerand_bat.h"
#include "smokerand/apidefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//////////////////////////////////////////////////////////
///// A simplified version of smokerand/cinterface.h /////
//////////////////////////////////////////////////////////

/**
 * @brief 32-bit LCG state.
 */
typedef struct {
    uint32_t x;
} Lcg32State;

/**
 * @brief 64-bit LCG state.
 */
typedef struct {
    uint64_t x;
} Lcg64State;


#define MAKE_UINT_PRNG(suffix, prng_name, selftest_func, numofbits) \
uint64_t get_bits_##suffix(void *state) { return get_bits_raw_##suffix(state); } \
static int gen_getinfo_##suffix(GeneratorInfo *gi) { \
    gi->name = prng_name; \
    gi->description = NULL; \
    gi->nbits = numofbits; \
    gi->get_bits = get_bits_##suffix; \
    gi->create = default_new_##suffix; \
    gi->free = default_delete_##suffix; \
    gi->get_sum = NULL; \
    gi->self_test = selftest_func; \
    gi->parent = NULL; \
    return 1; \
}

#define DECLARE_NEW_DELETE(suffix) \
static void *default_new_##suffix(const GeneratorInfo *gi, const CallerAPI *intf) { \
    (void) gi; \
    return create(intf); \
} \
static void default_delete_##suffix(void *state, const GeneratorInfo *gi, const CallerAPI *intf) { \
    (void) gi; \
    intf->free(state); \
}

#define MAKE_UINT32_PRNG(prng_name, selftest_func)
#define MAKE_UINT64_PRNG(prng_name, selftest_func)

///////////////////////////////
///// Embedded generators /////
///////////////////////////////

//----- lfib_par PRNG ------
#define create create_lfib
#define run_self_test run_self_test_lfib
#define get_bits_raw get_bits_raw_lfib
#include "../generators/lfib_par.c"
DECLARE_NEW_DELETE(lfib)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(lfib, "lfib", NULL, 32)
//----- chacha12 PRNG ------
#define create create_chacha
#define run_self_test run_self_test_chacha
#define get_bits_raw get_bits_raw_chacha
#include "../generators/chacha.c"
DECLARE_NEW_DELETE(chacha)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(chacha, "chacha12", run_self_test_chacha, 32)
//----- flea32x1 PRNG -----
#define create create_flea32x1
#define run_self_test run_self_test_flea32x1
#define get_bits_raw get_bits_raw_flea32x1
#include "../generators/flea32x1.c"
DECLARE_NEW_DELETE(flea32x1)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(flea32x1, "flea32x1", NULL, 32)
//----- kiss64 PRNG -----
#define create create_kiss64
#define run_self_test run_self_test_kiss64
#define get_bits_raw get_bits_raw_kiss64
#include "../generators/kiss64.c"
DECLARE_NEW_DELETE(kiss64)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(kiss64, "KISS64", run_self_test_kiss64, 64)
//----- kiss99 PRNG -----
#define create create_kiss99
#define run_self_test run_self_test_kiss99
#define get_bits_raw get_bits_raw_kiss99
#include "../generators/kiss99.c"
DECLARE_NEW_DELETE(kiss99)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(kiss99, "KISS99", run_self_test_kiss99, 32)
//----- HC256 PRNG -----
#define create create_hc256
#define run_self_test run_self_test_hc256
#define get_bits_raw get_bits_raw_hc256
#include "../generators/hc256.c"
DECLARE_NEW_DELETE(hc256)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(hc256, "HC256", run_self_test_hc256, 32)
//----- LCG64 PRNG -----
#define create create_lcg64
#define run_self_test run_self_test_lcg64
#define get_bits_raw get_bits_raw_lcg64
#include "../generators/lcg64.c"
DECLARE_NEW_DELETE(lcg64)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(lcg64, "LCG64", NULL, 32)
//----- LCG69069 -----
#define create create_lcg69069
#define run_self_test run_self_test_lcg69069
#define get_bits_raw get_bits_raw_lcg69069
#include "../generators/lcg69069.c"
DECLARE_NEW_DELETE(lcg69069)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(lcg69069, "LCG69069", NULL, 32)
//----- lcg96_portable -----
#define create create_lcg96_portable
#define run_self_test run_self_test_lcg96_portable
#define get_bits_raw get_bits_raw_lcg96_portable
#include "../generators/lcg96_portable.c"
DECLARE_NEW_DELETE(lcg96_portable)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(lcg96_portable, "lcg96_portable", run_self_test_lcg96_portable, 32)
//----- MT19937 PRNG -----
#define create create_mt19937
#define run_self_test run_self_test_mt19937
#define get_bits_raw get_bits_raw_mt19937
#include "../generators/mt19937.c"
DECLARE_NEW_DELETE(mt19937)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(mt19937, "MT19937", NULL, 32)
//----- MWC1616 PRNG -----
#define create create_mwc1616
#define get_bits_raw get_bits_raw_mwc1616
#include "../generators/mwc1616.c"
DECLARE_NEW_DELETE(mwc1616)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(mwc1616, "MWC1616", NULL, 32)
//----- MWC1616X PRNG -----
#define create create_mwc1616x
#define get_bits_raw get_bits_raw_mwc1616x
#include "../generators/mwc1616x.c"
DECLARE_NEW_DELETE(mwc1616x)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(mwc1616x, "MWC1616X", NULL, 32)
//----- MWC4691 PRNG -----
#define create create_mwc4691
#define run_self_test run_self_test_mwc4691
#define get_bits_raw get_bits_raw_mwc4691
#include "../generators/mwc4691.c"
DECLARE_NEW_DELETE(mwc4691)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(mwc4691, "MWC4691", run_self_test_mwc4691, 32)
//----- MWC64 PRNG -----
#define create create_mwc64
#define get_bits_raw get_bits_raw_mwc64
#include "../generators/mwc64.c"
DECLARE_NEW_DELETE(mwc64)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(mwc64, "MWC64", NULL, 32)
//----- SplitMix  PRNG -----
#define create create_splitmix
#define run_self_test run_self_test_splitmix
#define get_bits_raw get_bits_raw_splitmix
#include "../generators/splitmix.c"
DECLARE_NEW_DELETE(splitmix)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(splitmix, "SplitMix", NULL, 64)
//----- SplitMix32 PRNG -----
#define create create_splitmix32
#define run_self_test run_self_test_splitmix32
#define get_bits_raw get_bits_raw_splitmix32
#include "../generators/splitmix32.c"
DECLARE_NEW_DELETE(splitmix32)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(splitmix32, "SplitMix32", NULL, 32)
//----- SWB (subtract-with-borrow) PRNG -----
#define create create_swb
#define run_self_test run_self_test_swb
#define get_bits_raw get_bits_raw_swb
#include "../generators/swb.c"
DECLARE_NEW_DELETE(swb)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(swb, "SWB", NULL, 32)
//----- xoroshiro128+ PRNG (32-bit) -----
#define create create_xoroshiro128p
#define run_self_test run_self_test_xoroshiro128p
#define get_bits_raw get_bits_raw_xoroshiro128p
#include "../generators/xoroshiro128p.c"
DECLARE_NEW_DELETE(xoroshiro128p)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(xoroshiro128p, "xoroshiro128+", run_self_test_xoroshiro128p, 64)
//----- xoroshiro128++ PRNG (32-bit) -----
#define create create_xoroshiro128pp
#define run_self_test run_self_test_xoroshiro128pp
#define get_bits_raw get_bits_raw_xoroshiro128pp
#include "../generators/xoroshiro128pp.c"
DECLARE_NEW_DELETE(xoroshiro128pp)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(xoroshiro128pp, "xoroshiro128++", run_self_test_xoroshiro128pp, 64)
//----- xoshiro128+ PRNG (32-bit) -----
#define create create_xoshiro128p
#define run_self_test run_self_test_xoshiro128p
#define get_bits_raw get_bits_raw_xoshiro128p
#include "../generators/xoshiro128p.c"
DECLARE_NEW_DELETE(xoshiro128p)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(xoshiro128p, "xoshiro128+", NULL, 32)
//----- xoshiro128++ PRNG (32-bit) -----
#define create create_xoshiro128pp
#define run_self_test run_self_test_xoshiro128pp
#define get_bits_raw get_bits_raw_xoshiro128pp
#include "../generators/xoshiro128pp.c"
DECLARE_NEW_DELETE(xoshiro128pp)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(xoshiro128pp, "xoshiro128++", NULL, 32)
//----- xorwow PRNG -----
#define create create_xorwow
#define run_self_test run_self_test_xorwow
#define get_bits_raw get_bits_raw_xorwow
#include "../generators/xorwow.c"
DECLARE_NEW_DELETE(xorwow)
#undef create
#undef run_self_test
#undef get_bits_raw
MAKE_UINT_PRNG(xorwow, "xorwow", NULL, 32)


///////////////////////////////
///// Program entry point /////
///////////////////////////////


typedef struct {
    int (*gen_getinfo)(GeneratorInfo *gi);
    const char *name;
} GeneratorEntry;



int main(int argc, char *argv[])
{
    static const GeneratorEntry generators[] = {
        {gen_getinfo_lfib,           "alfib607"},
        {gen_getinfo_chacha,         "chacha"},
        {gen_getinfo_flea32x1,       "flea32x1"},
        {gen_getinfo_kiss64,         "kiss64"},
        {gen_getinfo_kiss99,         "kiss99"},
        {gen_getinfo_hc256,          "hc256"},
        {gen_getinfo_lcg64,          "lcg64"},
        {gen_getinfo_lcg69069,       "lcg69069"},
        {gen_getinfo_lcg96_portable, "lcg96"},
        {gen_getinfo_mt19937,        "mt19937"},
        {gen_getinfo_mwc1616,        "mwc1616"},
        {gen_getinfo_mwc1616x,       "mwc1616x"},
        {gen_getinfo_mwc64,          "mwc64"},
        {gen_getinfo_mwc4691,        "mwc4691"},
        {gen_getinfo_splitmix,       "splitmix"},
        {gen_getinfo_splitmix32,     "spiltmix32"},
        {gen_getinfo_swb,            "swb"},
        {gen_getinfo_xoroshiro128p,  "xoroshiro128+"},
        {gen_getinfo_xoroshiro128pp, "xoroshiro128++"},
        {gen_getinfo_xoshiro128p,    "xoshiro128+"},
        {gen_getinfo_xoshiro128pp,   "xoshiro128++"},
        {gen_getinfo_xorwow,         "xorwow"},
        {NULL, ""}
    };

    if (argc < 3) {
        printf("SmokeRand: a version with built-in generators\n"
            "Usage:\n"
            "  rungens battery generator\n"
            "  Batteries: express brief default full selftest speed @filename\n"
            "    filename is a text file with a custom battery description\n"
            "  Generators:");
        for (size_t i = 0; generators[i].gen_getinfo != NULL; i++) {
            if (i % 5 == 0) {
                printf("    \n");
            }
            printf("%14s ", generators[i].name);
        }
        printf("\n");
        return 0;
    }
    int (*gen_getinfo)(GeneratorInfo *gi) = NULL;
    for (const GeneratorEntry *ptr = generators; ptr->gen_getinfo; ptr++) {
        if (!strcmp(ptr->name, argv[2])) {
            gen_getinfo = ptr->gen_getinfo;
            break;
        }
    }
    if (gen_getinfo == NULL) {
        fprintf(stderr, "Unknown generator '%s'\n", argv[2]);
        return 1;
    }
    GeneratorInfo gi;
    gen_getinfo(&gi);
    GeneratorInfo_print(&gi, 1);
    CallerAPI intf = CallerAPI_init();
    if (!strcmp("express", argv[1])) {
        battery_express(&gi, &intf, TESTS_ALL, 1, REPORT_FULL);
    } else if (!strcmp("brief", argv[1])) {
        battery_brief(&gi, &intf, TESTS_ALL, 1, REPORT_FULL);
    } else if (!strcmp("default", argv[1])) {
        battery_default(&gi, &intf, TESTS_ALL, 1, REPORT_FULL);
    } else if (!strcmp("full", argv[1])) {
        battery_full(&gi, &intf, TESTS_ALL, 1, REPORT_FULL);
    } else if (!strcmp("selftest", argv[1])) {
        battery_self_test(&gi, &intf);
    } else if (!strcmp("speed", argv[1])) {
        battery_speed(&gi, &intf);
    } else if (strlen(argv[1]) > 0) {
        if (argv[1][0] != '@') {
            fprintf(stderr, "@filename argument must begin with '@'\n");
            return 1;
        }
        battery_file(argv[1] + 1, &gi, &intf, TESTS_ALL, 1, REPORT_FULL);
    }
    CallerAPI_free();
    return 0;
}
