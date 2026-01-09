/**
 * @file des.c
 * @brief DES based PRNG implementation.
 * @details DES is an obsolete block cipher with 64-bit blocks and 56-bit key.
 * This implementation has some simple optimizations and has a speed around
 * 30 cpb. It also equipped with an internal self-test based on the test
 * suggested by Ronald L. Rivest.
 *
 * References:
 *
 * - FIPS PUB 46-3. Data Encryption Standard (DES)
 *   https://csrc.nist.gov/pubs/fips/46-3/final
 * - Ronald L. Rivest. Testing implementations of DES. 1985.
 *   https://people.csail.mit.edu/rivest/pubs/pubs/Riv85.txt
 * - Bruce Schneier Twofish's Performance vs. Other Block Ciphers
 *   (on a Pentium) https://www.schneier.com/academic/twofish/performance/
 * - https://github.com/dhuertas/DES/blob/master/des.c
 * - https://page.math.tu-berlin.de/~kant/teaching/hess/krypto-ws2006/des.htm
 * - https://github.com/openwebos/qt/blob/master/src/3rdparty/des/des.cpp#L489
 *
 * @copyright This PRNG uses code from two other DES implementations licensed
 * under the MIT license:
 *
 * Non-optimized (naive) DES implementation by D.H. Gonzales.
 * (c) 2020 Daniel Huertas Gonzalez (huertas.dani@gmail.com)
 * https://github.com/dhuertas/DES/blob/master/des.c
 *
 * Optimized implementation of DES encryption for NTLM.
 * (c) 1997-2005 Simon Tatham.
 * https://github.com/openwebos/qt/blob/master/src/3rdparty/des/des.cpp#L489
 *
 * Adaptation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define LB64_MASK   0x0000000000000001
#define L64_MASK    0x00000000ffffffff
#define H64_MASK    0xffffffff00000000

/**
 * @brief The S-box table in the optimized format: we use
 * @details Possible code for conversion:
 *
 *     uint32_t des_pfunc(uint32_t x)
 *     {
 *         uint64_t out = x;
 *         for (int j = 0; j < 32; j++) {
 *             out <<= 1;
 *             out |= (x >> (32 - P[j])) & 0x1;
 *         }
 *         return out;
 *     }
 *
 *     int main()
 *     {
 *        for (int i = 0; i < 8; i++) {
 *            for (int j = 0; j < 64; j++) {
 *                int col = (j >> 1) & 0xF;
 *                int row = ((j >> 4) & 0x2) | (j & 0x1);
 *                int ind = (row << 4) | col;
 *                printf("%2d, ", S[i][ind]);
 *                if ((j + 1) % 16 == 0)
 *                    printf("\n");
 *            }
 *            printf("\n");
 *        }
 *        return 0;
 *     }
 *
 *     for (int i = 0; i < 8; i++) {
 *         for (int j = 0; j < 64; j++) {
 *             uint32_t s_output = ((uint32_t) (Sx[i][j])) << (28 - 4*i);
 *             printf("0x%8.8X, ", (uint32_t) des_pfunc(s_output));
 *             if ((j + 1) % 8 == 0) {
 *                 printf("\n");
 *             }
 *         }
 *         printf("\n");
 *     }
 */
