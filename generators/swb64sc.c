/**
 * @file swb64sc.c
 * @brief 64-bit SWB (subtract-with-borrow) generator with an output function
 * (scrambler) that hides its artefacts.
 * @details
 *
 * (9,8)>=16 TiB; smokerand full; TestU01 small/crush/big: +IL/+HI/+LO
 * (13,7)>=16 TiB; smokerand full; TestU01 small/crush/big: +IL/+HI/+LO
 * (99,35)>=16 TiB; TestU01 small/crush/big: +IL/+HI/+LO
 *
 * Fast check:
 *
 * (26,4)
 * (30,6)  
 * (67,59)
 * (71,47)
 * (77,56)
 * (89,11)
 * (98,90)
 * (117,50)
 *
 * Python code for verification:
 *
 *    def rotl64(x, r):
 *        return ((x << r) | (x >> (64 - r))) % 2**64
 *
 *    class Swb64:
 *        def __init__(self):
 *            self.r, self.s = 13, 7
 *            self.x = [x + 1000 for x in range(0, self.r)]
 *            self.x[0] = 2**64 - 1
 *            self.x[self.r - self.s] = 0
 *            self.c = 1
 *
 *        @staticmethod
 *        def scramble(x):
 *            t = (x + (x * x | 0x40000005)) % 2**64
 *            return t ^ rotl64(t, 13) ^ rotl64(t, 47)
 *        
 *        def next(self):
 *            xj, xi = self.x[self.r - self.s], self.x[0]
 *            d = xj - xi - self.c
 *            xn = d % 2**64
 *            self.c = 1 if d < 0 else 0
 *            self.x = self.x[1:] + [xn]
 *            return self.scramble(xn)
 *
 *    swb = Swb64()
 *
 *    for i in range(1_000_000):
 *        swb.next()
 *
 *    for i in range(16):
 *        print(hex(swb.next()))
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB64_DECIM 3

#define SWB64_UPDATE_BUFFER(r_swb, s_swb) \
    for (int j = 0; j < s_swb; j++) { \
        obj->x[j] = swb_u64(obj->x[j + (r_swb - s_swb)], obj->x[j], obj->c, &obj->c); \
    } \
    for (int j = s_swb; j < r_swb; j++) { \
        obj->x[j] = swb_u64(obj->x[j - s_swb], obj->x[j], obj->c, &obj->c); \
    } \


#define SWB64SC_TEMPLATE(r_swb, s_swb) \
typedef struct { \
    uint64_t x[r_swb]; \
    uint64_t c; \
    size_t pos; \
} Swb64ScR##r_swb##S##s_swb##State; \
static inline uint64_t get_bits_r##r_swb##s##s_swb##sc_raw(Swb64ScR##r_swb##S##s_swb##State *obj) { \
    if (obj->pos == r_swb) { \
        SWB64_UPDATE_BUFFER(r_swb, s_swb) \
        obj->pos = 0; \
    } \
    uint64_t out = obj->x[obj->pos++]; \
    out += out * out | 0x40000005; \
    out ^= rotl64(out, 13) ^ rotl64(out, 47); \
    return out; \
} \
static void *create_r##r_swb##s##s_swb##sc(const GeneratorInfo *gi, const CallerAPI *intf) { \
    Swb64ScR##r_swb##S##s_swb##State *obj = intf->malloc(sizeof(Swb64ScR##r_swb##S##s_swb##State)); \
    (void) gi; \
    expand_seed64_to_u64(obj->x, r_swb, intf->get_seed64()); \
    obj->c = (obj->x[0] == 0) ? 1 : 0; \
    obj->pos = r_swb; \
    return obj; \
} \
MAKE_GET_BITS_WRAPPERS(r##r_swb##s##s_swb##sc)


#define SWB64DEC_TEMPLATE(r_swb, s_swb) \
typedef struct { \
    uint64_t x[r_swb]; \
    uint64_t c; \
    size_t pos; \
    int decim; \
} Swb64DecR##r_swb##S##s_swb##State; \
static inline uint64_t get_bits_r##r_swb##s##s_swb##dec_raw(Swb64DecR##r_swb##S##s_swb##State *obj) { \
    if (obj->pos == r_swb) { \
        for (int i = 0; i < obj->decim; i++) { \
            SWB64_UPDATE_BUFFER(r_swb, s_swb) \
        } \
        obj->pos = 0; \
    } \
    return obj->x[obj->pos++]; \
} \
static void *create_r##r_swb##s##s_swb##dec(const GeneratorInfo *gi, const CallerAPI *intf) { \
    Swb64DecR##r_swb##S##s_swb##State *obj = intf->malloc(sizeof(Swb64DecR##r_swb##S##s_swb##State)); \
    (void) gi; \
    expand_seed64_to_u64(obj->x, r_swb, intf->get_seed64()); \
    obj->c = (obj->x[0] == 0) ? 1 : 0; \
    obj->pos = r_swb; \
    obj->decim = SWB64_DECIM + 1; \
    return obj; \
} \
MAKE_GET_BITS_WRAPPERS(r##r_swb##s##s_swb##dec)


// The most important generators (the shortest lags and the best lags)
SWB64SC_TEMPLATE(9, 8)
SWB64SC_TEMPLATE(13, 7)
SWB64SC_TEMPLATE(99, 35)
// The less important generators
SWB64SC_TEMPLATE(26, 4)
SWB64SC_TEMPLATE(30, 6)  
SWB64SC_TEMPLATE(67, 59)
SWB64SC_TEMPLATE(71, 47)
SWB64SC_TEMPLATE(77, 56)
SWB64SC_TEMPLATE(89, 11)
SWB64SC_TEMPLATE(98, 90)
SWB64SC_TEMPLATE(117, 50)

// --- Generators that use decimation
// The most important generators (the shortest lags and the best lags)
SWB64DEC_TEMPLATE(9, 8)
SWB64DEC_TEMPLATE(13, 7)
SWB64DEC_TEMPLATE(99, 35)
// The less important generators
SWB64DEC_TEMPLATE(26, 4)
SWB64DEC_TEMPLATE(30, 6)  
SWB64DEC_TEMPLATE(67, 59)
SWB64DEC_TEMPLATE(71, 47)
SWB64DEC_TEMPLATE(77, 56)
SWB64DEC_TEMPLATE(89, 11)
SWB64DEC_TEMPLATE(98, 90)
SWB64DEC_TEMPLATE(117, 50)



static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t u_ref[16] = { // For r=13, s=7
        0xd4076b83f48cb3df, 0xf333cb436a9ec32,
        0xace36c03140c2397, 0xed3a1bf9d97c9a39,
        0x491fee38d8d58df0, 0xaddb1325a17b8f95,
        0x4369d4b71fde91b4, 0x422fd412aeede7b,
        0x503ad2a192ecbec2, 0x1c80c03efd4bc81b,
        0x69940b6b875d05ab, 0x841f711f72fa7bb8,
        0x9c84ca32fabb8ec0, 0x5b4856e5b0539486,
        0xc2ff6d108c16cd47, 0x174838608b788585
    };
    const size_t r = 13, s = 7;
    Swb64ScR13S7State *obj = create_r13s7sc(NULL, intf);
    for (size_t i = 0; i < r; i++) {
        obj->x[i] = 1000U + i;
    }
    obj->x[0] = 0xFFFFFFFFFFFFFFFFU;
    obj->x[r - s] = 0;
    obj->c = 1;

    for (long i = 0; i < 1000000; i++) {
        (void) get_bits_r13s7sc_raw(obj);
    }
    int is_ok = 1;
    for (size_t i = 0; i < 16; i++) {
        const uint64_t u = get_bits_r13s7sc_raw(obj);
        intf->printf("%16.16llX %16.16llX\n",
            (unsigned long long) u, (unsigned long long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}


static void *create(const CallerAPI *intf)
{
    (void) intf;
    return NULL;
}

static const GeneratorParamVariant gen_list[] = {
    // Scrambled
    {"",          "SWB(2**64,7,13)[*]",   64, create_r13s7sc,   get_bits_r13s7sc,   get_sum_r13s7sc},
    {"13-7-sc",   "SWB(2**64,7,13)[*]",   64, create_r13s7sc,   get_bits_r13s7sc,   get_sum_r13s7sc},
    {"9-8-sc",    "SWB(2**64,8,9)[*]",    64, create_r9s8sc,    get_bits_r9s8sc,    get_sum_r9s8sc},
    {"99-35-sc",  "SWB(2**64,35,99)[*]",  64, create_r99s35sc,  get_bits_r99s35sc,  get_sum_r99s35sc},
    {"26-4-sc",   "SWB(2**64,4,26)[*]",   64, create_r26s4sc,   get_bits_r26s4sc,   get_sum_r26s4sc},
    {"30-6-sc",   "SWB(2**64,6,30)[*]",   64, create_r30s6sc,   get_bits_r30s6sc,   get_sum_r30s6sc},
    {"67-59-sc",  "SWB(2**64,59,67)[*]",  64, create_r67s59sc,  get_bits_r67s59sc,  get_sum_r67s59sc},
    {"71-47-sc",  "SWB(2**64,47,71)[*]",  64, create_r71s47sc,  get_bits_r71s47sc,  get_sum_r71s47sc},
    {"77-56-sc",  "SWB(2**64,56,77)[*]",  64, create_r77s56sc,  get_bits_r77s56sc,  get_sum_r77s56sc},
    {"89-11-sc",  "SWB(2**64,11,89)[*]",  64, create_r89s11sc,  get_bits_r89s11sc,  get_sum_r89s11sc},
    {"98-90-sc",  "SWB(2**64,90,98)[*]",  64, create_r98s90sc,  get_bits_r98s90sc,  get_sum_r98s90sc},
    {"117-50-sc", "SWB(2**64,50,117)[*]", 64, create_r117s50sc, get_bits_r117s50sc, get_sum_r117s50sc},
    // Decimation
    {"13-7-dec",   "SWB(2**64,7,13)[dec]",   64, create_r13s7dec,   get_bits_r13s7dec,   get_sum_r13s7dec},
    {"9-8-dec",    "SWB(2**64,8,9)[dec]",    64, create_r9s8dec,    get_bits_r9s8dec,    get_sum_r9s8dec},
    {"99-35-dec",  "SWB(2**64,35,99)[dec]",  64, create_r99s35dec,  get_bits_r99s35dec,  get_sum_r99s35dec},
    {"26-4-dec",   "SWB(2**64,4,26)[dec]",   64, create_r26s4dec,   get_bits_r26s4dec,   get_sum_r26s4dec},
    {"30-6-dec",   "SWB(2**64,6,30)[dec]",   64, create_r30s6dec,   get_bits_r30s6dec,   get_sum_r30s6dec},
    {"67-59-dec",  "SWB(2**64,59,67)[dec]",  64, create_r67s59dec,  get_bits_r67s59dec,  get_sum_r67s59dec},
    {"71-47-dec",  "SWB(2**64,47,71)[dec]",  64, create_r71s47dec,  get_bits_r71s47dec,  get_sum_r71s47dec},
    {"77-56-dec",  "SWB(2**64,56,77)[dec]",  64, create_r77s56dec,  get_bits_r77s56dec,  get_sum_r77s56dec},
    {"89-11-dec",  "SWB(2**64,11,89)[dec]",  64, create_r89s11dec,  get_bits_r89s11dec,  get_sum_r89s11dec},
    {"98-90-dec",  "SWB(2**64,90,98)[dec]",  64, create_r98s90dec,  get_bits_r98s90dec,  get_sum_r98s90dec},
    {"117-50-dec", "SWB(2**64,50,117)[dec]", 64, create_r117s50dec, get_bits_r117s50dec, get_sum_r117s50dec},
    GENERATOR_PARAM_VARIANT_EMPTY
};

static const char description[] =
"SWB64SC is the scrambled 64-bit subtract-with-borrow PRNG.\n"
"The next param values are supported:\n"
"  13-7-sc - default one for SWB(2**64,7,13)[*]\n"
"  9-8-sc  - the smallest one\n"
"  99-35-sc - recommended large\n"
"  26-4-sc; 30-6-sc; 67-59-sc; 71-47-sc; 77-56-sc; 89-11-sc; 98-90-sc; 117-50-sc\n"
"  Also `-dec` modification (decimation) are supported\n";

int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->self_test = run_self_test;
    return GeneratorParamVariant_find(gen_list, intf, param, gi);
}
