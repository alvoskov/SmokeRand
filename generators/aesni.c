/**
 * @file aesni.c
 * @brief An implementation of PRNG based on AES128 block cipher in the
 * counter mode. Contains both hardware and software versions.
 * @details This file contains two different AES128 implementations:
 *
 * 1. Hardware implementation based on AESNI instructions for x86-64
 *    architecture (`--param=aesni`, the default one). Hardware implementation
 *    is much simpler and faster than the software one, performance is around
 *    1-2 cpb.
 * 2. Software implementation based on lookup tables (`--param=c99`). It is
 *    portable but rather slow, around 10 cpb. 
 *
 * Test vectors are taken from [1], see Chapter F.5.1.
 *
 * The next simplifications are made in the AES algorithm here:
 *
 * 1. Only 128-bit key is supported.
 * 2. Only encryption procedure is implemented, decryption is not required
 *    for generation of pseudorandom numbers and even for encryption
 *    in the CTR mode.
 *
 * References:
 *
 * 1. Morris Dworkin. NIST Special Publication 800-38A. Recommendation for
 *    Block Cipher Modes of Operation. Methods and Techniques. 2001 edition.
 *    https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38a.pdf
 * 2. https://doi.org/10.6028/NIST.FIPS.197-upd1
 * 3. Daemen J., Rijmen V. The Design of Rijndael. The Advanced Encryption
 *    Standard (AES) Information Security and Cryptography Series. 2nd edition.
 *    Springer Berlin, Heidelberg 2020. ISBN 978-3-662-60769-5
 *    https://doi.org/10.1007/978-3-662-60769-5
 * 4. https://csrc.nist.gov/csrc/media/projects/cryptographic-standards-and-guidelines/documents/aes-development/rijndael-ammended.pdf
 * 5. https://csrc.nist.gov/projects/cryptographic-standards-and-guidelines/example-values
 * 6. AES128 how-to using GCC and Intel AES-NI
 *    https://gist.github.com/acapola/d5b940da024080dfaf5f
 * 7. Rijmen V., Bosselaers A., Barreto P. Optimized ANSI C code for the
 *    Rijndael cipher (now AES)
 *    https://github.com/zakird/zdlibc/blob/master/rijndael-alg-fst.c
 * 8. Tiny AES in C. https://github.com/kokke/tiny-AES-c
 *
 * @copyright Cross-platform implementation is based on the optimised ANSI C
 * code for the Rijndael cipher (now AES) by the next authors:
 *
 * Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>
 * Antoon Bosselaers <antoon.bosselaers@esat.kuleuven.ac.be>
 * Paulo Barreto <paulo.barreto@terra.com.br>
 *
 * That implementation is in public domain. Simplification, adaptation
 * for SmokeRand and hardware AESNI based implementation+:
 *
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


#ifdef __AES__
#define AESNI_ENABLED
#include "smokerand/x86exts.h"
#endif

/**
 * @brief AES 128-bit block friendy to strict aliasing.
 */
typedef union {
#ifdef AESNI_ENABLED
    __m128i u128;
#endif
    uint64_t u64[2];
    uint32_t u32[4];
    uint8_t u8[16];
} AESBlock;

/**
 * @brief AES128 key in the format friendly to construction from 64-bit seeds.
 */
typedef AESBlock AES128Key;

/**
 * @brief AES-128 counter mode state designed as pseudorandom number
 * generator.
 */
typedef struct {
    AESBlock key_schedule[11]; ///< The expanded key
    AESBlock ctr; ///< 128-bit counter.
    AESBlock out; ///< 128-bit output buffer.
    int pos; ///< Current position in the output buffer.
} AES128State;


///////////////////////////////////////////////
///// AESNI version for x86-64 processors /////
///////////////////////////////////////////////


#define AES128_EXPAND_KEY(ks_out, ks_in, rc) \
    keygened = _mm_aeskeygenassist_si128((ks_in), rc);\
	keygened = _mm_shuffle_epi32(keygened, _MM_SHUFFLE(3,3,3,3));\
    key = _mm_xor_si128((ks_in), _mm_slli_si128((ks_in), 4));\
    key = _mm_xor_si128(key, _mm_slli_si128((key), 4));\
    key = _mm_xor_si128(key, _mm_slli_si128((key), 4));\
    (ks_out) = _mm_xor_si128((key), keygened);

