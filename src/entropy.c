/**
 * @file entropy.c
 * @brief Seeds generator that uses hardware random number generator (RDSEED)
 * when possible. These values are balanced by means of XXTEA block cipher.
 * If no hardware RNG is accessible then the PRNG output is encrypted.
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/entropy.h"
#include <time.h>
#include <stdlib.h>

#ifdef NO_X86_EXTENSIONS
/**
 * @brief XORs input with output of hardware RNG in CPU (rdseed).
 */
static uint64_t mix_rdseed(const uint64_t x)
{
    return x;
}

uint64_t cpuclock(void)
{
    return 0;
}
#else
#if (defined(WIN32) || defined(WIN64)) && !defined(__MINGW32__) && !defined(__MINGW64__)
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#else
#include <x86intrin.h>
#endif
/**
 * @brief XORs input with output of hardware RNG in CPU (rdseed).
 */
static uint64_t mix_rdseed(const uint64_t x)
{
    long long unsigned int rd;
    while (!_rdseed64_step(&rd)) {}
    return x ^ rd;
}

uint64_t cpuclock(void)
{
    return __rdtsc();
}
#endif







static inline uint64_t ror64(uint64_t x, uint64_t r)
{
    return (x << r) | (x >> (64 - r));
}

/**
 * @brief XXTEA mixing function.
 */
#define MX(p) (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[((p)&3)^e] ^ z)))

/**
 * @brief XXTEA encryption subroutine for 64-bit block. Used for
 * generation of seeds from 64-bit state.
 */
uint64_t xxtea_encrypt(const uint64_t inp, const uint32_t *key)
{
    static const uint32_t DELTA = 0x9e3779b9;
    static const unsigned int nrounds = 32;
    uint32_t y, z, sum = 0;
    union {
        uint32_t v[2];
        uint64_t x;
    } out;
    out.x = inp;
    z = out.v[1];
    for (unsigned int i = 0; i < nrounds; i++) {
        sum += DELTA;
        unsigned int e = (sum >> 2) & 3;
        y = out.v[1]; 
        z = out.v[0] += MX(0);
        y = out.v[0];
        z = out.v[1] += MX(1);
    }
    return out.x;
}


/**
 * @brief rrmxmx hash from modified SplitMix PRNG.
 */
static uint64_t mix_hash(uint64_t z)
{
    static uint64_t const M = 0x9fb21c651e98df25ULL;    
    z ^= ror64(z, 49) ^ ror64(z, 24);
    z *= M;
    z ^= z >> 28;
    z *= M;
    return z ^ z >> 28;
}




/**
 * @brief Generates the next state and returns it in the hashed form.
 * It is essentially SplitMix PRNG that will be used as an input of XXTEA.
 */
static uint64_t Entropy_nextstate(Entropy *obj)
{
    obj->state += 0x9E3779B97F4A7C15;
    return mix_rdseed(mix_hash(obj->state));
}


void Entropy_init(Entropy *obj)
{
    uint64_t seed0 = mix_rdseed(mix_hash(time(NULL)));
    uint64_t seed1 = mix_rdseed(mix_hash(~seed0));
    seed1 ^= mix_rdseed(mix_hash(cpuclock()));
    obj->key[0] = (uint32_t) seed0; obj->key[1] = seed0 >> 32;
    obj->key[2] = (uint32_t) seed1; obj->key[3] = seed1 >> 32;
    obj->state = time(NULL);
    obj->slog_len = 1 << 20;
    obj->slog = calloc(obj->slog_len, sizeof(SeedLogEntry));
    obj->slog_pos = 0;
}

void Entropy_free(Entropy *obj)
{
    free(obj->slog);
    obj->slog = NULL;
}


/**
 * @brief Tests 64-bit XXTEA implementation correctness
 */
int xxtea_test()
{
    static const uint64_t OUT0 = 0x575d8c80053704ab;
    static const uint64_t OUT1 = 0xc4cc7f1cc007378c;
    uint32_t key[4] = {0, 0, 0, 0};
    // Test 1
    if (xxtea_encrypt(0, key) != OUT0) {
        //fprintf(stderr, "0: %llX %llX", xxtea_encrypt(0, key), OUT0);
        return 0;
    }
    // Test 2
    key[0] = 0x08040201; key[1] = 0x80402010;
    key[2] = 0xf8fcfeff; key[3] = 0x80c0e0f0; 
    if (xxtea_encrypt(0x80c0e0f0f8fcfeff, key) != OUT1) {
        //fprintf(stderr, "0: %llX %llX", xxtea_encrypt(0x80c0e0f0f8fcfeff, key), OUT1);
        return 0;
    }
    return 1;
}

/**
 * @brief Returns 64-bit random seed. Hardware RNG is used
 * when possible. WARNING! THIS FUNCTION IS NOT THREAD SAFE!
 */
uint64_t Entropy_seed64(Entropy *obj, uint64_t thread_id)
{
    uint64_t seed = xxtea_encrypt(Entropy_nextstate(obj), obj->key);
    if (obj->slog_pos != obj->slog_len - 1) {
        SeedLogEntry log_entry;
        log_entry.seed = seed;
        log_entry.thread_id = thread_id;
        obj->slog[obj->slog_pos++] = log_entry;
    }
    return seed;
}

void Entropy_print_seeds_log(const Entropy *obj, FILE *fp)
{
    fprintf(fp, "Number of seeds: %llu\n", (unsigned long long) obj->slog_pos);
    fprintf(fp, "%10s %18s %20s\n", "Thread", "Seed(HEX)", "Seed(DEC)");
    for (size_t i = 0; i < obj->slog_pos; i++) {
        fprintf(fp, "%10llu 0x%.16llX %.20llu\n",
            (unsigned long long) obj->slog[i].thread_id,
            (unsigned long long) obj->slog[i].seed,
            (unsigned long long) obj->slog[i].seed);
    }
}

