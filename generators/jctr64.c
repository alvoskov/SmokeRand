/**
 * @brief jctr64 is a counter-based PRNG based on an experimental block
 * cipher by Bob Jenkins.
 * @details The original cipher is a 512-bit block ARX cipher based on 64-bit
 * arithmetics. The author didn't made a formal cryptoanalysis and positioned
 * it as "use this only if you are a fool". It is also used as during the
 * official ISAAC64 initialization procedure.
 * 
 * Modifications made by A.L. Voskov:
 *
 * 1. Number of rounds was reduced to 6 (even 4 round pass SmokeRand `full`
 *    battery, and it is not for crypto anyway).
 * 2. It is converted to the construction similar to the stream cipher
 *    like ChaCha20.
 * 3. A version optimized for SIMD (AVX2) instructions.
 *
 * WARNING! NOT FOR CRYPTOGRAPHY! Use only as a general purpose CBPRNG!
 *
 * Refences:
 *
 * 1. https://burtleburtle.net/bob/crypto/myblock.html
 * 2. https://burtleburtle.net/bob/c/myblock.c
 * 3. https://oeis.org/A062964
 * 4. https://gist.github.com/retrohacker/e5fff72b7b75ee058924
 *
 * @copyright The original 512-bit ARX block cipher was designed
 * by Bob Jenkins and released as Public Domain.
 *
 * Conversion to `jctr64` counter-based PRNG:
 * 
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#ifdef __AVX2__
#include "smokerand/x86exts.h"
#endif

#define JCTR64_PI0 0x243F6A8885A308D3
#define JCTR64_PI1 0x13198A2E03707344
#define JCTR64_PI2 0xA4093822299F31D0
#define JCTR64_PI3 0x082EFA98EC4E6C89

////////////////////////////////////////////////////
///// Cross-platform (portable) implementation /////
////////////////////////////////////////////////////

/**
 * @brief Jctr64 counter-based PRNG state.
 * @details The state has the next layout:
 *
 *     | pi   key   ctr_lo  ctr_hi |
 *     | key  pi    pi      pi     |
 */
typedef struct {
    uint64_t x[8];   ///< Working state
    uint64_t out[8]; ///< Output state
    size_t pos;
} Jctr64State;

/**
 * @brief Taken from ISAAC64 source code equipped with test vectors.
 */
static inline void jctr64_round(uint64_t *x)
{
    x[0] -= x[4]; x[5] ^= x[7] >> 9;  x[7] += x[0];
    x[1] -= x[5]; x[6] ^= x[0] << 9;  x[0] += x[1];
    x[2] -= x[6]; x[7] ^= x[1] >> 23; x[1] += x[2];
    x[3] -= x[7]; x[0] ^= x[2] << 15; x[2] += x[3];
    x[4] -= x[0]; x[1] ^= x[3] >> 14; x[3] += x[4];
    x[5] -= x[1]; x[2] ^= x[4] << 20; x[4] += x[5];
    x[6] -= x[2]; x[3] ^= x[5] >> 17; x[5] += x[6];
    x[7] -= x[3]; x[4] ^= x[6] << 14; x[6] += x[7];
}


void Jctr64State_block(Jctr64State *obj)
{
    for (size_t i = 0; i < 8; i++) {
        obj->out[i] = obj->x[i];
    }
    // 4 rounds - pass SmokeRand `full` battery
    jctr64_round(obj->out);
    jctr64_round(obj->out);
    jctr64_round(obj->out);
    jctr64_round(obj->out);
    // 2 rounds for safety margin
    jctr64_round(obj->out);
    jctr64_round(obj->out);
    for (size_t i = 0; i < 8; i++) {
        obj->out[i] += obj->x[i];
    }
}


void Jctr64State_init(Jctr64State *obj, const uint64_t *key, uint64_t ctr)
{
    obj->x[0] = JCTR64_PI0; obj->x[1] = key[0];
    obj->x[2] = ctr;        obj->x[3] = 0; // Counter: lower and upper half
    obj->x[4] = key[1];     obj->x[5] = JCTR64_PI1;
    obj->x[6] = JCTR64_PI2; obj->x[7] = JCTR64_PI3;
    obj->pos = 0;
    Jctr64State_block(obj);
}


static inline void Jctr64State_inc_counter(Jctr64State *obj)
{
    obj->x[2]++;
}


static inline uint64_t get_bits_scalar_raw(void *state)
{
    Jctr64State *obj = state;
    uint64_t x = obj->out[obj->pos++];
    if (obj->pos == 8) {
        Jctr64State_inc_counter(obj);
        Jctr64State_block(obj);
        obj->pos = 0;
    }
    return x;
}


MAKE_GET_BITS_WRAPPERS(scalar);


