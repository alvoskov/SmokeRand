/**
 * @file mt19937_64.c
 * @brief MT19937: 64-bit version of Mersenne Twister
 * @details It contains two versions of the generator: `brief` (default, faster
 * and more widespread) and `full` (slightly slower but with denser polynomials)
 *
 * References:
 *
 * 1. https://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/emt64.html
 * 2. Takuji Nishimura. 64-bit Mersenne Twisters // ACM Transactions on Modeling
 *    and Computer Simulation, Vol. 10, No. 4, October 2000.
 *    https://doi.org/10.1145/369534.369540
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

// Buffer size
#define NN 312
// Mask for higher 33 bits
#define UMASK 0xFFFFFFFF80000000ULL
// Mask for lower 31 bits
#define LMASK 0x7FFFFFFFULL

/**
 * @brief MT19937-64 (64-bit version of Mersenne twister) pseudorandom number
 * generator state.
 */
typedef struct {
    uint64_t x[NN];
    int pos;
} MT19937x64State;

//////////////////////////////////////////////////////////
///// Version with improved (less sparse) polynomial /////
//////////////////////////////////////////////////////////

void MT19937x64State_init(MT19937x64State *obj, uint64_t seed)
{
    static const uint64_t mul = 2862933555777941757ULL, c = 1ULL;
    for (int i = 0; i < NN; i++) {
        uint64_t ux = seed & 0xFFFFFFFF00000000ULL;
        seed = mul * seed + c;
        uint64_t lx = seed >> 32;
        seed = mul * seed + c;
        obj->x[i] = ux | lx;
    }
    obj->pos = NN;
}

static inline void mt_iter(uint64_t *x, int i0, int i1, int m0, int m1, int m2)
{
    static const uint64_t mag01[2] = {0ULL, 0xB3815B624FC82E2FULL};
    uint64_t x_new = (x[i0] & UMASK) | (x[i1] & LMASK);
    x[i0] = (x_new >> 1) ^ mag01[(size_t) (x_new & 1ULL)];
    x[i0] ^= x[m0] ^ x[m1] ^ x[m2];
}


static inline uint64_t MT19937x64State_next(MT19937x64State *obj)
{
    static const int M0 = 63, M1 = 151, M2 = 224;
    if (obj->pos >= NN) {
        for (int i = 0; i < NN - M2; i++)
            mt_iter(obj->x, i, i + 1, i + M0, i + M1, i + M2);
        for (int i = NN - M2; i < NN - M1; i++)
            mt_iter(obj->x, i, i + 1, i + M0, i + M1, i + M2 - NN);
        for (int i = NN - M1; i < NN - M0; i++)
            mt_iter(obj->x, i, i + 1, i + M0, i + M1 - NN, i + M2 - NN);
        for (int i = NN - M0; i < NN - 1; i++) 
            mt_iter(obj->x, i, i + 1, i + M0 - NN, i + M1 - NN, i + M2 - NN);
        mt_iter(obj->x, NN - 1, 0, M0 - 1, M1 - 1, M2 - 1);
        obj->pos = 0;
    }
    uint64_t x = obj->x[obj->pos++];
    // Tempering
    x ^= (x >> 26);
    x ^= (x << 17) & 0x599CFCBFCA660000ULL;
    x ^= (x << 33) & 0xFFFAAFFE00000000ULL;
    x ^= (x >> 39);
    return x;
}

static inline uint64_t get_bits_full_raw(void *state)
{
    return MT19937x64State_next(state);
}

MAKE_GET_BITS_WRAPPERS(full)




void *create_full(const GeneratorInfo *gi, const CallerAPI *intf)
{
    MT19937x64State *mt = intf->malloc(sizeof(MT19937x64State));
    MT19937x64State_init(mt, intf->get_seed64());
    (void) gi;
    return mt;
}


//////////////////////////////////////////////////////////////////////
///// Widespread "classical" version with the sparser polynomial /////
//////////////////////////////////////////////////////////////////////

void MT19937x64State_init_brief(MT19937x64State *obj, uint64_t seed)
{
    for (unsigned int i = 0; i < NN; i++) {
        obj->x[i] = seed;
        seed = 6364136223846793005ULL * (seed ^ (seed >> 62)) + (i + 1);
    }
    obj->pos = NN;
}


