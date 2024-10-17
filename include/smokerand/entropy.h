#ifndef __SMOKERAND_ENTROPY_H
#define __SMOKERAND_ENTROPY_H
#include <stdint.h>
/**
 * @brief Generates seeds for PRNGs using some entropy sources: current time
 * in seconds, RDTSC instruction and RDSEED built-in hardware RNG. Everything
 * is mixed with PRNG counter and encrypted by XXTEA block cipher.
 *
 * DON'T USE FOR THIS CLASS FOR CRYPTOGRAPHY, E.G. GENERATION OF KEYS FOR
 * ENCRYPTION! IT IS DESIGNED ONLY FOR STATISTCAL TESTS AND PRNG SEEDING!
 *
 * @details. It uses the next algorithm of generation of seeds:
 *
 * - RND(x) function is defined as RND(SplitMixHash(x) ^ RDSEED) where
 *   RDSEED is RDSEED instruction, hardware RNG in CPU.
 * - Internal counter CTR is "Weyl sequence" from SplitMix.
 * - Output function is XXTEA(RND(CTR)) where XXTEA is block cipher with
 *   64-bit block and 128-bit key.
 * - XXTEA keys are made as RND(time(NULL)) and RND(~time(NULL)) ^ RND(RDTSC)
 *
 * XORing with RDSEED can be excluded if CPU doesn't support that instruction.
 * RDTSC can be excluded if CPU doesn't support this instruction. Even if all
 * hardware sources of entropy except time are excluded --- it will return
 * high-quality pseudorandom seeds.
 *
 * Usage of XXTEA over RDSEED is also intended to exclude any biases.
 */
typedef struct
{
    uint32_t key[4]; ///< XXTEA key
    uint64_t state; ///< Internal PRNG state
} Entropy;


uint64_t xxtea_encrypt(const uint64_t inp, const uint32_t *key);
int xxtea_test();
void Entropy_init(Entropy *obj);
uint64_t Entropy_seed64(Entropy *obj);
uint64_t cpuclock(void);

#endif
