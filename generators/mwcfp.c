/**
 * @file mwcfp.c
 * @brief A modification of MWC generator with base \f$ 2^k + 2 \f$ that has
 * full period and theoretically proven distribution properties.
 * @details MWC-FP PRNG family was designed by A.L. Voskov. See the
 * `misc/mwcfp` directory for some theoretical details, test vectors
 * generators etc.
 *
 * Some multipliers for 32-bit PRNGs:
 *
 *  Base       | lag | Multiplier | log2(m-1) | (m - 1) // 2
 * ------------|-----|------------|-----------|--------------------------
 * 2**64 + 2   | 2   | 4291122658 | 96.00     | 7 * 61609 * p
 * 2**128 + 2  | 4   | 4142557070 | 159.95    | 47 * p
 * 2**256 + 2  | 8   | 4238794375 | 287.97    | 7 * 23 * 439**2 * p
 * 2**512 + 2  | 16  | 4294137855 | 544.00    | 47 * 67 * 1789 * 1165579 * p
 * 2**1024 + 2 | 32  | 4287999874 | 1055.00   | 5 * 7 * 17 * 41 * 1151 * 3253 * 62633 * p
 * 2**2048 + 2 | 64  | 4273733971 | 2079.99   | 2 * 6373 * p
 * 2**8192 + 2 | 256 | 3649336883 | 8223.76   | 2 * p
 *
 * Some multipliers for 64-bit PRNGs:
 *
 *  Base       | lag | Multiplier           | log2(m-1) | (m - 1) // 2
 * ------------|-----|----------------------|-----------|-----------------------
 * 2**128 + 2  | 2   | 17741297344439402706 | 191.94    | 859 * 246131 * p
 * 2**256 + 2  | 4   | 17873945764845871615 | 319.95    | 103 * p
 * 2**512 + 2  | 8   | 16996179571824182298 | 575.88    | 14071 * 32999 * p
 * 2**1024 + 2 | 16  | 18439945329244120106 | 1088.00   | 73 * 647 * 2617 * 165709 * p
 * 2**2048 + 2 | 32  | 18235832631006504774 | 2111.98   | 5 * 11 * 23 * 23293 * p
 * 2**4096 + 2 | 64  | 17633152884372258591 | 4159.93   | 2 * 29 * p
 * 2**16384 + 2| 256 | 17183301495294525414 | 16447.90  | 5 * 5 * p
 *
 * All prime moduli and their m - 1 were certified by means of Primo 4.3.3.
 *
 * Tests results
 *
 *  Generator  | SmokeRand | TestU01     | PractRand 0.96 | cpb
 * ------------|-----------|-------------|----------------|-----
 *  mwc64u32   | full/b64  | +           | >= 16 TiB      | 0.68
 *  mwc128u32  | full      | +           | >= 16 TiB      | 0.68
 *  mwc256u32  | full      | +           |                | 0.90
 *  mwc512u32  | full      | +           |                | 1.3
 *  mwc1024u32 | full      | +           |                | 0.8
 *  mwc2048u32 | full      | +           |                | 1.0
 *  mwc8192u32 | full      | +           | >= 16 TiB      | 0.96
 *  mwc128u64  | full/b64  | +IL/+HI/+LO | >= 16 TiB      | 0.46
 *  mwc256u64  | full      | +IL         |                | 0.41
 *  mwc512u64  | full      |             |                | 0.61
 *  mwc1024u64 | full      |             |                | 0.80
 *  mwc2048u64 | full      |             |                | 0.45
 *  mwc4096u64 | full      |             | >= 16 TiB(?)   | 0.55
 *  mwc16384u64| full      | +IL         | >= 16 TiB      | 0.52
 *
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"

PRNG_CMODULE_PROLOG

#define MAKE_MWCFP_STRUCT(type, inttype, lag) \
typedef struct { \
    inttype x[(lag) + 1]; \
    inttype c; \
    int pos; \
} type;


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


#define GENERATE_GET_BITS64_RRX_FUNC(funcname, type) \
static inline uint64_t get_bits_##funcname##_rrx_raw(type *obj) \
{ \
    const uint64_t u = get_bits_##funcname##_raw(obj); \
    return u ^ rotl64(u, 17) ^ rotl64(u, 53); \
}


#define GENERATE_GET_BITS32_RRX_FUNC(funcname, type) \
static inline uint64_t get_bits_##funcname##_rrx_raw(type *obj) \
{ \
    uint32_t u = (uint32_t) get_bits_##funcname##_raw(obj); \
    u = u ^ rotl32(u, 11) ^ rotl32(u, 27); \
    return u;\
}


#define DECLARE_MWCFP32_VARIANT(funcname, type, mul, lag) \
MAKE_MWCFP_STRUCT(type, uint32_t, lag) \
GENERATE_GET_BITS32_FUNC(get_bits_##funcname##_raw, type, mul, lag) \
GENERATE_CREATE_BITS32_FUNC(create_##funcname, type, lag) \
GENERATE_GET_BITS32_RRX_FUNC(funcname, type) \
MAKE_GET_BITS_WRAPPERS(funcname) \
MAKE_GET_BITS_WRAPPERS(funcname##_rrx) \




#define DECLARE_MWCFP64_VARIANT(funcname, type, mul, lag) \
MAKE_MWCFP_STRUCT(type, uint64_t, lag) \
GENERATE_GET_BITS64_FUNC(get_bits_##funcname##_raw, type, mul, lag) \
GENERATE_CREATE_BITS64_FUNC(create_##funcname, type, lag) \
GENERATE_GET_BITS64_RRX_FUNC(funcname, type) \
MAKE_GET_BITS_WRAPPERS(funcname) \
MAKE_GET_BITS_WRAPPERS(funcname##_rrx)






DECLARE_MWCFP32_VARIANT(mwc64u32,    MwcFp64u32State,    4291122658U, 2)
DECLARE_MWCFP32_VARIANT(mwc128u32,   MwcFp128u32State,   4142557070U, 4)
DECLARE_MWCFP32_VARIANT(mwc256u32,   MwcFp256u32State,   4238794375U, 8)
DECLARE_MWCFP32_VARIANT(mwc512u32,   MwcFp512u32State,   4294137855U, 16)
DECLARE_MWCFP32_VARIANT(mwc1024u32,  MwcFp1024u32State,  4287999874U, 32)
DECLARE_MWCFP32_VARIANT(mwc2048u32,  MwcFp2048u32State,  4273733971U, 64)
DECLARE_MWCFP32_VARIANT(mwc8192u32,  MwcFp8192u32State,  3649336883U, 256)

DECLARE_MWCFP64_VARIANT(mwc128u64,   MwcFp128u64State,   17741297344439402706U, 2)
DECLARE_MWCFP64_VARIANT(mwc256u64,   MwcFp256u64State,   17873945764845871615U, 4)
DECLARE_MWCFP64_VARIANT(mwc512u64,   MwcFp512u64State,   16996179571824182298U, 8)
DECLARE_MWCFP64_VARIANT(mwc1024u64,  MwcFp1024u64State,  18439945329244120106U, 16)
DECLARE_MWCFP64_VARIANT(mwc2048u64,  MwcFp2048u64State,  18235832631006504774U, 32)
DECLARE_MWCFP64_VARIANT(mwc4096u64,  MwcFp4096u64State,  17633152884372258591U, 64)
DECLARE_MWCFP64_VARIANT(mwc16384u64, MwcFp16384u64State, 17183301495294525414U, 256)



DECLARE_MWCFP32_VARIANT(mwc64u32bad,   MwcFp64u32BadState,   123U, 2) // 123
DECLARE_MWCFP64_VARIANT(mwc128u64bad,  MwcFp128u64BadState,  243U, 2) // 243


static inline void *create(const CallerAPI *intf)
{
    intf->printf("Not implemented\n");
    return NULL;
}


static int run_test64(const CallerAPI *intf, uint64_t (*get_bits)(void *),
    void *obj, const uint64_t *u_ref, int lag)
{
    for (long i = 0; i < 10000 * lag; i++) {
        (void) get_bits(obj);
    }
    int is_ok = 1;
    for (int i = 0; i < 8; i++) {
        const uint64_t u = get_bits(obj);
        intf->printf("%llX ", (unsigned long long) u);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->printf("\n");
    return is_ok;
}


static int run_test32(const CallerAPI *intf, uint64_t (*get_bits)(void *),
    void *obj, const uint32_t *u_ref, int lag)
{
    for (long i = 0; i < 10000 * lag; i++) {
        (void) get_bits(obj);
    }
    int is_ok = 1;
    for (int i = 0; i < 8; i++) {
        const uint32_t u = (uint32_t) get_bits(obj);
        intf->printf("%lX ", (unsigned long) u);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->printf("\n");
    return is_ok;
}


static int test64(const CallerAPI *intf)
{
    MwcFp512u64State obj = {.x = {
        0x324486EF33B244DE, 0xBDF3EFA8BFFC4712, 0xC8DBBD5E28D756DF, 0xD30EE545B1860CE8,
        0x8812CF194A614701, 0xC8EF05BA91470D22, 0x15D944BA02AA4CE7, 0x0000000000000001,
        0x0000000000000000}, .c = 0x3B6DDCC704530974, .pos = 8
    };
    static const uint64_t u_ref[8] = {
        0xA4A7BCED2B2A12DA, 0xA87A2252C527DBC0, 0xF40FD080694601A9, 0x4B434187C33BC54B,
        0x7136A7C65B18A544, 0x6B34FD3E458AE6DF, 0x2EAAB4F627081604, 0xD21AE89EE2D61327
    };
    return run_test64(intf, get_bits_mwc512u64, &obj, u_ref, 8);
}


static int test32(const CallerAPI *intf)
{
    MwcFp256u32State obj = {.x = {
        0x9D2B5B2E, 0xD83D1A25, 0x867FCA2B, 0x20F8F49A, 0xAD432DE0,
        0x1673FAF4, 0x03647D52, 0x00000001, 0x00000000}, .c = 0x1D3D06BE, .pos = 8
    };
    static const uint32_t u_ref[8] = {
        0x293E4C79, 0x2883B11C, 0x87454D93, 0xC7341131,
        0x1D1E3837, 0x83D663FE, 0x2EC235C2, 0xB1AD09BA
    };
    return run_test32(intf, get_bits_mwc256u32, &obj, u_ref, 8);
}


static int test64_sm(const CallerAPI *intf)
{
    MwcFp128u64State obj = {.x = {
        0x0A2DE7FD1B0B2669, 0x0000000000000001, 0x0000000000000000},
        .c = 0xE93C76E554BC3DDE, .pos = 2
    };
    static const uint64_t u_ref[8] = {
        0x481CB82ECABB99BA, 0x73A05D57E9365E0E, 0x41E47A1CE2DBDE18, 0xB18E46EC2E938B17,
        0x8D667D5038185DD4, 0x21054F6D3FF80F10, 0x1B6CD39E1B27B198, 0x87DD038B41026317
    };
    return run_test64(intf, get_bits_mwc128u64, &obj, u_ref, 2);
}


static int test32_sm(const CallerAPI *intf)
{
    MwcFp64u32State obj = {.x = {
        0x003AB792, 0x00000001, 0x00000000}, .c = 0x9BDC771C, .pos = 2
    };
    static const uint32_t u_ref[8] = {
        0x662AE453, 0xC23220FD, 0xC82713AC, 0xE0F99B0F,
        0x23DD0069, 0x885B140D, 0xC2589D18, 0x22E5CCFB
    };
    return run_test32(intf, get_bits_mwc64u32, &obj, u_ref, 2);
}




static int run_self_test(const CallerAPI *intf)
{
    const int is32 = test32(intf);
    const int is64 = test64(intf);
    const int is32_sm = test32_sm(intf);
    const int is64_sm = test64_sm(intf);
    return is32 & is64 & is32_sm & is64_sm;
}

#define MAKE_MWCFP_ENTRY(param, tag, nbits, name) \
    {param,       tag,       nbits, create_##name, get_bits_##name,       get_sum_##name}, \
    {param "rrx", tag "rrx", nbits, create_##name, get_bits_##name##_rrx, get_sum_##name##_rrx},

static const GeneratorParamVariant gen_list[] = {
    MAKE_MWCFP_ENTRY("",        "mwc512u64",   64, mwc512u64)
    // 32-bit generators
    MAKE_MWCFP_ENTRY("64u32",   "mwc64u32",    32, mwc64u32)
    MAKE_MWCFP_ENTRY("128u32",  "mwc128u32",   32, mwc128u32)
    MAKE_MWCFP_ENTRY("256u32",  "mwc256u32",   32, mwc256u32)
    MAKE_MWCFP_ENTRY("512u32",  "mwc512u32",   32, mwc512u32)
    MAKE_MWCFP_ENTRY("1024u32", "mwc1024u32",  32, mwc1024u32)
    MAKE_MWCFP_ENTRY("2048u32", "mwc2048u32",  32, mwc2048u32)
    MAKE_MWCFP_ENTRY("8192u32", "mwc8192u32",  32, mwc8192u32)
    // 64-bit generators
    MAKE_MWCFP_ENTRY("128u64",  "mwc128u64",   64, mwc128u64)
    MAKE_MWCFP_ENTRY("256u64",  "mwc256u64",   64, mwc256u64)
    MAKE_MWCFP_ENTRY("512u64",  "mwc512u64",   64, mwc512u64)
    MAKE_MWCFP_ENTRY("1024u64", "mwc1024u64",  64, mwc1024u64)
    MAKE_MWCFP_ENTRY("2048u64", "mwc2048u64",  64, mwc2048u64)
    MAKE_MWCFP_ENTRY("4096u64", "mwc4096u64",  64, mwc4096u64)
    MAKE_MWCFP_ENTRY("16384u64","mwc16384u64", 64, mwc16384u64)
    // Bad generators
    MAKE_MWCFP_ENTRY("64u32bad",   "mwc64u32bad",   32, mwc64u32bad)
    MAKE_MWCFP_ENTRY("128u64bad",  "mwc128u64bad",  64, mwc128u64bad)
    GENERATOR_PARAM_VARIANT_EMPTY
};


static char description[8192] = "";

static void fill_description(const CallerAPI *intf)
{
    int pos = 0;
    pos += intf->snprintf(description + pos, (size_t) (8192 - pos), "%s", "MWCFP\n");
    for (const GeneratorParamVariant *gen = gen_list; gen->name != NULL; gen++) {
        pos += intf->snprintf(description + pos, (size_t) (8192 - pos), "  %12s : %s\n",
            gen->param, gen->name);
    }
}

int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    if (description[0] == 0) {
        fill_description(intf);
    }
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
