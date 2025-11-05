/**
 * @file kuzn.c
 * @brief "Kuznyechik" Block Cipher implementation (RFC7801/GOST R 34.12-2015)
 * for pseudorandom numbers generation.
 * @details This version uses optimizations based on lookup tables. L
 * transformation of the algorithm can be represented as 16x16 matrix.
 * \f$ L = R^{16} \f$.
 *
 * Testing:
 *
 * - 2 rounds - fails `express`.
 * - 3 rounds - passes `express`, `brief` but fails `default`. The failed tests
 *   are `matrixrank_4096` and `matrixrank_4096_low8` (but not `linearcomp`) tests.
 *   In the `full` battery it also fails all `matrixrank` tests.
 * - 4 rounds - passes `full` battery.
 *
 * This is better result than the 2 rounds distinguisher decribed by Klinec
 * et al. [9].
 * 
 * References:
 * 
 * 1. RFC7801. GOST R 34.12-2015: Block Cipher "Kuznyechik"
 *    https://datatracker.ietf.org/doc/html/rfc7801
 * 2. GOST R 34.12-2015 (in Russian)
 *    https://tc26.ru/standard/gost/GOST_R_3412-2015.pdf
 * 3. Rybkin A.S. On software implementation of Kuznyechik on Intel CPUs //
 *    // Matem. Vopr. Kryptogr. 2018. V 9. N 2. P. 117-127 [in Russian]
 *    https://doi.org/10.4213/mvk255
 * 4. Ищукова Е.А., Кошуцкий Р.А., Бабенко Л.К. Разработка и реализация
 *    высокоскоростного шифрования данных с использованием алгоритма
 *    Кузнечик // Auditorium. 2015. N 4 (8).
 *    https://cyberleninka.ru/article/n/razrabotka-i-realizatsiya-vysokoskorostnogo-shifrovaniya-dannyh-s-ispolzovaniem-algoritma-kuznechik
 * 5. Гафуров И. Р. Высокоскоростная программная реализация алгоритмов
 *    шифрования из ГОСТ Р 34.12-2015 // Ученые записки УлГУ. Серия "Математика
 *    и информационные технологии". 2022. N 2. P.38-48. http://mi.mathnet.ru/ulsu147.
 * 6. Perrin L. Partitions in the S-Box of Streebog and Kuznyechik // IACR
 *    Transactions on Symmetric Cryptology. 2019. N 1. P. 302-329.
 *    https://doi.org/10.13154/tosc.v2019.i1.302-329
 * 7. Perrin L. On S-Box Reverse-Engineering.
 *    https://who.paris.inria.fr/Leo.Perrin/sbox-reverse-engineering.html
 * 8. Perrin L. On the S-Box of Streebog and Kuznyechik.
 *    https://who.paris.inria.fr/Leo.Perrin/pi.html
 * 9. Klinec D., Sýs M., Kubíček K., Švenda P., Matyáš V. Large-scale
 *    Randomness Study of Security Margins for 100+ Cryptographic Functions //
 *    Proceedings of the 19th International Conference on Security and
 *    Cryptography - SECRYPT. 2022. ISBN 978-989-758-590-6; ISSN 2184-7711,
 *    SciTePress, pages 134-146. DOI: https://doi.org/10.5220/0011267600003283
 * 10. https://rekovalev.site/kuznechik-crypto/#code
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief 16x16 matrix of bytes.
 */
typedef struct {
    uint8_t a[16][16];
} Mat16;

/**
 * @brief Vector of 16 bytes.
 */
typedef struct {
    union {
        uint8_t u8[16];
        uint64_t u64[2];
    } data;
} Vec16;

/**
 * @brief "Kuznyechik" block cipher based PRNG state (GOST R 34.12-2015).
 */
typedef struct {
    Vec16 rk[10]; ///< Round keys
    Vec16 ctr; ///< Counter (plaintext)
    Vec16 out; ///< Output buffer (ciphertext)
    int pos;
} KuznState;

/**
 * @brief 256-bit key for "Kuznyechik" block cipher.
 * @relates KuznState
 */
typedef struct {
    Vec16 lo; ///< Lower 128 bits of the key
    Vec16 hi; ///< Higher 128 bits of the key
} Key256;