static const uint32_t Sw[8][64] = {{ // 0
    0x00808200, 0x00000000, 0x00008000, 0x00808202, 0x00808002, 0x00008202, 0x00000002, 0x00008000, 
    0x00000200, 0x00808200, 0x00808202, 0x00000200, 0x00800202, 0x00808002, 0x00800000, 0x00000002, 
    0x00000202, 0x00800200, 0x00800200, 0x00008200, 0x00008200, 0x00808000, 0x00808000, 0x00800202, 
    0x00008002, 0x00800002, 0x00800002, 0x00008002, 0x00000000, 0x00000202, 0x00008202, 0x00800000, 
    0x00008000, 0x00808202, 0x00000002, 0x00808000, 0x00808200, 0x00800000, 0x00800000, 0x00000200, 
    0x00808002, 0x00008000, 0x00008200, 0x00800002, 0x00000200, 0x00000002, 0x00800202, 0x00008202, 
    0x00808202, 0x00008002, 0x00808000, 0x00800202, 0x00800002, 0x00000202, 0x00008202, 0x00808200, 
    0x00000202, 0x00800200, 0x00800200, 0x00000000, 0x00008002, 0x00008200, 0x00000000, 0x00808002
},{ // 1
    0x40084010, 0x40004000, 0x00004000, 0x00084010, 0x00080000, 0x00000010, 0x40080010, 0x40004010, 
    0x40000010, 0x40084010, 0x40084000, 0x40000000, 0x40004000, 0x00080000, 0x00000010, 0x40080010, 
    0x00084000, 0x00080010, 0x40004010, 0x00000000, 0x40000000, 0x00004000, 0x00084010, 0x40080000, 
    0x00080010, 0x40000010, 0x00000000, 0x00084000, 0x00004010, 0x40084000, 0x40080000, 0x00004010, 
    0x00000000, 0x00084010, 0x40080010, 0x00080000, 0x40004010, 0x40080000, 0x40084000, 0x00004000, 
    0x40080000, 0x40004000, 0x00000010, 0x40084010, 0x00084010, 0x00000010, 0x00004000, 0x40000000, 
    0x00004010, 0x40084000, 0x00080000, 0x40000010, 0x00080010, 0x40004010, 0x40000010, 0x00080010, 
    0x00084000, 0x00000000, 0x40004000, 0x00004010, 0x40000000, 0x40080010, 0x40084010, 0x00084000
},{ // 2
    0x00000104, 0x04010100, 0x00000000, 0x04010004, 0x04000100, 0x00000000, 0x00010104, 0x04000100, 
    0x00010004, 0x04000004, 0x04000004, 0x00010000, 0x04010104, 0x00010004, 0x04010000, 0x00000104, 
    0x04000000, 0x00000004, 0x04010100, 0x00000100, 0x00010100, 0x04010000, 0x04010004, 0x00010104, 
    0x04000104, 0x00010100, 0x00010000, 0x04000104, 0x00000004, 0x04010104, 0x00000100, 0x04000000, 
    0x04010100, 0x04000000, 0x00010004, 0x00000104, 0x00010000, 0x04010100, 0x04000100, 0x00000000, 
    0x00000100, 0x00010004, 0x04010104, 0x04000100, 0x04000004, 0x00000100, 0x00000000, 0x04010004, 
    0x04000104, 0x00010000, 0x04000000, 0x04010104, 0x00000004, 0x00010104, 0x00010100, 0x04000004, 
    0x04010000, 0x04000104, 0x00000104, 0x04010000, 0x00010104, 0x00000004, 0x04010004, 0x00010100
},{ // 3
    0x80401000, 0x80001040, 0x80001040, 0x00000040, 0x00401040, 0x80400040, 0x80400000, 0x80001000, 
    0x00000000, 0x00401000, 0x00401000, 0x80401040, 0x80000040, 0x00000000, 0x00400040, 0x80400000, 
    0x80000000, 0x00001000, 0x00400000, 0x80401000, 0x00000040, 0x00400000, 0x80001000, 0x00001040, 
    0x80400040, 0x80000000, 0x00001040, 0x00400040, 0x00001000, 0x00401040, 0x80401040, 0x80000040, 
    0x00400040, 0x80400000, 0x00401000, 0x80401040, 0x80000040, 0x00000000, 0x00000000, 0x00401000, 
    0x00001040, 0x00400040, 0x80400040, 0x80000000, 0x80401000, 0x80001040, 0x80001040, 0x00000040, 
    0x80401040, 0x80000040, 0x80000000, 0x00001000, 0x80400000, 0x80001000, 0x00401040, 0x80400040, 
    0x80001000, 0x00001040, 0x00400000, 0x80401000, 0x00000040, 0x00400000, 0x00001000, 0x00401040
},{ // 4
    0x00000080, 0x01040080, 0x01040000, 0x21000080, 0x00040000, 0x00000080, 0x20000000, 0x01040000, 
    0x20040080, 0x00040000, 0x01000080, 0x20040080, 0x21000080, 0x21040000, 0x00040080, 0x20000000, 
    0x01000000, 0x20040000, 0x20040000, 0x00000000, 0x20000080, 0x21040080, 0x21040080, 0x01000080, 
    0x21040000, 0x20000080, 0x00000000, 0x21000000, 0x01040080, 0x01000000, 0x21000000, 0x00040080, 
    0x00040000, 0x21000080, 0x00000080, 0x01000000, 0x20000000, 0x01040000, 0x21000080, 0x20040080, 
    0x01000080, 0x20000000, 0x21040000, 0x01040080, 0x20040080, 0x00000080, 0x01000000, 0x21040000, 
    0x21040080, 0x00040080, 0x21000000, 0x21040080, 0x01040000, 0x00000000, 0x20040000, 0x21000000, 
    0x00040080, 0x01000080, 0x20000080, 0x00040000, 0x00000000, 0x20040000, 0x01040080, 0x20000080
},{ // 5
    0x10000008, 0x10200000, 0x00002000, 0x10202008, 0x10200000, 0x00000008, 0x10202008, 0x00200000, 
    0x10002000, 0x00202008, 0x00200000, 0x10000008, 0x00200008, 0x10002000, 0x10000000, 0x00002008, 
    0x00000000, 0x00200008, 0x10002008, 0x00002000, 0x00202000, 0x10002008, 0x00000008, 0x10200008, 
    0x10200008, 0x00000000, 0x00202008, 0x10202000, 0x00002008, 0x00202000, 0x10202000, 0x10000000, 
    0x10002000, 0x00000008, 0x10200008, 0x00202000, 0x10202008, 0x00200000, 0x00002008, 0x10000008, 
    0x00200000, 0x10002000, 0x10000000, 0x00002008, 0x10000008, 0x10202008, 0x00202000, 0x10200000, 
    0x00202008, 0x10202000, 0x00000000, 0x10200008, 0x00000008, 0x00002000, 0x10200000, 0x00202008, 
    0x00002000, 0x00200008, 0x10002008, 0x00000000, 0x10202000, 0x10000000, 0x00200008, 0x10002008
},{ // 6
    0x00100000, 0x02100001, 0x02000401, 0x00000000, 0x00000400, 0x02000401, 0x00100401, 0x02100400, 
    0x02100401, 0x00100000, 0x00000000, 0x02000001, 0x00000001, 0x02000000, 0x02100001, 0x00000401, 
    0x02000400, 0x00100401, 0x00100001, 0x02000400, 0x02000001, 0x02100000, 0x02100400, 0x00100001, 
    0x02100000, 0x00000400, 0x00000401, 0x02100401, 0x00100400, 0x00000001, 0x02000000, 0x00100400, 
    0x02000000, 0x00100400, 0x00100000, 0x02000401, 0x02000401, 0x02100001, 0x02100001, 0x00000001, 
    0x00100001, 0x02000000, 0x02000400, 0x00100000, 0x02100400, 0x00000401, 0x00100401, 0x02100400, 
    0x00000401, 0x02000001, 0x02100401, 0x02100000, 0x00100400, 0x00000000, 0x00000001, 0x02100401, 
    0x00000000, 0x00100401, 0x02100000, 0x00000400, 0x02000001, 0x02000400, 0x00000400, 0x00100001
},{ // 7
    0x08000820, 0x00000800, 0x00020000, 0x08020820, 0x08000000, 0x08000820, 0x00000020, 0x08000000, 
    0x00020020, 0x08020000, 0x08020820, 0x00020800, 0x08020800, 0x00020820, 0x00000800, 0x00000020, 
    0x08020000, 0x08000020, 0x08000800, 0x00000820, 0x00020800, 0x00020020, 0x08020020, 0x08020800, 
    0x00000820, 0x00000000, 0x00000000, 0x08020020, 0x08000020, 0x08000800, 0x00020820, 0x00020000, 
    0x00020820, 0x00020000, 0x08020800, 0x00000800, 0x00000020, 0x08020020, 0x00000800, 0x00020820, 
    0x08000800, 0x00000020, 0x08000020, 0x08020000, 0x08020020, 0x08000000, 0x00020000, 0x08000820, 
    0x00000000, 0x08020820, 0x00020020, 0x08000020, 0x08020000, 0x08000800, 0x08000820, 0x00000000, 
    0x08020820, 0x00020800, 0x00020800, 0x00000820, 0x00000820, 0x00020020, 0x08000000, 0x08020800
}};

