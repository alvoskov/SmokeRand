/*
c, x, b = 0, 2**512, 2**512 + 1
for i in range(10_000 + 8):
    u = 16166422908038702740 * x + c
    x, c = u % b, u // b
    if i >= 10_000:
        for j in range(8):
            print(f"0x{(x >> (j * 64)) % 2**64:016X}", end=' ')
        print("")



import sympy, random
b = 2**32 + 1

for i in range(10000000):
    #a = random.randint(2**63 + 1, 2**64 - 1)
    a = random.randint(2**31 + 1, 2**32 - 1)
    m = a*b - 1
    if sympy.isprime(m):
        print(a)
        f = sympy.factorint((m - 1) // 2, limit=100_000)
        if all([sympy.isprime(x) for x in f]):
            o = sympy.n_order(2, m)
            if o == m - 1:
                print("WOW!", a)                
                break
            else:
                print("====>", o, o / m)


 */

#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t x[9];
    uint64_t c;
    int pos;
} MwcFpState;


typedef struct {
    uint64_t x[17];
    uint64_t c;
    int pos;
} MwcFp1024u64State;


typedef struct {
    uint64_t x[5];
    uint64_t c;
    int pos;
} MwcFp256u64State;


typedef struct {
    uint32_t x[9];
    uint32_t c;
    int pos;
} MwcFp256u32State;


typedef struct {
    uint32_t x[2];
    uint32_t c;
    int pos;
} MwcFp32u32State;


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

GENERATE_GET_BITS64_FUNC(get_bits_mwc256u64_raw, MwcFp256u64State, 18145911855674681826U, 4)
GENERATE_CREATE_BITS64_FUNC(create_mwc256u64, MwcFp256u64State, 4)
MAKE_GET_BITS_WRAPPERS(mwc256u64)


GENERATE_GET_BITS64_FUNC(get_bits_mwc512u64_raw, MwcFpState, 13115896780146644418U, 8)
GENERATE_CREATE_BITS64_FUNC(create_mwc512u64, MwcFpState, 8)
MAKE_GET_BITS_WRAPPERS(mwc512u64)

GENERATE_GET_BITS64_FUNC(get_bits_mwc1024u64_raw, MwcFp1024u64State, 16279984197873894135U, 16)
GENERATE_CREATE_BITS64_FUNC(create_mwc1024u64, MwcFp1024u64State, 1)
MAKE_GET_BITS_WRAPPERS(mwc1024u64)

GENERATE_GET_BITS32_FUNC(get_bits_mwc256u32_raw, MwcFp256u32State, 3906776790U, 8)
GENERATE_CREATE_BITS32_FUNC(create_mwc256u32, MwcFp256u32State, 8)
MAKE_GET_BITS_WRAPPERS(mwc256u32)





static inline void *create(const CallerAPI *intf)
{
    intf->printf("Not implemented\n");
    return NULL;
}


static int test64(const CallerAPI *intf)
{
    MwcFpState *obj = intf->malloc(sizeof(MwcFpState));    
    for (int i = 0; i < 8; i++) {
        obj->x[i] = 0;
    }
    obj->x[8] = 1;
    obj->c = 0;
    obj->pos = 8;
    for (long i = 0; i < 80000; i++) {
        (void) get_bits_mwc512u64_raw(obj);
    }
    for (int i = 0; i < 32; i++) {
        const uint64_t u = get_bits_mwc512u64_raw(obj);
        intf->printf("%llX ", (unsigned long long) u);
        if (i % 8 == 7) {
            intf->printf("| %llX %llX\n",
                (unsigned long long) obj->c, (unsigned long long) obj->x[8]);
        }
    }
    intf->printf("\n");
    intf->free(obj);
    return 1;
}


static int test32(const CallerAPI *intf)
{
    MwcFp256u32State *obj = intf->malloc(sizeof(MwcFp256u32State));
    for (int i = 0; i < 8; i++) {
        obj->x[i] = 0;
    }
    obj->x[8] = 1;
    obj->c = 0;
    obj->pos = 8;
    for (long i = 0; i < 80000; i++) {
        (void) get_bits_mwc256u32_raw(obj);
    }
    for (int i = 0; i < 32; i++) {
        const uint32_t u = (uint32_t) get_bits_mwc256u32_raw(obj);
        intf->printf("%lX ", (unsigned long) u);
        if (i % 8 == 7) {
            intf->printf("| %lX %lX\n",
                (unsigned long) obj->c, (unsigned long) obj->x[8]);
        }
    }
    intf->printf("\n");
    intf->free(obj);
    return 1;
}



static int run_self_test(const CallerAPI *intf)
{
    const int is32 = test32(intf);
    const int is64 = test64(intf);
    return is32 | is64;
}

static const GeneratorParamVariant gen_list[] = {
    {"",       "mwc512u64", 64, create_mwc512u64, get_bits_mwc512u64, get_sum_mwc512u64},
    {"512u64", "mwc512u64", 64, create_mwc512u64, get_bits_mwc512u64, get_sum_mwc512u64},
    {"1024u64", "mwc1024u64", 64, create_mwc1024u64, get_bits_mwc1024u64, get_sum_mwc1024u64},
    {"256u64", "mwc256u64", 64, create_mwc256u64, get_bits_mwc256u64, get_sum_mwc256u64},
    {"256u32", "mwc256u32", 32, create_mwc256u32, get_bits_mwc256u32, get_sum_mwc256u32},
//    {"32u32",  "mwc32u32",  32, create_mwc32u32,  get_bits_mwc32u32,  get_sum_mwc32u32},
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