static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    uint64_t key[2];    
    Jctr64State *obj = intf->malloc(sizeof(Jctr64State));
    (void) gi;
    key[0] = intf->get_seed64();
    key[1] = intf->get_seed64();
    Jctr64State_init(obj, key, 0);
    return obj;
}


////////////////////////////////////////
///// AVX2 (vector) implementation /////
////////////////////////////////////////

#define JCTR64_NCOPIES 4

typedef union {
    uint64_t u64[JCTR64_NCOPIES];
} Jctr64Element;

/**
 * @brief Jctr64 counter-based PRNG state.
 * @details It contains for copies of the generator, each
 * copy has the next layout:
 *
 *     | pi   key   ctr_lo  ctr_hi |
 *     | key  pi    pi      pi     |
 *
 * The main idea is to load x[i] from four copies of the generator to the
 * single 256-bit AVX2 register.
 */
typedef struct {
    Jctr64Element x[8];   ///< Working state
    Jctr64Element out[8]; ///< Output state
    size_t pos;
} Jctr64VecState;


#ifdef __AVX2__
static inline void jctr64_vec_step_a(__m256i *x,
    int i0, int i1, int i2, int i3, int r)
{
    x[i0] = _mm256_sub_epi64(x[i0], x[i1]); // x[i0] -= x[i1]
    x[i2] = _mm256_xor_si256(x[i2], _mm256_srli_epi64(x[i3], r)); // x[i2] ^= x[i3] >> r
    x[i3] = _mm256_add_epi64(x[i3], x[i0]); // x[i3] += x[i0]    
}

static inline void jctr64_vec_step_b(__m256i *x,
    int i0, int i1, int i2, int i3, int r)
{
    x[i0] = _mm256_sub_epi64(x[i0], x[i1]); // x[i0] -= x[i1]
    x[i2] = _mm256_xor_si256(x[i2], _mm256_slli_epi64(x[i3], r)); // x[i2] ^= x[i3] << r
    x[i3] = _mm256_add_epi64(x[i3], x[i0]); // x[i3] += x[i0]
}


/**
 * @brief Based on ISAAC64 source code.
 */
static inline void jctr64_vec_round(__m256i *x)
{
    jctr64_vec_step_a(x, 0, 4, 5, 7,  9);
    jctr64_vec_step_b(x, 1, 5, 6, 0,  9);
    jctr64_vec_step_a(x, 2, 6, 7, 1,  23);
    jctr64_vec_step_b(x, 3, 7, 0, 2,  15);
    jctr64_vec_step_a(x, 4, 0, 1, 3,  14);
    jctr64_vec_step_b(x, 5, 1, 2, 4,  20);
    jctr64_vec_step_a(x, 6, 2, 3, 5,  17);
    jctr64_vec_step_b(x, 7, 3, 4, 6,  14);
}
#endif

void Jctr64VecState_block(Jctr64VecState *obj)
{
#ifdef __AVX2__
    __m256i out[8], x[8];
    for (size_t i = 0; i < 8; i++) {
        out[i] = _mm256_loadu_si256((__m256i *) (void *) &obj->x[i].u64[0]);
        x[i] = out[i];
    }
    // 4 rounds - pass SmokeRand `full` battery
    jctr64_vec_round(out);
    jctr64_vec_round(out);
    jctr64_vec_round(out);
    jctr64_vec_round(out);
    // 2 rounds for safety margin
    jctr64_vec_round(out);
    jctr64_vec_round(out);
    for (size_t i = 0; i < 8; i++) {
        out[i] = _mm256_add_epi64(out[i], x[i]);
        _mm256_storeu_si256((__m256i *) (void *) &obj->out[i].u64[0], out[i]);
    }
#else
    (void) obj;
#endif
}


void Jctr64VecState_init(Jctr64VecState *obj, const uint64_t *key, uint64_t ctr)
{
    for (size_t i = 0; i < JCTR64_NCOPIES; i++) {
        obj->x[0].u64[i] = JCTR64_PI0; obj->x[1].u64[i] = key[0];
        obj->x[2].u64[i] = ctr + i;    obj->x[3].u64[i] = 0; // Counter: lower and upper half
        obj->x[4].u64[i] = key[1];     obj->x[5].u64[i] = JCTR64_PI1;
        obj->x[6].u64[i] = JCTR64_PI2; obj->x[7].u64[i] = JCTR64_PI3;
    }
    obj->pos = 0;
    Jctr64VecState_block(obj);
}


static inline void Jctr64VecState_inc_counter(Jctr64VecState *obj)
{
    for (size_t i = 0; i < JCTR64_NCOPIES; i++) {
        obj->x[2].u64[i] += JCTR64_NCOPIES;
    }
}


