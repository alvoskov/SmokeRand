/**
 * @file rc4_shared.c
 * @brief Implementation of RC4 CSPRNG (obsolete algorithm). 
 * @details This algorithm passes TestU01 BigCrush but fails practrand
 * on 1 TB of data. It also fails frequency tests for 16-bit words at
 * large samples (about 0.5 TB).
 *
 * 1. Press W.H., Teukolsky S.A., Vetterling W.T., Flannery B.P.
 *    Numerical recipes. The Art of Scientific Computing. Third Edition.
 *    2007. Cambridge University Press. ISBN 978-0-511-33555-6.
 * 2. Sleem L., Couturier R. TestU01 and Practrand: Tools for a randomness
 *    evaluation for famous multimedia ciphers. Multimedia Tools and
 *    Applications, 2020, 79 (33-34), pp.24075-24088. ffhal-02993846f
 * 3. Khovayko O., Schelkunov D. RC4OK. An improvement of the RC4 stream
 *    cipher // Cryptology ePrint Archive, Paper 2023/1486.
 *    https://eprint.iacr.org/2023/1486
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RC4 PRNG state.
 */
typedef struct {
    uint8_t s[256];
    uint8_t i;
    uint8_t j;
} RC4State;


static inline uint64_t get_bits_raw(void *state)
{
    RC4State *obj = state;
    uint64_t v = 0;
    uint8_t *s = obj->s, i = obj->i, j = obj->j;
    for (size_t k = 0; k < 4; k++) {
        uint8_t ss = s[++i];
        j += ss;
        s[i] = s[j];
        s[j] = ss;
        uint8_t u = s[i] + s[j];
        v = (v << 8) | s[u];
    }
    obj->i = i; obj->j = j;
    return v;
}


static void *create(const CallerAPI *intf)
{
    RC4State *obj = intf->malloc(sizeof(RC4State));
    uint64_t v = 0x9E3779B97F4A7C15 ^ intf->get_seed64();
    for (size_t i = 0; i < 256; i++) {
        obj->s[i] = (uint8_t) i;
    }
    for (size_t i = 0, j = 0; i < 256; i++) {
        uint8_t ss = obj->s[i];
        j = (j + ss + (v >> 56)) & 0xFF;
        obj->s[i] = obj->s[j];
        obj->s[j] = ss;
        v = (v << 56) | (v >> 8);
    }
    obj->i = 0;
    obj->j = 0;
    for (size_t k = 0; k < 32; k++) {
        (void) get_bits_raw(obj);
    }
    return (void *) obj;
}


MAKE_UINT32_PRNG("RC4", NULL)