/**
 * @brief Initialize AES128 based PRNG state, i.e. fill key schedule
 * and counters.
 */
void AES128State_init(AES128State *obj, const AES128Key *enc_key)
{
#ifdef AESNI_ENABLED
    __m128i keygened, key;    
    AESBlock *ks = obj->key_schedule;
    ks[0].u128 = _mm_loadu_si128(&enc_key->u128);
    AES128_EXPAND_KEY(ks[1].u128,  ks[0].u128, 0x01);
    AES128_EXPAND_KEY(ks[2].u128,  ks[1].u128, 0x02);
    AES128_EXPAND_KEY(ks[3].u128,  ks[2].u128, 0x04);
    AES128_EXPAND_KEY(ks[4].u128,  ks[3].u128, 0x08);
    AES128_EXPAND_KEY(ks[5].u128,  ks[4].u128, 0x10);
    AES128_EXPAND_KEY(ks[6].u128,  ks[5].u128, 0x20);
    AES128_EXPAND_KEY(ks[7].u128,  ks[6].u128, 0x40);
    AES128_EXPAND_KEY(ks[8].u128,  ks[7].u128, 0x80);
    AES128_EXPAND_KEY(ks[9].u128,  ks[8].u128, 0x1B);
    AES128_EXPAND_KEY(ks[10].u128, ks[9].u128, 0x36);
    obj->ctr.u64[0] = obj->ctr.u64[1] = 0;
    obj->pos = 2;
#else
    (void) obj; (void) enc_key;
#endif
}

/**
 * @brief Encode (encrypt) 128-bit block.
 * @param[out] output  Output data(128-bit block).
 * @param[in]  input   Input data (128-bit block).
 */
void AES128State_encode(const AES128State *obj, uint8_t *output, const uint8_t *input)
{
#ifdef AESNI_ENABLED
    __m128i m = _mm_loadu_si128((__m128i *) input);
    const AESBlock *ks = obj->key_schedule;
    m = _mm_xor_si128(m, ks[0].u128);
    m = _mm_aesenc_si128(m, ks[1].u128);
    m = _mm_aesenc_si128(m, ks[2].u128);
    m = _mm_aesenc_si128(m, ks[3].u128);
    m = _mm_aesenc_si128(m, ks[4].u128);
    m = _mm_aesenc_si128(m, ks[5].u128);
    m = _mm_aesenc_si128(m, ks[6].u128);
    m = _mm_aesenc_si128(m, ks[7].u128);
    m = _mm_aesenc_si128(m, ks[8].u128);
    m = _mm_aesenc_si128(m, ks[9].u128);
    m = _mm_aesenclast_si128(m, ks[10].u128);
    _mm_storeu_si128((__m128i *) output, m);
#else
    (void) obj; (void) output; (void) input;
#endif
}

/**
 * @brief Returns 64-bit unsigned integer from the 128-bit output buffer.
 */
static inline uint64_t get_bits_aesni_raw(void *state)
{
    AES128State *obj = state;
    if (obj->pos == 2) {
        AES128State_encode(obj, obj->out.u8, obj->ctr.u8);
        if (++obj->ctr.u64[0] == 0) {
            obj->ctr.u64[1]++;
        }
        obj->pos = 0;
    }
    return obj->out.u64[obj->pos++];    
}

/**
 * @brief Initializes AES128 based PRNG. Two random 64-bit seeds are used
 * as a key.
 */
static void *create_aesni(const GeneratorInfo *gi, const CallerAPI *intf)
{
#ifdef AESNI_ENABLED
    AES128State *obj = intf->malloc(sizeof(AES128State));
    AES128Key key;
    key.u64[0] = intf->get_seed64();
    key.u64[1] = intf->get_seed64();
    AES128State_init(obj, &key);
    (void) gi;
    return obj;
#else
    intf->printf("AESNI is not supported on this platform\n");
    (void) gi;
    return NULL;
#endif
}

MAKE_GET_BITS_WRAPPERS(aesni)

//////////////////////////////////
///// Cross-platform version /////
//////////////////////////////////

