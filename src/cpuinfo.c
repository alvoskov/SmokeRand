/**
 * @file cpuinfo.c
 * @brief Functions that give access to CPU tick counters, frequency information
 * and other characteristics.
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cpuinfo.h"
#include <stdio.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64) || defined(__MINGW32__) || defined(__MINGW64__)
    #include <windows.h>
    #define WINDOWS_PLATFORM
#endif

/**
 * @brief A crude estimation of CPU frequency based on `clock` function
 * and execution of arithmetic loops that cannot be optimized by compilers.
 * It also uses a default CPU frequency as 3 GHz as fallback (it is rather
 * close to it since ~2005).
 */
static uint64_t estimate_cpu_freq()
{
    const clock_t tic_init = clock();
    const double ticks_per_iter = 2.5;
    const uint64_t default_freq = 3000000000; // 3 GHz
    clock_t tic;
    do {
        tic = clock();
    } while (tic == tic_init);
    volatile uint32_t sum = 0;
    uint32_t x = (uint32_t) tic;
    double time_elapsed = 0.0;
    long long niters = 0;
    do {
        const long niters_per_step = 200000;
        // Note about cycle: don't split arithmetic expression into "nicer"
        // and "more human readable" chunks: it may influence speed at -O0
        // and -O1.
        for (long i = 0; i < niters_per_step; i++) {
            sum += (x = 69069U * x + 12345U);
        }
        niters += niters_per_step;
        const clock_t toc = clock();
        time_elapsed = (double) (toc - tic) / (double) CLOCKS_PER_SEC;
    } while (time_elapsed < 0.5 && time_elapsed >= 0);
    if (time_elapsed <= 0.001) {
        return default_freq;
    }
    const uint64_t freq = (uint64_t) (ticks_per_iter * (double) niters / time_elapsed);
    return (freq < 1000U * default_freq) ? freq : default_freq;
}


double get_cpu_freq(void)
{
#ifdef WINDOWS_PLATFORM
    HKEY hKey;
    const char *subKey = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
    const char *valueName = "~MHz";
    DWORD cpuSpeedMHz = 0, dataSize = sizeof(cpuSpeedMHz);
    // Open the registry key
    LSTATUS status = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, // Top-level key
        subKey,             // Subkey path
        0,                  // Options
        KEY_QUERY_VALUE,    // Desired access rights
        &hKey               // Handle to the opened key
    );
    if (status != ERROR_SUCCESS) {
        return (double) estimate_cpu_freq() / 1e6;
    } else {
        // Query the value
        status = RegQueryValueEx(
            hKey,           // Handle to the opened key
            valueName,      // Value name
            NULL,           // Reserved
            NULL,           // Type (can be NULL if not needed)
            (LPBYTE) &cpuSpeedMHz, // Buffer for data
            &dataSize       // Size of data buffer
        );
        // Close the registry key
        RegCloseKey(hKey);
        return (status == ERROR_SUCCESS) ? (double) cpuSpeedMHz : 0.0;
    }
#elif !defined(NO_POSIX)
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        return 0.0;
    }
    char buf[256];
    double freq = 0.0;
    while (fgets(buf, 256, fp) != NULL) {
        if (sscanf(buf, "cpu MHz : %lf", &freq) == 1) {
            return freq;
        }
    }
    if (freq == 0.0) {
        freq = (double) estimate_cpu_freq() / 1e6;
    }
    fclose(fp);
    return freq;
#else
    return (double) estimate_cpu_freq() / 1e6;
#endif
}

/**
 * @brief Emulation of RDTSC instruction: estimates number of CPU tics
 * from the program start using `clock` function and crude estimation
 * of CPU frequency as 3 GHz (it is rather close to it since ~2005).
 * @details NOTE: THIS FUNCTION MAY BE NOT SUITABLE FOR MULTITHREADING!
 * Its initialization at the first run is not thread-safe!
 */
uint64_t emulate_cpuclock(void)
{
    static uint64_t cpu_freq = 0;
    if (cpu_freq == 0) {
        cpu_freq = (uint64_t) (get_cpu_freq() * 1000.0);
    }
    const uint64_t clocks_to_tics = cpu_freq / CLOCKS_PER_SEC;
    const uint64_t clk = (uint64_t) clock();
    return clk * clocks_to_tics;
}


/**
 * @brief Calls x86-64 CPU rdseed instruction. If it is absent then 0
 * is returned.
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
#if !defined(__RDSEED__) || defined(NO_CPU_EXTENSIONS)
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


#ifdef NO_CPU_EXTENSIONS
    #define NO_RDTSC
#elif defined(__WATCOMC__)
    #define X86_RDTSC
    uint64_t __rdtsc(void);
    #pragma aux __rdtsc = " .586 " \
        "rdtsc " value [eax edx];
#elif (defined(WIN32) || defined(WIN64)) && !defined(__MINGW32__) && !defined(__MINGW64__)
    #define X86_RDTSC
    #include <intrin.h>
    #pragma intrinsic(__rdtsc)
#elif defined(__x86_64__) || defined(__i386__)
    #define X86_RDTSC
    #include <x86intrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define AARCH64_RDTSC
    static inline uint64_t get_cntvct_el0(void)
    {
        uint64_t ctr;
        __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(ctr));
        // pmccntr_el0 is usually disabled in a user-space mode
        //__asm__ __volatile__("mrs %0, pmccntr_el0" : "=r"(ctr));
        return ctr;
    }

    static inline uint64_t get_cntfrq_el0(void)
    {
        uint64_t freq;
        __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(freq));
        return freq;

    }
#else
    #define NO_RDTSC
#endif


uint64_t cpuclock()
{
#ifdef X86_RDTSC
    return __rdtsc();
#elif defined(AARCH64_RDTSC)
    static uint64_t coeff = 0;
    if (coeff == 0) {
        const double coeff_dbl = (double) get_cpu_freq() * 512e6 /
            (double) get_cntfrq_el0();
        coeff = (uint64_t) coeff_dbl;
    }
    return get_cntvct_el0() * coeff / 512;
#else
    return emulate_cpuclock();
#endif
}
