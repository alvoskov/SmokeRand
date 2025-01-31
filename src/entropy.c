/**
 * @file entropy.c
 * @brief Seeds generator based on XXTEA block cipher and TRNG from CPU.
 * @details It uses hardware random number generator (RDSEED) when possible.
 * The algorithm is based on XXTEA block cipher (with 64-bit blocks) that
 * processes an output from a simple 64-bit PRNG. If RDSEED instruction is
 * available then its output is injected by XORing with:
 *
 * - 64-bit PRNG (before encryption with XXTEA).
 * - 128-bit key for XXTEA.
 *
 * This seeds generator is resistant to the RDSEED failure. It uses
 * the next backup entropy sources:
 *
 * - RDTSC instruction (built-in CPU clocks)
 * - `time` function (from C99 standard).
 * - Current process ID.
 * - Tick count from the system clock.
 * - Machine ID (an attempt to make unique seeds even RDSEED and RDTSC
 *   are broken or unaccessible).
 *
 * DON'T USE IT FOR CRYPTOGRAPHY AND SECURITY PURPOSES! IT IS DESIGNED JUST
 * TO MAKE GOOD RANDOM SEEDS FOR PRNG EMPIRICAL TESTING!
 *
 * Examples of RDRAND failure on some CPUs:
 *
 * - https://github.com/systemd/systemd/issues/11810#issuecomment-489727505
 * - https://github.com/rust-random/getrandom/issues/228
 * - https://news.ycombinator.com/item?id=19848953
 *
 * @copyright (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif

#include "smokerand/entropy.h"
#include <time.h>
#include <stdlib.h>


static inline uint64_t ror64(uint64_t x, uint64_t r)
{
    return (x << r) | (x >> (64 - r));
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
 * @brief A simple non-cryptographic hash for ASCIIZ strings.
 * @details It is a modification of DJB2 algorithm.
 */
uint64_t str_hash(const char *str)
{
    uint64_t hash = 0xB7E151628AED2A6B;
    for (unsigned char c = *str; c != 0; c = *str++) {
        hash = (6906969069 * hash + 12345) ^ c;
    }
    return mix_hash(hash);
}

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
#include <windows.h>
#define WINDOWS_PLATFORM
#else
#include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////
///// Functions for collecting entropy from system information /////
////////////////////////////////////////////////////////////////////

/**
 * @brief Returns current process id.
 */
uint32_t get_current_process_id()
{
#ifdef WINDOWS_PLATFORM
    return (uint32_t) GetCurrentProcessId();
#else
    return (uint32_t) getpid();
#endif
}

/**
 * @brief Returns ticks count from a system clock.
 */
uint32_t get_tick_count()
{
#ifdef WINDOWS_PLATFORM
    return (uint32_t) GetTickCount();
#else
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (uint32_t) t.tv_nsec;
#endif
}

/**
 * @brief Returns a hash made from an unique machine ID.
 */
uint64_t get_machine_id()
{
    uint64_t machine_id = 0;
    char value[64];
#ifdef WINDOWS_PLATFORM
    DWORD size = 64, type = REG_SZ;
    HKEY key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography",
        0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
        return machine_id;
    }
    if (RegQueryValueExA(key, "MachineGuid",
        NULL, &type, (LPBYTE)value, &size) == ERROR_SUCCESS) {
        machine_id = str_hash(value);
    }
    //for (int i = 0; value[i] != 0; i++) {
    //    printf("%c", (unsigned char) value[i]);
    //}
    RegCloseKey(key);
#else
    // Try to find a file with machine ID
    FILE *fp = fopen("/etc/lib/dbus/machine-id", "r");
    if (fp == NULL) {
        fp = fopen("/etc/machine-id", "r");
        if (fp == NULL) {
            fclose(fp);
            return machine_id;
        }
    }
    // Read the line with machine ID
    machine_id = fgets(value, 64, fp) ? str_hash(value) : 0;
    fclose(fp);
#endif
    return machine_id;
}

////////////////////////////////////////////////
///// CPU architecture dependent functions /////
////////////////////////////////////////////////

#ifdef NO_X86_EXTENSIONS
/**
 * @brief XORs input with output of hardware RNG in CPU (rdseed).
 */
static uint64_t mix_rdseed(const uint64_t x)
{
    return x;
}

/**
 * @brief Emulation of RDTSC instruction: estimates number of CPU tics
 * from the program start using `clock` function and crude estimation
 * of CPU frequency as 3 GHz (it is rather close to it since ~2005).
 */
uint64_t cpuclock(void)
{
    const uint64_t default_freq = 3000000000; // 3 GHz
    const uint64_t clocks_to_tics = default_freq / CLOCKS_PER_SEC;
    uint64_t clk = (uint64_t) clock();
    return clk * clocks_to_tics;
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
    unsigned long long rd;
#if SIZE_MAX == UINT32_MAX
    // For 32-bit x86 executables
    unsigned int rd_hi, rd_lo;
    while (!_rdseed32_step(&rd_hi)) {}
    while (!_rdseed32_step(&rd_lo)) {}
    rd = (((unsigned long long) rd_hi) << 32) | rd_lo;
#else
    // For 64-bit x86-64 executables
    while (!_rdseed64_step(&rd)) {}
#endif
    return x ^ rd;
}

uint64_t cpuclock(void)
{
    return __rdtsc();
}
#endif

/////////////////////////////////////////////
///// XXTEA block cipher implementation /////
/////////////////////////////////////////////

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
    uint64_t mid = get_machine_id();
    uint64_t seed0 = mix_rdseed(mix_hash(time(NULL)) ^ mix_hash(get_tick_count()));
    uint64_t seed1 = mix_rdseed(mix_hash(get_current_process_id()));
    seed1 ^= mix_rdseed(mix_hash(cpuclock()));
    seed0 ^= mid; seed1 ^= mid;
    obj->key[0] = (uint32_t) seed0; obj->key[1] = seed0 >> 32;
    obj->key[2] = (uint32_t) seed1; obj->key[3] = seed1 >> 32;
    obj->state = mix_hash(time(NULL) ^ mid);
    obj->slog_len = 1 << 20;
    obj->slog = calloc(obj->slog_len, sizeof(SeedLogEntry));
    obj->slog_pos = 0;
    fprintf(stderr, "Entropy pool (seed generator) initialized\n");
    fprintf(stderr, "  machine_id: 0x%16.16llX\n", (unsigned long long) mid);
    fprintf(stderr, "  seed0:      0x%16.16llX\n", (unsigned long long) seed0);
    fprintf(stderr, "  seed1:      0x%16.16llX\n", (unsigned long long) seed1);
    fprintf(stderr, "  state:      0x%16.16llX\n", (unsigned long long) obj->state);
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