/* Permuted Choice 1 Table */
static const char PC1[] = {
    57, 49, 41, 33, 25, 17,  9,
     1, 58, 50, 42, 34, 26, 18,
    10,  2, 59, 51, 43, 35, 27,
    19, 11,  3, 60, 52, 44, 36,
    
    63, 55, 47, 39, 31, 23, 15,
     7, 62, 54, 46, 38, 30, 22,
    14,  6, 61, 53, 45, 37, 29,
    21, 13,  5, 28, 20, 12,  4
};

/* Permuted Choice 2 Table */
static const char PC2[] = {
    14, 17, 11, 24,  1,  5,
     3, 28, 15,  6, 21, 10,
    23, 19, 12,  4, 26,  8,
    16,  7, 27, 20, 13,  2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32
};

/* Iteration Shift Array */
static const char iteration_shift[] = {
 /* 1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 */
    1,  1,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2,  2,  2,  1
};


/**
 * @brief DES round keys with 6-bit digits unwrapped into bytes.
 * Odd and even 6-bit digits are kept in different 32-bit words.
 */
typedef struct {
    uint32_t k0246; ///< Keeps 6-bit parts/digits 0,2,4,6 (0 is the highest digit)
    uint32_t k1357; ///< Keeps 6-bit parts/digits 1,3,5,7 (1 is the highest digit)
} DESSubkey;

