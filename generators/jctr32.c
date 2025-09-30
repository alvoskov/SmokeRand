/**
 * @brief jctr32 is a counter-based PRNG based on an experimental block
 * cipher by Bob Jenkins.
 * @details The original cipher is a 256-bit block ARX cipher based on 32-bit
 * arithmetics. The author didn't made a formal cryptoanalysis and positioned
 * it as "guaranteed to be stronger than rot-13".
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
 * @copyright The original 256-bit ARX block cipher was designed
 * by Bob Jenkins and released as Public Domain.
 *
 * Conversion to `jctr32` counter-based PRNG:
 * 
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

#define JCTR32_PI0 0x243F6A88
#define JCTR32_PI1 0x85A308D3


PRNG_CMODULE_PROLOG

#ifdef __AVX2__
#include "smokerand/x86exts.h"
#endif

////////////////////////////////////////////////////
///// Cross-platform (portable) implementation /////
////////////////////////////////////////////////////

/**
 * @brief Jctr32 counter-based PRNG state.
 * @details The state has the next layout:
 *
 *     | pi   key  ctr_lo  ctr_hi |
 *     | key  pi   key     key    |
 */
typedef struct {
    uint32_t x[8];   ///< Working state
    uint32_t out[8]; ///< Output state
    size_t pos;
} Jctr32State;


static inline void jctr32_round(uint32_t *x)
{
    x[0] -= x[4]; x[5] ^= x[7] >> 8;  x[7] += x[0];
    x[1] -= x[5]; x[6] ^= x[0] << 8;  x[0] += x[1];
    x[2] -= x[6]; x[7] ^= x[1] >> 11; x[1] += x[2];
    x[3] -= x[7]; x[0] ^= x[2] << 3;  x[2] += x[3];
    x[4] -= x[0]; x[1] ^= x[3] >> 6;  x[3] += x[4];
    x[5] -= x[1]; x[2] ^= x[4] << 4;  x[4] += x[5];
    x[6] -= x[2]; x[3] ^= x[5] >> 13; x[5] += x[6];
    x[7] -= x[3]; x[4] ^= x[6] << 13; x[6] += x[7];
}

void Jctr32State_block(Jctr32State *obj)
{
    for (size_t i = 0; i < 8; i++) {
        obj->out[i] = obj->x[i];
    }
    // 4 rounds - pass SmokeRand `full` battery
    jctr32_round(obj->out);
    jctr32_round(obj->out);
    jctr32_round(obj->out);
    jctr32_round(obj->out);
    // 2 rounds for safety margin
    jctr32_round(obj->out);
    jctr32_round(obj->out);
    for (size_t i = 0; i < 8; i++) {
        obj->out[i] += obj->x[i];
    }
}

void Jctr32State_init(Jctr32State *obj, const uint32_t *key, uint64_t ctr)
{
    obj->x[0] = JCTR32_PI0; obj->x[1] = key[0];
    obj->x[2] = (uint32_t) ctr; // Counter: lower half
    obj->x[3] = ctr >> 32;      // Counter: upper half
    obj->x[4] = key[1];     obj->x[5] = JCTR32_PI1;
    obj->x[6] = key[2];     obj->x[7] = key[3];
    obj->pos = 0;
    Jctr32State_block(obj);
}


static inline void Jctr32State_inc_counter(Jctr32State *obj)
{
    if (++obj->x[2] == 0) obj->x[3]++;
}


static inline uint64_t get_bits_scalar_raw(void *state)
{
    Jctr32State *obj = state;
    uint32_t x = obj->out[obj->pos++];
    if (obj->pos == 8) {
        Jctr32State_inc_counter(obj);
        Jctr32State_block(obj);
        obj->pos = 0;
    }
    return x;
}

MAKE_GET_BITS_WRAPPERS(scalar);


static inline void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    uint32_t key[4];
    Jctr32State *obj = intf->malloc(sizeof(Jctr32State));
    uint64_t seed0 = intf->get_seed64();
    uint64_t seed1 = intf->get_seed64();
    (void) gi;
    key[0] = (uint32_t) seed0;
    key[1] = seed0 >> 32;
    key[2] = (uint32_t) seed1;
    key[3] = seed1 >> 32;
    Jctr32State_init(obj, key, 0);
    return obj;
}


////////////////////////////////////////
///// AVX2 (vector) implementation /////
////////////////////////////////////////

#define JCTR32_NCOPIES 8

typedef union {
    uint32_t u32[JCTR32_NCOPIES];
} Jctr32Element;

