/**
 * @file entropy.c
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
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif

#include "smokerand/entropy.h"
#include "smokerand/blake2s.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>


static inline uint32_t rotl32(uint32_t x, int r)
{
    return (x << r) | (x >> ((-r) & 31));
}


static inline uint64_t ror64(uint64_t x, uint64_t r)
{
    return (x << (64 - r)) | (x >> r);
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

//////////////////////////////////////////////
///// ChaCha20State class implementation /////
//////////////////////////////////////////////

static inline void chacha_qround(uint32_t *x, size_t ai, size_t bi, size_t ci, size_t di)
{
    x[ai] += x[bi]; x[di] ^= x[ai]; x[di] = rotl32(x[di], 16);
    x[ci] += x[di]; x[bi] ^= x[ci]; x[bi] = rotl32(x[bi], 12);
    x[ai] += x[bi]; x[di] ^= x[ai]; x[di] = rotl32(x[di], 8);
    x[ci] += x[di]; x[bi] ^= x[ci]; x[bi] = rotl32(x[bi], 7);
}

void ChaCha20State_init(ChaCha20State *obj, const uint32_t *key)
{
    // Constants: the upper row of the matrix
    obj->x[0] = 0x61707865; obj->x[1] = 0x3320646e;
    obj->x[2] = 0x79622d32; obj->x[3] = 0x6b206574;
    // Rows 1-2: key (seed)
    for (size_t i = 0; i < 8; i++) {
        obj->x[i + 4] = key[i];
    }
    // Row 3: counter and nonce
    for (size_t i = 12; i <= 15; i++) {
        obj->x[i] = 0;
    }
    // Output counter
    obj->pos = 16;
}

void ChaCha20State_generate(ChaCha20State *obj)
{
    for (size_t k = 0; k < 16; k++) {
        obj->out[k] = obj->x[k];
    }
    for (size_t k = 0; k < 10; k++) {
        // Vertical qrounds
        chacha_qround(obj->out, 0, 4, 8,12);
        chacha_qround(obj->out, 1, 5, 9,13);
        chacha_qround(obj->out, 2, 6,10,14);
        chacha_qround(obj->out, 3, 7,11,15);
        // Diagonal qrounds
        chacha_qround(obj->out, 0, 5,10,15);
        chacha_qround(obj->out, 1, 6,11,12);
        chacha_qround(obj->out, 2, 7, 8,13);
        chacha_qround(obj->out, 3, 4, 9,14);
    }
    for (size_t i = 0; i < 16; i++) {
        obj->out[i] += obj->x[i];
    }
}

static void ChaCha20State_inc_counter(ChaCha20State *obj)
{
    if (++obj->x[12] == 0) {
        obj->x[13]++;
    }
}

static void ChaCha20State_set_nonce(ChaCha20State *obj, uint64_t nonce)
{
    obj->x[14] = (uint32_t) nonce;
    obj->x[15] = (uint32_t) (nonce >> 32);
}

uint32_t ChaCha20State_next32(ChaCha20State *obj)
{
    if (obj->pos >= 16) {
        ChaCha20State_inc_counter(obj);
        ChaCha20State_generate(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

uint64_t ChaCha20State_next64(ChaCha20State *obj)
{
    uint64_t lo = ChaCha20State_next32(obj);
    uint64_t hi = ChaCha20State_next32(obj);
    return (hi << 32) | lo;
}


int blake2s_test(void)
{
    static const char in[] = "The quick brown fox jumps over the lazy dog|"
        "The quick brown fox jumps over the lazy dog";
    static const uint8_t ref[32] = {
        0xad, 0xd6, 0x41, 0x6a,  0x0c, 0x13, 0xa8, 0x35,
        0xf0, 0xba, 0xe3, 0x53,  0x69, 0x8b, 0x1c, 0x02,
        0x52, 0x70, 0x34, 0xaa,  0xd4, 0x60, 0x08, 0x5d,
        0x02, 0x74, 0x5b, 0x78,  0xc9, 0x65, 0x92, 0x84
    };
    uint8_t out[32];    
    blake2s(out, 32, NULL, 0, in, strlen(in));
    int is_ok = 1;
    for (size_t i = 0; i < 32; i++) {
        if (out[i] != ref[i])
            is_ok = 0;
    }
    return is_ok;
}


int chacha20_test(void)
{
    static const uint32_t x_init[] = { // Input values
        0x03020100,  0x07060504,  0x0b0a0908,  0x0f0e0d0c,
        0x13121110,  0x17161514,  0x1b1a1918,  0x1f1e1d1c,
        0x00000001,  0x09000000,  0x4a000000,  0x00000000
    };
    static const uint32_t out_final[] = { // Refernce values from RFC 7359
       0xe4e7f110,  0x15593bd1,  0x1fdd0f50,  0xc47120a3,
       0xc7f4d1c7,  0x0368c033,  0x9aaa2204,  0x4e6cd4c3,
       0x466482d2,  0x09aa9f07,  0x05d7c214,  0xa2028bd9,
       0xd19c12b5,  0xb94e16de,  0xe883d0cb,  0x4e3c50a2
    };
    ChaCha20State obj;
    ChaCha20State_init(&obj, x_init);
    for (size_t i = 0; i < 4; i++) {
        obj.x[i + 12] = x_init[i + 8];
    }
    ChaCha20State_generate(&obj);
    for (int i = 0; i < 16; i++) {
        if (out_final[i] != obj.out[i]) {
            return 0;
        }
    }
    return 1;
}


int entfuncs_test(void)
{
    return chacha20_test() && blake2s_test();
}

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
    #include <windows.h>
    #define WINDOWS_PLATFORM
#else
    #include <fcntl.h>
    #include <unistd.h>
#endif


#if defined(__WATCOMC__) && defined(__386__) && defined(__DOS__)
    #define DOS386_PLATFORM
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
#elif !defined(NO_POSIX)
    return (uint32_t) getpid();
#else
    return 0;
#endif
}

/**
 * @brief Returns ticks count from a system clock.
 */