/**
 * @brief Calculate DES key schedule.
 * @param[in]  key     Input 56-bit key.
 * @param[out] sub_key Output buffer for 16 round keys.
 */
void fill_key_schedule(DESSubkey *sub_key, uint64_t key)
{
    uint64_t permuted_choice_1 = 0; // 56-bit
    uint64_t permuted_choice_2 = 0; // 56-bit
    /* initial key schedule calculation */
    for (int i = 0; i < 56; i++) {
        permuted_choice_1 <<= 1;
        permuted_choice_1 |= (key >> (64-PC1[i])) & LB64_MASK;

    }
    // C and D are 28-bit
    uint32_t C = (uint32_t) ((permuted_choice_1 >> 28) & 0x000000000fffffff);
    uint32_t D = (uint32_t) (permuted_choice_1 & 0x000000000fffffff);
    /* Calculation of the 16 keys */
    for (int i = 0; i < 16; i++) {
        /* key schedule */
        // shifting Ci and Di
        for (int j = 0; j < iteration_shift[i]; j++) {
            C = (0x0fffffff & (C << 1)) | (0x00000001 & (C >> 27));
            D = (0x0fffffff & (D << 1)) | (0x00000001 & (D >> 27));
        }
        permuted_choice_2 = 0;
        permuted_choice_2 = (((uint64_t) C) << 28) | (uint64_t) D ;
        uint64_t sk = 0;
        for (int j = 0; j < 48; j++) {
            sk <<= 1;
            sk |= (permuted_choice_2 >> (56-PC2[j])) & LB64_MASK;
        }
        sub_key[i].k0246 = (uint32_t) (
            ((sk >> (42 - 24)) & 0x3F000000) |
            ((sk >> (30 - 16)) & 0x3F0000) |
            ((sk >> (18 - 8)) & 0x3F00) |
            ((sk >> 6) & 0x3F)
        );
        sub_key[i].k1357 = (uint32_t) (
            ((sk >> (36 - 24)) & 0x3F000000) |
            ((sk >> (24 - 16)) & 0x3F0000) |
            ((sk >> (12 - 8)) & 0x3F00) |
            (sk & 0x3F)
        );
    }
}


/**
 * @brief DES PRNG state.
 */
typedef struct {
    uint64_t ctr;
    uint64_t key;
    uint64_t out;
    DESSubkey sub_key[16];
} DESState;


/**
 * @brief Initialize DES-based PRNG.
 */
void DESState_init(DESState *obj, uint64_t key)
{
    obj->ctr = 0;
    obj->key = key;
    fill_key_schedule(obj->sub_key, key);
}

/**
 * @brief DES round function that uses pre-calculated lookup tables
 * and a pre-parsed round key.
 * @details It consists of the next step:
 *
 * 1. Extract 6-bit digits from the input 32-bit word and apply
 *    pre-parsed round key.
 * 2. Apply S-boxes and output permutations using the pre-calculated
 *    lookup tables
 */
static inline uint32_t des_ff(uint32_t x, DESSubkey key)
{
    uint32_t x0246 = (rotr32(x, 3) & 0x3F3F3F3F) ^ key.k0246;
    uint32_t x1357 = (rotl32(x, 1) & 0x3F3F3F3F) ^ key.k1357;
    uint32_t out =
            Sw[0][x0246 >> 24] |
            Sw[1][x1357 >> 24] |
            Sw[2][(x0246 >> 16) & 0xFF] |
            Sw[3][(x1357 >> 16) & 0xFF] |
            Sw[4][(x0246 >> 8) & 0xFF] |
            Sw[5][(x1357 >> 8) & 0xFF] |
            Sw[6][x0246 & 0xFF] |
            Sw[7][x1357 & 0xFF];
    return out;
}