/**
 * @brief Jctr64 counter-based PRNG state.
 * @details It contains for copies of the generator, each
 * copy has the next layout:
 *
 *     | pi   key  ctr_lo  ctr_hi |
 *     | key  pi   key     key    |
 *
 * The main idea is to load x[i] from four copies of the generator to the
 * single 256-bit AVX2 register.
 */
typedef struct {
    Jctr32Element x[8];   ///< Working state
    Jctr32Element out[8]; ///< Output state
    size_t pos;
} Jctr32VecState;


#ifdef __AVX2__
static inline void jctr32_vec_step_a(__m256i *x,
    int i0, int i1, int i2, int i3, int r)
{
    x[i0] = _mm256_sub_epi32(x[i0], x[i1]); // x[i0] -= x[i1]
    x[i2] = _mm256_xor_si256(x[i2], _mm256_srli_epi32(x[i3], r)); // x[i2] ^= x[i3] >> r
    x[i3] = _mm256_add_epi32(x[i3], x[i0]); // x[i3] += x[i0]    
}

static inline void jctr32_vec_step_b(__m256i *x,
    int i0, int i1, int i2, int i3, int r)
{
    x[i0] = _mm256_sub_epi32(x[i0], x[i1]); // x[i0] -= x[i1]
    x[i2] = _mm256_xor_si256(x[i2], _mm256_slli_epi32(x[i3], r)); // x[i2] ^= x[i3] << r
    x[i3] = _mm256_add_epi32(x[i3], x[i0]); // x[i3] += x[i0]
}


static inline void jctr32_vec_round(__m256i *x)
{
    jctr32_vec_step_a(x, 0, 4, 5, 7,  8);
    jctr32_vec_step_b(x, 1, 5, 6, 0,  8);
    jctr32_vec_step_a(x, 2, 6, 7, 1,  11);
    jctr32_vec_step_b(x, 3, 7, 0, 2,  3);
    jctr32_vec_step_a(x, 4, 0, 1, 3,  6);
    jctr32_vec_step_b(x, 5, 1, 2, 4,  4);
    jctr32_vec_step_a(x, 6, 2, 3, 5,  13);
    jctr32_vec_step_b(x, 7, 3, 4, 6,  13);
}
#endif

void Jctr32VecState_block(Jctr32VecState *obj)
{
#ifdef __AVX2__
    __m256i out[8], x[8];
    for (size_t i = 0; i < 8; i++) {
        out[i] = _mm256_loadu_si256((__m256i *) &obj->x[i].u32[0]);
        x[i] = out[i];
    }
    // 4 rounds - pass SmokeRand `full` battery
    jctr32_vec_round(out);
    jctr32_vec_round(out);
    jctr32_vec_round(out);
    jctr32_vec_round(out);
    // 2 rounds for safety margin
    jctr32_vec_round(out);
    jctr32_vec_round(out);
    for (size_t i = 0; i < 8; i++) {
        out[i] = _mm256_add_epi32(out[i], x[i]);
        _mm256_storeu_si256((__m256i *) &obj->out[i].u32[0], out[i]);
    }
#else
    (void) obj;
#endif
}


void Jctr32VecState_init(Jctr32VecState *obj, const uint32_t *key, uint64_t ctr)
{
    for (size_t i = 0; i < JCTR32_NCOPIES; i++) {
        uint64_t ctr_i = ctr + i;
        obj->x[0].u32[i] = JCTR32_PI0; obj->x[1].u32[i] = key[0];
        obj->x[2].u32[i] = (uint32_t) ctr_i; // Counter: lower half
        obj->x[3].u32[i] = ctr_i >> 32;      // Counter: upper half
        obj->x[4].u32[i] = key[1];     obj->x[5].u32[i] = JCTR32_PI1;
        obj->x[6].u32[i] = key[2];     obj->x[7].u32[i] = key[3];
    }
    obj->pos = 0;
    Jctr32VecState_block(obj);
}


static inline void Jctr32VecState_inc_counter(Jctr32VecState *obj)
{
    for (size_t i = 0; i < JCTR32_NCOPIES; i++) {
        uint64_t ctr = (obj->x[2].u32[i]) | ((uint64_t) obj->x[3].u32[i] << 32);
        ctr += JCTR32_NCOPIES;
        obj->x[2].u32[i] = (uint32_t) ctr;
        obj->x[3].u32[i] = ctr >> 32;
    }
}


static inline uint64_t get_bits_vector_raw(void *state)
{
    Jctr32VecState *obj = state;
    size_t i = (obj->pos & 0x7), j = (obj->pos >> 3);    
    uint64_t x = obj->out[i].u32[j];
    obj->pos++;
    if (obj->pos == 8 * JCTR32_NCOPIES) {
        Jctr32VecState_inc_counter(obj);
        Jctr32VecState_block(obj);
        obj->pos = 0;
    }
    return x;
}


