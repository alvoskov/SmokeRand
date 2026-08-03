/**
 * @file lfsr_period.c
 * @brief Simple tools for proving the LFSR period using the theoretical
 * (algebraic) methods. Allow to check small xorshift-style generators with
 * states up to 1024 bits.
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
 * @brief `2**32 - 1 =  [3, 5, 17, 257, 65537]`
 */
static const LargeInt lfsr32_exps[] = {
    {{0x0000000055555555}}, // 0x55555555
    {{0x0000000033333333}}, // 0x33333333
    {{0x000000000F0F0F0F}}, // 0xF0F0F0F
    {{0x0000000000FF00FF}}, // 0xFF00FF
    {{0x000000000000FFFF}}, // 0xFFFF
    {{0x0}}
};

/**
 * @brief `2**64 - 1 =  [3, 5, 17, 257, 641, 65537, 6700417]`
 */
static const LargeInt lfsr64_exps[] = {
    {{0x5555555555555555}}, // 0x5555555555555555
    {{0x3333333333333333}}, // 0x3333333333333333
    {{0x0F0F0F0F0F0F0F0F}}, // 0xF0F0F0F0F0F0F0F
    {{0x00FF00FF00FF00FF}}, // 0xFF00FF00FF00FF
    {{0x00663D80FF99C27F}}, // 0x663D80FF99C27F
    {{0x0000FFFF0000FFFF}}, // 0xFFFF0000FFFF
    {{0x00000280FFFFFD7F}}, // 0x280FFFFFD7F
    {{0x0}}
};

/**
 * @brief `2**96 - 1 =  [3, 5, 7, 13, 17, 97, 193, 241, 257, 673, 65537, 22253377]`
 */
static const LargeInt lfsr96_exps[] = {
    {{0x5555555555555555, 0x0000000055555555}}, // 0x555555555555555555555555
    {{0x3333333333333333, 0x0000000033333333}}, // 0x333333333333333333333333
    {{0x9249249249249249, 0x0000000024924924}}, // 0x249249249249249249249249
    {{0xB13B13B13B13B13B, 0x0000000013B13B13}}, // 0x13B13B13B13B13B13B13B13B
    {{0x0F0F0F0F0F0F0F0F, 0x000000000F0F0F0F}}, // 0xF0F0F0F0F0F0F0F0F0F0F0F
    {{0x5C5F02A3A0FD5C5F, 0x0000000002A3A0FD}}, // 0x2A3A0FD5C5F02A3A0FD5C5F
    {{0x8F40FEAC6F6B70BF, 0x0000000001539094}}, // 0x15390948F40FEAC6F6B70BF
    {{0x0FEF010FEF010FEF, 0x00000000010FEF01}}, // 0x10FEF010FEF010FEF010FEF
    {{0x00FF00FF00FF00FF, 0x0000000000FF00FF}}, // 0xFF00FF00FF00FF00FF00FF
    {{0x9E9F006160FF9E9F, 0x00000000006160FF}}, // 0x6160FF9E9F006160FF9E9F
    {{0x0000FFFF0000FFFF, 0x000000000000FFFF}}, // 0xFFFF0000FFFF0000FFFF
    {{0x00C0FFFFFF3EFF3F, 0x00000000000000C1}}, // 0xC100C0FFFFFF3EFF3F
    {{0x0}}
};

/**
 * @brief `2**128 - 1 =  [3, 5, 17, 257, 641, 65537, 274177, 6700417, 67280421310721]`
 */
