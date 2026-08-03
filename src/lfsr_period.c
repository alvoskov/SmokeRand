/**
 * @file lfsr_period.c
 * @brief Simple tools for proving the LFSR period using the theoretical
 * (algebraic) methods. Allow to check small xorshift-style generators with
 * states up to 256 bits.
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/lfsr_period.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define LARGEINT_SIZE 16

typedef struct {
    uint64_t x[LARGEINT_SIZE];
} LargeInt;


typedef struct {
    uint8_t *x;
    size_t n;
} LfsrMatrix;


// 2**32 - 1 =  [3, 5, 17, 257, 65537]
static const LargeInt lfsr32_divs[] = {
    {{0x0000000055555555}}, // 0x55555555
    {{0x0000000033333333}}, // 0x33333333
    {{0x000000000F0F0F0F}}, // 0xF0F0F0F
    {{0x0000000000FF00FF}}, // 0xFF00FF
    {{0x000000000000FFFF}}, // 0xFFFF
    {{0x0}}
};

// 2**64 - 1 =  [3, 5, 17, 257, 641, 65537, 6700417]
static const LargeInt lfsr64_divs[] = {
    {{0x5555555555555555}}, // 0x5555555555555555
    {{0x3333333333333333}}, // 0x3333333333333333
    {{0x0F0F0F0F0F0F0F0F}}, // 0xF0F0F0F0F0F0F0F
    {{0x00FF00FF00FF00FF}}, // 0xFF00FF00FF00FF
    {{0x00663D80FF99C27F}}, // 0x663D80FF99C27F
    {{0x0000FFFF0000FFFF}}, // 0xFFFF0000FFFF
    {{0x00000280FFFFFD7F}}, // 0x280FFFFFD7F
    {{0x0}}
};

// 2**128 - 1 =  [3, 5, 17, 257, 641, 65537, 274177, 6700417, 67280421310721]
static const LargeInt lfsr128_divs[] = {
    {{0x5555555555555555, 0x5555555555555555}}, // 0x55555555555555555555555555555555
    {{0x3333333333333333, 0x3333333333333333}}, // 0x33333333333333333333333333333333
    {{0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F}}, // 0xF0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F
    {{0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF}}, // 0xFF00FF00FF00FF00FF00FF00FF00FF
    {{0x00663D80FF99C27F, 0x00663D80FF99C27F}}, // 0x663D80FF99C27F00663D80FF99C27F
    {{0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF}}, // 0xFFFF0000FFFF0000FFFF0000FFFF
    {{0xFFFFC2CF0E632EFF, 0x00003D30F19CD100}}, // 0x3D30F19CD100FFFFC2CF0E632EFF
    {{0x00000280FFFFFD7F, 0x00000280FFFFFD7F}}, // 0x280FFFFFD7F00000280FFFFFD7F
    {{0xFFFFFFFFFFFBD0FF, 0x0000000000042F00}}, // 0x42F00FFFFFFFFFFFBD0FF
    {{0x0}}
};

// 2**256 - 1 =  [3, 5, 17, 257, 641, 65537, 274177, 6700417, 67280421310721, 59649589127497217, 5704689200685129054721]
static const LargeInt lfsr256_divs[] = {
    {{0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555}}, // 0x5555555555555555555555555555555555555555555555555555555555555555
    {{0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333}}, // 0x3333333333333333333333333333333333333333333333333333333333333333
    {{0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F}}, // 0xF0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F
    {{0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF}}, // 0xFF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF
    {{0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F}}, // 0x663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F
    {{0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF}}, // 0xFFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF
    {{0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100}}, // 0x3D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF
    {{0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F}}, // 0x280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F
    {{0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00}}, // 0x42F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF
    {{0xBF88A4B733CD45FF, 0xFFFFFFFFFFFFFECA, 0x40775B48CC32BA00, 0x0000000000000135}}, // 0x13540775B48CC32BA00FFFFFFFFFFFFFECABF88A4B733CD45FF
    {{0xFF2C1503C50EB9FF, 0xFFFFFFFFFFFFFFFF, 0x00D3EAFC3AF14600}}, // 0xD3EAFC3AF14600FFFFFFFFFFFFFFFFFF2C1503C50EB9FF
    {{0x0}}
};


static const LargeInt *get_lfsr_divs(size_t n)
{
    if (n == 32) {
        return lfsr32_divs;
    } else if (n == 64) {
        return lfsr64_divs;
    } else if (n == 128) {
        return lfsr128_divs;
    } else if (n == 256) {
        return lfsr256_divs;
    } else {
        return NULL;
    }
}


//////////////////////////////////////////
///// LargeInt class implemenetation /////
//////////////////////////////////////////


LargeInt LargeInt_from_u64(uint64_t x)
{
    LargeInt obj;
    obj.x[0] = x;
    for (size_t i = 1; i < LARGEINT_SIZE; i++) {
        obj.x[i] = 0;
    }
    return obj;
}


LargeInt LargeInt_from_pow2(unsigned int p)
{
    LargeInt obj;
    for (size_t i = 0; i < LARGEINT_SIZE; i++) {
        obj.x[i] = 0;
    }
    if (p < LARGEINT_SIZE * 64) {
        obj.x[p >> 6] = 1ULL << (p & 0x3FU);
    }
    return obj;
}

int LargeInt_getbit(const LargeInt *obj, unsigned int ind)
{
    return (obj->x[ind >> 6] & (1ULL << (ind & 0x3FU))) ? 1 : 0;
}


void LargeInt_subtract_u64(LargeInt *obj, uint64_t val)
{
    for (size_t i = 0; i < LARGEINT_SIZE - 1; i++) {
        const uint64_t xi_old = obj->x[i];
        obj->x[i] -= val;
        if (obj->x[i] <= xi_old) {
            break;
        } else {
            val = 1;
        }
    }
}


int LargeInt_is_odd(const LargeInt *obj)
{
    return (obj->x[0] & 1) == 1;
}


int LargeInt_is_u64(const LargeInt *obj, uint64_t val)
{
    for (size_t i = LARGEINT_SIZE - 1; i >= 1; i--) {
        if (obj->x[i] != 0) {
            return 0;
        }
    }
    return obj->x[0] == val;
}



void LargeInt_div_2(LargeInt *obj)
{
    obj->x[0] >>= 1;
    for (size_t i = 1; i < LARGEINT_SIZE; i++) {
        const uint64_t hi = (obj->x[i] & 1U) << 63;
        obj->x[i] >>= 1;
        obj->x[i - 1] |= hi;
    }
}

unsigned int LargeInt_get_nbits(const LargeInt *obj)
{
    unsigned int nbits = LARGEINT_SIZE * 64;
    for (size_t i = LARGEINT_SIZE; i-- != 0; ) {
        if (obj->x[i] == 0) {
            nbits -= 64;
        } else {
            const uint64_t mask = 0x8000000000000000U;
            uint64_t xi = obj->x[i];
            unsigned int leading_zeros = 0;
            while ((xi & mask) == 0) {
                xi <<= 1;
                leading_zeros++;
            }
            nbits -= leading_zeros;
            break;
        }
    }
    return nbits;
}


void LargeInt_print_hex(const LargeInt *obj)
{
    int is_inside = 0;
    for (size_t i = LARGEINT_SIZE; i-- != 0; ) {
        if (!is_inside && obj->x[i] != 0) {
            is_inside = 1;
            printf("%16.16llX", (unsigned long long) obj->x[i]);
        } else if (is_inside) {
            printf(".%16.16llX", (unsigned long long) obj->x[i]);
        }
    }
}



///////////////////////////////////////////
///// LfsrMatrix class implementation /////
///////////////////////////////////////////

LfsrMatrix LfsrMatrix_create(size_t n)
{
    LfsrMatrix obj;
    obj.x = calloc(n * n, sizeof(uint8_t));
    obj.n = n;
    return obj;
}

inline void LfsrMatrix_setbit(LfsrMatrix *obj, size_t i, size_t j, uint8_t val)
{
    obj->x[i*obj->n + j] = (val == 0) ? 0 : 1;
}

inline uint8_t LfsrMatrix_getbit(const LfsrMatrix *obj, size_t i, size_t j)
{
    return obj->x[i*obj->n + j];
}


LfsrMatrix LfsrMatrix_create_prod(const LfsrMatrix *a, const LfsrMatrix *b)
{
    // TODO: add some checks!
    const size_t n = a->n;
    LfsrMatrix c = LfsrMatrix_create(n);
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            uint8_t cij = 0;
            for (size_t k = 0; k < n; k++) {
                const uint8_t aik = LfsrMatrix_getbit(a, i, k);
                const uint8_t bkj = LfsrMatrix_getbit(b, k, j);
                cij ^= aik & bkj;
            }
            LfsrMatrix_setbit(&c, i, j, cij);
        }
    }
    return c;
}

int LfsrMatrix_are_equal(const LfsrMatrix *a, const LfsrMatrix *b)
{
    if (a->n != b->n) {
        return 0;
    }
    for (size_t i = 0; i < a->n; i++) {
        for (size_t j = 0; j < a->n; j++) {
            if (LfsrMatrix_getbit(a, i, j) != LfsrMatrix_getbit(b, i, j)) {
                return 0;
            }
        }
    }
    return 1;
}


int LfsrMatrix_is_eye(const LfsrMatrix *a)
{
    for (size_t i = 0; i < a->n; i++) {
        for (size_t j = 0; j < a->n; j++) {
            const uint8_t dij = (i == j) ? 1 : 0;
            if (LfsrMatrix_getbit(a, i, j) != dij) {
                return 0;
            }
        }
    }
    return 1;
}


LfsrMatrix LfsrMatrix_clone(const LfsrMatrix *obj)
{
    const size_t n = obj->n;
    LfsrMatrix cpy = LfsrMatrix_create(n);
    memcpy(cpy.x, obj->x, n*n);
    return cpy;
}


void LfsrMatrix_destruct(LfsrMatrix *obj)
{
    free(obj->x);
}


LfsrMatrix LfsrMatrix_create_pow(const LfsrMatrix *x, const LargeInt *e)
{
    if (LargeInt_is_u64(e, 1)) {
        return LfsrMatrix_clone(x);
    } else if (LargeInt_is_u64(e, 2)) {
        return LfsrMatrix_create_prod(x, x);
    } else {
        LfsrMatrix y = LfsrMatrix_clone(x);
        const unsigned int nbits = LargeInt_get_nbits(e);
        for (unsigned int i = nbits - 1; i-- != 0; ) {
            // y = y * y
            LfsrMatrix sq = LfsrMatrix_create_prod(&y, &y);
            LfsrMatrix_destruct(&y);
            y = sq;
            if (LargeInt_getbit(e, i)) {
                // y = y * x
                LfsrMatrix yx = LfsrMatrix_create_prod(&y, x);
                LfsrMatrix_destruct(&y);
                y = yx;
            }
        }
        return y;
    }
}


void LfsrMatrix_print(const LfsrMatrix *obj)
{
    for (size_t i = 0; i < obj->n; i++) {
        for (size_t j = 0; j < obj->n; j++) {
            printf("%s", LfsrMatrix_getbit(obj, i, j) ? "X" : ".");
        }
        printf("\n");
    }
}

//////////////////////////////////////////////////
///// GeneratorStateExt class implementation /////
//////////////////////////////////////////////////

typedef struct {
    GeneratorState state;
    size_t nbytes; ///< State size in bytes
}  GeneratorStateExt;

static unsigned int malloc_ncalls = 0;
static size_t malloc_nbytes = 0;
static CallerAPI intf_hooked;
void *(*malloc_original)(size_t len);


void *malloc_loghook(size_t len)
{
    malloc_ncalls++;
    malloc_nbytes += len;
    return malloc_original(len);
}


GeneratorStateExt GeneratorStateExt_create(const GeneratorInfo *gen, const CallerAPI *intf)
{
    malloc_ncalls = 0;
    malloc_nbytes = 0;
    malloc_original = intf->malloc;
    intf_hooked = *intf;
    intf_hooked.malloc = malloc_loghook;
    GeneratorStateExt ext;
    ext.state = GeneratorState_create(gen, &intf_hooked);
    ext.nbytes = (malloc_ncalls == 1) ? malloc_nbytes : 0;
    return ext;
}




LfsrMatrix GeneratorStateExt_get_matrix(GeneratorStateExt *obj, unsigned long long niters)
{
    const size_t nbits = obj->nbytes * 8;
    LfsrMatrix mat = LfsrMatrix_create(nbits);
    uint8_t *buf = obj->state.state;
    for (size_t i = 0; i < nbits; i++) {
        memset(buf, 0, obj->nbytes);
        buf[i >> 3] = 1U << (i & 0x7U);
        for (unsigned long long j = 0; j < niters; j++) {
            (void) obj->state.gi->get_bits(obj->state.state);
        }
        for (size_t j = 0; j < nbits; j++) {
            const uint8_t b = buf[j >> 3] & (1U << (j & 0x7U));
            LfsrMatrix_setbit(&mat, i, j, b);
        }
    }
    return mat;
}

/**
 * @brief Check if the PRNG is LFSR or not.
 */