MAKE_GET_BITS_WRAPPERS(vector);


static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
#ifdef __AVX2__
    uint32_t key[4];
    Jctr32VecState *obj = intf->malloc(sizeof(Jctr32VecState));    
    (void) gi;
    uint64_t seed0 = intf->get_seed64();
    uint64_t seed1 = intf->get_seed64();
    key[0] = (uint32_t) seed0;
    key[1] = seed0 >> 32;
    key[2] = (uint32_t) seed1;
    key[3] = seed1 >> 32;
    Jctr32VecState_init(obj, key, 0);
    return obj;
#else
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
    static const uint32_t u_ref[64] = {
        0xBC99FFB2, 0x11F0BC79, 0xB4BB91B3, 0x115A006D,
        0x2770438E, 0x2BE445F2, 0x4154F996, 0x9914AFA6,
        0x833A1D67, 0xEBC5D298, 0x658FA8A9, 0xE8679729,
        0xBF6A62C0, 0x3B2617BD, 0x9B655A7C, 0xB51C5FF0,
        0xA460521F, 0x2156A896, 0x15D98962, 0x831B4012,
        0x880128F4, 0x88505887, 0x38EBDDD2, 0x882257EE,
        0xC1F2AE8B, 0x3C0FB275, 0x0F30A373, 0xE2313BC5,
        0xBC4EDCAF, 0x9BF18C60, 0x9642535A, 0x354BE016,
        0xA525BE55, 0xB6F8DDF6, 0x07291C71, 0x2D5F51B3,
        0xADFA95D3, 0x2BDB973E, 0xADBADE81, 0x0769C978,
        0x0A1F2F55, 0x7D3FD2C4, 0x8F427C33, 0xDD0E62FE,
        0x6E0CF202, 0xBCAB8322, 0x1BFA35D0, 0xC6FC45AE,
        0x04F07189, 0xE60EA42E, 0x0B22B830, 0xC5B0EB08,
        0x1D12409F, 0xA30C02DA, 0x81A69895, 0x1C0DFCB1,
        0xA7D91A1D, 0xFF025734, 0xBE5637F0, 0xB8359A77,
        0x830B740E, 0x4B2304CF, 0x62A230B5, 0x32FB8B7A
    };
    for (size_t i = 0; i < 8192; i++) {
        (void) get_u64(obj);
    }
    int is_ok = 1;
    for (size_t i = 0; i < 64; i++) {
        uint32_t u = (uint32_t) get_u64(obj);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
        if (i % 4 == 3) {
            intf->printf("0x%8.8lX,\n", (unsigned long) u);
        } else {
            intf->printf("0x%8.8lX, ", (unsigned long) u);
        }
    }
    return is_ok;
}

static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t key[4] = {0x12345678, 0x87654321, 0xABCDEF42, 0x42FEDCBA};
    // Portable version testing
    intf->printf("----- Portable version -----\n");
    Jctr32State *obj_c99 = intf->malloc(sizeof(Jctr32State));
    Jctr32State_init(obj_c99, key, 0);
    int is_ok = test_output(intf, obj_c99, get_bits_scalar);
    intf->free(obj_c99);    
    // AVX2 version testing
    intf->printf("----- AVX2 version -----\n");
#ifdef __AVX2__
    Jctr32VecState *obj = intf->malloc(sizeof(Jctr32VecState));
    Jctr32VecState_init(obj, key, 0);
    is_ok = is_ok & test_output(intf, obj, get_bits_vector);
    intf->free(obj);
#else
    intf->printf("Not implemented\n");
#endif
    return is_ok;
}


static const char description[] =
"Jctr32: a counter-based PRNG based on an experimental 256-bit block cipher\n"
"developed by Bob Jenkins. The number of rounds is halved, the mixed itself\n"
"works in the mode similar to a stream cipher (inspired by ChaCha20).\n"
"The next param values are supported:\n"
"  c99  - portable version, default. Performance is around - cpb.\n"
"  avx2 - AVX2 version. Performance is around - cpb.\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->nbits = 32;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "c99") || !intf->strcmp(param, "")) {
        gi->name = "jctr32:c99";
        gi->create = create_scalar;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "avx2")) {
        gi->name = "jctr32:avx2";
        gi->create = create_vector;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
#ifndef __AVX2__
        intf->printf("Not implemented\n");
        return 0;
#endif
    } else {
        gi->name = "jctr32:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
        return 0;
    }
    return 1;
}