/**
 * @brief "Kuznyechik" lookup tables for the LS transformation.
 * @relates KuznState
 */
static Vec16 lookup_table_LS[16 * 256];


/**
 * @brief Multiplication in the finite field GF(2)[x]/p(x)
 * where p(x)=x^8+x^7+x^6+x+1 belongs to GF(2)[x].
 * @details No lookup tables and other sophisticated optimizations are needed
 * here: this function is used only in initialization subroutines. 
 */
uint8_t gf256_mul(uint8_t a, uint8_t b)
{
    uint8_t result = 0;
    while (a && b) {
        if (b & 1) result ^= a;
        a = (a << 1) ^ (a & 0x80 ? 0xC3 : 0x00);
        b >>= 1;
    }
    return result;
}

void mul_mat_vec(Vec16 *out, const Mat16 *mat, const Vec16 *vec)
{
    for (int i = 0; i < 16; i++) {
        uint8_t c = 0;
        for (int j = 0; j < 16; j++) {
            c ^= gf256_mul(mat->a[i][j], vec->data.u8[j]);
        }
        out->data.u8[i] = c;
    }
}


static inline void Vec16_xor(Vec16 *obj, const Vec16 *in)
{
    obj->data.u64[0] ^= in->data.u64[0];
    obj->data.u64[1] ^= in->data.u64[1];
}

void Vec16_print(const CallerAPI *intf, const Vec16 *vec)
{
    for (int i = 0; i < 16; i++) {
        intf->printf("%.2X ", vec->data.u8[i]);
    }
    intf->printf("\n");
}

int Vec16_compare(const Vec16 *v1, const Vec16 *v2)
{
    for (int i = 0; i < 16; i++) {
        if (v1->data.u8[i] != v2->data.u8[i]) {
            return 0;
        }
    }
    return 1;
}


void Key256_fill(Key256 *key, uint64_t *data)
{
    key->lo.data.u64[0] = data[0];
    key->lo.data.u64[1] = data[1];
    key->hi.data.u64[0] = data[2];
    key->hi.data.u64[1] = data[3];
}



/**
 * @brief Apply the linear transformation L.
 * @details It is not optimized because it is required only in computation of round keys.
 */ 
void apply_L(Vec16 *out, const Vec16 *in)
{
    static const Mat16 Lmat = {.a = {
        {0x01, 0x94, 0x20, 0x85 ,0x10, 0xC2, 0xC0, 0x01, 0xFB, 0x01, 0xC0, 0xC2, 0x10, 0x85, 0x20, 0x94},
        {0x94, 0xA5, 0x3C, 0x44, 0xD1, 0x8D, 0xB4, 0x54, 0xDE, 0x6F, 0x77, 0x5D, 0x96, 0x74, 0x2D, 0x84},
        {0x84, 0x64, 0x48, 0xDF, 0xD3, 0x31, 0xA6, 0x30, 0xE0, 0x5A, 0x44, 0x97, 0xCA, 0x75, 0x99, 0xDD},
        {0xDD, 0x0D, 0xF8, 0x52, 0x91, 0x64, 0xFF, 0x7B, 0xAF, 0x3D, 0x94, 0xF3, 0xD9, 0xD0, 0xE9, 0x10},
        {0x10, 0x89, 0x48, 0x7F, 0x91, 0xEC, 0x39, 0xEF, 0x10, 0xBF, 0x60, 0xE9, 0x30, 0x5E, 0x95, 0xBD},
        {0xBD, 0xA2, 0x48, 0xC6, 0xFE, 0xEB, 0x2F, 0x84, 0xC9, 0xAD, 0x7C, 0x1A, 0x68, 0xBE, 0x9F, 0x27},
        {0x27, 0x7F, 0xC8, 0x98, 0xF3, 0x0F, 0x54, 0x08, 0xF6, 0xEE, 0x12, 0x8D, 0x2F, 0xB8, 0xD4, 0x5D},
        {0x5D, 0x4B, 0x8E, 0x60, 0x01, 0x2A, 0x6C, 0x09, 0x49, 0xAB, 0x8D, 0xCB, 0x14, 0x87, 0x49, 0xB8},
        {0xB8, 0x6E, 0x2A, 0xD4, 0xB1, 0x37, 0xAF, 0xD4, 0xBE, 0xF1, 0x2E, 0xBB, 0x1A, 0x4E, 0xE6, 0x7A},
        {0x7A, 0x16, 0xF5, 0x52, 0x78, 0x99, 0xEB, 0xD5, 0xE7, 0xC4, 0x2D, 0x06, 0x17, 0x62, 0xD5, 0x48},
        {0x48, 0xC3, 0x02, 0x0E, 0x58, 0x90, 0xE1, 0xA3, 0x6E, 0xAF, 0xBC, 0xC5, 0x0C, 0xEC, 0x76, 0x6C},
        {0x6C, 0x4C, 0xDD, 0x65, 0x01, 0xC4, 0xD4, 0x8D, 0xA4, 0x02, 0xEB, 0x20, 0xCA, 0x6B, 0xF2, 0x72},
        {0x72, 0xE8, 0x14, 0x07, 0x49, 0xF6, 0xD7, 0xA6, 0x6A, 0xD6, 0x11, 0x1C, 0x0C, 0x10, 0x33, 0x76},
        {0x76, 0xE3, 0x30, 0x9F, 0x6B, 0x30, 0x63, 0xA1, 0x2B, 0x1C, 0x43, 0x68, 0x70, 0x87, 0xC8, 0xA2},
        {0xA2, 0xD0, 0x44, 0x86, 0x2D, 0xB8, 0x64, 0xC1, 0x9C, 0x89, 0x48, 0x90, 0xDA, 0xC6, 0x20, 0x6E},
        {0x6E, 0x4D, 0x8E, 0xEA, 0xA9, 0xF6, 0xBF, 0x0A, 0xF3, 0xF2, 0x8E, 0x93, 0xBF, 0x74, 0x98, 0xCF}}
    };
    mul_mat_vec(out, &Lmat, in);
}