#define bitswap(L, R, n, mask) { \
    uint32_t swap = mask & ( (R >> n) ^ L ); \
    R ^= swap << n; \
    L ^= swap; }

/* Initial permutation */
#define IP(L, R) {\
    bitswap(R, L,  4, 0x0F0F0F0F) \
    bitswap(R, L, 16, 0x0000FFFF) \
    bitswap(L, R,  2, 0x33333333) \
    bitswap(L, R,  8, 0x00FF00FF) \
    bitswap(R, L,  1, 0x55555555)}

/* Final permutation */
#define FP(L, R) {\
	bitswap(R, L,  1, 0x55555555) \
	bitswap(L, R,  8, 0x00FF00FF) \
	bitswap(L, R,  2, 0x33333333) \
	bitswap(R, L, 16, 0x0000FFFF) \
	bitswap(R, L,  4, 0x0F0F0F0F)}


/**
 * @brief Encrypt 64-bit block using DES.
 */
uint64_t DESState_encrypt(DESState *obj, uint64_t in)
{
    uint32_t L = (uint32_t) (in >> 32) & L64_MASK;
    uint32_t R = (uint32_t) (in) & L64_MASK;
    IP(L, R);
    for (int i = 0; i < 16; i += 2) {
        L ^= des_ff(R, obj->sub_key[i]);
        R ^= des_ff(L, obj->sub_key[i + 1]);
    }
    FP(R, L);
    return (((uint64_t) R) << 32) | (uint64_t) L;
}

/**
 * @brief Decrypt 64-bit block using DES.
 */
uint64_t DESState_decrypt(DESState *obj, uint64_t in)
{
    uint32_t L = (uint32_t) (in >> 32) & L64_MASK;
    uint32_t R = (uint32_t) (in) & L64_MASK;
    IP(L, R);
    for (int i = 15; i >= 1; i -= 2) {
        L ^= des_ff(R, obj->sub_key[i]);
        R ^= des_ff(L, obj->sub_key[i - 1]);
    }
    // inverse initial permutation
    FP(R, L);
    return (((uint64_t) R) << 32) | (uint64_t) L;
}




static inline uint64_t get_bits_raw(DESState *obj)
{
    obj->out = DESState_encrypt(obj, obj->ctr++);
    return obj->out;
}


static void *create(const CallerAPI *intf)
{
    DESState *obj = intf->malloc(sizeof(DESState));
    DESState_init(obj, intf->get_seed64() >> 8);
    return obj;
}


/**
 * @brief Internal self-test
 * @details Based on "Testing implementation of DES" by Ronald L. Rivest.
 * X0:  9474B8E8C73BCA7D; X16: 1B1A2DDB4C642438
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t refval[] = {
        0x8da744e0c94e5e17, 0x0cdb25e3ba3c6d79, // E/D
        0x4784c4ba5006081f, 0x1cf1fc126f2ef842,
        0xe4be250042098d13, 0x7bfc5dc6adb5797c,
        0x1ab3b4d82082fb28, 0xc1576a14de707097,
        0x739b68cd2e26782a, 0x2a59f0c464506edb,
        0xa5c39d4251f0a81e, 0x7239ac9a6107ddb1,
        0x070cac8590241233, 0x78f87b6e3dfecf61,
        0x95ec2578c2c433f0, 0x1b1a2ddb4c642438 // X16
    };
    DESState *obj = intf->malloc(sizeof(DESState));
    uint64_t input = 0x9474B8E8C73BCA7D;
    //uint64_t key = 0x0000000000000000;
    uint64_t result = input;
    int is_ok = 1;
    for (int i = 0; i < 16; i++) {
        char mode;
        DESState_init(obj, result);
        obj->ctr = result;
        if (i % 2 == 0) {
            result = DESState_encrypt(obj, result);
            mode = 'e';
        } else {
            result = DESState_decrypt(obj, result);
            mode = 'd';
        }
        intf->printf("%c: %016llx %016llx\n", mode, result, refval[i]);
        if (result != refval[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT64_PRNG("DES", run_self_test)

    