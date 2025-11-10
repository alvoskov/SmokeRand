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
#include "smokerand/base64.h"
#include "smokerand/blake2s.h"
#include "smokerand/coredefs.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#ifdef __DJGPP__
    #include <go32.h>
    #include <sys/farptr.h>
#endif


#define CHACHA_DEFAULT_NONCE 0x90ABCDEF12345678

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


void ChaCha20State_init(ChaCha20State *obj, const uint32_t *key, uint64_t nonce)
{
    // Constants: the upper row of the matrix
    obj->x[0] = 0x61707865; obj->x[1] = 0x3320646e;
    obj->x[2] = 0x79622d32; obj->x[3] = 0x6b206574;
    // Rows 1-2: key (seed)
    for (size_t i = 0; i < 8; i++) {
        obj->x[i + 4] = key[i];
    }
    // Row 3: counter and nonce
    obj->x[12] = 0;
    obj->x[13] = 0;
    obj->x[14] = (uint32_t) nonce;
    obj->x[15] = (uint32_t) (nonce >> 32);
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


static int chacha20_selftest(void)
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
    ChaCha20State_init(&obj, x_init, 0);
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
    return chacha20_selftest() && (blake2s_selftest() == BLAKE2S_SUCCESS);
}

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
    #include <windows.h>
    #define WINDOWS_PLATFORM
#else
    #include <fcntl.h>
    #include <unistd.h>
#endif


#if (defined(__WATCOMC__) && defined(__386__) && defined(__DOS__)) || defined(__DJGPP__)
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
#elif defined(__DJGPP__) && defined(DOS386_PLATFORM)
    // Get tick count from PIT (BIOS data area) for DJGPP
    // Works only under DOS!
    return _farpeekl(_dos_ds, 0x46C);
#elif defined(__WATCOM__) && defined(DOS386_PLATFORM)
    // Get tick count from PIT (BIOS data area) for Open Watcom
    // Works only under DOS!
    return *(volatile uint32_t *) 0x46C;
#elif !defined(NO_POSIX)
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (uint32_t) t.tv_nsec;
#else
    // Just make some noisy stuff
    return (uint32_t) (time(NULL) * 69069u);
#endif
}

/**
 * @brief 128-bit bit machine ID obtained from Blake2s-128 hash function.
 * @details We don't care about little/big-endianness because this is
 * also a characteristic of the computer.
 */
typedef union {
    uint8_t u8[16];
    uint64_t u64[2];
} MachineID;

/**
 * @brief Returns a hash made from an unique machine ID.
 */
static MachineID get_machine_id()
{
    MachineID machine_id = {.u64 = {0, 0}};
    char value[64];
    memset(value, 0, 64);
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
        blake2s_128(machine_id.u8, value, strlen(value));
    }
    RegCloseKey(key);
#elif defined(__DJGPP__)
    // DJGPP DOS: calculate ROM BIOS checksum
    uint64_t *bios_buf = malloc(8192 * sizeof(uint64_t));
    for (unsigned int i = 0; i < 8192; i++) {
        uint64_t lo = _farpeekl(_dos_ds, 0xF0000U + 8*i);
        uint64_t hi = _farpeekl(_dos_ds, 0xF0000U + 8*i + 4);
        bios_buf[i] = (hi << 32) | lo;
    }
    blake2s_128(machine_id.u8, bios_buf, 65536U);
    free(bios_buf);
    (void) value;
#elif defined(DOS386_PLATFORM)
    // DOS: calculate ROM BIOS checksum
    uint64_t *bios_data = (uint64_t *) 0xF0000;
    blake2s_128(machine_id.u8, bios_data, 65536u);
    (void) value;
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
    if (fgets(value, 64, fp)) {
        blake2s_128(machine_id.u8, value, strlen(value));
    }
    fclose(fp);
#endif    
    return machine_id;
}

////////////////////////////////////////////////
///// CPU architecture dependent functions /////
////////////////////////////////////////////////

#ifdef NO_X86_EXTENSIONS
/**
 * @brief rdseed is unavailable: just return 0.
 */
