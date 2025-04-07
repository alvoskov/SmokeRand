/**
 * @file lcg128.c
 * @brief Just 128-bit LCG with \f$ m = 2^{128}\f$.
 * @details Several variants are implemented in this module:
 *
 * 1. Easy to memorize 64-bit multiplier 18000 69069 69069 69069 suggested
 *    by A.L. Voskov. Its replacement to the slightly better multiplier
 *    0xfc0072fa0b15f4fd from [1] doesn't improve the tests results.
 * 2. 128-bit multiplier from [1] and output from the upper 64 bits.
 * 3. 128-bit multiplier from [1] and output from the upper 32 bits.
 * 4. The portable version with the 64-bit multiplier from [1] that returns
 *    the upper 32 bits.
 *
 * These generators pass SmallCrush, Crush and BigCrush. However, its higher
 * 64 bits fail PractRand 0.94 at 128GiB sample. Usage of slightly better
 * (but hard to memorize) multiplier 0xfc0072fa0b15f4fd from [1]
 * doesn't improve PractRand 0.94 results.
 *
 * References:
 *
 * 1. Steele G.L., Vigna S. Computationally easy, spectrally good multipliers
 *    for congruential pseudorandom number generators // Softw Pract Exper. 2022
 *    V. 52. N. 2. P. 443-458. https://doi.org/10.1002/spe.3030
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static void *create(const CallerAPI *intf)
{
    Lcg128State *obj = intf->malloc(sizeof(Lcg128State));
    Lcg128State_seed(obj, intf);
    return (void *) obj;
}

/////////////////////////////
///// 64-bit multiplier /////
/////////////////////////////

/**
 * @brief A cross-compiler implementation of 128-bit LCG.
 */
static inline uint64_t get_bits_x64u64_raw(void *state)
{
    const uint64_t a = 18000690696906969069ull;
    return Lcg128State_a64_iter(state, a, 1ull);
}

MAKE_GET_BITS_WRAPPERS(x64u64)

