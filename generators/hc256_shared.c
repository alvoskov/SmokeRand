/**
 * @file hc256_shared.c
 * @brief HC256 cipher based 32-bit CSPRNG.
 * @details References:
 *
 * 1. Wu, H. (2004). A New Stream Cipher HC-256. In: Roy, B., Meier, W. (eds)
 *    Fast Software Encryption. FSE 2004. Lecture Notes in Computer Science,
 *    vol 3017. Springer, Berlin, Heidelberg.
 *    https://doi.org/10.1007/978-3-540-25937-4_15
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief HC256 CSPRNG state.
 */
typedef struct {
    uint32_t P[1024], Q[1024];
    uint32_t X[16], Y[16];
    uint32_t out[16]; ///< Encrypted data (and output buffer)
    uint32_t ctr; ///< Internal counter: i mod 2048
    uint32_t pos; ///< Position in output buffer
} Hc256State;

#define rotr(x,n) (((x)>>(n))|((x)<<(32-(n))))

#define hx(V,x,y) { \
    uint8_t a = (uint8_t) (x); \
    uint8_t b = (uint8_t) ((x) >> 8); \
    uint8_t c = (uint8_t) ((x) >> 16); \
    uint8_t d = (uint8_t) ((x) >> 24); \
    (y) = V[a] + V[256 + b] + V[512+c] + V[768+d]; \
}

#define step_AB(V,u,v,a,b,c,d,m) { \
    uint32_t tem0, tem1, tem2, tem3; \
    tem0 = rotr((v), 23); \
    tem1 = rotr((c), 10); \
    tem2 = ((v) ^ (c)) & 0x3ff; \
    (u) += (b) + (tem0 ^ tem1) + V[tem2]; \
    (a) = (u); \
    hx(V,(d),tem3); \
    (m) ^= tem3 ^ (u) ; \
}

#define step_A(u,v,a,b,c,d,m) step_AB(Q,u,v,a,b,c,d,m)
#define step_B(u,v,a,b,c,d,m) step_AB(P,u,v,a,b,c,d,m)

//each time it encrypts 512-bit data
void Hc256State_encrypt(Hc256State *obj)
{
    uint32_t cc = obj->ctr & 0x3FF;
    uint32_t dd = (cc + 16) & 0x3FF;
    uint32_t *P = obj->P, *X = obj->X, *data = obj->out;
    uint32_t *Q = obj->Q, *Y = obj->Y;
    if (obj->ctr < 1024) {
        obj->ctr = (obj->ctr + 16) & 0x7ff;
        step_A(P[cc+0], P[cc+1], X[0], X[6], X[13],X[4], data[0]);
        step_A(P[cc+1], P[cc+2], X[1], X[7], X[14],X[5], data[1]);
        step_A(P[cc+2], P[cc+3], X[2], X[8], X[15],X[6], data[2]);
        step_A(P[cc+3], P[cc+4], X[3], X[9], X[0], X[7], data[3]);
        step_A(P[cc+4], P[cc+5], X[4], X[10],X[1], X[8], data[4]);
        step_A(P[cc+5], P[cc+6], X[5], X[11],X[2], X[9], data[5]);
        step_A(P[cc+6], P[cc+7], X[6], X[12],X[3], X[10],data[6]);
        step_A(P[cc+7], P[cc+8], X[7], X[13],X[4], X[11],data[7]);
        step_A(P[cc+8], P[cc+9], X[8], X[14],X[5], X[12],data[8]);
        step_A(P[cc+9], P[cc+10],X[9], X[15],X[6], X[13],data[9]);
        step_A(P[cc+10],P[cc+11],X[10],X[0], X[7], X[14],data[10]);
        step_A(P[cc+11],P[cc+12],X[11],X[1], X[8], X[15],data[11]);
        step_A(P[cc+12],P[cc+13],X[12],X[2], X[9], X[0], data[12]);
        step_A(P[cc+13],P[cc+14],X[13],X[3], X[10],X[1], data[13]);
        step_A(P[cc+14],P[cc+15],X[14],X[4], X[11],X[2], data[14]);
        step_A(P[cc+15],P[dd+0], X[15],X[5], X[12],X[3], data[15]);
    } else {
        obj->ctr = (obj->ctr + 16) & 0x7ff;
        step_B(Q[cc+0], Q[cc+1], Y[0], Y[6], Y[13],Y[4], data[0]);
        step_B(Q[cc+1], Q[cc+2], Y[1], Y[7], Y[14],Y[5], data[1]);
        step_B(Q[cc+2], Q[cc+3], Y[2], Y[8], Y[15],Y[6], data[2]);
        step_B(Q[cc+3], Q[cc+4], Y[3], Y[9], Y[0], Y[7], data[3]);
        step_B(Q[cc+4], Q[cc+5], Y[4], Y[10],Y[1], Y[8], data[4]);
        step_B(Q[cc+5], Q[cc+6], Y[5], Y[11],Y[2], Y[9], data[5]);
        step_B(Q[cc+6], Q[cc+7], Y[6], Y[12],Y[3], Y[10],data[6]);
        step_B(Q[cc+7], Q[cc+8], Y[7], Y[13],Y[4], Y[11],data[7]);
        step_B(Q[cc+8], Q[cc+9], Y[8], Y[14],Y[5], Y[12],data[8]);
        step_B(Q[cc+9], Q[cc+10],Y[9], Y[15],Y[6], Y[13],data[9]);
        step_B(Q[cc+10],Q[cc+11],Y[10],Y[0], Y[7], Y[14],data[10]);
        step_B(Q[cc+11],Q[cc+12],Y[11],Y[1], Y[8], Y[15],data[11]);
        step_B(Q[cc+12],Q[cc+13],Y[12],Y[2], Y[9], Y[0], data[12]);
        step_B(Q[cc+13],Q[cc+14],Y[13],Y[3], Y[10],Y[1], data[13]);
        step_B(Q[cc+14],Q[cc+15],Y[14],Y[4], Y[11],Y[2], data[14]);
        step_B(Q[cc+15],Q[dd+0], Y[15],Y[5], Y[12],Y[3], data[15]);
    }
}

