/**
 * @file lfib_par.c
 * @brief A shared library that implements the additive and subtractive
 * 64-bit Lagged Fibbonaci generators that return only upper 32 bits.
 * The default one is \f$ LFib(2^{64}, 607, 273, +) \f$.
 *
 * @details It uses the next recurrent formula:
 * \f[
 * X_{n} = X_{n - r} \pm X_{n - s} \mathop{\mathrm{mod}} 2^{64}
 * \f]
 *
 * and returns higher 32 bits. The initial values in the ring buffer
 * are filled by the 64-bit PCG generator.
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
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
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
    int pos;
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
    {.name = "",        .r = 607,    .s = 273,    .is_additive = 1},
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

/**
 * @brief A bufferized implementation of lagged Fibonacci generator.
 * @details It is sligtly (20%) faster than "naive" implementation
 * for moderate lags. For small lags it may be slower, for large --
 * has very similar speed.
 */
static inline uint64_t get_bits_raw(void *state)
{
    LFibDyn_State *obj = state;
    uint64_t *x = obj->u;
    if (obj->pos < obj->r) {
        return x[obj->pos++] >> 32;
    }
    int dlag = obj->r - obj->s;
    if (obj->is_additive) {
        // Additive generator
        for (int i = 0; i < obj->s; i++) {
            x[i] = x[i] + x[i + dlag];
        }
        for (int i = obj->s; i < obj->r; i++) {
            x[i] = x[i] + x[i - obj->s];
        }
    } else {
        // Subtractive generator
        for (int i = 0; i < obj->s; i++) {
            x[i] = x[i] - x[i + dlag];
        }
        for (int i = obj->s; i < obj->r; i++) {
            x[i] = x[i] - x[i - obj->s];
        }
    }
    obj->pos = 0;
    return x[obj->pos++] >> 32;
}


static LFibDynDescr parse_parameters(const CallerAPI *intf)
{
    LFibDynDescr obj = {.name = NULL, .r = 0, .s = 0, .is_additive = 0};
    const char *param = intf->get_param();
    for (const LFibDynDescr *ptr = generators; ptr->name != NULL; ptr++) {
        if (!intf->strcmp(ptr->name, param)) {
            obj = *ptr;
        }        
    }
    return obj;
}

static void *create(const CallerAPI *intf)
{
    LFibDynDescr par = parse_parameters(intf);
    if (par.r == 0) {
        intf->printf("Unknown parameter %s\n", intf->get_param());
        return NULL;
    }
    intf->printf("LFib(%d,%d,%s)\n", par.r, par.s,
        par.is_additive ? "+" : "-");
    // Allocate buffers
    size_t len = sizeof(LFibDyn_State) + (par.r + 2) * sizeof(uint64_t);
    LFibDyn_State *obj = intf->malloc(len);
    obj->r = par.r; obj->s = par.s;
    obj->is_additive = par.is_additive;
    obj->pos = obj->r;
    obj->u = (uint64_t *) ( (char *) obj + sizeof(LFibDyn_State) );
    // pcg_rxs_m_xs64 for initialization
    uint64_t state = intf->get_seed64();
    for (int k = 0; k <= obj->r; k++) {
        obj->u[k] = pcg_bits64(&state);
    }
    return (void *) obj;
}


static const char *param_to_name(const CallerAPI *intf)
{    
    LFibDynDescr descr = parse_parameters(intf);
    if (descr.name == NULL) {
        return "LFib:unknown";
    } else {
        return descr.name;
    }
}


MAKE_UINT_PRNG("LFib", NULL, 32, param_to_name)