static const LargeInt lfsr128_exps[] = {
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

/**
 * @brief `2**160 - 1 =  [3, 5, 11, 17, 31, 41, 257, 61681, 65537, 414721, 4278255361, 44479210368001]`
 */
static const LargeInt lfsr160_exps[] = {
    {{0x5555555555555555, 0x5555555555555555, 0x0000000055555555}}, // 0x5555555555555555555555555555555555555555
    {{0x3333333333333333, 0x3333333333333333, 0x0000000033333333}}, // 0x3333333333333333333333333333333333333333
    {{0xD1745D1745D1745D, 0x5D1745D1745D1745, 0x000000001745D174}}, // 0x1745D1745D1745D1745D1745D1745D1745D1745D
    {{0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x000000000F0F0F0F}}, // 0xF0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F
    {{0x1084210842108421, 0x2108421084210842, 0x0000000008421084}}, // 0x842108421084210842108421084210842108421
    {{0x7063E7063E7063E7, 0xE7063E7063E7063E, 0x00000000063E7063}}, // 0x63E7063E7063E7063E7063E7063E7063E7063E7
    {{0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x0000000000FF00FF}}, // 0xFF00FF00FF00FF00FF00FF00FF00FF00FF00FF
    {{0x0FFFEF00010FFFEF, 0xEF00010FFFEF0001, 0x0000000000010FFF}}, // 0x10FFFEF00010FFFEF00010FFFEF00010FFFEF
    {{0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x000000000000FFFF}}, // 0xFFFF0000FFFF0000FFFF0000FFFF0000FFFF
    {{0xD78BB803347653FF, 0x47FCCB89AC00FFFF, 0x0000000000002874}}, // 0x287447FCCB89AC00FFFFD78BB803347653FF
    {{0x000100FFFFFFFEFF, 0x00FFFFFFFEFF0000, 0x0000000000000001}}, // 0x100FFFFFFFEFF0000000100FFFFFFFEFF
    {{0xFFFFFFF9ABF8ABFF, 0x000654075400FFFF}}, // 0x654075400FFFFFFFFFFF9ABF8ABFF
    {{0x0}}
};

/**
 * @brief `2**192 - 1 =  [3, 5, 7, 13, 17, 97, 193, 241, 257, 641, 673, 65537, 6700417, 22253377, 18446744069414584321]`
 */
static const LargeInt lfsr192_exps[] = {
    {{0x5555555555555555, 0x5555555555555555, 0x5555555555555555}}, // 0x555555555555555555555555555555555555555555555555
    {{0x3333333333333333, 0x3333333333333333, 0x3333333333333333}}, // 0x333333333333333333333333333333333333333333333333
    {{0x9249249249249249, 0x4924924924924924, 0x2492492492492492}}, // 0x249249249249249249249249249249249249249249249249
    {{0xB13B13B13B13B13B, 0x3B13B13B13B13B13, 0x13B13B13B13B13B1}}, // 0x13B13B13B13B13B13B13B13B13B13B13B13B13B13B13B13B
    {{0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F}}, // 0xF0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F
    {{0x5C5F02A3A0FD5C5F, 0xA0FD5C5F02A3A0FD, 0x02A3A0FD5C5F02A3}}, // 0x2A3A0FD5C5F02A3A0FD5C5F02A3A0FD5C5F02A3A0FD5C5F
    {{0x8F40FEAC6F6B70BF, 0x6F6B70BF01539094, 0x015390948F40FEAC}}, // 0x15390948F40FEAC6F6B70BF015390948F40FEAC6F6B70BF
    {{0x0FEF010FEF010FEF, 0xEF010FEF010FEF01, 0x010FEF010FEF010F}}, // 0x10FEF010FEF010FEF010FEF010FEF010FEF010FEF010FEF
    {{0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF}}, // 0xFF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF
    {{0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F}}, // 0x663D80FF99C27F00663D80FF99C27F00663D80FF99C27F
    {{0x9E9F006160FF9E9F, 0x60FF9E9F006160FF, 0x006160FF9E9F0061}}, // 0x6160FF9E9F006160FF9E9F006160FF9E9F006160FF9E9F
    {{0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF}}, // 0xFFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF
    {{0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F}}, // 0x280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F
    {{0x00C0FFFFFF3EFF3F, 0xFF3EFF3F000000C1, 0x000000C100C0FFFF}}, // 0xC100C0FFFFFF3EFF3F000000C100C0FFFFFF3EFF3F
    {{0xFFFFFFFEFFFFFFFF, 0x00000000FFFFFFFF, 0x0000000000000001}}, // 0x100000000FFFFFFFFFFFFFFFEFFFFFFFF
    {{0x0}}
};

/**
 * @brief `2**256 - 1 =  [3, 5, 17, 257, 641, 65537, 274177, 6700417, 67280421310721, 59649589127497217, 5704689200685129054721]`
 */
static const LargeInt lfsr256_exps[] = {
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

/**
 * @brief `2**512 - 1 =  [3, 5, 17, 257, 641, 65537, 274177, 6700417, 67280421310721, 1238926361552897, 59649589127497217, 5704689200685129054721, 93461639715357977769163558199606896584051237541638188580280321]`
 */
static const LargeInt lfsr512_exps[] = {
    {{0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555}}, // 0x55555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
    {{0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333}}, // 0x33333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333
    {{0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F}}, // 0xF0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F
    {{0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF}}, // 0xFF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF
    {{0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F}}, // 0x663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F
    {{0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF}}, // 0xFFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF
    {{0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100}}, // 0x3D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF
    {{0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F}}, // 0x280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F
    {{0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00}}, // 0x42F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF
    {{0xB6455F45D36EE7FF, 0x911C9C8354DA792F, 0xB3A7A570A38F8C1C, 0xFFFFFFFFFFFFC5D6, 0x49BAA0BA2C911800, 0x6EE3637CAB2586D0, 0x4C585A8F5C7073E3, 0x0000000000003A29}}, // 0x3A294C585A8F5C7073E36EE3637CAB2586D049BAA0BA2C911800FFFFFFFFFFFFC5D6B3A7A570A38F8C1C911C9C8354DA792FB6455F45D36EE7FF
    {{0xBF88A4B733CD45FF, 0xFFFFFFFFFFFFFECA, 0x40775B48CC32BA00, 0x0000000000000135, 0xBF88A4B733CD45FF, 0xFFFFFFFFFFFFFECA, 0x40775B48CC32BA00, 0x0000000000000135}}, // 0x13540775B48CC32BA00FFFFFFFFFFFFFECABF88A4B733CD45FF000000000000013540775B48CC32BA00FFFFFFFFFFFFFECABF88A4B733CD45FF
    {{0xFF2C1503C50EB9FF, 0xFFFFFFFFFFFFFFFF, 0x00D3EAFC3AF14600, 0x0000000000000000, 0xFF2C1503C50EB9FF, 0xFFFFFFFFFFFFFFFF, 0x00D3EAFC3AF14600}}, // 0xD3EAFC3AF14600FFFFFFFFFFFFFFFFFF2C1503C50EB9FF000000000000000000D3EAFC3AF14600FFFFFFFFFFFFFFFFFF2C1503C50EB9FF
    {{0xFFFB9933FA5117FF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x000466CC05AEE800}}, // 0x466CC05AEE800FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFB9933FA5117FF
    {{0x0}}
};

/**
 * @brief `2**1024 - 1 =  [3, 5, 17, 257, 641, 65537, 274177, 2424833, 6700417, 67280421310721, 1238926361552897, 59649589127497217, 5704689200685129054721, 7455602825647884208337395736200454918783366342657, 93461639715357977769163558199606896584051237541638188580280321, 741640062627530801524787141901937474059940781097519023905821316144415759504705008092818711693940737]`
 */
static const LargeInt lfsr1024_exps[] = {
    {{0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555, 0x5555555555555555}}, // 0x5555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555555
    {{0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333, 0x3333333333333333}}, // 0x3333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333
    {{0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F}}, // 0xF0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F0F
    {{0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF}}, // 0xFF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FF
    {{0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F, 0x00663D80FF99C27F}}, // 0x663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F00663D80FF99C27F
    {{0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF}}, // 0xFFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF0000FFFF
    {{0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100, 0xFFFFC2CF0E632EFF, 0x00003D30F19CD100}}, // 0x3D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF00003D30F19CD100FFFFC2CF0E632EFF
    {{0xC5DCFAA70024FFFF, 0xE41F23591AB8670F, 0x1C0C1AEAEA84077F, 0x891278D5FCBBF240, 0x398A741103C83054, 0x121DC828B566AEFD, 0xA1557142E14361B2, 0xFFFFF914C1EAAEA6, 0x3A230558FFDB0000, 0x1BE0DCA6E54798F0, 0xE3F3E515157BF880, 0x76ED872A03440DBF, 0xC6758BEEFC37CFAB, 0xEDE237D74A995102, 0x5EAA8EBD1EBC9E4D, 0x000006EB3E155159}}, // 0x6EB3E1551595EAA8EBD1EBC9E4DEDE237D74A995102C6758BEEFC37CFAB76ED872A03440DBFE3F3E515157BF8801BE0DCA6E54798F03A230558FFDB0000FFFFF914C1EAAEA6A1557142E14361B2121DC828B566AEFD398A741103C83054891278D5FCBBF2401C0C1AEAEA84077FE41F23591AB8670FC5DCFAA70024FFFF
    {{0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F, 0x00000280FFFFFD7F}}, // 0x280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F00000280FFFFFD7F
    {{0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00, 0xFFFFFFFFFFFBD0FF, 0x0000000000042F00}}, // 0x42F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF0000000000042F00FFFFFFFFFFFBD0FF
    {{0xB6455F45D36EE7FF, 0x911C9C8354DA792F, 0xB3A7A570A38F8C1C, 0xFFFFFFFFFFFFC5D6, 0x49BAA0BA2C911800, 0x6EE3637CAB2586D0, 0x4C585A8F5C7073E3, 0x0000000000003A29, 0xB6455F45D36EE7FF, 0x911C9C8354DA792F, 0xB3A7A570A38F8C1C, 0xFFFFFFFFFFFFC5D6, 0x49BAA0BA2C911800, 0x6EE3637CAB2586D0, 0x4C585A8F5C7073E3, 0x0000000000003A29}}, // 0x3A294C585A8F5C7073E36EE3637CAB2586D049BAA0BA2C911800FFFFFFFFFFFFC5D6B3A7A570A38F8C1C911C9C8354DA792FB6455F45D36EE7FF0000000000003A294C585A8F5C7073E36EE3637CAB2586D049BAA0BA2C911800FFFFFFFFFFFFC5D6B3A7A570A38F8C1C911C9C8354DA792FB6455F45D36EE7FF
    {{0xBF88A4B733CD45FF, 0xFFFFFFFFFFFFFECA, 0x40775B48CC32BA00, 0x0000000000000135, 0xBF88A4B733CD45FF, 0xFFFFFFFFFFFFFECA, 0x40775B48CC32BA00, 0x0000000000000135, 0xBF88A4B733CD45FF, 0xFFFFFFFFFFFFFECA, 0x40775B48CC32BA00, 0x0000000000000135, 0xBF88A4B733CD45FF, 0xFFFFFFFFFFFFFECA, 0x40775B48CC32BA00, 0x0000000000000135}}, // 0x13540775B48CC32BA00FFFFFFFFFFFFFECABF88A4B733CD45FF000000000000013540775B48CC32BA00FFFFFFFFFFFFFECABF88A4B733CD45FF000000000000013540775B48CC32BA00FFFFFFFFFFFFFECABF88A4B733CD45FF000000000000013540775B48CC32BA00FFFFFFFFFFFFFECABF88A4B733CD45FF
    {{0xFF2C1503C50EB9FF, 0xFFFFFFFFFFFFFFFF, 0x00D3EAFC3AF14600, 0x0000000000000000, 0xFF2C1503C50EB9FF, 0xFFFFFFFFFFFFFFFF, 0x00D3EAFC3AF14600, 0x0000000000000000, 0xFF2C1503C50EB9FF, 0xFFFFFFFFFFFFFFFF, 0x00D3EAFC3AF14600, 0x0000000000000000, 0xFF2C1503C50EB9FF, 0xFFFFFFFFFFFFFFFF, 0x00D3EAFC3AF14600}}, // 0xD3EAFC3AF14600FFFFFFFFFFFFFFFFFF2C1503C50EB9FF000000000000000000D3EAFC3AF14600FFFFFFFFFFFFFFFFFF2C1503C50EB9FF000000000000000000D3EAFC3AF14600FFFFFFFFFFFFFFFFFF2C1503C50EB9FF000000000000000000D3EAFC3AF14600FFFFFFFFFFFFFFFFFF2C1503C50EB9FF
    {{0xF3CF5894831427FF, 0xB34F58C9C0A683BD, 0xDBE6EA075F8BD06A, 0x0663969AE959DC35, 0xBEEE0D3E809F343D, 0xFFFFFFFFCDD127C4, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x0C30A76B7CEBD800, 0x4CB0A7363F597C42, 0x241915F8A0742F95, 0xF99C696516A623CA, 0x4111F2C17F60CBC2, 0x00000000322ED83B}}, // 0x322ED83B4111F2C17F60CBC2F99C696516A623CA241915F8A0742F954CB0A7363F597C420C30A76B7CEBD800FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFCDD127C4BEEE0D3E809F343D0663969AE959DC35DBE6EA075F8BD06AB34F58C9C0A683BDF3CF5894831427FF
    {{0xFFFB9933FA5117FF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x000466CC05AEE800, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000, 0xFFFB9933FA5117FF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x000466CC05AEE800}}, // 0x466CC05AEE800FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFB9933FA5117FF000000000000000000000000000000000000000000000000000466CC05AEE800FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFB9933FA5117FF
    {{0xE2A763C16E86D7FF, 0x280FEF0D94722CAE, 0xFF43402D8C0D41FF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0x1D589C3E91792800, 0xD7F010F26B8DD351, 0x00BCBFD273F2BE00}}, // 0xBCBFD273F2BE00D7F010F26B8DD3511D589C3E91792800FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF43402D8C0D41FF280FEF0D94722CAEE2A763C16E86D7FF
    {{0x0}}
};

/**
 * @brief Returns the pointer to the 0-terminated array of exponents
 * for the LFSR maximal period verification.
 * @param n LFSR state size, bits.
 */
static const LargeInt *get_lfsr_exps(size_t n)
{
    if (n == 32) {
        return lfsr32_exps;
    } else if (n == 64) {
        return lfsr64_exps;
    } else if (n == 96) {
        return lfsr96_exps;
    } else if (n == 128) {
        return lfsr128_exps;
    } else if (n == 160) {
        return lfsr160_exps;
    } else if (n == 192) {
        return lfsr192_exps;
    } else if (n == 256) {
        return lfsr256_exps;
    } else if (n == 512) {
        return lfsr512_exps;
    } else if (n == 1024) {
        return lfsr1024_exps;
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
    for (size_t i = 0; i < LARGEINT_SIZE; i++) {
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

/**
 * @brief Get the number of significant bits, i.e. without leading 0s.
 */
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


void LargeInt_print_hex(const LargeInt *obj, const CallerAPI *intf)
{
    int is_inside = 0;
    for (size_t i = LARGEINT_SIZE; i-- != 0; ) {
        if (!is_inside && obj->x[i] != 0) {
            is_inside = 1;
            intf->printf("%16.16llX", (unsigned long long) obj->x[i]);
        } else if (is_inside) {
            intf->printf(".%16.16llX", (unsigned long long) obj->x[i]);
        }
    }
}



///////////////////////////////////////////
///// LfsrMatrix class implementation /////
///////////////////////////////////////////

LfsrMatrix LfsrMatrix_create(size_t n)
{
    LfsrMatrix obj;
    if (n > 0) {
        obj.x = calloc(n * n, sizeof(uint8_t));
    } else {
        obj.x = calloc(1, sizeof(uint8_t));
    }
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

/**
 * @brief Matrix multiplication in the GF(2) field.
 * @param a The first matrix.
 * @param b The second matrix.
 * @return The matrix product, must be destructed by the caller.
 */
LfsrMatrix LfsrMatrix_create_prod(const LfsrMatrix *a, const LfsrMatrix *b)
{
    // Check the matrices size
    const size_t n = a->n;
    if (b->n != n) {
        LfsrMatrix c = LfsrMatrix_create(0);
        return c;
    }
    // Cache rows/columns
    // It is O(n^2) and the matrix multiplication is O(n^3)
    size_t nwords = n / 64;
    if (n % 64 > 0) {
        nwords++;
    }
    uint64_t *arows = calloc(nwords * n, sizeof(uint64_t));
    uint64_t *bcols = calloc(nwords * n, sizeof(uint64_t));
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            const uint64_t aij = LfsrMatrix_getbit(a, i, j);
            const uint64_t bij = LfsrMatrix_getbit(b, i, j);
            arows[i*nwords + (j >> 6)] |= aij << (j & 0x3FU);
            bcols[j*nwords + (i >> 6)] |= bij << (i & 0x3FU);
        }
    }    
    // The multiplication procedure
    // It is based on the cij ^= aik & bki formula but uses bitwise
    // operations on 64-bit words and Hamming weights for optimization.
    LfsrMatrix c = LfsrMatrix_create(n);
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            uint8_t cij = 0;
            for (size_t k = 0; k < nwords; k++) {
                const uint64_t prods = arows[i*nwords + k] & bcols[j*nwords + k];
                cij = (uint8_t) (cij + get_uint64_hamming_weight(prods));
            }
            LfsrMatrix_setbit(&c, i, j, cij & 1);

        }
    }
    // Free buffers and return the resulting matrix
    free(arows);
    free(bcols);
    return c;
}

/**
 * @brief Check if the two matrices are equal.
 */
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

/**
 * @brief Destructor: deallocates all internal buffers but
 * not the structure itself.
 */
void LfsrMatrix_destruct(LfsrMatrix *obj)
{
    free(obj->x);
}

/**
 * @brief Calculate the matrix power.
 * @param x  Matrix (base)
 * @param e  Exponent
 * @return The matrix power, must be destructed by the caller.
 */
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

/**
 * @brief Check if PRNG has counters and/or constants. It is important
 * to prevent memory corruption and segmentation fault during the 
 * period deduction attempts.
 */
int GeneratorStateExt_has_counters(GeneratorStateExt *obj)
{
    const unsigned long niters = 10000000;
    const size_t nbytes = obj->nbytes;
    const uint8_t *cur = obj->state.state;
    uint8_t *prev = malloc(nbytes);
    uint8_t *is_byte_ctr = malloc(nbytes);
    memset(is_byte_ctr, 1, nbytes);
    // Save an initial PRNG state
    memcpy(prev, obj->state.state, nbytes);
    // Check if any bytes behave like a counter
    for (unsigned long i = 0; i < niters; i++) {
        // Iterate the PRNG state
        (void) obj->state.gi->get_bits(obj->state.state);
        // Check if some bytes are not counters
        for (size_t j = 0; j < nbytes; j++) {
            const uint8_t delta = cur[j] - prev[j];
            if (delta != 0 && delta != 1) {
                is_byte_ctr[j] = 0;
            }
        }
    }
    // Get the final result, free buffers and finish
    int has_ctr = 0;
    for (size_t i = 0; i < nbytes; i++) {
        if (is_byte_ctr[i]) {
            has_ctr = 1;
            break;
        }
    }
    free(prev);
    free(is_byte_ctr);
    return has_ctr;
}

/**
 * @brief Check if the generator is valid, i.e. is LFSR and has no counters or
 * constants inside its state.
 */
int GeneratorStateExt_is_valid(GeneratorStateExt *obj, const CallerAPI *intf)
{
    // Check if the PRNG has counters (by empirical testing)
    if (GeneratorStateExt_has_counters(obj)) {
        intf->printf("  The PRNG has constants and/or counters inside its state. Automated\n");
        intf->printf("  period deduction is impossible and may cause memory corruption\n");
        intf->printf("  and/or segmentation fault.\n");
        return 0;
    }

    // Check if the PRNG is LFSR (by empirical testing)
    if (GeneratorStateExt_is_lfsr(obj)) {
        intf->printf("  The PRNG is probably a LFSR\n");
    } else {
        intf->printf("  The PRNG is not a LFSR\n");
        return 0;
    }
    // The generator is valid
    return 1;
}


void GeneratorStateExt_destruct(GeneratorStateExt *obj)
{
    GeneratorState_destruct(&(obj->state));
}

////////////////////////////////////
///// Battery implemenentation /////
////////////////////////////////////

void LfsrPeriodResult_print(const CallerAPI *intf, LfsrPeriodResult res)
{
    switch (res) {
    case LFSR_PERIOD_MAX:
        intf->printf("The LFSR has a maximal period\n");
        break;
    case LFSR_PERIOD_NOT_MAX:
        intf->printf("The LFSR period is not maximal\n");
        break;
    default:
        intf->printf("The verification cannot be applied to this PNG\n");
    }
}


LfsrPeriodResult lfsr_period_test(const GeneratorInfo *gen, const CallerAPI *intf,
    const LfsrPeriodOptions *opts)
{
    if (gen->parent != NULL) {
        intf->printf("  LFSR period checker error: cannot analyze an enveloped generator");
        return LFSR_PERIOD_ERROR;
    }

    GeneratorStateExt ext = GeneratorStateExt_create(gen, intf);

    intf->printf("LFSR period checker\n");
    intf->printf("  malloc: nbytes = %llu; ptr = 0x%llu\n",
        (unsigned long long) ext.nbytes,
        (unsigned long long) ext.state.state);
    (void) opts;
    // Check the generator validity
    if (opts->check_validity && !GeneratorStateExt_is_valid(&ext, intf)) {
        GeneratorStateExt_destruct(&ext);
        LfsrPeriodResult_print(intf, LFSR_PERIOD_ERROR);
        return LFSR_PERIOD_ERROR;
    }
    // Calculate the maximal period
    const unsigned int nbits = (unsigned int) (ext.nbytes * 8);
    LargeInt period = LargeInt_from_pow2(nbits);
    LargeInt_subtract_u64(&period, 1U);
    intf->printf("  The maximal period to be verified: 2**%u - 1\n", nbits);
    // Check if the maximal period is possible
    intf->printf("Beginning the period verification\n");
    LfsrMatrix mat = GeneratorStateExt_get_matrix(&ext, 1);
    LfsrMatrix matp = LfsrMatrix_create_pow(&mat, &period);
    LfsrPeriodResult result = LFSR_PERIOD_MAX;
    if (LfsrMatrix_is_eye(&matp)) {
        intf->printf("  A^period = I: passed\n");
    } else {
        intf->printf("  A^period = I: failed\n");
        result = LFSR_PERIOD_NOT_MAX;
        goto finished;
    }
    const LargeInt *lfsr_exps = get_lfsr_exps(ext.nbytes * 8);
    if (lfsr_exps == NULL) {
        intf->printf("  The tables are absent for this LFSR size\n");
        result = LFSR_PERIOD_ERROR;
    } else {
        intf->printf("  Verifying the A^(period/prime) = A^e <> I exponents\n");
        for (const LargeInt *d = lfsr_exps; !LargeInt_is_u64(d, 0); d++) {
            intf->printf("  Exponent (%4u bits): ", LargeInt_get_nbits(d));
            LargeInt_print_hex(d, intf);
            LfsrMatrix matd = LfsrMatrix_create_pow(&mat, d);
            if (LfsrMatrix_is_eye(&matd)) {
                intf->printf(" <<< FAIL\n");
                result = LFSR_PERIOD_ERROR;
            } else {
                intf->printf(" OK\n");
            }
            LfsrMatrix_destruct(&matd);
        }
    }

finished:
    LfsrPeriodResult_print(intf, result);
    LfsrMatrix_destruct(&mat);
    LfsrMatrix_destruct(&matp);
    GeneratorStateExt_destruct(&ext);
    return result;
}


BatteryExitCode battery_lfsr_period(const GeneratorInfo *gen, const CallerAPI *intf,
    const BatteryOptions *opts)
{
    (void) opts;
    const LfsrPeriodOptions test_opts = {.check_validity = 1};
    const LfsrPeriodResult res = lfsr_period_test(gen, intf, &test_opts);
    switch (res) {
    case LFSR_PERIOD_MAX:
        return BATTERY_PASSED;
    case LFSR_PERIOD_NOT_MAX:
        return BATTERY_FAILED;
    case LFSR_PERIOD_ERROR:
        return BATTERY_ERROR;
    default:
        return BATTERY_ERROR;
    }
}
