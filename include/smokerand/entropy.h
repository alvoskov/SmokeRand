/**
 * @file entropy.h
 * @brief Seeds generator that uses hardware random number generator (RDSEED)
 * when possible. These values are balanced by means of XXTEA block cipher.
 * If no hardware RNG is accessible then the PRNG output is encrypted.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_ENTROPY_H
#define __SMOKERAND_ENTROPY_H
#include <stdint.h>
#include <stdio.h>

/**
 * @brief Entry for seeds logger.
 */
typedef struct {
    uint64_t thread_id;
    uint64_t seed;
} SeedLogEntry;

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
typedef struct {
    uint32_t key[4]; ///< XXTEA key
    uint64_t state; ///< Internal PRNG state
    SeedLogEntry *slog; ///< Log of returned seeds
    size_t slog_pos; ///< Current position inside the seeds log
    size_t slog_len; ///< Maximal length of the log
} Entropy;


uint64_t xxtea_encrypt(const uint64_t inp, const uint32_t *key);
int xxtea_test(void);
void Entropy_init(Entropy *obj);
void Entropy_free(Entropy *obj);
uint64_t Entropy_seed64(Entropy *obj, uint64_t thread_id);
void Entropy_print_seeds_log(const Entropy *obj, FILE *fp);
uint64_t cpuclock(void);

#endif
