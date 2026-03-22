/**
 * @file tychei64w.c
 * @brief 64-bit modification of Tyche-i nonlinear chaotic PRNG that
 * uses rotation constants from BlaBla experimental stream cipher.
 * @details
 *
 * References:
 *
 * 1. Samuel Neves & Filipe Araujo. Fast and Small Nonlinear Pseudorandom
 *    Number Generators for Computer Simulation // Parallel Processing and
 *    Applied Mathematics (PPAM 2011) P.92-101.
 *    https://doi.org/10.1007/978-3-642-31464-3_10
 * 2. https://github.com/veorq/blabla/blob/master/BlaBla.swift
 *
 * @copyright
 * Tyche-i PRNG was designed by Samuel Neves & Filipe Araujo.
 *
 * 64-bit modification based on BlaBla rotation constants:
 * 
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"
#include <stdint.h>

PRNG_CMODULE_PROLOG


typedef struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
    uint64_t ctr;
} Tychei64WeylState;



static uint64_t get_bits_raw(Tychei64WeylState *obj)
{
    obj->c += obj->ctr++;
    obj->b = rotl64(obj->b, 63) ^ obj->c; obj->c -= obj->d;
    obj->d = rotl64(obj->d, 16) ^ obj->a; obj->a -= obj->b;
    obj->b = rotl64(obj->b, 24) ^ obj->c; obj->c -= obj->d;
    obj->d = rotl64(obj->d, 32) ^ obj->a; obj->a -= obj->b;
    return obj->a;
}

static void *create(const CallerAPI *intf)
{
    Tychei64WeylState *obj = intf->malloc(sizeof(Tychei64WeylState));
    obj->a = intf->get_seed64();
    obj->b = intf->get_seed64();
    obj->c = 0x517CC1B727220A95;
    obj->d = 0x9E3779B97F4A7C15;
    obj->ctr = 0;
    for (int i = 0; i < 20; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}


MAKE_UINT64_PRNG("Tychei64-Weyl", NULL)