/**
 * @brief AES S-box.
 */
static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b,  0xf2, 0x6b, 0x6f, 0xc5,
    0x30, 0x01, 0x67, 0x2b,  0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d,  0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf,  0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26,  0x36, 0x3f, 0xf7, 0xcc,
    0x34, 0xa5, 0xe5, 0xf1,  0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3,  0x18, 0x96, 0x05, 0x9a,
    0x07, 0x12, 0x80, 0xe2,  0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a,  0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3,  0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed,  0x20, 0xfc, 0xb1, 0x5b,
    0x6a, 0xcb, 0xbe, 0x39,  0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb,  0x43, 0x4d, 0x33, 0x85,
    0x45, 0xf9, 0x02, 0x7f,  0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f,  0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21,  0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec,  0x5f, 0x97, 0x44, 0x17,
    0xc4, 0xa7, 0x7e, 0x3d,  0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc,  0x22, 0x2a, 0x90, 0x88,
    0x46, 0xee, 0xb8, 0x14,  0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a,  0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62,  0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d,  0x8d, 0xd5, 0x4e, 0xa9,
    0x6c, 0x56, 0xf4, 0xea,  0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e,  0x1c, 0xa6, 0xb4, 0xc6,
    0xe8, 0xdd, 0x74, 0x1f,  0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66,  0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9,  0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11,  0x69, 0xd9, 0x8e, 0x94,
    0x9b, 0x1e, 0x87, 0xe9,  0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d,  0xbf, 0xe6, 0x42, 0x68,
    0x41, 0x99, 0x2d, 0x0f,  0xb0, 0x54, 0xbb, 0x16
};

// Multiplication by '0x02'
static inline uint8_t xtime(uint8_t x)
{
    return ((x<<1) ^ (((x>>7) & 1) * 0x1b));
}


/**
 * @brief AES lookup tables for encryption subroutinese, must be initialized
 * by the fill_lookup_tables function before any usage of software AES.
 * @details Beginning of lookup tables:
 *
 *     t0: 0xc66363a5U, 0xf87c7c84U, 0xee777799U, 0xf67b7b8dU, ...
 *     t1: 0xa5c66363U, 0x84f87c7cU, 0x99ee7777U, 0x8df67b7bU, ...
 *     t2: 0x63a5c663U, 0x7c84f87cU, 0x7799ee77U, 0x7b8df67bU, ...
 *     t3: 0x6363a5c6U, 0x7c7c84f8U, 0x777799eeU, 0x7b7b8df6U, ...
 *     t4: 0x63636363U, 0x7c7c7c7cU, 0x77777777U, 0x7b7b7b7bU, ...
 */
static uint32_t te_tbl[5][256];

// Calculate the T0 entry for the lookup tables
uint32_t a_to_t0(uint8_t a)
{
    uint8_t sa = sbox[a];
    uint8_t sa_x_02 = xtime(sa);
    uint8_t sa_x_03 = sa_x_02 ^ sa;
    uint32_t t0 = ((uint32_t) sa_x_02 << 24) |
        ((uint32_t) sa << 16) |
        ((uint32_t) sa << 8) |
        ((uint32_t) sa_x_03);
    return t0;
}

uint32_t a_to_t4(uint8_t a)
{
    uint32_t sa = sbox[a];
    return (sa << 24) | (sa << 16) | (sa << 8) | sa;
}

void fill_lookup_tables(void)
{
    for (unsigned int i = 0; i < 256; i++) {
        uint32_t t0 = a_to_t0(i); te_tbl[0][i] = t0;
        te_tbl[1][i] = (t0 << 24) | (t0 >> 8);
        te_tbl[2][i] = (t0 << 16) | (t0 >> 16);
        te_tbl[3][i] = (t0 << 8)  | (t0 >> 24);
        te_tbl[4][i] = a_to_t4(i);
    }
}

static inline uint32_t get_u32(const uint8_t *pt)
{
    return ((uint32_t)(pt[0]) << 24) ^ ((uint32_t)(pt[1]) << 16) ^
           ((uint32_t)(pt[2]) <<  8) ^ ((uint32_t)(pt[3]));
}

