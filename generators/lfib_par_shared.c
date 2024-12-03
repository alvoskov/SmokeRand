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

typedef struct {
    const char *name;
    int is_additive;
    int r;
    int s;
} LFibDynDescr;


static const LFibDynDescr generators[] = {
    {.name = "31+",     .r = 31,     .s = 3,      .is_additive = 1}, // from glibc
    {.name = "55+",     .r = 55,     .s = 24,     .is_additive = 1},
    {.name = "55-",     .r = 55,     .s = 24,     .is_additive = 0},
    {.name = "127+",    .r = 127,    .s = 97,     .is_additive = 1},
    {.name = "127-",    .r = 127,    .s = 97,     .is_additive = 0},
    {.name = "258+",    .r = 258,    .s = 83,     .is_additive = 1},
    {.name = "258-",    .r = 258,    .s = 83,     .is_additive = 0},
    {.name = "378+",    .r = 378,    .s = 107,    .is_additive = 1},
    {.name = "378-",    .r = 378,    .s = 107,    .is_additive = 0},
    {.name = "607+",    .r = 607,    .s = 273,    .is_additive = 1}, // from golang
    {.name = "607-",    .r = 607,    .s = 273,    .is_additive = 0},
    {.name = "1279+",   .r = 1279,   .s = 418,    .is_additive = 1},
    {.name = "1279-",   .r = 1279,   .s = 418,    .is_additive = 0},
    {.name = "2281+",   .r = 2281,   .s = 1252,   .is_additive = 1},
    {.name = "2281-",   .r = 2281,   .s = 1252,   .is_additive = 0},
    {.name = "3217+",   .r = 3217,   .s = 576,    .is_additive = 1},
    {.name = "3217-",   .r = 3217,   .s = 576,    .is_additive = 0},
    {.name = "4423+",   .r = 4423,   .s = 2098,   .is_additive = 1},
    {.name = "4423-",   .r = 4423,   .s = 2098,   .is_additive = 0},
    {.name = "9689+",   .r = 9689,   .s = 5502,   .is_additive = 1},
    {.name = "9689-",   .r = 9689,   .s = 5502,   .is_additive = 0},
    {.name = "19937+",  .r = 19937,  .s = 9842,   .is_additive = 1},
    {.name = "19937-",  .r = 19937,  .s = 9842,   .is_additive = 0},
    {.name = "23209+",  .r = 23209,  .s = 13470,  .is_additive = 1},
    {.name = "23209-",  .r = 23209,  .s = 13470,  .is_additive = 0},
    {.name = "44497+",  .r = 44497,  .s = 21034,  .is_additive = 1},
    {.name = "44497-",  .r = 44497,  .s = 21034,  .is_additive = 0},
    {.name = "110503+", .r = 110503, .s = 53719,  .is_additive = 1},
    {.name = "110503-", .r = 110503, .s = 53719,  .is_additive = 0},
    {.name = "756839+", .r = 756839, .s = 279695, .is_additive = 1},
    {.name = "756839-", .r = 756839, .s = 279695, .is_additive = 0},
    {.name = NULL,      .r = 0,      .s = 0,      .is_additive = 0}
};

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
    LFibDyn_State obj = {.r = 0, .s = 0, .is_additive = 0};
    const char *param = intf->get_param();
    for (const LFibDynDescr *ptr = generators; ptr->name != NULL; ptr++) {
        if (!intf->strcmp(ptr->name, param)) { // from glibc
            obj.r = ptr->r; obj.s = ptr->s;
            obj.is_additive = ptr->is_additive;
        }        
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
