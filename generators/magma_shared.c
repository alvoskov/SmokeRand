// https://dl.acm.org/doi/10.1145/2388576.2388595
// https://meganorm.ru/Data2/1/4293732/4293732907.pdf

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t k[8];
} MagmaKey256;

typedef struct {
    uint32_t sbox8[4][256];
    MagmaKey256 key;
    uint64_t ctr;
} MagmaState;

static const unsigned char sbox4[8][16] = {
    {12, 4, 6, 2, 10, 5, 11, 9, 14, 8, 13, 7, 0, 3, 15, 1}, // 0
    {6, 8, 2, 3, 9, 10, 5, 12, 1, 14, 4, 7, 1, 13, 0, 15},
    {11, 3, 5, 8, 2, 15, 10, 13, 14, 1, 7, 4, 12, 9, 6, 0},
    {12, 8, 2, 1, 13, 4, 15, 6, 7, 0, 10, 5, 3, 14, 9, 11},
    {7, 15, 5, 10, 8, 1, 6, 13, 0, 9, 3, 14, 11, 4, 2, 12},
    {5, 13, 15, 6, 9, 2, 12, 10, 11, 7, 8, 1, 4, 3, 14, 0},
    {8, 14, 2, 5, 6, 9, 1, 12, 15, 4, 11, 0, 13, 10, 3, 7},
    {1, 7, 14, 13, 0, 5, 8, 3, 4, 15, 10, 6, 9, 12, 11, 2} // 7
};

void MagmaState_init(MagmaState *obj, const uint32_t *key)
{
    for (int i = 0; i < 4; i++) {
        for (int j1 = 0; j1 < 16; j1++) {
            for (int j2 = 0; j2 < 16; j2++) {
                int index = (j1 << 4) | j2;
                uint32_t s = (sbox4[2*i + 1][j1] << 4) | sbox4[2*i][j2];
                s <<= (8 * i);
                s = (s << 11) | (s >> 21);
                obj->sbox8[i][index] = s;
            }
        }
    }
    for (int i = 0; i < 8; i++) {
        obj->key.k[i] = key[i];
    }
    obj->ctr = 0;
}

static inline uint32_t gfunc(const MagmaState *obj, uint32_t k, uint32_t x)
{
    uint32_t y;
    x += k;
    y  = obj->sbox8[0][x & 0xFF]; //x >>= 8;
    y |= obj->sbox8[1][(x >> 8) & 0xFF]; //x >>= 8;
    y |= obj->sbox8[2][(x >> 16) & 0xFF]; //x >>= 8;
    y |= obj->sbox8[3][(x >> 24)];
    return y;
}

#define MAGMA_ROUND(ind) { t = a1 ^ gfunc(obj, a0, obj->key.k[ind]); a1 = a0; a0 = t; }

EXPORT uint64_t MagmaState_encrypt(const MagmaState *obj, uint64_t a)
{
    uint32_t a1 = a >> 32, a0 = (uint32_t) a;
    uint32_t t;
    for (int i = 0; i < 3; i++) {
        MAGMA_ROUND(0);  MAGMA_ROUND(1);  MAGMA_ROUND(2);  MAGMA_ROUND(3);
        MAGMA_ROUND(4);  MAGMA_ROUND(5);  MAGMA_ROUND(6);  MAGMA_ROUND(7);
    }
    MAGMA_ROUND(7);  MAGMA_ROUND(6);  MAGMA_ROUND(5);  MAGMA_ROUND(4);
    MAGMA_ROUND(3);  MAGMA_ROUND(2);  MAGMA_ROUND(1);  MAGMA_ROUND(0);
    return ( (uint64_t) a0 << 32) | a1;
}

static inline uint64_t get_bits_raw(void *state)
{
    MagmaState *obj = state;
    return MagmaState_encrypt(obj, obj->ctr++);
}

static void *create(const CallerAPI *intf)
{
    MagmaState *obj = intf->malloc(sizeof(MagmaState));
    uint32_t key[8];
    for (int i = 0; i < 4; i++) {
        uint64_t seed = intf->get_seed64();
        key[2*i] = seed >> 32;
        key[2*i + 1] = (uint32_t) seed;
    }
    MagmaState_init(obj, key);
    return (void *) obj;
}


/*
int main()
{

    fill_sbox8();
    printf("%X\n", tfunc(0xfdb97531));
    printf("%X\n", tfunc(0xb039bb3d));


    printf("%X\n", gfunc(0x87654321, 0xfedcba98)); // fdcbc20c,
    printf("%X\n", gfunc(0xc76549ec, 0x7e791a4b)); // 9791c849.


    uint32_t a1 = 0xfedcba98, a0 = 0x76543210;
    printf("%llX\n", MagmaKey256_block(&key, 0xfedcba9876543210));
    // 4ee901e5c2d8ca3d

f(fdb97531) = 2a196f34,
f(2a196f34) = ebd9f03a,
f(ebd9f03a) = b039bb3d,
f(b039bb3d) = 68695433.    
*/

static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t key[] = {
        0xffeeddcc, 0xbbaa9988, 0x77665544, 0x33221100,
        0xf0f1f2f3, 0xf4f5f6f7, 0xf8f9fafb, 0xfcfdfeff
    };
    static const uint64_t u_ref = 0x4ee901e5c2d8ca3d;
    MagmaState *obj = intf->malloc(sizeof(MagmaState));
    MagmaState_init(obj, key);
    obj->ctr = 0xfedcba9876543210;
    uint64_t u = get_bits_raw(obj);
    intf->printf("Out = 0x%llX; ref = 0x%llX", u, u_ref);
    intf->free(obj);
    return (u == u_ref);
};

MAKE_UINT64_PRNG("MAGMA-GOST89", run_self_test)