/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test_x64u64(const CallerAPI *intf)
{
    Lcg128State obj;
    Lcg128State_init(&obj, 0, 1234567890);
    uint64_t u, u_ref = 0x8E878929D96521D7;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_x64u64_raw(&obj);
    }
    intf->printf("x64u64 result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}

//////////////////////////////////////////////////////////
///// 128-bit multiplier (output from upper 64 bits) /////
//////////////////////////////////////////////////////////


/**
 * @brief A portable implementation of the \f$ ax + c \f$ operation with 64-bit
 * arguments and 128-bit output. Useful for LCG and MWC generators.
 * @param[in] a Input value.
 * @param[in,out] x Input value that will be overwritten.
 * @param[in] c Input value.
 * @return Lower 64 bits.
 */
static inline void umuladd_128x128p64_c99(uint32_t *a, uint32_t *x, uint64_t c)
{
    static const uint64_t MASK32 = 0xFFFFFFFF;
    uint32_t out[4];
    uint64_t mul, sum;
    // Row 0
    mul = 0;
    for (int i = 0; i < 4; i++) {
        mul = ((uint64_t) a[0]) * x[i] + (mul >> 32);
        out[i] = (uint32_t) mul;
    }
    // Row 1
    mul = 0, sum = 0;
    for (int i = 0; i < 3; i++) {
        mul = ((uint64_t) a[1]) * x[i] + (mul >> 32);
        sum = (mul & MASK32) + out[i + 1] + (sum >> 32);
        out[i + 1] = (uint32_t) sum;
    }
    // Row 2
    mul = 0; sum = 0;
    for (int i = 0; i < 2; i++) {
        mul = ((uint64_t) a[2]) * x[i] + (mul >> 32);
        sum = (mul & MASK32) + out[i + 2] + (sum >> 32);
        out[i + 2] = (uint32_t) sum;
    }
    // Row 3
    out[3] += a[3] * x[0];
    if (c != 0) {
        uadd_128p64_ary_c99(out, c);
    }
    // Update output
    for (int i = 0; i < 4; i++) {
        x[i] = out[i];
    }
}



/**
 * @brief A cross-compiler implementation of 128-bit LCG.
 */
static inline uint64_t get_bits_x128u64_raw(void *state)
{
#ifdef UMUL128_FUNC_ENABLED
    return Lcg128State_a128_iter(state, 0xdb36357734e34abb, 0x0050d0761fcdfc15, 1);
#else
    Lcg128State *obj = state;
    uint64_t a_hi = 0xdb36357734e34abb, a_lo = 0x0050d0761fcdfc15;
    uint32_t x[4];
    uint32_t a[4]; 

    x[0] = (uint32_t) (obj->x_low);  x[1] = (obj->x_low) >> 32;
    x[2] = (uint32_t) (obj->x_high); x[3] = (obj->x_high) >> 32;
    a[0] = (uint32_t) a_lo; a[1] = a_lo >> 32;
    a[2] = (uint32_t) a_hi; a[3] = a_hi >> 32;

    umuladd_128x128p64_c99(a, x, 1);

    obj->x_high = ((uint64_t) x[2]) | (((uint64_t) x[3]) << 32);
    obj->x_low  = ((uint64_t) x[0]) | (((uint64_t) x[1]) << 32);
    return obj->x_high;
#endif
}


MAKE_GET_BITS_WRAPPERS(x128u64)

/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 * @details Reference value may be obtained as:
 *    
 *    a = 0xdb36357734e34abb0050d0761fcdfc15
 *    x = 1234567890
 *    for i in range(0,1000000):
 *        x = (a*x + 1) % 2**128
 *    print(hex(x // 2**64))
 *
 */
static int run_self_test_x128u64(const CallerAPI *intf)
{
    Lcg128State obj;
    Lcg128State_init(&obj, 1234567890, 0);
    uint64_t u, u_ref = 0x23fe67ffa50c941f;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_x128u64_raw(&obj);
    }
    intf->printf("x128u64 result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}


//////////////////////////////////////////////////////////
///// 128-bit multiplier (output from upper 32 bits) /////
//////////////////////////////////////////////////////////


/**
 * @brief A cross-compiler implementation of 128-bit LCG. Returns only upper
 * 32 bits.
 */
static inline uint64_t get_bits_x128u32_raw(void *state)
{
#ifdef UMUL128_FUNC_ENABLED
    return Lcg128State_a128_iter(state, 0xdb36357734e34abb, 0x0050d0761fcdfc15, 1) >> 32;
#else
    (void) state;
    return 0;
#endif
}

MAKE_GET_BITS_WRAPPERS(x128u32)


/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test_x128u32(const CallerAPI *intf)
{
#ifdef UMUL128_FUNC_ENABLED
    Lcg128State obj;
    Lcg128State_init(&obj, 1234567890, 0);
    uint64_t u, u_ref = 0x23fe67ff;// a50c941f;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_x128u32_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
#else
    intf->printf("x64u64 not supported on this platform\n");
    return 1;
#endif
}

////////////////////////////////
///// Portable C99 version /////
////////////////////////////////


/**
 * @brief 128-bit portable LCG state.
 * @details Not optimized for byte order (low endian/big endian) of any
 * specific platform. But `x[0]` is the lower 32-bit word and `x[3]` is
 * the higher 32-bit word.
 */
typedef struct {
    uint32_t x[4];
} Lcg128x32State;


#define HI64(x) ((x) >> 32)
#define LO64(x) ((x) & 0xFFFFFFFF)
#define MUL64(x,y) ((uint64_t)(x) * (uint64_t)(y))
#define SUM64(x,y) ((uint64_t)(x) + (uint64_t)(y))

/**
 * @brief A portable implementation of 128-bit LCG.
 */
static inline uint64_t get_bits_c99_raw(void *state)
{
    //                           lower        higher
    static const uint32_t a[] = {0x0b15f4fd, 0xfc0072fa};
    static const uint32_t c = 12345;
    uint32_t row0[4], row1[3];
    uint64_t mul, sum;
    Lcg128x32State *obj = state;
    // Row 0
    mul = MUL64(a[0], obj->x[0]); row0[0] = LO64(mul);
    mul = MUL64(a[0], obj->x[1]) + HI64(mul); row0[1] = LO64(mul);
    mul = MUL64(a[0], obj->x[2]) + HI64(mul); row0[2] = LO64(mul);
    mul = MUL64(a[0], obj->x[3]) + HI64(mul); row0[3] = LO64(mul);
    // Row 1
    mul = MUL64(a[1], obj->x[0]); row1[0] = LO64(mul);
    mul = MUL64(a[1], obj->x[1]) + HI64(mul); row1[1] = LO64(mul);
    mul = MUL64(a[1], obj->x[2]) + HI64(mul); row1[2] = LO64(mul);
    // Sum rows (update state)
    sum = SUM64(row0[0], c);                   obj->x[0] = LO64(sum);
    sum = SUM64(row0[1], row1[0]) + HI64(sum); obj->x[1] = LO64(sum);
    sum = SUM64(row0[2], row1[1]) + HI64(sum); obj->x[2] = LO64(sum);
    sum = SUM64(row0[3], row1[2]) + HI64(sum); obj->x[3] = LO64(sum);
    // Return upper 32 bits
    return obj->x[3];
}

MAKE_GET_BITS_WRAPPERS(c99)


static void *create_c99(const GeneratorInfo *gi, const CallerAPI *intf)
{
    Lcg128x32State *obj = intf->malloc(sizeof(Lcg128x32State));
    uint64_t seed0 = intf->get_seed64();
    uint64_t seed1 = intf->get_seed64();
    obj->x[0] = LO64(seed0) | 0x1; // For the case of MCG
    obj->x[1] = HI64(seed0);
    obj->x[2] = LO64(seed1);
    obj->x[3] = HI64(seed1);
    (void) gi;
    return obj;
}

/**
 * @brief Self-test to check portable implementation of 96-bit
 * multiplication and addition.
 * @details It is easy to reproduce reference values in Python:
 *
 *     a = 0xfc0072fa0b15f4fd
 *     x = 1234567890
 *     for i in range(0,1000000):
 *         x = (a*x + 12345) % 2**128
 *     print(hex(x))
 *
 */
static int run_self_test_c99(const CallerAPI *intf)
{
    uint64_t u, u_ref = 0x63ea2cac;
    Lcg128x32State obj;
    obj.x[0] = 1234567890; obj.x[1] = 0; obj.x[2] = 0; obj.x[3] = 0;
    for (size_t i = 0; i < 1000000; i++) {
        u = get_bits_c99_raw(&obj);
    }
    intf->printf("Result: %llX; reference value: %llX\n", u, u_ref);
    return u == u_ref;
}

//////////////////////
///// Interfaces /////
//////////////////////


/**
 * @brief Self-test to prevent problems during re-implementation
 * in MSVC and other plaforms that don't support int128.
 */
static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    is_ok = is_ok & run_self_test_x64u64(intf);
    is_ok = is_ok & run_self_test_x128u64(intf);
    is_ok = is_ok & run_self_test_x128u32(intf);
    is_ok = is_ok & run_self_test_c99(intf);
    return is_ok;
}


static const char description[] =
"128-bit LCG with m = 2^128 that returns the upper 32 or 64 bits. The next\n"
"param values are supported:\n"
"  x64 - 64-bit multiplier, 64-bit output (default version)\n"
"  x128u64 - 128-bit multiplier, 128-bit output\n"
"  x128u32 - 128-bit multiplier, 32-bit output\n"
"  c99 - 64-bit multiplier, 32-bit output (portable version)\n"
"These generators pass BigCrush, those ones with 64-bit output fail\n"
"PractRand 0.94 at 128 GiB sample. All of them fail the bspace4_8d_dec test.\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "x64") || !intf->strcmp(param, "")) {
        gi->name = "Lcg128:x64";
        gi->nbits = 64;
        gi->get_bits = get_bits_x64u64;
        gi->get_sum = get_sum_x64u64;
    } else if (!intf->strcmp(param, "x128u64")) {
        gi->name = "Lcg128:x128u64";
        gi->nbits = 64;
        gi->get_bits = get_bits_x128u64;
        gi->get_sum = get_sum_x128u64;
    } else if (!intf->strcmp(param, "x128u32")) {
        gi->name = "Lcg128:x128u64";
        gi->nbits = 32;
        gi->get_bits = get_bits_x128u32;
        gi->get_sum = get_sum_x128u32;
    } else if (!intf->strcmp(param, "c99")) {
        gi->name = "Lcg128:c99";
        gi->nbits = 32;
        gi->create = create_c99;
        gi->get_bits = get_bits_c99;
        gi->get_sum = get_sum_c99;
    } else {
        gi->name = "Lcg128:unknown";
        gi->nbits = 64;
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