static inline void mt_iter_brief(uint64_t *x, int i0, int i1, int m0)
{
    static const uint64_t mag01[2] = {0ULL, 0xB5026F5AA96619E9ULL};
    uint64_t x_new = (x[i0] & UMASK) | (x[i1] & LMASK);
    x[i0] = x[m0] ^ (x_new >> 1) ^ mag01[(size_t) (x_new & 1ULL)];
}


static inline uint64_t MT19937x64State_next_brief(MT19937x64State *obj)
{
    static const int MM = 156;
    if (obj->pos >= NN) {
        for (int i = 0; i < NN - MM; i++)
            mt_iter_brief(obj->x, i, i + 1, i + MM);
        for (int i = NN - MM; i < NN - 1; i++)
            mt_iter_brief(obj->x, i, i + 1, i + (MM - NN));
        mt_iter_brief(obj->x, NN - 1, 0, MM - 1);
        obj->pos = 0;
    }
    uint64_t x = obj->x[obj->pos++];
    // Tempering
    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);
    return x;
}


static inline uint64_t get_bits_brief_raw(void *state)
{
    return MT19937x64State_next_brief(state);
}

MAKE_GET_BITS_WRAPPERS(brief)


void *create_brief(const GeneratorInfo *gi, const CallerAPI *intf)
{
    MT19937x64State *mt = intf->malloc(sizeof(MT19937x64State));
    MT19937x64State_init_brief(mt, intf->get_seed64());
    (void) gi;
    return mt;
}

//////////////////////
///// Interfaces /////
//////////////////////

static void *create(const CallerAPI *intf)
{
    intf->printf("'%s' not implemented\n", intf->get_param());
    return NULL;
}


int run_self_test(const CallerAPI *intf)
{
    // Obtained from the original implemenation
    static const uint64_t u_ref_brief[] = {
         9884911784069064543ULL,   66523890809771624ULL, 7206781173289933430ULL,
        14831977845650434642ULL, 5392944121040915686ULL};
    // Obtained in this work
    static const uint64_t u_ref_full[] = {
         1365578372932012986ULL, 1081426838276904543ULL, 4103721562241844714ULL,
         1360060612188662340ULL, 7500443010050942054ULL};

    int is_ok = 1;
    MT19937x64State *mt = intf->malloc(sizeof(MT19937x64State));
    // Checking the "brief" version
    MT19937x64State_init_brief(mt, 0x123456789ABCDEF);
    for (int i = 0; i < 995; i++) {
        (void) MT19937x64State_next_brief(mt);
    }
    intf->printf("%20s %20s\n", "x", "x_ref_brief");
    for (int i = 0; i < 5; i++) {
        uint64_t x = MT19937x64State_next_brief(mt);
        intf->printf("%20llu %20llu\n",
            (unsigned long long) x, (unsigned long long) u_ref_brief[i]);
        if (x != u_ref_brief[i]) {
            is_ok = 0;
        }
    }
    // Checkint the "full" version
    MT19937x64State_init(mt, 0x123456789ABCDEF);
    for (int i = 0; i < 995; i++) {
        (void) MT19937x64State_next(mt);
    }
    intf->printf("%20s %20s\n", "x", "x_ref_full");
    for (int i = 0; i < 5; i++) {
        uint64_t x = MT19937x64State_next(mt);
        intf->printf("%20llu %20llu\n",
            (unsigned long long) x, (unsigned long long) u_ref_full[i]);
        if (x != u_ref_full[i]) {
            is_ok = 0;
        }
    }
    // Clear memory and return the result
    intf->free(mt);
    return is_ok;
}

static const char description[] =
"MT19937-64\n: a 64-bit version of Mersenne twister.\n"
"The next param values are supported:\n"
"  brief     - a default and faster version with sparser polynomials.\n"
"  full      - a slower version with denser polynomials.\n";



int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->nbits = 64;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "brief") || !intf->strcmp(param, "")) {
        gi->name = "mt19937_64";
        gi->create = create_brief;
        gi->get_bits = get_bits_brief;
        gi->get_sum = get_sum_brief;
    } else if (!intf->strcmp(param, "full")) {
        gi->name = "mt19937_64:full";
        gi->create = create_full;
        gi->get_bits = get_bits_full;
        gi->get_sum = get_sum_full;
    } else {
        gi->name = "mt19937_64:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
        return 0;
    }
    return 1;
}