static inline void put_u32(uint8_t *out, uint32_t val)
{
    out[0] = (uint8_t) (val >> 24);
    out[1] = (uint8_t) (val >> 16);
    out[2] = (uint8_t) (val >>  8);
    out[3] = (uint8_t) val;
}

static inline uint32_t aes_transform(uint32_t w0, uint32_t w1,
    uint32_t w2, uint32_t w3, uint32_t kw)
{
    return kw ^
        te_tbl[0][(w0 >> 24)       ] ^  te_tbl[1][(w1 >> 16) & 0xff] ^
        te_tbl[2][(w2 >>  8) & 0xff] ^  te_tbl[3][(w3      ) & 0xff];
}


static inline uint32_t aes_final_transform(uint32_t w0, uint32_t w1,
    uint32_t w2, uint32_t w3, uint32_t kw)
{
    return kw ^
        (te_tbl[4][(w0 >> 24)       ] & 0xff000000) ^
        (te_tbl[4][(w1 >> 16) & 0xff] & 0x00ff0000) ^
        (te_tbl[4][(w2 >>  8) & 0xff] & 0x0000ff00) ^
        (te_tbl[4][(w3      ) & 0xff] & 0x000000ff);
}



void AES128State_init_c99(AES128State *obj, const AES128Key *enc_key)
{
    uint32_t *rk = obj->key_schedule[0].u32;
    static const uint32_t rcon[] = {
        0x01000000, 0x02000000, 0x04000000, 0x08000000,
        0x10000000, 0x20000000, 0x40000000, 0x80000000,
        0x1B000000, 0x36000000
    };
	rk[0] = get_u32(enc_key->u8     );
	rk[1] = get_u32(enc_key->u8 +  4);
	rk[2] = get_u32(enc_key->u8 +  8);
	rk[3] = get_u32(enc_key->u8 + 12);
    for (int i = 0; i < 10; i++) {
        uint32_t temp  = (rk[3] << 8) | (rk[3] >> 24);
        rk[4] = rk[0] ^ aes_final_transform(temp, temp, temp, temp, rcon[i]);
        rk[5] = rk[1] ^ rk[4];
        rk[6] = rk[2] ^ rk[5];
        rk[7] = rk[3] ^ rk[6];
        rk += 4;
    }
    obj->ctr.u64[0] = obj->ctr.u64[1] = 0;
    obj->pos = 2;
}


void AES128State_encode_c99(const AES128State *obj, uint8_t *ct, const uint8_t *pt)
{
    static const int Nr = 10;
    uint32_t t0, t1, t2, t3;
    int r;
    const uint32_t *rk = obj->key_schedule[0].u32;
    // Map byte array block to cipher state
	// and add initial round key:
    uint32_t s0 = get_u32(pt     ) ^ rk[0];
    uint32_t s1 = get_u32(pt +  4) ^ rk[1];
    uint32_t s2 = get_u32(pt +  8) ^ rk[2];
    uint32_t s3 = get_u32(pt + 12) ^ rk[3];
    // Nr - 1 full rounds:
    r = Nr >> 1;
    for (;;) {
        t0 = aes_transform(s0, s1, s2, s3, rk[4]);
        t1 = aes_transform(s1, s2, s3, s0, rk[5]);
        t2 = aes_transform(s2, s3, s0, s1, rk[6]);
        t3 = aes_transform(s3, s0, s1, s2, rk[7]);
        rk += 8;
        if (--r == 0) {
            break;
        }

        s0 = aes_transform(t0, t1, t2, t3, rk[0]);
        s1 = aes_transform(t1, t2, t3, t0, rk[1]);
        s2 = aes_transform(t2, t3, t0, t1, rk[2]);
        s3 = aes_transform(t3, t0, t1, t2, rk[3]);
    }
    // Apply last round
    s0 = aes_final_transform(t0, t1, t2, t3, rk[0]);
    s1 = aes_final_transform(t1, t2, t3, t0, rk[1]);
    s2 = aes_final_transform(t2, t3, t0, t1, rk[2]);
    s3 = aes_final_transform(t3, t0, t1, t2, rk[3]);
	// Map cipher state to byte array block
    put_u32(ct,      s0);
    put_u32(ct +  4, s1);
    put_u32(ct +  8, s2);
    put_u32(ct + 12, s3);
}




