/**
 * @file xorgens.c
 * @brief 64-bit version of xor4096i ("xorgens") generator by R.P. Brent.
 * @details This PRNG is based on xorgens 3.06 library by R.P. Brent.
 * The code was simplified by exclusion of 32-bit architectures support.
 * It was also made reentrant.
 *
 * References:
 *
 * - https://maths-people.anu.edu.au/~brent/random.html
 *
 * @copyright The original algorithm and its original implementations were
 * suggested by R.P. Brent. It was adapted to SmokeRand, C99 and multithreaded
 * environment by A.L. Voskov.
 *
 * Copyright (C) 2004, 2006, 2008, 2017 R. P. Brent.
 *
 * Copyright (C) 2025-2026 A.L. Voskov.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, June 1991, as published by the Free Software Foundation.
 * For details see http://www.gnu.org/copyleft/gpl.html .
 *
 * If you would like to use this software but the GNU GPL creates legal
 * problems, then please contact the author to negotiate a special
 * agreement.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

enum {
    wlen = 64,
    r    = 64,
    s    = 53,
    a    = 33,
    b    = 26,
    c    = 27,
    d    = 29,
    ws   = 27 
};


typedef struct {
    uint64_t w;
    uint64_t weyl;
    uint64_t x[r];
    int i;
} Xorgens4096;

static inline uint64_t xor4096i_lfsr(Xorgens4096 *obj)
{
    uint64_t t, v;
    obj->i = (obj->i + 1) & (r - 1);          // Assumes that r is a power of two
    t = obj->x[obj->i];                       
    v = obj->x[(obj->i + (r - s)) & (r - 1)]; // Index is (i-s) mod r
    t ^= t << a;  t ^= t >> b;                // (I + L^a)(I + R^b)
    v ^= v << c;  v ^= v >> d;                // (I + L^c)(I + R^d)
    obj->x[obj->i] = (v ^= t);                // Update circular array
    return obj->x[obj->i];
}


static inline uint64_t xor4096i(Xorgens4096 *obj)
{
    uint64_t xs = xor4096i_lfsr(obj);         // Update LFSR
    obj->w += obj->weyl;                      // Update Weyl generator
    return (xs + (obj->w ^ (obj->w >> ws)));  // Return combination
}


void xor4096i_init(Xorgens4096 *obj, uint64_t seed)
{
    uint64_t v;
    unsigned int k;
    obj->i = -1;
    // weyl = odd approximation to 2**wlen * (3-sqrt(5))/2.
    obj->weyl = 0x61c8864680b583eb;                 
    v = (seed != 0) ? seed : ~seed;  // v must be nonzero

    for (k = wlen; k > 0; k--) {     // Avoid correlations for close seeds
        v ^= v << 10; v ^= v >> 15;  // Recurrence has period 2**wlen-1 
        v ^= v << 4;  v ^= v >> 13;  // for wlen = 32 or 64
    }
    for (obj->w = v, k = 0; k < r; k++) { // Initialise circular array
        v ^= v << 10; v ^= v >> 15; 
        v ^= v << 4;  v ^= v >> 13;
        obj->x[k] = v + (obj->w += obj->weyl);                
    }
    for (obj->i = r - 1, k = 4 * r; k > 0; k--) { // Discard first 4*r results
        (void) xor4096i_lfsr(obj);
    }
}


uint64_t get_bits_raw(Xorgens4096 *obj)
{
    return xor4096i(obj);
}

static void *create(const CallerAPI *intf)
{
    // 0xEE552A03E4F2D9FA --strange linearcomp seed
    Xorgens4096 *obj = intf->malloc(sizeof(Xorgens4096));
    xor4096i_init(obj, intf->get_seed64());
    return obj;
}

/**
 * @details Values for an internal self-tests were obtained by running the
 * slightly modified tests from the original xorgens. The modification allowed
 * to directly obtain unsigned integer from the PRNG core.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint64_t uref[10] = {
        0xA1A8A7CEEB703467, 0xB64F3C5A739862DA,
        0xC02DFB658C76F794, 0x0B7694BD970612F7,
        0xA07901F43DAA81A0, 0xC0B176887D3CEF96,
        0x098136DE8A5C1921, 0xACE18F82B4CEFCEA,
        0x16A958D8B76FE78A, 0x9C9B1830F7CD3609
    };
    Xorgens4096 *obj = intf->malloc(sizeof(Xorgens4096));
    int is_ok = 1;
    xor4096i_init(obj, 12345);
    for (int i = 0; i < 10; i++) {
        uint64_t u = xor4096i(obj);
        intf->printf("0x%16.16llX 0x%16.16llX\n", u, uref[i]);
        if (u != uref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;    
}

MAKE_UINT64_PRNG("xorgens4096", run_self_test)
