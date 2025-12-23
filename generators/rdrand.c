/**
 * @file rdrand.c
 * @brief HWRNG rdrand pseudorandom number generator.
 * @details
 *
 * Based on the next code:
 * https://github.com/scottchiefbaker/perl-Random-RDTSC/blob/main/lib/Random/rdtsc_rand.h
 *
 * @copyright
 * An initial plugin for SmokeRand: (C) 2025 Scott Baker
 *
 * Refactoring with replacement of assembly language commands to compilers
 * intrinsics:
 * 
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#ifdef __RDRND__
    #include "smokerand/x86exts.h"
    // CPUID stuff is only for X86 + GCC/Clang, MSVC has it in intrin.h
    #if (defined(__x86_64) || defined(__i386__)) && (defined(__GNUC__) || defined(__clang__))
        #include <cpuid.h>
    #endif
#endif

PRNG_CMODULE_PROLOG

/**
 * @brief Returns 1 if hardware has RNG, 0 otherwise
 */
static int has_hwrng()
{
	int ret = 0;
#ifdef __RDRND__
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    ret = (ecx & bit_RDRND) != 0;
#endif
	return ret;
}


typedef struct {
    int8_t dummy;
} RdrandState;


static inline uint64_t get_bits_raw(RdrandState *obj)
{
    (void) obj;
#ifdef __RDRND__
    unsigned long long rd;
    #if SIZE_MAX == UINT32_MAX
        // For 32-bit x86 executables
        unsigned int rd_hi, rd_lo;
        while (!_rdrand32_step(&rd_hi)) {}
        while (!_rdrand32_step(&rd_lo)) {}
        rd = (((unsigned long long) rd_hi) << 32) | rd_lo;
    #else
        // For 64-bit x86-64 executables
        while (!_rdrand64_step(&rd)) {}
    #endif
    return (uint64_t) rd;
#else
    return 0;
#endif
}

/**
 * @brief Will return NULL if hardware RNG is not supported.
 */
static void *create(const CallerAPI *intf)
{
    if (!has_hwrng()) {
        return NULL;
    }
#if defined(__RDRND__)
    RdrandState *obj = intf->malloc(sizeof(RdrandState));
    obj->dummy = 0;
    return obj;
#else
    return NULL;
#endif
}

MAKE_UINT64_PRNG("rdrand", NULL)
