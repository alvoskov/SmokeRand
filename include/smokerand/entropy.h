/**
 * @file entropy.h
 * @brief Generates seeds for PRNGs using the ChaCha20 based counter-based PRNG
 * seeded with external entropy sources. NOT FOR CRYPTOGRAPHY!
 * @details Seeds for ChaCha20 generator are taken from the system CSPRNG
 * such as `/dev/urandom/`. If it is unaccessible - then it will rely only on
 * its built-in entropy sources:
 *
 * - RDRAND instruction (if available)
 * - System time: `time`, `RDTSC`, higher-resolution system clock.
 * - System info: computer ID, process id etc.
 *
 * Even if all hardware sources of entropy except time are excluded --- it will
 * still return high-quality pseudorandom seeds.
 *
 * DON'T USE FOR THIS GENERATOR FOR CRYPTOGRAPHY, E.G. GENERATION OF KEYS FOR
 * ENCRYPTION! IT IS DESIGNED ONLY FOR STATISTCAL TESTS AND PRNG SEEDING!
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
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
 * @brief Contains the ChaCha20 state.
 * @details The next memory layout in 1D array is used:
 * 
 *     | 0   1  2  3 |
 *     | 4   5  6  7 |
 *     | 8   9 10 11 |
 *     | 12 13 14 15 |
 */
typedef struct {
    uint32_t x[16];   ///< Working state
    uint32_t out[16]; ///< Output state
    size_t pos; ///< Current position inside the output buffer
} ChaCha20State;

#define CHACHA20STATE_INITIALIZER { \
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0}, \
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0}, 0 \
}

void ChaCha20State_init(ChaCha20State *obj, const uint32_t *key);
void ChaCha20State_generate(ChaCha20State *obj);
uint32_t ChaCha20State_next32(ChaCha20State *obj);
uint64_t ChaCha20State_next64(ChaCha20State *obj);
int entfuncs_test(void);

/**
 * @brief Generates seeds for PRNGs using the ChaCha20 based counter-based PRNG
 * seeded with external entropy sources. NOT FOR CRYPTOGRAPHY!
 * @details Seeds for ChaCha20 generator are taken from the system CSPRNG
 * such as `/dev/urandom/`. If it is unaccessible - then it will rely only on
 * its built-in entropy sources:
 *
 * - RDRAND instruction (if available)
 * - System time: `time`, `RDTSC`, higher-resolution system clock.
 * - System info: computer ID, process id etc.
 *
 * Even if all hardware sources of entropy except time are excluded --- it will
 * still return high-quality pseudorandom seeds.
 *
 * DON'T USE FOR THIS GENERATOR FOR CRYPTOGRAPHY, E.G. GENERATION OF KEYS FOR
 * ENCRYPTION! IT IS DESIGNED ONLY FOR STATISTCAL TESTS AND PRNG SEEDING!
 */
typedef struct {
    ChaCha20State gen; ///< ChaCha20 generator state.
    SeedLogEntry *slog; ///< Log of returned seeds
    size_t slog_pos; ///< Current position inside the seeds log
    size_t slog_len; ///< Current length of the log
    size_t slog_maxlen; ///< Maximal length of the log
} Entropy;


#define ENTROPY_INITIALIZER { CHACHA20STATE_INITIALIZER, NULL, 0, 0, 0 }

static inline int Entropy_is_init(const Entropy *obj)
{
    return obj->gen.x[0] == 0x61707865 && obj->gen.x[1] == 0x3320646e &&
           obj->gen.x[2] == 0x79622d32 && obj->gen.x[3] == 0x6b206574;
}


void Entropy_init(Entropy *obj);
void Entropy_free(Entropy *obj);
uint64_t Entropy_seed64(Entropy *obj, uint64_t thread_id);
void Entropy_print_seeds_log(const Entropy *obj, FILE *fp);
uint64_t mix_rdseed(const uint64_t x);
uint64_t cpuclock(void);

#endif
