/**
 * @file lfsr_period.h
 * @brief Simple tools for proving the LFSR period using the theoretical
 * (algebraic) methods. Allow to check small xorshift-style generators with
 * states up to 256 bits.
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_LFSR_PERIOD_H
#define __SMOKERAND_LFSR_PERIOD_H
#include "smokerand/core.h"

enum {
    LFSR_NBYTES_DEFAULT = 0
};

typedef struct {
    int check_validity;
} LfsrPeriodOptions;


typedef enum {
    LFSR_PERIOD_MAX     = 0,
    LFSR_PERIOD_NOT_MAX = 1,
    LFSR_PERIOD_ERROR   = 2
} LfsrPeriodResult;

typedef struct {
    GeneratorState state;
    size_t nbytes; ///< State size in bytes
}  GeneratorStateExt;

#define LARGEINT_SIZE 16

/**
 * @brief Wide integers required for the exponents.
 */
typedef struct {
    uint64_t x[LARGEINT_SIZE];
} LargeInt;

/**
 * @brief Square matrix over the GF(2) field that represents the LFSR
 * transition function.
 */
typedef struct {
    uint8_t *x; ///< Pointer to the data
    size_t n;   ///< Matrix size (n x n)
} LfsrMatrix;


/**
 * @brief LFSR related polynomials over the GF(2) field: e.g. characteristic
 * polynomials, jump polynomials etc.
 * @details It is packed in 64-bit words, the highest bit is omitted.
 */
typedef struct {
    uint64_t *w64; ///< Polynomial packed in 64-bit words
    size_t degree; ///< Polynomial degree
    size_t nwords; ///< Number of 64-bit words used for storage
} LfsrPoly;


// GeneratorStateExt functions
GeneratorStateExt
GeneratorStateExt_create(const GeneratorInfo *gen, const CallerAPI *intf);
GeneratorStateExt
GeneratorStateExt_create_sized(const GeneratorInfo *gen, const CallerAPI *intf, size_t nbytes);
int GeneratorStateExt_is_lfsr(GeneratorStateExt *obj);
int GeneratorStateExt_has_counters(GeneratorStateExt *obj);
int GeneratorStateExt_is_valid(GeneratorStateExt *obj, const CallerAPI *intf);
void GeneratorStateExt_destruct(GeneratorStateExt *obj);
LfsrMatrix
GeneratorStateExt_get_matrix(GeneratorStateExt *obj, unsigned long long niters);
LfsrMatrix GeneratorStateExt_get_krylov_matrix(GeneratorStateExt *obj);
LfsrPoly GeneratorStateExt_get_poly(GeneratorStateExt *obj);
LfsrPoly GeneratorStateExt_get_jump_poly_pow2(GeneratorStateExt *obj, unsigned int p);
void GeneratorStateExt_apply_jump_poly(GeneratorStateExt *obj, const LfsrPoly *jump_poly);
void GeneratorStateExt_make_jump_pow2(GeneratorStateExt *obj, unsigned int p);

// LargeInt functions
LargeInt LargeInt_from_u64(uint64_t x);
LargeInt LargeInt_from_pow2(unsigned int p);
void LargeInt_subtract_u64(LargeInt *obj, uint64_t val);
int LargeInt_is_u64(const LargeInt *obj, uint64_t val);
void LargeInt_div_2(LargeInt *obj);
unsigned int LargeInt_get_nbits(const LargeInt *obj);
void LargeInt_print_hex(const LargeInt *obj, const CallerAPI *intf);


static inline int LargeInt_is_odd(const LargeInt *obj)
{
    return (obj->x[0] & 1) == 1;
}

static inline int LargeInt_getbit(const LargeInt *obj, unsigned int ind)
{
    return (obj->x[ind >> 6] & (1ULL << (ind & 0x3FU))) ? 1 : 0;
}

// LfsrPoly functions
LfsrPoly LfsrPoly_create(size_t degree);
void LfsrPoly_print(const LfsrPoly *obj, const CallerAPI *intf);
void LfsrPoly_print_hex(const LfsrPoly *obj, const CallerAPI *intf);
void LfsrPoly_destruct(LfsrPoly *obj);
void LfsrPoly_mulx(LfsrPoly *a, const LfsrPoly *charpoly);
void LfsrPoly_mulmod(LfsrPoly *a, const LfsrPoly *b, const LfsrPoly *charpoly);
LfsrPoly LfsrPoly_jumppoly_ce(const LfsrPoly *charpoly, uint64_t c, uint32_t e);

static inline void LfsrPoly_setbit(LfsrPoly *obj, size_t ind)
{
    obj->w64[ind >> 6] |= (1ULL << (ind & 0x3FU));
}

static inline int LfsrPoly_getbit(const LfsrPoly *obj, size_t ind)
{
    return (obj->w64[ind >> 6] & (1ULL << (ind & 0x3FU))) ? 1 : 0;
}

// LfsrMatrix functions
LfsrMatrix LfsrMatrix_create(size_t n);
LfsrMatrix LfsrMatrix_create_prod(const LfsrMatrix *a, const LfsrMatrix *b);
int LfsrMatrix_are_equal(const LfsrMatrix *a, const LfsrMatrix *b);
int LfsrMatrix_is_eye(const LfsrMatrix *a);
LfsrMatrix LfsrMatrix_clone(const LfsrMatrix *obj);
void LfsrMatrix_destruct(LfsrMatrix *obj);
LfsrMatrix LfsrMatrix_create_pow(const LfsrMatrix *x, const LargeInt *e);
void LfsrMatrix_print(const LfsrMatrix *obj, const CallerAPI *intf);
int LfsrMatrix_is_period_possible(const LfsrMatrix *mat, const LargeInt *period);
LfsrMatrix LfsrMatrix_get_krylov_matrix(const LfsrMatrix *mat);
LfsrPoly LfsrMatrix_krylov_to_charpoly(LfsrMatrix *mat);

static inline void LfsrMatrix_setbit(LfsrMatrix *obj, size_t i, size_t j, uint8_t val)
{
    obj->x[i*obj->n + j] = (val == 0) ? 0 : 1;
}

static inline uint8_t LfsrMatrix_getbit(const LfsrMatrix *obj, size_t i, size_t j)
{
    return obj->x[i*obj->n + j];
}


// LfsrPeriodResult function
void LfsrPeriodResult_print(const CallerAPI *intf, LfsrPeriodResult res);


// Tests and batteries
LfsrPeriodResult lfsr_period_test(GeneratorStateExt *ext, const CallerAPI *intf,
    const LfsrPeriodOptions *opts);
BatteryExitCode battery_lfsr_period(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts);

#endif // __SMOKERAND_LFSR_PERIOD_H