//The following defines the initialization functions
#define f1(x) (rotr((x),7) ^ rotr((x),18) ^ ((x) >> 3))
#define f2(x) (rotr((x),17) ^ rotr((x),19) ^ ((x) >> 10))
#define f(a,b,c,d) (f2((a)) + (b) + f1((c)) + (d))
#define feedback_12(V,u,v,b,c) { \
    uint32_t tem0 = rotr((v),23), tem1 = rotr((c),10); \
    uint32_t tem2 = ((v) ^ (c)) & 0x3ff; \
    (u) += (b) + (tem0 ^ tem1) + V[tem2]; \
}
#define feedback_1(u,v,b,c) feedback_12(Q,u,v,b,c)
#define feedback_2(u,v,b,c) feedback_12(P,u,v,b,c)

void Hc256State_init(Hc256State *obj,
    const uint32_t key[], const uint32_t iv[])
{
    uint32_t *P = obj->P, *X = obj->X;
    uint32_t *Q = obj->Q, *Y = obj->Y;
    //expand the key and iv into P and Q
    for (int i = 0; i < 8; i++) P[i] = key[i];
    for (int i = 8; i < 16; i++) P[i] = iv[i-8];
    for (int i = 16; i < 528; i++)
        P[i] = f(P[i-2], P[i-7], P[i-15], P[i-16])+i;
    for (int i = 0; i < 16; i++)
        P[i] = P[i+512];
    for (int i = 16; i < 1024; i++)
        P[i] = f(P[i-2], P[i-7], P[i-15], P[i-16]) + 512 + i;
    for (int i = 0; i < 16; i++)
        Q[i] = P[1024-16+i];
    for (int i = 16; i < 32; i++)
        Q[i] = f(Q[i-2], Q[i-7], Q[i-15], Q[i-16]) + 1520 + i;
    for (int i = 0; i < 16; i++)
        Q[i] = Q[i+16];
    for (int i = 16; i < 1024;i++)
        Q[i] = f(Q[i-2], Q[i-7], Q[i-15], Q[i-16]) + 1536 + i;
    //run the cipher 4096 steps without generating output
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 10; j++)
            feedback_1(P[j], P[j+1], P[(j-10) & 0x3ff], P[(j-3)&0x3ff]);
        for (int j = 10; j < 1023; j++)
            feedback_1(P[j], P[j+1], P[j-10],P[j-3]);
        feedback_1(P[1023],P[0],P[1013],P[1020]);
        for (int j = 0; j < 10; j++)
            feedback_2(Q[j],Q[j+1],Q[(j-10)&0x3ff],Q[(j-3)&0x3ff]);
        for (int j = 10; j < 1023; j++)
            feedback_2(Q[j],Q[j+1],Q[j-10],Q[j-3]);
        feedback_2(Q[1023],Q[0],Q[1013],Q[1020]);
    }
    //initialize counter2048, and tables X and Y
    obj->ctr = 0;
    for (int i = 0; i < 16; i++) X[i] = P[1008+i];
    for (int i = 0; i < 16; i++) Y[i] = Q[1008+i];
    // Output buffer position
    for (int i = 0; i < 16; i++) obj->out[i] = 0;
    obj->pos = 16;
}



static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t key[8] = {0x55, 0, 0, 0, 0, 0, 0, 0};
    static const uint32_t iv[8]  = {0, 0, 0, 0, 0, 0, 0, 0};
    static const uint32_t x_ref[16] = {
        0xfe4a401c, 0xed5fe24f, 0xd19a8f95, 0x6fc036ae,
        0x3c5aa688, 0x23e2abc0, 0x2f90b3ae, 0xa8d30e42,
        0x59f03a6c, 0x6e39eb44, 0x8f7579fb, 0x70137a5e,
        0x6d10b7d8, 0xadd0f7cd, 0x723423da, 0xf575dde6};
    int is_ok = 1;
    Hc256State *obj = intf->malloc(sizeof(Hc256State));
    Hc256State_init(obj, key, iv);
    Hc256State_encrypt(obj);
    intf->printf("%10s %10s\n", "Out.", "Ref.");
    for (int i = 0; i < 16; i++) {        
        intf->printf("%10.8X %10.8X\n", obj->out[i], x_ref[i]);
        if (obj->out[i] != x_ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}

static inline uint64_t get_bits_raw(void *state)
{
    Hc256State *obj = state;
    if (obj->pos == 16) {
        Hc256State_encrypt(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

static void *create(const CallerAPI *intf)
{
    uint32_t key[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t iv[8]  = {0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 4; i++) {
        uint64_t seed = intf->get_seed64();
        key[2*i] = seed & 0xFFFFFFFF;
        key[2*i + 1] = seed >> 32;
    }
    Hc256State *obj = intf->malloc(sizeof(Hc256State));
    Hc256State_init(obj, key, iv);
    return obj;
}

MAKE_UINT32_PRNG("HC256", run_self_test)
