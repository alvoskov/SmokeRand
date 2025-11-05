/**
 * @file blake2s.c
 * @brief A simple blake2s Reference Implementation.
 * @details https://datatracker.ietf.org/doc/html/rfc7693.html
 */

#include "smokerand/blake2s.h"
#include "smokerand/coredefs.h"

// Little-endian byte access.
static uint32_t b2s_get32(const void *src)
{
    const uint8_t *p = (const uint8_t *) src;
    return ((uint32_t) (p[0]) ^
           ((uint32_t) (p[1]) << 8) ^
           ((uint32_t) (p[2]) << 16) ^
           ((uint32_t) (p[3]) << 24));
}


static inline void b2s_g(uint32_t *v,
    int a, int b, int c, int d,
    uint32_t x, uint32_t y)
{
    v[a] = v[a] + v[b] + x;
    v[d] = rotr32(v[d] ^ v[a], 16);
    v[c] = v[c] + v[d];
    v[b] = rotr32(v[b] ^ v[c], 12);
    v[a] = v[a] + v[b] + y;
    v[d] = rotr32(v[d] ^ v[a], 8);
    v[c] = v[c] + v[d];
    v[b] = rotr32(v[b] ^ v[c], 7);
}


static inline void b2s_round(uint32_t *v, const uint32_t *m, int i)
{
    static const uint8_t sigma[10][16] = {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
        { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
        { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
        { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
        { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
        { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
        { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
        { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
        { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
        { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 }
    };
    b2s_g(v,  0, 4,  8, 12, m[sigma[i][ 0]], m[sigma[i][ 1]]);
    b2s_g(v,  1, 5,  9, 13, m[sigma[i][ 2]], m[sigma[i][ 3]]);
    b2s_g(v,  2, 6, 10, 14, m[sigma[i][ 4]], m[sigma[i][ 5]]);
    b2s_g(v,  3, 7, 11, 15, m[sigma[i][ 6]], m[sigma[i][ 7]]);
    b2s_g(v,  0, 5, 10, 15, m[sigma[i][ 8]], m[sigma[i][ 9]]);
    b2s_g(v,  1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
    b2s_g(v,  2, 7,  8, 13, m[sigma[i][12]], m[sigma[i][13]]);
    b2s_g(v,  3, 4,  9, 14, m[sigma[i][14]], m[sigma[i][15]]);
}

/**
 * @brief Initialization Vector.
 */
static const uint32_t blake2s_iv[8] =
{
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

/**
 * @brief Compression function. "last" flag indicates last block.
 */
static void blake2s_compress(blake2s_ctx *ctx, int last)
{
    uint32_t v[16], m[16];

    for (int i = 0; i < 8; i++) {       // init work variables
        v[i] = ctx->h[i];
        v[i + 8] = blake2s_iv[i];
    }

    v[12] ^= ctx->t[0];                 // low 32 bits of offset
    v[13] ^= ctx->t[1];                 // high 32 bits
    if (last)                           // last block flag set ?
        v[14] = ~v[14];


    for (int i = 0; i < 16; i++)        // get little-endian words
        m[i] = b2s_get32(&ctx->b[4 * i]);

    for (int i = 0; i < 10; i++)        // ten rounds
        b2s_round(v, m, i);
        
    for (int i = 0; i < 8; i++)
        ctx->h[i] ^= v[i] ^ v[i + 8];
}

/**
 * @brief Initialize the hashing context "ctx" with optional key "key".
 * 1 <= outlen <= 32 gives the digest size in bytes.
 * Secret key (also <= 32 bytes) is optional (keylen = 0).
 */
int blake2s_init(blake2s_ctx *ctx, size_t outlen,
    const void *key, size_t keylen)     // (keylen=0: no key)
{
    if (outlen == 0 || outlen > 32 || keylen > 32)
        return -1;                      // illegal parameters

    for (size_t i = 0; i < 8; i++)      // state, "param block"
        ctx->h[i] = blake2s_iv[i];
    ctx->h[0] ^= 0x01010000 ^ ((uint32_t) keylen << 8) ^ (uint32_t) outlen;

    ctx->t[0] = 0;                      // input count low word
    ctx->t[1] = 0;                      // input count high word
    ctx->c = 0;                         // pointer within buffer
    ctx->outlen = outlen;

    for (size_t i = keylen; i < 64; i++) // zero input block
        ctx->b[i] = 0;
    if (keylen > 0) {
        blake2s_update(ctx, key, keylen);
        ctx->c = 64;                    // at the end
    }

    return 0;
}


/**
 * @brief Add "inlen" bytes from "in" into the hash.
 */
void blake2s_update(blake2s_ctx *ctx,
    const void *in, size_t inlen)       // data bytes
{
    for (size_t i = 0; i < inlen; i++) {
        if (ctx->c == 64) {                  // buffer full ?
            ctx->t[0] += (uint32_t) ctx->c;  // add counters
            if (ctx->t[0] < ctx->c)     // carry overflow ?
                ctx->t[1]++;            // high word
            blake2s_compress(ctx, 0);   // compress (not last)
            ctx->c = 0;                 // counter to zero
        }
        ctx->b[ctx->c++] = ((const uint8_t *) in)[i];
    }
}

/**
 * @brief Generate the message digest (size given in init).
 * Result placed in "out".
 */
void blake2s_final(blake2s_ctx *ctx, void *out)
{
    ctx->t[0] += (uint32_t) ctx->c;     // mark last block offset
    if (ctx->t[0] < ctx->c)             // carry overflow
        ctx->t[1]++;                    // high word

    while (ctx->c < 64)                 // fill up with zeros
        ctx->b[ctx->c++] = 0;
    blake2s_compress(ctx, 1);           // final block flag = 1

    // little endian convert and store
    for (size_t i = 0; i < ctx->outlen; i++) {
        ((uint8_t *) out)[i] =
            (ctx->h[i >> 2] >> (8 * (i & 3))) & 0xFF;
    }
}

/**
 * @brief Convenience function for all-in-one computation.
 */
int blake2s(void *out, size_t outlen,
    const void *key, size_t keylen,
    const void *in, size_t inlen)
{
    blake2s_ctx ctx;
    if (blake2s_init(&ctx, outlen, key, keylen))
        return -1;
    blake2s_update(&ctx, in, inlen);
    blake2s_final(&ctx, out);
    return 0;
}