int GeneratorStateExt_is_lfsr(GeneratorStateExt *obj)
{
    const unsigned long niters = 65537;
    const LargeInt niters_lint = LargeInt_from_u64(niters);
    LfsrMatrix mat = GeneratorStateExt_get_matrix(obj, 1);
    LfsrMatrix matp_exp = GeneratorStateExt_get_matrix(obj, niters);
    LfsrMatrix matp_calc = LfsrMatrix_create_pow(&mat, &niters_lint);

    const int is_lfsr = LfsrMatrix_are_equal(&matp_exp, &matp_calc);

    LfsrMatrix_destruct(&mat);
    LfsrMatrix_destruct(&matp_exp);
    LfsrMatrix_destruct(&matp_calc);

    return is_lfsr;
}


void GeneratorStateExt_destruct(GeneratorStateExt *obj)
{
    GeneratorState_destruct(&(obj->state));
}

////////////////////////////////////
///// Battery implemenentation /////
////////////////////////////////////

BatteryExitCode battery_lfsr_period(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts)
{

    intf->printf("LFSR period checker\n");
    if (gen->parent != NULL) {
        intf->printf("  Error: cannot analyze an enveloped generator");
        return BATTERY_ERROR;
    }

    GeneratorStateExt ext = GeneratorStateExt_create(gen, intf);

    intf->printf("  malloc: nbytes = %llu; ptr = 0x%llu\n",
        (unsigned long long) ext.nbytes,
        (unsigned long long) ext.state.state);
    (void) opts;

    // Check if the PRNG is LFSR (by empirical testing)
    if (GeneratorStateExt_is_lfsr(&ext)) {
        intf->printf("  The PRNG is probably a LFSR\n");
    } else {
        intf->printf("  The PRNG is not a LFSR\n");
        GeneratorStateExt_destruct(&ext);
        return BATTERY_FAILED;
    }

    // Calculate the maximal period
    LargeInt period = LargeInt_from_pow2((unsigned int) (ext.nbytes * 8));
    LargeInt_subtract_u64(&period, 1U);
    LargeInt_print_hex(&period); intf->printf("\n");

    // Check if the maximal period is possible
    intf->printf("Beginning the period proof\n");
    LfsrMatrix mat = GeneratorStateExt_get_matrix(&ext, 1);
    LfsrMatrix matp = LfsrMatrix_create_pow(&mat, &period);
    if (LfsrMatrix_is_eye(&matp)) {
        intf->printf("  LFSR period can be maximal\n");
    } else {
        intf->printf("  LFSR period cannot be maximal\n");
        LfsrMatrix_destruct(&mat);
        LfsrMatrix_destruct(&matp);
        GeneratorStateExt_destruct(&ext);
        return BATTERY_FAILED;
    }
    const LargeInt *lfsr_divs = get_lfsr_divs(ext.nbytes * 8);
    if (lfsr_divs == NULL) {
        intf->printf("  The tables are absent for this LFSR size\n");
    } else {
        int is_full = 1;
        for (const LargeInt *d = lfsr_divs; !LargeInt_is_u64(d, 0); d++) {
            intf->printf("  Divisor (%4u bits): ", LargeInt_get_nbits(d));
            LargeInt_print_hex(d);
            LfsrMatrix matd = LfsrMatrix_create_pow(&mat, d);
            if (LfsrMatrix_is_eye(&matd)) {
                intf->printf("<<< FAIL\n");
                is_full = 0;
            } else {
                intf->printf(" OK\n");
            }
            LfsrMatrix_destruct(&matd);
        }
        if (is_full) {
            intf->printf("The period is full\n");
        } else {
            intf->printf("The period is not full\n");
        }
    }

    LfsrMatrix_destruct(&mat);
    LfsrMatrix_destruct(&matp);
    GeneratorStateExt_destruct(&ext);
    return BATTERY_PASSED;

}
