/**
 * @file rdrand.c
 * @brief HWRNG rdrand pseudorandom number generator.
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include <inttypes.h>

/* =================================================================== */

// Borrowed from: https://github.com/scottchiefbaker/perl-Random-RDTSC/blob/main/lib/Random/rdtsc_rand.h

#if defined(_WIN32) || defined(_WIN64)
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#endif

// CPUID stuff is only for X86 + GCC/Clang, MSVC has it in intrin.h
#if defined(__x86_64) && (defined(__GNUC__) || defined(__clang__))
#include <cpuid.h>
#endif

#ifdef __x86_64
#define HAS_RDRAND 1
#endif

/* =================================================================== */

// Returns 1 if hardware has RNG, 0 otherwise
int8_t has_hwrng() {
	int8_t ret = 0;

#ifdef HAS_RDRAND
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    ret = (ecx & bit_RDRND) != 0;
#endif

	return ret;
}

// Returns 1 on success, 0 on failure
int get_hw_rand64(uint64_t* value) {
#ifdef HAS_RDRAND
    unsigned char ok;
    __asm__ volatile("rdrand %0; setc %1" : "=r" (*value), "=qm" (ok) : : "cc");
    return ok ? 1 : 0;
#else
	*value = 0;
	return 0;
#endif
}

/* =================================================================== */

PRNG_CMODULE_PROLOG

typedef struct {
	int8_t cpu_has_rdrand;
} rdrand_state;

static inline uint64_t get_bits_raw(rdrand_state *obj)
{
	// RDRAND is supported by x86_64
	if (obj->cpu_has_rdrand) {
		uint64_t num = 0;
		// Returns 0/1 if the hwrng is good
		if (get_hw_rand64(&num)) {
			return num;
		}
	}

	return 0;
}

// Just store the CPU capabilities (no state needed)
static void *create(const CallerAPI *intf)
{
    rdrand_state *obj   = intf->malloc(sizeof(rdrand_state));
	obj->cpu_has_rdrand = has_hwrng();

    return obj;
}

MAKE_UINT64_PRNG("rdrand", NULL)