static void *create_c99(const GeneratorInfo *gi, const CallerAPI *intf)
{
    AES128State *obj = intf->malloc(sizeof(AES128State));
    AES128Key key;
    key.u64[0] = intf->get_seed64();
    key.u64[1] = intf->get_seed64();
    AES128State_init_c99(obj, &key);
    (void) gi;
    return obj;
}


/**
 * @brief Returns 64-bit unsigned integer from the 128-bit output buffer.
 */
static inline uint64_t get_bits_c99_raw(void *state)
{
    AES128State *obj = state;
    if (obj->pos == 2) {
        AES128State_encode_c99(obj, obj->out.u8, obj->ctr.u8);
        if (++obj->ctr.u64[0] == 0) {
            obj->ctr.u64[1]++;
        }
        obj->pos = 0;
    }
    return obj->out.u64[obj->pos++];    
}


MAKE_GET_BITS_WRAPPERS(c99)




//////////////////////
///// Interfaces /////
//////////////////////


/**
 * @brief An internal self-test based on NIST Special Publication 800-38A
 * by Morris Dworkin.
 */
int run_self_test_template(const CallerAPI *intf,
    void (*init_func)(AES128State *, const AES128Key *),
    void (*encode_func)(const AES128State *, uint8_t *, const uint8_t *))
{
    uint8_t input[] = {
        0xf0, 0xf1, 0xf2, 0xf3,  0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb,  0xfc, 0xfd, 0xfe, 0xff
    };
    uint8_t output_ref[] = {
        0xec, 0x8c, 0xdf, 0x73,  0x98, 0x60, 0x7c, 0xb0,
        0xf2, 0xd2, 0x16, 0x75,  0xea, 0x9e, 0xa1, 0xe4
    };
    AES128Key key = {
        .u8 = {
            0x2b, 0x7e, 0x15, 0x16,  0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88,  0x09, 0xcf, 0x4f, 0x3c}
    };
    uint8_t output_comp[16];
    int is_ok = 1;
    AES128State *obj = intf->malloc(sizeof(AES128State));
    init_func(obj, &key);
    encode_func(obj, output_comp, input);
    intf->printf("Output:      ");
    for (int i = 0; i < 16; i++) {
        intf->printf("%.2X ", output_comp[i]);
        if (output_comp[i] != output_ref[i]) {
            is_ok = 0;
        }
    }
    intf->printf("\nReference:   ");
    for (int i = 0; i < 16; i++) {
        intf->printf("%.2X ", output_ref[i]);
    }
    intf->printf("\n");
    intf->free(obj);
    return is_ok;
}



static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    intf->printf("----- AESNI based hardware implementation -----\n");
#ifdef AESNI_ENABLED
    is_ok = is_ok & run_self_test_template(intf,
        AES128State_init, AES128State_encode);
#else
    intf->printf("AESNI is not supported on this platform\n");
#endif
    intf->printf("----- Software implementation -----\n");
    is_ok = is_ok & run_self_test_template(intf,
        AES128State_init_c99, AES128State_encode_c99);
    return is_ok;
}

static void *create(const CallerAPI *intf)
{
    intf->printf("Unknown parameter '%s'\n", intf->get_param());
    return NULL;
}

static const char description[] =
"AES-128 based PRNG. This block cipher is used in the CTR (counter) mode\n"
"param values are supported:\n"
"  aesni - hardware implementation for x86-64 processors (fast)\n"
"  c99   - software cross-platform implementation (slow)\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->create = default_create;
    gi->free = default_free;
    gi->nbits = 64;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    fill_lookup_tables();
    if (!intf->strcmp(param, "aesni") || !intf->strcmp(param, "")) {
        gi->name = "AES128:aesni";
        gi->create = create_aesni;
        gi->get_bits = get_bits_aesni;
        gi->get_sum = get_sum_aesni;
    } else if (!intf->strcmp(param, "c99")) {
        gi->name = "AES128:c99";
        gi->create = create_c99;
        gi->get_bits = get_bits_c99;
        gi->get_sum = get_sum_c99;
    } else {
        gi->name = "AES128:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
    }
    return 1;
}
