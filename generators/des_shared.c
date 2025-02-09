/*
NOT OPTIMIZED!
https://github.com/dhuertas/DES/blob/master/des.c
https://page.math.tu-berlin.de/~kant/teaching/hess/krypto-ws2006/des.htm
https://people.csail.mit.edu/rivest/pubs/pubs/Riv85.txt
https://github.com/openwebos/qt/blob/master/src/3rdparty/des/des.cpp#L489
*/

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/*
 * Data Encryption Standard
 * An approach to DES algorithm
 * 
 * By: Daniel Huertas Gonzalez
 * Email: huertas.dani@gmail.com
 * Version: 0.1
 * 
 * Based on the document FIPS PUB 46-3
 */

#define LB32_MASK   0x00000001
#define LB64_MASK   0x0000000000000001
#define L64_MASK    0x00000000ffffffff
#define H64_MASK    0xffffffff00000000

/* Initial Permutation Table */
static const char IP[] = {
    58, 50, 42, 34, 26, 18, 10,  2, 
    60, 52, 44, 36, 28, 20, 12,  4, 
    62, 54, 46, 38, 30, 22, 14,  6, 
    64, 56, 48, 40, 32, 24, 16,  8, 
    57, 49, 41, 33, 25, 17,  9,  1, 
    59, 51, 43, 35, 27, 19, 11,  3, 
    61, 53, 45, 37, 29, 21, 13,  5, 
    63, 55, 47, 39, 31, 23, 15,  7
};

/* Inverse Initial Permutation Table */
static const char PI[] = {
    40,  8, 48, 16, 56, 24, 64, 32, 
    39,  7, 47, 15, 55, 23, 63, 31, 
    38,  6, 46, 14, 54, 22, 62, 30, 
    37,  5, 45, 13, 53, 21, 61, 29, 
    36,  4, 44, 12, 52, 20, 60, 28, 
    35,  3, 43, 11, 51, 19, 59, 27, 
    34,  2, 42, 10, 50, 18, 58, 26, 
    33,  1, 41,  9, 49, 17, 57, 25
};

/**
 * @brief The S-box table in the optimized format: we use
 * @details Possible code for conversion:
 *
uint32_t des_pfunc(uint32_t x)
{
    uint64_t out = x;
    for (int j = 0; j < 32; j++) {
        out <<= 1;
        out |= (x >> (32 - P[j])) & 0x1;
    }
    return out;
}

 * int main()
 * {
 *    for (int i = 0; i < 8; i++) {
 *        for (int j = 0; j < 64; j++) {
 *            int col = (j >> 1) & 0xF;
 *            int row = ((j >> 4) & 0x2) | (j & 0x1);
 *            int ind = (row << 4) | col;
 *            printf("%2d, ", S[i][ind]);
 *            if ((j + 1) % 16 == 0)
 *                printf("\n");
 *        }
 *        printf("\n");
 *    }
 *    return 0;
 * }

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 64; j++) {
            uint32_t s_output = ((uint32_t) (Sx[i][j])) << (28 - 4*i);
            printf("0x%8.8X, ", (uint32_t) des_pfunc(s_output));
            if ((j + 1) % 8 == 0) {
                printf("\n");
            }
        }
        printf("\n");
    }
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


/* initial permutation */
static inline uint64_t init_perm_func(uint64_t x)
{
    uint64_t out = 0;
    for (int i = 0; i < 64; i++) {
        out <<= 1;
        out |= (x >> (64-IP[i])) & LB64_MASK;
    }
    return out;
}


/* inverse initial permutation */
static inline uint64_t inv_init_perm_func(uint32_t L, uint32_t R)
{
    uint64_t x = (((uint64_t) R) << 32) | (uint64_t) L;
    uint64_t out = 0;
    for (int i = 0; i < 64; i++) {
        out <<= 1;
        out |= (x >> (64 - PI[i])) & LB64_MASK;
    }
    return out;
}

void fill_key_schedule(uint64_t *sub_key, uint64_t key)
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
        sub_key[i] = 0;
        for (int j = 0; j < 48; j++) {
            sub_key[i] <<= 1;
            sub_key[i] |= (permuted_choice_2 >> (56-PC2[j])) & LB64_MASK;
        }
    }
}



