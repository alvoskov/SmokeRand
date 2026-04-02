/**
 * @file mwcfp.c
 * @details See the `misc/mwcfp` directory.
 */

#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

#define MAKE_MWCFP_STRUCT(type, inttype, lag) \
typedef struct { \
    inttype x[lag + 1]; \
    inttype c; \
    int pos; \
} type;

MAKE_MWCFP_STRUCT(MwcFp64u32State,   uint32_t, 2) // lag 2
MAKE_MWCFP_STRUCT(MwcFp128u64State,  uint64_t, 2)
MAKE_MWCFP_STRUCT(MwcFp256u64State,  uint64_t, 4) // lag 4

MAKE_MWCFP_STRUCT(MwcFp256u32State,  uint32_t, 8) // lag 8
MAKE_MWCFP_STRUCT(MwcFp512u64State,  uint64_t, 8)
MAKE_MWCFP_STRUCT(MwcFp1024u64State, uint64_t, 16) // lag 16

static inline uint32_t sub32b(uint32_t *a, uint32_t b)
{
    const uint32_t t = *a - b;
    const uint32_t c = (t > *a) ? 1 : 0; // borrow
    *a = t;
    return c;
}

static inline uint32_t add32c(uint32_t *a, uint32_t b)
{
    const uint32_t t = *a + b;
    const uint32_t c = (t < *a) ? 1 : 0; // carry
    *a = t;
    return c;
}


static inline uint64_t sub64b(uint64_t *a, uint64_t b)
{
    const uint64_t t = *a - b;
    const uint64_t c = (t > *a) ? 1 : 0; // borrow
    *a = t;
    return c;
}

static inline uint64_t add64c(uint64_t *a, uint64_t b)
{
    const uint64_t t = *a + b;
    const uint64_t c = (t < *a) ? 1 : 0; // carry
    *a = t;
    return c;
}


#define GENERATE_GET_BITS64_FUNC(funcname, type, mul, lag) \
static inline uint64_t funcname(type *obj) \
{ \
    if (obj->pos == (lag)) { \
        static const uint64_t a = (mul); \
        /* U = aX + c = H*2^(kw) + L */ \
        uint64_t c = obj->c; \
        for (int i = 0; i < (lag); i++) { \
            obj->x[i] = unsigned_muladd128(a, obj->x[i], c, &c); \
        } \
        obj->c = a * obj->x[(lag)] + c; /* H */ \
        /* H*2^(kw) + L = H*(2^(kw) + 2) + (L - 2*H) */ \
        uint64_t b = sub64b(&obj->x[0], obj->c << 1); \
        b = sub64b(&obj->x[1], (obj->c >> 63) + b); \
        for (int i = 2; i < (lag); i++) { \
            b = sub64b(&obj->x[i], b); \
        } \
        /* Process a special case: L < 2*H \
           H*2^(kw) + L = (H - 1)*(2^(kw) + 2) + (2^(kw) + 2 + L - 2*H) \
           Note that (L - 2*H) mod 2^(kw) gives (L - H + 2^(kw)) */ \
        if (b == 0) { \
            obj->x[(lag)] = 0; \
        } else { \
            obj->c--; \
            c = 2; \
            for (int i = 0; i < (lag); i++) { \
                c = add64c(&obj->x[i], c); \
            } \
            obj->x[(lag)] = c; \
        } \
        obj->pos = 0; \
    } \
    const uint64_t u = obj->x[obj->pos++]; \
    return u; \
}