static inline uint64_t get_bits_vector_raw(void *state)
{
    Jctr64VecState *obj = state;
    size_t i = (obj->pos & 0x7), j = (obj->pos >> 3);    
    uint64_t x = obj->out[i].u64[j];
    obj->pos++;
    if (obj->pos == 8 * JCTR64_NCOPIES) {
        Jctr64VecState_inc_counter(obj);
        Jctr64VecState_block(obj);
        obj->pos = 0;
    }
    return x;
}


MAKE_GET_BITS_WRAPPERS(vector);


static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
#ifdef __AVX2__
    uint64_t key[2];
    Jctr64VecState *obj = intf->malloc(sizeof(Jctr64VecState));
    (void) gi;
    key[0] = intf->get_seed64();
    key[1] = intf->get_seed64();
    Jctr64VecState_init(obj, key, 0);
    return obj;
#else
    (void) gi; (void) intf;
    return NULL;
#endif
}


//////////////////////
///// Interfaces /////
//////////////////////


static inline void *create(const CallerAPI *intf)
{
    intf->printf("Not implemented\n");
    return NULL;
}


static int test_output(const CallerAPI *intf, void *obj, uint64_t (*get_u64)(void *))
{
    static const uint64_t u_ref[32] = {
        0x7F626B08221F1AA9, 0x0CABB87FE295DD48,
        0xE1487D1D97641E88, 0x7891945A4DC245E4,
        0x76FD2D20F13FF048, 0xE1AA7AC1B6C06484,
        0xF9533CC158E686EA, 0x6C46DD0A4B51350D,
        0x61DF1053C0032A35, 0xB2418B570F9FA76D,
        0x4B1EAB7A1447C800, 0x38F06489E489D396,
        0xC7288E0376594AFE, 0x3FDB55AEEE23A733,
        0x0F58157F97DB7A62, 0x3DFDC2BBB011AAC2,
        0x94E795C9E4051E08, 0x7AB06374C94C968D,
        0x4BEE196E5FA5D20B, 0xBA85C42D288A0632,
        0xAE33610A15E11CD3, 0x5369ED09642987BB,
        0xCC3C0E44013C0A79, 0xCD7A74889EC5CA91,
        0x5C82F11BD9556CF0, 0x85A37766804C5EB2,
        0xD9653C71BD305D4E, 0x943224AA1E218F61,
        0xAF7D984F58163013, 0xB8BA169C393FFBC0,
        0x0AC6DCDC886451BC, 0xE268CABBFC5E12AA
    };
    for (size_t i = 0; i < 8192; i++) {
        (void) get_u64(obj);
    }
    int is_ok = 1;
    for (size_t i = 0; i < 32; i++) {
        unsigned long long u = (unsigned long long) get_u64(obj);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
        if (i % 2 == 1) {
            intf->printf("0x%16.16llX,\n", u);
        } else {
            intf->printf("0x%16.16llX, ", u);
        }
    }
    return is_ok;
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t key[2] = {0x123456789ABCDEF, 0xFEDCBA987654321};
    // Portable version testing
    intf->printf("----- Portable version -----\n");
    Jctr64State *obj_c99 = intf->malloc(sizeof(Jctr64State));
    Jctr64State_init(obj_c99, key, 0);
    int is_ok = test_output(intf, obj_c99, get_bits_scalar);
    intf->free(obj_c99);    
    // AVX2 version testing
    intf->printf("----- AVX2 version -----\n");
#ifdef __AVX2__
    Jctr64VecState *obj = intf->malloc(sizeof(Jctr64VecState));
    Jctr64VecState_init(obj, key, 0);
    is_ok = is_ok & test_output(intf, obj, get_bits_vector);
    intf->free(obj);
#else
    intf->printf("Not implemented\n");
#endif
    return is_ok;
}


static const char description[] =
"Jctr64: a counter-based PRNG based on an experimental 512-bit block cipher\n"
"developed by Bob Jenkins. The number of rounds is halved, the mixed itself\n"
"works in the mode similar to a stream cipher (inspired by ChaCha20).\n"
"The next param values are supported:\n"
"  c99  - portable version, default. Performance is around 1.1-1.3 cpb.\n"
"  avx2 - AVX2 version. Performance is around 0.5-0.6 cpb.\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->nbits = 64;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "c99") || !intf->strcmp(param, "")) {
        gi->name = "jctr64:c99";
        gi->create = create_scalar;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "avx2")) {
        gi->name = "jctr64:avx2";
        gi->create = create_vector;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
#ifndef __AVX2__
        intf->printf("Not implemented\n");
        return 0;
#endif
    } else {
        gi->name = "jctr64:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
        return 0;
    }
    return 1;
}