typedef struct {
    uint64_t ctr;
    uint64_t key;
    uint64_t out;
    uint64_t sub_key[16];
} DESState;


void DESState_init(DESState *obj, uint64_t key)
{
    obj->ctr = 0;
    obj->key = key;
    fill_key_schedule(obj->sub_key, key);
}





// Expansion permutation
// Highest byte is 1, lowest byte is 32
// 32**1  2  3  4  5** 4  5  6  7  8  9**   <== 1-12
//  8  9 10 11 12 13**12 13 14 15 16 17**   <== 13-24
// 16 17 18 19 20 21**20 21 22 23 24 25**   <== 25-36
// 24 25 26 27 28 29**28 29 30 31 32**1**   <== 37-48
//
//
//            1-8  9-16 17-24 25-32: w32
// 1-8 9-16 17-24 25-32 33-40 41-48: w48
uint64_t des_expand(uint32_t R)
{
    uint64_t r = R;
    uint64_t e = ((r & 0x1) << 47) | (r >> 31);
    e |= (r & 0xF8000000) << (42 - 27);
    e |= (r & 0x1F800000) << (36 - 23);
    e |= (r & 0x01F80000) << (30 - 19);
    e |= (r & 0x001F8000) << (24 - 15);
    e |= (r & 0x0001F800) << (18 - 11);
    e |= (r & 0x00001F80) << (12 - 7);
    e |= (r & 0x000001F8) << (6 - 3);
    e |= (r & 0x0000001F) << 1;
    return e;
}


static inline uint32_t des_spbox_perm(uint64_t s_input)
{
    uint32_t s_output = Sw[0][s_input >> 42] |
            Sw[1][(s_input >> 36) & 0x3F] |
            Sw[2][(s_input >> 30) & 0x3F] |
            Sw[3][(s_input >> 24) & 0x3F] |
            Sw[4][(s_input >> 18) & 0x3F] |
            Sw[5][(s_input >> 12) & 0x3F] |
            Sw[6][(s_input >> 6) & 0x3F] |
            Sw[7][s_input & 0x3F];
    return s_output;
}

/*
 * The DES function
 * input: 64 bit message
 * key: 64 bit key for encryption/decryption
 * mode: 'e' = encryption; 'd' = decryption
 */
void DESState_go(DESState *obj, char mode)
{
    uint64_t init_perm_res      = init_perm_func(obj->ctr);
    uint32_t L = (uint32_t) (init_perm_res >> 32) & L64_MASK;
    uint32_t R = (uint32_t) init_perm_res & L64_MASK;
    // DES rounds
    if (mode == 'e') {
        // Encryption
        for (int i = 0; i < 16; i++) {
            uint64_t s_input = des_expand(R) ^ obj->sub_key[i];
            uint32_t temp = R;
            R = L ^ des_spbox_perm(s_input);
            L = temp;
        }
    } else {
        // Decryption
        for (int i = 0; i < 16; i++) {
            uint64_t s_input = des_expand(R) ^ obj->sub_key[15-i];            
            uint32_t temp = R;
            R = L ^ des_spbox_perm(s_input);
            L = temp;
        }
    }
    // inverse initial permutation
    obj->out = inv_init_perm_func(L, R);
}



static inline uint64_t get_bits_raw(void *state)
{
    DESState *obj = state;
    DESState_go(obj, 'e');
    obj->ctr++;
    return obj->out;
}


static void *create(const CallerAPI *intf)
{
    DESState *obj = intf->malloc(sizeof(DESState));
    DESState_init(obj, intf->get_seed64() >> 8);
    return (void *) obj;
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
        char mode = (i % 2 == 0) ? 'e' : 'd';
        DESState_init(obj, result);
        obj->ctr = result;
        DESState_go(obj, mode);
        result = obj->out;
        intf->printf("%c: %016llx %016llx\n", mode, result, refval[i]);
        if (result != refval[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}

MAKE_UINT64_PRNG("DES", run_self_test)

    