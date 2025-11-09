/**
 * @file blake2s.h
 * @brief BLAKE2s Hashing Context and API Prototypes
 */
#ifndef __SMOKERAND_BLAKE2S_H
#define __SMOKERAND_BLAKE2S_H

#include <stdint.h>
#include <stddef.h>

enum blake2s_constant
{
    BLAKE2S_BLOCKBYTES = 64,
    BLAKE2S_KEYBYTES   = 32,
    BLAKE2S_OUTBYTES   = 32,
};

/**
 * @brief Blake2s state
 */
typedef struct {
    uint8_t buf[BLAKE2S_BLOCKBYTES]; ///< Input buffer
    uint32_t h[BLAKE2S_OUTBYTES / 4];  ///< Chained state
    uint32_t t[2];  ///< Total number of bytes
    size_t c;       ///< Pointer for b[]
    size_t outlen;  ///< Digest size
} blake2s_state;

/** 
 * @brief Initialize the hashing context "ctx" with optional key "key".
 *   1 <= outlen <= 32 gives the digest size in bytes.
 */
int blake2s_init(blake2s_state *ctx, size_t outlen,
    const void *key, size_t keylen);

/**
 * @brief Add "inlen" bytes from "in" into the hash.
 * @param ctx    Context
 * @param in     Pointer to data to be hashed
 * @param inlen  Data size to be hashed, bytes
 */
void blake2s_update(blake2s_state *ctx, const void *in, size_t inlen);

/**
 * @brief Generate the message digest (size given in init).
 * @param ctx  State context
 * @param out  Output buffer (result will be placed in it)
 */
void blake2s_final(blake2s_state *ctx, void *out);

/**
 * @brief All-in-one convenience function.
 * @param out  Output buffer for the digest
 * @param in   Data to be hashed
 */
int blake2s(void *out, size_t outlen,
    const void *key, size_t keylen,
    const void *in, size_t inlen);

static inline int blake2s_256(void *out, const void *in, size_t inlen)
{
    return blake2s(out, 32, NULL, 0, in, inlen);
}

static inline int blake2s_128(void *out, const void *in, size_t inlen)
{
    return blake2s(out, 16, NULL, 0, in, inlen);
}

int blake2s_self_test(void);

#endif // __SMOKERAND_BLAKE2S_H