uint32_t get_tick_count()
{
#ifdef WINDOWS_PLATFORM
    return (uint32_t) GetTickCount();
#elif !defined(NO_POSIX)
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (uint32_t) t.tv_nsec;
#else
    return (uint32_t) (time(NULL) * 69069);
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
    // Windows: get machine GUID from registry
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
#elif defined(DOS386_PLATFORM)
    // DOS: calculate ROM BIOS checksum
    uint64_t *bios_data = (uint64_t *) 0xF0000;
    for (int i = 0; i < 8192; i++) {
        machine_id = (6906969069 * machine_id + 12345) ^ bios_data[i];
    }
#else
    // UNIX: try to find a file with machine ID
    FILE *fp = fopen("/etc/lib/dbus/machine-id", "r");
    if (fp == NULL) {
        fp = fopen("/etc/machine-id", "r");
        if (fp == NULL) {
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
uint64_t mix_rdseed(const uint64_t x)
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
#if defined(__WATCOMC__)
uint64_t __rdtsc(void);
#pragma aux __rdtsc = " .586 " \
    "rdtsc " value [eax edx];
#elif (defined(WIN32) || defined(WIN64)) && !defined(__MINGW32__) && !defined(__MINGW64__)
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#else
#include <x86intrin.h>
#endif
/**
 * @brief XORs input with output of hardware RNG in CPU (rdseed).
 * @details It is good but not very reliable source of entropy. Examples of
 * RDRAND failure on some CPUs:
 *
 * - https://github.com/systemd/systemd/issues/11810#issuecomment-489727505
 * - https://github.com/rust-random/getrandom/issues/228
 * - https://news.ycombinator.com/item?id=19848953
 */
uint64_t mix_rdseed(const uint64_t x)
{
    unsigned long long rd;
#if !defined(__RDSEED__)
    rd = 0;
#elif SIZE_MAX == UINT32_MAX
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


#ifdef DOS386_PLATFORM

/**
 * @brief Calculate number of bits required to represent the input value
 * without leading zeros.
 */
static int dos386_get_nbits(uint64_t value)
{
    int nbits = 0;
    while (value != 0) {
        value >>= 1;
        nbits++;
    }
    return nbits;
}


/**
 * @brief Extracts entropy bits from an interaction of RDTSC instruction
 * and PIT (Programmable Interval Timer). Used as a fallback for DOS32
 * platform where the system-level CSPRNG such as /dev/urandom is not
 * implemented.
 * @param      fp   File descriptor (e.g. stderr) for diagnostic output.
 * @param[out] out  Output buffer.
 * @param[in]  len  Size of output buffer in 64-bit words.
 */
static int dos386_random_rdtsc(FILE *fp, uint64_t *out, size_t len)
{
    volatile uint32_t *bios_timer = (uint32_t *) 0x46C;
    uint64_t rdtsc_prev = __rdtsc(), delta_prev = 0;
    uint64_t accum = *bios_timer;
    for (size_t i = 0; i < len; i++) {
        int nbits_total = 0;
        for (int j = 0; j < 64 && nbits_total < 128; j++) {
            // Wait for the next tick of the PIT
            for (uint32_t t = *bios_timer; t == *bios_timer; ) {
                for (uint64_t ct = __rdtsc(); ct % 1021 == 0; ct = __rdtsc()) {
                }
            }
            // Extract random bits as "delta of the delta"
            uint64_t rdtsc_cur =  __rdtsc();
            uint64_t delta_cur = rdtsc_cur - rdtsc_prev;
            uint64_t rndbits;
            if (delta_cur > delta_prev) {
                rndbits = delta_cur - delta_prev;
            } else {
                rndbits = delta_prev - delta_cur;
            }
            rdtsc_prev = rdtsc_cur;
            delta_prev = delta_cur;
            // Add extracted bits to the accumulator
            int nbits = dos386_get_nbits(rndbits);
            nbits_total += nbits;
            if (nbits != 0) {
                accum = (accum << nbits) | (accum >> (64 - nbits));
                accum ^= rndbits;
            }
            // Diagnostic output (also slightly increases entropy)
            fprintf(fp, "\rCollecting entropy: %d of %d bits collected  ",
                (int) (i * 64 + nbits_total / 2), (int) (64 * len));
            fflush(fp);
        }
        out[i] = accum;
    }
    fprintf(fp, "\n");
    return 1;
}

#endif

/**
 * @brief Gains access to the system CSPRNG and XORs content of the output
 * buffer with output of this generator.
 * @param[out] out  Output buffer.
 * @param[in]  len  Size of output buffer in 64-bit words.
 * @return 0/1 - failure/success.
 */
static int inject_rand(uint64_t *out, size_t len)
{
#ifdef WINDOWS_PLATFORM
    HCRYPTPROV hCryptProv;
    DWORD nbytes = sizeof(uint64_t) * len;
    // Acquire a cryptographic provider context
    // CRYPT_VERIFYCONTEXT for a temporary context
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL,
        PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return 0;
    }
    // Generate random bytes
    uint64_t *buf = malloc(nbytes);
    if (buf == NULL || !CryptGenRandom(hCryptProv, nbytes, (BYTE *) buf)) {
        CryptReleaseContext(hCryptProv, 0);
        free(buf);
        return 0;
    }
    // Inject the bytes into the output buffer with XOR
    for (size_t i = 0; i < len; i++) {
        //printf("%llX\n", (unsigned long long) buf[i]);
        out[i] ^= buf[i];
    }
    free(buf);
    // Release the cryptographic provider context
    CryptReleaseContext(hCryptProv, 0);
    return 1;
#elif !defined(NO_POSIX)
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        return 0;
    }

    size_t nbytes = sizeof(uint64_t) * len;
    uint64_t *buf = malloc(nbytes);

    if (buf == NULL || read(fd, buf, nbytes) != (ssize_t) nbytes) {
        close(fd);
        free(buf);
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        //printf("-->%llX\n", (unsigned long long) buf[i]);
        out[i] ^= buf[i];
    }
    free(buf);
    close(fd);
    return 1;
#else
    (void) out; (void) len;
    return 0;
#endif
}


typedef union {
    uint64_t u64[16];
    uint8_t u8[128];
} EntropyBuffer;


void Entropy_init(Entropy *obj)
{
    EntropyBuffer *ent_buf = calloc(1, sizeof(EntropyBuffer));
    uint32_t key[8] = {0, 0, 0, 0,  0, 0, 0, 0};
    uint64_t timestamp = time(NULL);
    uint64_t cpu = cpuclock();
    // 256 bits of entropy from system CSPRNG
    if (!inject_rand(&ent_buf->u64[0], 4)) {
        fprintf(stderr,
            "Warning: system CSPRNG is unaccessible. An internal seeder will be used.\n");
#ifdef DOS386_PLATFORM
        dos386_random_rdtsc(stderr, &ent_buf->u64[0], 2);
#endif
    }
    // 256 bits of entropy from RDSEED
    for (size_t i = 4; i < 8; i++) {
        ent_buf->u64[i] = mix_rdseed(0);
    }
    // Some fallback entropy sources
    ent_buf->u64[8] = timestamp;
    ent_buf->u64[9] = cpu;
    ent_buf->u64[10] = get_machine_id();
    ent_buf->u64[11] = get_tick_count();
    ent_buf->u64[12] = get_current_process_id();
    // Make a key using the cryptographic hash function
    blake2s(key, 32, NULL, 0, &ent_buf->u8[0], 128);
    free(ent_buf);
    // Initialize the seed generator
    ChaCha20State_init(&obj->gen, key);
    uint64_t nonce = timestamp ^ (cpu << 40);
    ChaCha20State_set_nonce(&obj->gen, nonce);
    obj->slog_len = 1 << 8;
    obj->slog_maxlen = 1 << 20;
    obj->slog = calloc(obj->slog_len, sizeof(SeedLogEntry));
    obj->slog_pos = 0;
    fprintf(stderr, "Entropy pool (seed generator) initialized\n");
    fprintf(stderr, "ChaCha20 initial state\n");
    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            size_t ind = 4 * i + j;
            fprintf(stderr, "%.8lX ", (unsigned long) obj->gen.x[ind]);
        }
        fprintf(stderr, "\n");
    }
    for (size_t i = 0; i < 4; i++) {
        fprintf(stderr, "Output %d: 0x%16.16llX\n",
            (int) i, (unsigned long long) ChaCha20State_next64(&obj->gen));
    }
}

void Entropy_free(Entropy *obj)
{
    free(obj->slog);
    obj->slog = NULL;
}

/**
 * @brief Returns 64-bit random seed. Hardware RNG is used
 * when possible. WARNING! THIS FUNCTION IS NOT THREAD SAFE!
 */
uint64_t Entropy_seed64(Entropy *obj, uint64_t thread_id)
{
    uint64_t seed = ChaCha20State_next64(&obj->gen);
    if (obj->slog_pos != obj->slog_maxlen - 1) {
        // Check if buffer resizing is needed
        if (obj->slog_pos >= obj->slog_len - 1) {
            size_t newlen = obj->slog_len * 2;
            if (newlen > obj->slog_maxlen) newlen = obj->slog_maxlen;
            obj->slog_len = newlen;
            obj->slog = realloc(obj->slog, newlen * sizeof(SeedLogEntry));
        }
        // Save seed to the log
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