void make_table_LS_for_byte(Vec16 *tbl, int byte_ind)
{
    static const uint8_t pi[256] = {
        252, 238, 221,  17,  207, 110,  49,  22,  251, 196, 250, 218,   35, 197,   4,  77,
        233, 119, 240, 219,  147,  46, 153, 186,   23,  54, 241, 187,   20, 205,  95, 193,
        249,  24, 101,  90,  226,  92, 239,  33,  129,  28,  60,  66,  139,   1, 142,  79,
          5, 132,   2, 174,  227, 106, 143, 160,    6,  11, 237, 152,  127, 212, 211,  31,
        235,  52,  44,  81,  234, 200,  72, 171,  242,  42, 104, 162,  253,  58, 206, 204,
        181, 112,  14,  86,    8,  12, 118,  18,  191, 114,  19,  71,  156, 183,  93, 135,
         21, 161, 150,  41,   16, 123, 154, 199,  243, 145, 120, 111,  157, 158, 178, 177,
         50, 117,  25,  61,  255,  53, 138, 126,  109,  84, 198, 128,  195, 189,  13,  87,
        223, 245,  36, 169,   62, 168,  67, 201,  215, 121, 214, 246,  124,  34, 185,   3,
        224,  15, 236, 222,  122, 148, 176, 188,  220, 232,  40,  80,   78,  51,  10,  74,
        167, 151,  96, 115,   30,   0,  98,  68,   26, 184,  56, 130,  100, 159,  38,  65,
        173,  69,  70, 146,   39,  94,  85,  47,  140, 163, 165, 125,  105, 213, 149,  59,
          7,  88, 179,  64,  134, 172,  29, 247,   48,  55, 107, 228,  136, 217, 231, 137,
        225,  27, 131,  73,   76,  63, 248, 254,  141,  83, 170, 144,  202, 216, 133,  97,
         32, 113, 103, 164,   45,  43,   9,  91,  203, 155,  37, 208,  190, 229, 108,  82,
         89, 166, 116, 210,  230, 244, 180, 192,  209, 102, 175, 194,   57,  75 , 99, 182};
    Vec16 v = {.data = {.u8 = {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0}}};
    for (int i = 0; i < 256; i++) {
        v.data.u8[byte_ind] = pi[i];
        apply_L(&tbl[i], &v);
    }
}

/**
 * @brief Fill the lookup table for the LS transformation.
 * @relates KuznState
 */
void KuznState_make_table_LS()
{
    for (int i = 0; i < 16; i++) {
        make_table_LS_for_byte(lookup_table_LS + 256*i, i);
    }
}


