#include "smokerand/lfsr_period.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

//////////////////////////////////////////
///// LargeInt class implemenetation /////
//////////////////////////////////////////

#define LARGEINT_SIZE 16

typedef struct {
    uint64_t x[LARGEINT_SIZE];
} LargeInt;


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

typedef struct {
    uint8_t *x;
    size_t n;
} LfsrMatrix;


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


LfsrMatrix LfsrMatrix_create_pow(const LfsrMatrix *a, unsigned long long y)
{
    if (y == 1) {
        return LfsrMatrix_clone(a);
    } else if (y == 2) {
        return LfsrMatrix_create_prod(a, a);
    } else if (y % 2 == 0) {
        LfsrMatrix half = LfsrMatrix_create_pow(a, y / 2);
        LfsrMatrix out = LfsrMatrix_create_prod(&half, &half);
        LfsrMatrix_destruct(&half);
        return out;
    } else {
        LfsrMatrix half = LfsrMatrix_create_pow(a, (y - 1) / 2);
        LfsrMatrix sq = LfsrMatrix_create_prod(&half, &half);
        LfsrMatrix out = LfsrMatrix_create_prod(a, &sq);
        LfsrMatrix_destruct(&sq);
        LfsrMatrix_destruct(&half);
        return out;
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
static uint8_t *malloc_buf = NULL;
void *(*malloc_original)(size_t len);


void *malloc_loghook(size_t len)
{
    malloc_ncalls++;
    malloc_nbytes += len;
    void *ptr = malloc_original(len);
    malloc_buf = ptr;
    return ptr;
}


GeneratorStateExt GeneratorStateExt_create(const GeneratorInfo *gen, const CallerAPI *intf)
{
    malloc_ncalls = 0;
    malloc_nbytes = 0;
    malloc_original = intf->malloc;
    CallerAPI intf_hooked = *intf;
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
    const unsigned long long niters = 65537;
    LfsrMatrix mat = GeneratorStateExt_get_matrix(obj, 1);
    LfsrMatrix matp_exp = GeneratorStateExt_get_matrix(obj, niters);
    LfsrMatrix matp_calc = LfsrMatrix_create_pow(&mat, niters);

    const int is_lfsr = LfsrMatrix_are_equal(&matp_exp, &matp_calc);

    LfsrMatrix_destruct(&mat);
    LfsrMatrix_destruct(&matp_exp);
    LfsrMatrix_destruct(&matp_calc);

    return is_lfsr;
}


void GeneratorStateExt_destruct(GeneratorStateExt *obj)
{
    GeneratorState_destruct(&obj->state);
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

    // Restore the generator matrix
    //LfsrMatrix mat = GeneratorStateExt_get_matrix(&ext, 1);
    //LfsrMatrix_print(&mat);
    //LfsrMatrix_destruct(&mat);

    LargeInt period = LargeInt_from_pow2((unsigned int) (ext.nbytes * 8));
    LargeInt_subtract_u64(&period, 1U);
    LargeInt_print_hex(&period); intf->printf("\n");
    printf("-----------\n");



/*
    LfsrMatrix matp_exp = GeneratorStateExt_get_matrix(&ext, 65537);
    LfsrMatrix_print(&matp_exp);
    LfsrMatrix_destruct(&matp_exp);
    printf("-----------\n");
    LfsrMatrix matp_calc = LfsrMatrix_create_pow(&mat, 65537);
    LfsrMatrix_print(&matp_calc);
    LfsrMatrix_destruct(&matp_calc);

}
*/

    GeneratorStateExt_destruct(&ext);
    printf("-----------\n");
    return BATTERY_PASSED;

}
