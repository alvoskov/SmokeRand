/**
 * @file lcg96.c
 * @brief Just 96-bit LCG with \f$ m = 2^{96}\f$: 128-bit and portable
 * versions (`--param=ext` and `--param=c99` respectively)
 * @details Two variants are implemented in this module:
 *
 * - `ext` (default) that uses C extensions for 128-bit arithmetics.
 *   64-bit versions of GCC, Clang and MSVC are supported. Uses
 *   a 96-bit multipiler from [2].
 * - `c99` that is written without usage of C extensions and can be
 *   compiled by GCC for 32-bit platforms such as x86 before AMD64.
 *   Requires `uint64_t`/`unsigned long long` types introduced in C99.
 *   Uses a 48-bit multiplier from [1].
 * 
 * The multipliers can be taken from:
 *
 * 1. P. L'Ecuyer. Tables of linear congruential generators of different
 *    sizes and good lattice structure // Mathematics of Computation. 1999.
 *    V. 68. N. 225. P. 249-260
 *    http://dx.doi.org/10.1090/S0025-5718-99-00996-5
 * 2. https://www.pcg-random.org/posts/does-it-beat-the-minimal-standard.html
 *
 * However, both versions fail 32-bit 8-dimensional decimated birthday spacings
 * test `bspace4_8d_dec`. They also fail TMFn test from PractRand 0.94.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/////////////////////////////////////////////////////////
///// Version for compilers with 128-bit extensions /////
/////////////////////////////////////////////////////////

/**
 * @brief A cross-compiler implementation of 96-bit LCG.
 */
static inline uint64_t get_bits_ext_raw(void *state)
{
#ifdef UMUL128_FUNC_ENABLED
    Lcg128State *obj = state;
    Lcg128State_a128_iter(obj, 0xdc879768, 0x60b11728995deb95, 1);
#ifdef UINT128_ENABLED
    obj->x = ((obj->x << 32) >> 32); // mod 2^96
    return (uint64_t)(obj->x >> 64);
#else
    // mod 2^96
    obj->x_high = ((obj->x_high << 32) >> 32);
    return obj->x_high;
#endif
#else
    (void) state;
    return 0;
#endif
}


MAKE_GET_BITS_WRAPPERS(ext)


/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test_ext(const CallerAPI *intf)
{
#ifdef UMUL128_FUNC_ENABLED
    Lcg128State obj;
    Lcg128State_init(&obj, 0, 1234567890);
    uint64_t u, u_ref = 0xea5267e2;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_ext_raw(&obj);
    }
    intf->printf("---- Extended (128-bit) version -----\n");
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
#else
    intf->printf("'ext' version is not supported on this platform");
    return 1;
#endif
}


static void *create_ext(const GeneratorInfo *gi, const CallerAPI *intf)
{
#ifdef UMUL128_FUNC_ENABLED
    Lcg128State *obj = intf->malloc(sizeof(Lcg128State));
    Lcg128State_init(obj, 0, intf->get_seed64() | 0x1);
    (void) gi;
    return (void *) obj;
#else
    (void) gi; (void) intf;
    return NULL;
#endif
}

////////////////////////////////
///// Portable C99 version /////
////////////////////////////////

/**
 * @brief 96-bit portable LCG state.
 * @details Not optimized for byte order (low endian/big endian) of any
 * specific platform. But `x[0]` is the lower 32-bit word and `x[2]` is
 * the higher 32-bit word.
 */
typedef struct {
    uint32_t x[3];
} Lcg96x32State;


#define HI64(x) ((x) >> 32)
#define LO64(x) ((x) & 0xFFFFFFFF)
#define MUL64(x,y) ((uint64_t)(x) * (uint64_t)(y))
#define SUM64(x,y) ((uint64_t)(x) + (uint64_t)(y))



/**
 * @brief A portable implementation of 96-bit LCG.
 */
static inline uint64_t get_bits_c99_raw(void *state)
{
    //                           lower        medium     high
    static const uint32_t a[] = {0x3bda4a15, 0xfa75832c, 0xf429e3c0};
    static const uint32_t c = 1;
    uint32_t row0[3], row1[2], row2;
    uint64_t mul, sum;
    Lcg96x32State *obj = state;
    // Row 0
    mul = MUL64(a[0], obj->x[0]); row0[0] = LO64(mul);
    mul = MUL64(a[0], obj->x[1]) + HI64(mul); row0[1] = LO64(mul);
    mul = MUL64(a[0], obj->x[2]) + HI64(mul); row0[2] = LO64(mul);
    // Row 1
    mul = MUL64(a[1], obj->x[0]); row1[0] = LO64(mul);
    mul = MUL64(a[1], obj->x[1]) + HI64(mul); row1[1] = LO64(mul);
    // Row 2
    row2 = LO64(MUL64(a[2], obj->x[0]));
    // Sum rows (update state)
    sum = SUM64(row0[0], c);                   obj->x[0] = LO64(sum);
    sum = SUM64(row0[1], row1[0]) + HI64(sum); obj->x[1] = LO64(sum);
    sum = SUM64(row0[2], row1[1]) + row2 + HI64(sum);
    obj->x[2] = LO64(sum);
    // Return upper 32 bits
    return obj->x[2];
}


MAKE_GET_BITS_WRAPPERS(c99)


static void *create_c99(const GeneratorInfo *gi, const CallerAPI *intf)
{
    Lcg96x32State *obj = intf->malloc(sizeof(Lcg96x32State));
    uint64_t seed = intf->get_seed64();
    obj->x[0] = LO64(seed) | 0x1; // For the case of MCG
    obj->x[1] = HI64(seed);
    obj->x[2] = 0;
    (void) gi;
    return (void *) obj;
}


/**
 * @brief Self-test to check portable implementation of 96-bit
 * multiplication and addition.
 * @details It is easy to reproduce reference values in Python:
 *
 *     a = 0xf429e3c0fa75832c3bda4a15
 *     x = 1234567890
 *     for i in range(0,1000000):
 *         x = (a*x + 1) % 2**96
 *     print(hex(x))
 *
 */
static int run_self_test_c99(const CallerAPI *intf)
{
    uint64_t u, u_ref = 0x6a5efd72;
    Lcg96x32State obj;
    obj.x[0] = 1234567890; obj.x[1] = 0; obj.x[2] = 0;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_c99_raw(&obj);
    }
    intf->printf("---- Portable (C99) version -----\n");
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


/////////////////////
///// Interface /////
/////////////////////


static void *create(const CallerAPI *intf)
{
    (void) intf;
    return NULL;
}

/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    is_ok = is_ok & run_self_test_ext(intf);
    is_ok = is_ok & run_self_test_c99(intf);
    return is_ok;
}



int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = NULL;
    gi->nbits = 32;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "ext") || !intf->strcmp(param, "")) {
        gi->name = "Lcg96:ext";
        gi->create = create_ext;
        gi->get_bits = get_bits_ext;
        gi->get_sum = get_sum_ext;
    } else if (!intf->strcmp(param, "c99")) {
        gi->name = "Lcg96:c99";
        gi->create = create_c99;
        gi->get_bits = get_bits_c99;
        gi->get_sum = get_sum_c99;
    } else {
        gi->name = "Lcg96:unknown";
        gi->create = default_create;
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
