/**
 * @file lfib_par_shared.c
 * @brief A shared library that implements the additive and subtractive
 * 64-bit Lagged Fibbonaci generators that return only upper 32 bits.
 * The default one is \f$ LFib(2^{64}, 607, 273, +) \f$.
 *
 * @details It uses the next recurrent formula:
 * \f[
 * X_{n} = X_{n - r} \pm X_{n - s}
 * \f]
 *
 * and returns higher 32 bits. The initial values in the ring buffer
 * are filled by the 64-bit PCG generator.
 *
 *                   | brief | default | full | PractRand
 * ------------------|-------|---------|------|-----------
 * LFib(55,24,+)     | 3     | 3       |      | 2 GiB
 * LFib(55,24,-)     | 3     |         |      | 2 GiB
 * LFib(127,97,+)    | 3     |         |      | 512 MiB
 * LFib(127,97,-)    | 3     |         |      | 512 MiB
 * LFib(607,207,+)   | 3     |         |      | 128 GiB
 * LFib(607,207,-)   | 3     |         |      | 256 GiB
 * LFib(1279,418,+)  | 2     | 2       |      |
 * LFib(1279,418,-)  | 2     | 2       |      |
 * LFib(2281,1252,+) | 2     | 2       |      |
 * LFib(2281,1252,-) | 1     | 2       |      |
 * LFib(3217,576,+)  | +     | +       |      |
 * LFib(3217,576,-)  | +     |         |      |
 *
 * Sources of parameters:
 *
 * 1. D. Knuth. TAOCP Vol. 2 (Chapter 3.2.2)
 * 2. https://www.boost.org/doc/libs/master/boost/random/lagged_fibonacci.hpp
 * 3. Brent R.P. Uniform Random Number Generators for Supercomputers
 * 4. Brent R.P. Uniform Random Number Generators for Vector and Parallel
 *    Computers. Report Report TR-CS-92-02. March 1992.
 *    https://maths-people.anu.edu.au/~brent/pd/rpb132tr.pdf   
 * 5. Brent, R.P., Zimmermann, P. (2003). Random Number Generators with Period
 *    Divisible by a Mersenne Prime. In: Kumar, V., Gavrilova, M.L., Tan, C.J.K.,
 *    L'Ecuyer, P. (eds) Computational Science and Its Applications - ICCSA 2003.
 *    ICCSA 2003. Lecture Notes in Computer Science, vol 2667. Springer, Berlin, Heidelberg.
 *    https://doi.org/10.1007/3-540-44839-X_1
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    int is_additive; ///< 0/1 - subtractive/additive
    int r; ///< Larger lag
    int s; ///< Smaller lag
    int i;
    int j;
    uint64_t *u; ///< Ring buffer (u[0] is not used)
} LFibDyn_State;

static inline uint64_t get_bits_raw(void *state)
{
    LFibDyn_State *obj = state;
    uint64_t x;
    if (obj->is_additive) {
        x = obj->u[obj->i] + obj->u[obj->j];
    } else {
        x = obj->u[obj->i] - obj->u[obj->j];
    }
    obj->u[obj->i] = x;
    if(--obj->i == 0) obj->i = obj->r;
	if(--obj->j == 0) obj->j = obj->r;
    return x >> 32;
}


static LFibDyn_State parse_parameters(const CallerAPI *intf)
{
    LFibDyn_State obj;
    const char *param = intf->get_param();
    if (!intf->strcmp("31+", param)) { // from glibc
        obj.r = 31; obj.s = 3; obj.is_additive = 1;
    } else if (!intf->strcmp("55+", param)) {
        obj.r = 55; obj.s = 24; obj.is_additive = 1;
    } else if (!intf->strcmp("55-", param)) {
        obj.r = 55; obj.s = 24; obj.is_additive = 0;
    } else if (!intf->strcmp("127+", param)) {
        obj.r = 127; obj.s = 97; obj.is_additive = 1;
    } else if (!intf->strcmp("127-", param)) {
        obj.r = 127; obj.s = 97; obj.is_additive = 0;
    } else if (!intf->strcmp("607+", param) || !intf->strcmp("", param)) {
        obj.r = 607; obj.s = 273; obj.is_additive = 1;
    } else if (!intf->strcmp("607-", param)) {
        obj.r = 607; obj.s = 273; obj.is_additive = 0;
    } else if (!intf->strcmp("1279+", param)) {
        obj.r = 1279; obj.s = 418; obj.is_additive = 1;
    } else if (!intf->strcmp("1279-", param)) {
        obj.r = 1279; obj.s = 418; obj.is_additive = 0;
    } else if (!intf->strcmp("2281+", param)) {
        obj.r = 2281; obj.s = 1252; obj.is_additive = 1;
    } else if (!intf->strcmp("2281-", param)) {
        obj.r = 2281; obj.s = 1252; obj.is_additive = 0;
    } else if (!intf->strcmp("3217+", param)) {
        obj.r = 3217; obj.s = 576; obj.is_additive = 1;
    } else if (!intf->strcmp("3217-", param)) {
        obj.r = 3217; obj.s = 576; obj.is_additive = 0;
    } else if (!intf->strcmp("4423+", param)) {
        obj.r = 4423; obj.s = 2098; obj.is_additive = 1;
    } else if (!intf->strcmp("4423-", param)) {
        obj.r = 4423; obj.s = 2098; obj.is_additive = 0;
    } else if (!intf->strcmp("9689+", param)) {
        obj.r = 9689; obj.s = 5502; obj.is_additive = 1;
    } else if (!intf->strcmp("9689-", param)) {
        obj.r = 9689; obj.s = 5502; obj.is_additive = 0;
    } else if (!intf->strcmp("19937+", param)) {
        obj.r = 19937; obj.s = 9842; obj.is_additive = 1;
    } else if (!intf->strcmp("19937-", param)) {
        obj.r = 19937; obj.s = 9842; obj.is_additive = 0;
    } else if (!intf->strcmp("23209+", param)) {
        obj.r = 23209; obj.s = 13470; obj.is_additive = 1;
    } else if (!intf->strcmp("23209-", param)) {
        obj.r = 23209; obj.s = 13470; obj.is_additive = 0;
    } else if (!intf->strcmp("44497+", param)) {
        obj.r = 44497; obj.s = 21034; obj.is_additive = 1;
    } else if (!intf->strcmp("44497-", param)) {
        obj.r = 44497; obj.s = 21034; obj.is_additive = 0;
    } else if (!intf->strcmp("110503+", param)) {
        obj.r = 110503; obj.s = 53719; obj.is_additive = 1;
    } else if (!intf->strcmp("110503-", param)) {
        obj.r = 110503; obj.s = 53719; obj.is_additive = 0;
    } else if (!intf->strcmp("756839+", param)) {
        obj.r = 756839; obj.s = 279695; obj.is_additive = 1;
    } else if (!intf->strcmp("756839-", param)) {
        obj.r = 756839; obj.s = 279695; obj.is_additive = 0;
    } else {
        obj.r = 0; obj.s = 0; obj.is_additive = 0;
    }
    obj.i = obj.r; obj.j = obj.s;
    return obj;
}

static void *create(const CallerAPI *intf)
{
    LFibDyn_State par = parse_parameters(intf);
    if (par.r == 0) {
        intf->printf("Unknown parameter %s\n", intf->get_param());
        return NULL;
    }
    intf->printf("LFib(%d,%d,%s)\n", par.r, par.s,
        par.is_additive ? "+" : "-");
    // Allocate buffers
    size_t len = sizeof(LFibDyn_State) + (par.r + 2) * sizeof(uint64_t);    
    LFibDyn_State *obj = intf->malloc(len);
    *obj = par;
    obj->u = (uint64_t *) ( (char *) obj + sizeof(LFibDyn_State) );
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    for (int k = 1; k <= obj->r; k++) {
        obj->u[k] = pcg_bits64(&state);
    }
    return (void *) obj;
}

MAKE_UINT32_PRNG("LFib", NULL)