/**
 * @brief Apply the combined LS transformation using precalculated lookup tables
 * stored in global variables.
 */
static inline void apply_fast_LS(Vec16 *out, Vec16 in)
{
    out->data.u64[0] = 0; out->data.u64[1] = 0;
    for (int i = 0; i < 16; i++) {
        uint8_t b = in.data.u8[i];
        out->data.u64[0] ^= lookup_table_LS[256*i + b].data.u64[0];
        out->data.u64[1] ^= lookup_table_LS[256*i + b].data.u64[1];
    }
}


/**
 * @brief Expand key: calculate all round keys.
 * @relates KuznState
 */
void KuznState_expand_key(KuznState *obj, const Key256 *key)
{
    int pos = 0;
    Vec16 c_in = {.data = {.u8 = {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0}}};
    Vec16 c, tmp, f, k1 = key->hi, k2 = key->lo;
    for (int i = 1; i <= 33; i++) {
        if ((i - 1) % 8 == 0) {
            obj->rk[pos++] = k1;
            obj->rk[pos++] = k2;
        }
        c_in.data.u8[0] = i;
        apply_L(&c, &c_in);
        tmp = k1;
        Vec16_xor(&tmp, &c);
        apply_fast_LS(&f, tmp);
        Vec16_xor(&k2, &f);
        tmp = k1;
        k1 = k2;
        k2 = tmp;
    }
}

/**
 * @brief Encrypt counter: generate 128 bits of pseudorandom numbers.
 * @relates KuznState
 */
void KuznState_block(KuznState *obj)
{
    static const int nrounds = 10;
    obj->out = obj->ctr;
    for (int i = 0; i < nrounds - 1; i++) {
        Vec16_xor(&obj->out, &obj->rk[i]);
        apply_fast_LS(&obj->out, obj->out);
    }
    Vec16_xor(&obj->out, &obj->rk[nrounds - 1]);
}


/**
 * @brief Increase the 64-bit counter.
 * @relates KuznState
 */
static inline void KuznState_inc_counter(KuznState *obj)
{
    obj->ctr.data.u64[0]++;
}


/**
 * @brief Initialize PNG state: reset the counter and initialize round keys.
 * @relates KuznState
 */
void KuznState_init(KuznState *obj, const Key256 *key)
{
    KuznState_expand_key(obj, key);
    for (int i = 0; i < 16; i++) {
        obj->ctr.data.u8[i] = 0;
        obj->out.data.u8[i] = 0;
    }
    obj->pos = 2;
}

/**
 * @brief Test L and LS transformations using test vectors from RFC7801
 * and GOST R 34.12-2015.
 */