#define GENERATE_GET_BITS32_FUNC(funcname, type, mul, lag) \
static inline uint64_t funcname(type *obj) \
{ \
    if (obj->pos == (lag)) { \
        static const uint64_t a = (mul); \
        /* U = aX + c = H*2^(kw) + L */ \
        uint32_t c = obj->c; \
        for (int i = 0; i < (lag); i++) { \
            uint64_t ui = a * obj->x[i] + c; \
            obj->x[i] = (uint32_t) ui; c = (uint32_t) (ui >> 32); \
        } \
        obj->c = (uint32_t) (a * obj->x[(lag)] + c); /* H */ \
        /* H*2^(kw) + L = H*(2^(kw) + 1) + (L - 2H) */ \
        uint32_t b = sub32b(&obj->x[0], obj->c << 1); \
        b = sub32b(&obj->x[1], (obj->c >> 31) + b); \
        for (int i = 2; i < (lag); i++) { \
            b = sub32b(&obj->x[i], b); \
        } \
        /* Process a special case: L < H \
           H*2^(kw) + L = (H - 1)*(2^(kw) + 2) + (2^(kw) + 2 + L - H) \
           Note that (L - H) mod 2^(kw) gives (L - H + 2^(kw)) */ \
        if (b == 0) { \
            obj->x[(lag)] = 0; \
        } else { \
            obj->c--; \
            c = 2; \
            for (int i = 0; i < (lag); i++) { \
                c = add32c(&obj->x[i], c); \
            } \
            obj->x[(lag)] = c; \
        } \
        obj->pos = 0; \
    } \
    const uint64_t u = obj->x[obj->pos++]; \
    return u; \
}


#define GENERATE_CREATE_BITS64_FUNC(funcname, type, lag) \
static void *funcname(const GeneratorInfo *gi, const CallerAPI *intf) \
{ \
    type *obj = intf->malloc(sizeof(type)); \
    obj->x[(lag)] = 0; \
    obj->c = 1; \
    obj->pos = (lag); \
    for (int i = 0; i < (lag); i++) { \
        obj->x[i] = intf->get_seed64(); \
    } \
    (void) gi; \
    return obj; \
}



#define GENERATE_CREATE_BITS32_FUNC(funcname, type, lag) \
static void *funcname(const GeneratorInfo *gi, const CallerAPI *intf) \
{ \
    type *obj = intf->malloc(sizeof(type)); \
    obj->x[(lag)] = 0; \
    obj->c = 1; \
    obj->pos = (lag); \
    for (int i = 0; i < (lag); i++) { \
        obj->x[i] = intf->get_seed32(); \
    } \
    (void) gi; \
    return obj; \
}

// Smokerand `full`, >= 1 TiB in PractRand
GENERATE_GET_BITS32_FUNC(get_bits_mwc64u32_raw, MwcFp64u32State, 4280626598U, 2)
GENERATE_CREATE_BITS32_FUNC(create_mwc64u32, MwcFp64u32State, 2)
MAKE_GET_BITS_WRAPPERS(mwc64u32)


GENERATE_GET_BITS64_FUNC(get_bits_mwc128u64_raw, MwcFp128u64State, 15588520075796777254U, 2)
GENERATE_CREATE_BITS64_FUNC(create_mwc128u64, MwcFp128u64State, 2)
MAKE_GET_BITS_WRAPPERS(mwc128u64)


GENERATE_GET_BITS64_FUNC(get_bits_mwc256u64_raw, MwcFp256u64State, 18145911855674681826U, 4)
GENERATE_CREATE_BITS64_FUNC(create_mwc256u64, MwcFp256u64State, 4)
MAKE_GET_BITS_WRAPPERS(mwc256u64)


GENERATE_GET_BITS32_FUNC(get_bits_mwc256u32_raw, MwcFp256u32State, 3906776790U, 8)
GENERATE_CREATE_BITS32_FUNC(create_mwc256u32, MwcFp256u32State, 8)
MAKE_GET_BITS_WRAPPERS(mwc256u32)

GENERATE_GET_BITS64_FUNC(get_bits_mwc512u64_raw, MwcFp512u64State, 13115896780146644418U, 8)
GENERATE_CREATE_BITS64_FUNC(create_mwc512u64, MwcFp512u64State, 8)
MAKE_GET_BITS_WRAPPERS(mwc512u64)

GENERATE_GET_BITS64_FUNC(get_bits_mwc1024u64_raw, MwcFp1024u64State, 16279984197873894135U, 16)
GENERATE_CREATE_BITS64_FUNC(create_mwc1024u64, MwcFp1024u64State, 1)
MAKE_GET_BITS_WRAPPERS(mwc1024u64)