uint64_t call_rdseed()
{
    return 0;
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
 * @brief Calls x86-64 CPU rdseed instruction.
 * @details It is good but not very reliable source of entropy. Examples of
 * RDRAND failure on some CPUs:
 *
 * - https://github.com/systemd/systemd/issues/11810#issuecomment-489727505
 * - https://github.com/rust-random/getrandom/issues/228
 * - https://news.ycombinator.com/item?id=19848953
 */
uint64_t call_rdseed()
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
    return rd;
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
static unsigned int dos386_get_nbits(uint64_t value)
{
    unsigned int nbits = 0;
    while (value != 0) {
        value >>= 1;
        nbits++;
    }
    return nbits;
}

/**
 * @brief Get the current 32-bit number of ticks from PIT (Programmable
 * Interval Timer).
 */
static inline uint32_t dos386_get_bios_pit(void)
{
#ifdef __DJGPP__
    return _farpeekl(_dos_ds, 0x46C);
#else
    return *(volatile uint32_t *) 0x46C;
#endif
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
    uint64_t rdtsc_prev = __rdtsc(), delta_prev = 0;
    uint64_t accum = dos386_get_bios_pit();
    for (size_t i = 0; i < len; i++) {
        size_t nbits_total = 0;
        for (int j = 0; j < 64 && nbits_total < 128; j++) {
            // Wait for the next tick of the PIT
            for (uint32_t t = dos386_get_bios_pit(); t == dos386_get_bios_pit(); ) {
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
            size_t nbits = dos386_get_nbits(rndbits);
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
 * @brief Gains access to the system CSPRNG and writes its output
 * to the output buffer.
 * @param[out] out  Output buffer.
 * @param[in]  len  Size of output buffer in bytes.
 * @return 0/1 - failure/success.
 */
static int fill_from_random_device(unsigned char *out, size_t len)
{
#ifdef WINDOWS_PLATFORM
    HCRYPTPROV hCryptProv;
    DWORD nbytes = (DWORD) (sizeof(unsigned char) * len);
    // Acquire a cryptographic provider context
    // CRYPT_VERIFYCONTEXT for a temporary context
    if (!CryptAcquireContext(&hCryptProv, NULL, NULL,
        PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return 0;
    }
    // Generate random bytes
    if (!CryptGenRandom(hCryptProv, nbytes, (BYTE *) out)) {
        CryptReleaseContext(hCryptProv, 0);
        return 0;
    }
    // Release the cryptographic provider context
    CryptReleaseContext(hCryptProv, 0);
    return 1;
#elif !defined(NO_POSIX)
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        return 0;
    }
    size_t nbytes = sizeof(unsigned char) * len;
    if (read(fd, out, nbytes) != (ssize_t) nbytes) {
        close(fd);
        return 0;
    }
    close(fd);
    return 1;
#else
    (void) out; (void) len;
    return 0;
#endif
}

#define ENTROPY_BUFFER_SIZE_U64 16
#define ENTROPY_BUFFER_SIZE (16 * 8)

typedef union {
    uint64_t u64[ENTROPY_BUFFER_SIZE_U64];
    uint8_t u8[ENTROPY_BUFFER_SIZE];
} EntropyBuffer;


/**
 * @brief Initialize the seed generator using existing 256-bit key
 * and 64-bit nonce.
 */
void Entropy_init_from_key(Entropy *obj, const uint32_t *key, uint64_t nonce)
{
    ChaCha20State_init(&obj->gen, key, nonce);
    obj->slog_len = 1 << 8;
    obj->slog_maxlen = 1 << 20;
    obj->slog = calloc(obj->slog_len, sizeof(SeedLogEntry));
    obj->slog_pos = 0;
    fprintf(stderr, "Entropy pool (seed generator) initialized\n");
    fprintf(stderr, "ChaCha20 initial state\n");
    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            size_t ind = 4 * i + j;
            fprintf(stderr, "%.8" PRIX32 " ", obj->gen.x[ind]);
        }
        fprintf(stderr, "\n");
    }

    for (int i = 0; i < 4; i++) {
        fprintf(stderr, "Output %d: 0x%16.16" PRIX64 "\n",
            i, ChaCha20State_next64(&obj->gen));
    }
}

/**
 * @brief Initialize the seed generator using some text seed string.
 * @details The input text seed will be sent to the Blake2s hash function
 * and the obtained 256-bit hash will be used as a key for ChaCha20.
 * @param obj   Entropy class example to be initialized.
 * @param seed  ASCIIZ string with the text seed.
 */
void Entropy_init_from_textseed(Entropy *obj, const char *seed)
{
    uint8_t key_bytes[32];
    uint32_t key_words[8];
    size_t len = strlen(seed);
    fprintf(stderr, "Initializing from a user-defined text seed\n");
    fprintf(stderr, "  Seed value: %s\n", seed);
    fprintf(stderr, "  Seed size:  %u byte(s)\n", (unsigned int) len);
    blake2s_256(key_bytes, seed, len);
    // Big-endian portable converation of bytes to 32-bit words.
    // BE was selected to show Blake2s hash by sequential printing of
    // 32-bit words in hexadecimal format.
    for (size_t i = 0; i < 8; i++) {
        key_words[i] =
            ( (uint32_t) key_bytes[4*i]     << 24 ) |
            ( (uint32_t) key_bytes[4*i + 1] << 16 ) |
            ( (uint32_t) key_bytes[4*i + 2] << 8  ) |
            ( (uint32_t) key_bytes[4*i + 3] << 0  );
    }    
    Entropy_init_from_key(obj, key_words, CHACHA_DEFAULT_NONCE);
}



int Entropy_init_from_base64_seed(Entropy *obj, const char *seed)
{
    fprintf(stderr, "Initializing from a user-defined base64 seed\n");
    fprintf(stderr, "  Seed value: %s\n", seed);
    size_t nwords;
    uint32_t *key_words = sr_base64_to_u32_bigendian(seed, &nwords);
    if (key_words == NULL) {
        fprintf(stderr, "  Error: base64 input is corrupted\n");
        return 0;
    } else if (nwords != 8) {
        fprintf(stderr, "  Error: base64 input is shorter/longer than 256 bits\n");
        free(key_words);
        return 0;
    }
    Entropy_init_from_key(obj, key_words, CHACHA_DEFAULT_NONCE);
    free(key_words);
    return 1;
}


/**
 * @brief Initialize the seed generator using available sources of entropy
 * that include system-level CSPRNG such as `dev/urandom`, `time(NULL)`, RDTSC,
 * machine and process id.
 */
void Entropy_init(Entropy *obj)
{
    EntropyBuffer *ent_buf = calloc(1, sizeof(EntropyBuffer));
    uint32_t key[8] = {0, 0, 0, 0,  0, 0, 0, 0};
    // 256 bits of entropy from system CSPRNG
    int csprng_present = fill_from_random_device(&ent_buf->u8[0], 8 * sizeof(uint32_t));
    //for (size_t i = 0; i < 4; i++) {
    //    printf("%8.8" PRIX64 " ", ent_buf->u64[i]);
    //}
    if (csprng_present) {
        fprintf(stderr, "System CSPRNG is available and was used\n");
    } else {
        fprintf(stderr,
            "Warning: system CSPRNG is unaccessible. Fallback entropy sources will be used.\n");
#ifdef DOS386_PLATFORM
        dos386_random_rdtsc(stderr, &ent_buf->u64[0], 2);
#endif
    }
    // 256 bits of entropy from RDSEED
    for (size_t i = 4; i < 8; i++) {
        ent_buf->u64[i] = call_rdseed();
    }
    // Some fallback entropy sources
    uint64_t timestamp = (uint64_t) time(NULL);
    uint64_t cpu = cpuclock();
    MachineID machine_id = get_machine_id();
    ent_buf->u64[8] = timestamp;
    ent_buf->u64[9] = cpu;
    ent_buf->u64[10] = machine_id.u64[0];
    ent_buf->u64[11] = machine_id.u64[1];
    ent_buf->u64[12] = get_tick_count();
    ent_buf->u64[13] = get_current_process_id();
    if (!csprng_present || 1) {
        fprintf(stderr, "Time stamp:   0x%16.16" PRIx64 "\n", timestamp);
        fprintf(stderr, "CPU ticks:    0x%16.16" PRIx64 "\n", cpu);
        fprintf(stderr, "Machine ID:   0x%16.16" PRIx64 "-0x%16.16" PRIx64 "\n",
            ent_buf->u64[10], ent_buf->u64[11]);
        fprintf(stderr, "System timer: 0x%16.16" PRIx64 "\n", ent_buf->u64[12]);
        fprintf(stderr, "Process ID:   0x%16.16" PRIx64 "\n", ent_buf->u64[13]);
    }
    // Make a key using the cryptographic hash function
    blake2s_256(key, &ent_buf->u8[0], ENTROPY_BUFFER_SIZE);
    free(ent_buf);
    // Initialize the seed generator
    Entropy_init_from_key(obj, key, CHACHA_DEFAULT_NONCE);
}

void Entropy_free(Entropy *obj)
{
    free(obj->slog);
    obj->slog = NULL;
}

/**
 * @brief Returns the pointer to the 256-bit ChaCha20 key used for generator
 * of seeds.
 */
const uint32_t *Entropy_get_key(const Entropy *obj)
{
    return &(obj->gen.x[4]);
}

/**
 * @brief Returns dynamically allocated buffer with base64 representation
 * of the key returned by the Entropy_get_key function. If Entropy class
 * is not initialized then NULL will be returned.
 * @return Buffer with ASCIIZ string with base64 key representation, must
 * be deallocated by `free` function by the caller.
 */
char *Entropy_get_base64_key(const Entropy *obj)
{
    if (Entropy_is_init(obj)) {
        return sr_u32_bigendian_to_base64(Entropy_get_key(obj), 8);
    } else {
        return NULL;
    }
}

/**
 * @brief Returns 64-bit random seed. Hardware RNG is used
 * when possible. WARNING! THIS FUNCTION IS NOT THREAD SAFE!
 */
uint64_t Entropy_seed64(Entropy *obj, uint64_t thread_id)
{
    if (!Entropy_is_init(obj)) {
        Entropy_init(obj);
    }
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
    const size_t max_log_length = 1024;
    int filler_printed = 0;
    fprintf(fp, "Number of seeds: %llu\n", (unsigned long long) obj->slog_pos);
    fprintf(fp, "%10s %18s %20s\n", "Thread", "Seed(HEX)", "Seed(DEC)");
    for (size_t i = 0; i < obj->slog_pos; i++) {
        if (i < max_log_length || i > obj->slog_pos - 5) {
            fprintf(fp, "%10" PRIX64 " 0x%.16" PRIX64 " %.20" PRIu64 "\n",
                obj->slog[i].thread_id, obj->slog[i].seed, obj->slog[i].seed);
        } else if (!filler_printed) {
            fprintf(fp, "    ...................................................\n");
            filler_printed = 1;
        }
    }
}