int test_LS(const CallerAPI *intf)
{
    Vec16 v;
    // Test 1
    Vec16 in1_for_L = {.data = {.u8 = {
        0x8a, 0x74, 0x1b, 0xe8,  0x5a, 0x4a, 0x8f, 0xb7,
        0xab, 0x7a, 0x94, 0xa7,  0x37, 0xca, 0x98, 0x09
    }}};
    Vec16 in1_for_LS = {.data = {.u8 = {
        0x76, 0xf2, 0xd1, 0x99,  0x23, 0x9f, 0x36, 0x5d,
        0x47, 0x94, 0x95, 0xa0,  0xc9, 0xdc, 0x3b, 0xe6
    }}};
    Vec16 out1 = {.data = {.u8 = {
        0xa6, 0x44, 0x61, 0x5e,  0x1d, 0x07, 0x57, 0x92,
        0x6a, 0x5d, 0xb7, 0x9d,  0x99, 0x40, 0x09, 0x3d
    }}};
    // Test 2
    Vec16 in2_for_L = {.data = {.u8 = {
        0xb6, 0xb6, 0xb6, 0xb6,  0xb6, 0xb6, 0xb6, 0xb6,
        0xb6, 0xe8, 0x7d, 0xe8,  0xb6, 0xe8, 0x7d, 0xe8
    }}};

    Vec16 in2_for_LS = {.data = {.u8 = {
        0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff,
        0xff, 0x99, 0xbb, 0x99,  0xff, 0x99, 0xbb, 0x99
    }}};
    Vec16 out2 = {.data = {.u8 = {
        0x30, 0x08, 0x14, 0x49,  0x92, 0x2f, 0x4a, 0xcf,
        0xa1, 0xb0, 0x55, 0xe3,  0x86, 0xb6, 0x97, 0xe2
    }}};
    int is_ok = 1;
    // Tests implementation
    intf->printf("----- test_LS -----\n");
    intf->printf("--- Test 1 ---\n");
    apply_L(&v, &in1_for_L);
    intf->printf("L output:     ");  Vec16_print(intf, &v);
    intf->printf("L reference:  ");  Vec16_print(intf, &out1);
    if (!Vec16_compare(&v, &out1)) {
        intf->printf("^^^^^ FAILURE ^^^^^\n");
        is_ok = 0;
    }
    apply_fast_LS(&v, in1_for_LS);
    intf->printf("LS output:    "); Vec16_print(intf, &v);
    intf->printf("LS reference: "); Vec16_print(intf, &out1);
    if (!Vec16_compare(&v, &out1)) {
        intf->printf("^^^^^ FAILURE ^^^^^\n");
        is_ok = 0;
    }

    intf->printf("--- Test 2 ---\n");
    apply_L(&v, &in2_for_L);
    intf->printf("L output:     ");  Vec16_print(intf, &v);
    intf->printf("L reference:  ");  Vec16_print(intf, &out2);
    apply_fast_LS(&v, in2_for_LS);
    if (!Vec16_compare(&v, &out2)) {
        intf->printf("^^^^^ FAILURE ^^^^^\n");
        is_ok = 0;
    }
    intf->printf("LS output:    "); Vec16_print(intf, &v);
    intf->printf("LS reference: "); Vec16_print(intf, &out2);
    if (!Vec16_compare(&v, &out2)) {
        intf->printf("^^^^^ FAILURE ^^^^^\n");
        is_ok = 0;
    }
    if (is_ok) {
        intf->printf("test_LS: success\n");
    } else {
        intf->printf("test_LS: failure\n");
    }
    return is_ok;
}

/**
 * @brief Test the block encryption subroutine using test vectors from RFC7801
 * and GOST R 34.12-2015.
 */