static inline void *create(const CallerAPI *intf)
{
    intf->printf("Not implemented\n");
    return NULL;
}


static int test64(const CallerAPI *intf)
{
    MwcFp512u64State obj = {.x = {
        0xFDA936613D964DE2, 0x230E2714AB73277A, 0x2D7BE8A211C78E77, 0x3338F1F72C51C29D,
        0xB999BD769AADFFC3, 0x5CD57E1B2A0CD2AC, 0x680C9013808B8B4B, 0x0000000000000001,
        0x0000000000000000}, .c = 0x59B3E275D49C70BC, .pos = 8
    };
    static const uint64_t u_ref[8] = {
        0x2B5551C4D471D646, 0x06C82C311FE8E96B, 0x0970F57639CF763D, 0xF9B6C42413F7E6C0,
        0x442E1F05A9761116, 0x2A37666B6EACFAF4, 0x7CA892C9C8A8C893, 0x58748796433867A6
    };
    for (long i = 0; i < 80000; i++) {
        (void) get_bits_mwc512u64_raw(&obj);
    }
    int is_ok = 1;
    for (int i = 0; i < 8; i++) {
        const uint64_t u = get_bits_mwc512u64_raw(&obj);
        intf->printf("%llX ", (unsigned long long) u);
        if (i % 8 == 7) {
            intf->printf("| %llX %llX\n",
                (unsigned long long) obj.c, (unsigned long long) obj.x[8]);
        }
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->printf("\n");
    return is_ok;
}


static int test32(const CallerAPI *intf)
{
    MwcFp256u32State obj = {.x = {
        0xACB71003, 0x6F48A711, 0xAC5BE03C, 0xC8F6F2ED, 0x5E9FAD90,
        0xB5C426CA, 0x196FE0B6, 0x00000001, 0x00000000}, .c = 0xB782937E, .pos = 8
    };
    static const uint32_t u_ref[8] = {
        0xB57B4261, 0x1AAAD0F7, 0x4AC01366, 0x3C8356DE,
        0x5F3224B0, 0x851648E9, 0xCBA327C0, 0xD9731AC3
    };
    for (long i = 0; i < 80000; i++) {
        (void) get_bits_mwc256u32_raw(&obj);
    }
    int is_ok = 1;
    for (int i = 0; i < 8; i++) {
        const uint32_t u = (uint32_t) get_bits_mwc256u32_raw(&obj);
        intf->printf("%lX ", (unsigned long) u);
        if (i % 8 == 7) {
            intf->printf("| %lX %lX\n",
                (unsigned long) obj.c, (unsigned long) obj.x[8]);
        }
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->printf("\n");
    return is_ok;
}



static int run_self_test(const CallerAPI *intf)
{
    const int is32 = test32(intf);
    const int is64 = test64(intf);
    return is32 | is64;
}

static const GeneratorParamVariant gen_list[] = {
    {"",       "mwc512u64", 64, create_mwc512u64, get_bits_mwc512u64, get_sum_mwc512u64},
    {"64u32",  "mwc64u32", 32, create_mwc64u32, get_bits_mwc64u32, get_sum_mwc64u32},
    {"128u64", "mwc128u64", 64, create_mwc128u64, get_bits_mwc128u64, get_sum_mwc128u64},
    {"256u64", "mwc256u64", 64, create_mwc256u64, get_bits_mwc256u64, get_sum_mwc256u64},
    {"256u32", "mwc256u32", 32, create_mwc256u32, get_bits_mwc256u32, get_sum_mwc256u32},
    {"512u64", "mwc512u64", 64, create_mwc512u64, get_bits_mwc512u64, get_sum_mwc512u64},
    {"1024u64", "mwc1024u64", 64, create_mwc1024u64, get_bits_mwc1024u64, get_sum_mwc1024u64},
    GENERATOR_PARAM_VARIANT_EMPTY
};


static const char description[] =
"mwcfp\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