int test_block(const CallerAPI *intf)
{
    int is_ok = 1;
    static const Key256 key = {
        .lo = {.data = { .u8 =
            {0xef, 0xcd, 0xab, 0x89,  0x67, 0x45, 0x23, 0x01,
             0x10, 0x32, 0x54, 0x76,  0x98, 0xba, 0xdc, 0xfe}}},
        .hi = {.data = { .u8 =
            {0x77, 0x66, 0x55, 0x44,  0x33, 0x22, 0x11, 0x00,
             0xff, 0xee, 0xdd, 0xcc,  0xbb, 0xaa, 0x99, 0x88}}}
    };
    static const Vec16 ctr = {.data = {.u8 =
            {0x88, 0x99, 0xaa, 0xbb,  0xcc, 0xdd, 0xee, 0xff,
             0x00, 0x77, 0x66, 0x55,  0x44, 0x33, 0x22, 0x11
    }}};
    static const Vec16 out = {.data = {.u8 =
            {0xcd, 0xed, 0xd4, 0xb9,  0x42, 0x8d, 0x46, 0x5a,
             0x30, 0x24, 0xbc, 0xbe,  0x90, 0x9d, 0x67, 0x7f
    }}};
    static const Vec16 rk[10] = {
        {.data = {.u8 = 
            {0x77, 0x66, 0x55, 0x44,  0x33, 0x22, 0x11, 0x00,
             0xff, 0xee, 0xdd, 0xcc,  0xbb, 0xaa, 0x99, 0x88}}},
        {.data = {.u8 =
            {0xef, 0xcd, 0xab, 0x89,  0x67, 0x45, 0x23, 0x01,
             0x10, 0x32, 0x54, 0x76,  0x98, 0xba, 0xdc, 0xfe}}},
        {.data = {.u8 =
            {0x44, 0x8c, 0xc7, 0x8c,  0xef, 0x6a, 0x8d, 0x22,
             0x43, 0x43, 0x69, 0x15,  0x53, 0x48, 0x31, 0xdb}}},
        {.data = {.u8 = 
            {0x04, 0xfd, 0x9f, 0x0a,  0xc4, 0xad, 0xeb, 0x15,
             0x68, 0xec, 0xcf, 0xe9,  0xd8, 0x53, 0x45, 0x3d}}},
        {.data = {.u8 =
            {0xac, 0xf1, 0x29, 0xf4,  0x46, 0x92, 0xe5, 0xd3,
             0x28, 0x5e, 0x4a, 0xc4,  0x68, 0x64, 0x64, 0x57}}},
        {.data = {.u8 =
            {0x1b, 0x58, 0xda, 0x34,  0x28, 0xe8, 0x32, 0xb5,
             0x32, 0x64, 0x5c, 0x16,  0x35, 0x94, 0x07, 0xbd}}},
        {.data = {.u8 =
            {0xb1, 0x98, 0x00, 0x5a,  0x26, 0x27, 0x57, 0x70,
             0xde, 0x45, 0x87, 0x7e,  0x75, 0x40, 0xe6, 0x51}}},
        {.data = {.u8 =
            {0x84, 0xf9, 0x86, 0x22,  0xa2, 0x91, 0x2a, 0xd7,
             0x3e, 0xdd, 0x9f, 0x7b,  0x01, 0x25, 0x79, 0x5a}}},
        {.data = {.u8 =
            {0x17, 0xe5, 0xb6, 0xcd,  0x73, 0x2f, 0xf3, 0xa5,
             0x23, 0x31, 0xc7, 0x78,  0x53, 0xe2, 0x44, 0xbb}}},
        {.data = {.u8 =
            {0x43, 0x40, 0x4a, 0x8e,  0xa8, 0xba, 0x5d, 0x75,
             0x5b, 0xf4, 0xbc, 0x16,  0x74, 0xdd, 0xe9, 0x72}}}
    };

    KuznState obj;
    KuznState_init(&obj, &key);
    obj.ctr = ctr;
    KuznState_block(&obj);
    intf->printf("----- test_block -----\n");

    for (int i = 0; i < 10; i++) {
        intf->printf("RK%d(out): ", i); Vec16_print(intf, &obj.rk[i]);
        intf->printf("RK%d(ref): ", i); Vec16_print(intf, &rk[i]);
        is_ok = is_ok & Vec16_compare(&obj.rk[i], &rk[i]);
        if (!is_ok) {
            intf->printf("^^^^ FAILURE ^^^^^\n");
        }
    }
    if (is_ok) {
        intf->printf("test_block (round keys): success\n");
    } else {
        intf->printf("test_block (round keys): failure\n");
        return 0;
    }

    intf->printf("Output:    "); Vec16_print(intf, &obj.out);
    intf->printf("Reference: "); Vec16_print(intf, &out);

    is_ok = is_ok & Vec16_compare(&out, &obj.out);
    if (is_ok) {
        intf->printf("test_block (ciphertext): success\n");
    } else {
        intf->printf("test_block: failure\n");
    }
    return is_ok;
}



static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
    is_ok = is_ok & test_LS(intf);
    is_ok = is_ok & test_block(intf);
    return is_ok;
}


static void *create(const CallerAPI *intf)
{
    KuznState *obj = intf->malloc(sizeof(KuznState));
    Key256 key;
    uint64_t seeds[4];
    for (int i = 0; i < 4; i++) {
        seeds[i] = intf->get_seed64();
    }
    Key256_fill(&key, seeds);
    KuznState_init(obj, &key);
    return obj;
}


static inline uint64_t get_bits_raw(void *state)
{
    KuznState *obj = state;
    uint64_t *out = obj->out.data.u64;
    if (obj->pos >= 2) {
        KuznState_block(obj);
        KuznState_inc_counter(obj);
        obj->pos = 0;
    }
    return out[obj->pos++];
}


EXPORT uint64_t get_bits(void *state)
{
    return get_bits_raw(state);
}

EXPORT uint64_t get_sum(void *state, size_t len)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += get_bits_raw(state);
    }
    return sum;
}


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    // Initialize the lookup table for the LS transformation
    KuznState_make_table_LS();
    // Fill the output structure
    gi->name = "Kuznyechik";
    gi->description = NULL;
    gi->nbits = 64;
    gi->get_bits = get_bits;
    gi->create = default_create;
    gi->free = default_free;
    gi->get_sum = get_sum;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    (void) intf;
    return 1;
}
