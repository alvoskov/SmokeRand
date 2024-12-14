/**
 * @file rc4ok_shared.c
 * @brief RC4OK, a modification of classical RC4 (obsolete) PRNG. The authors
 * claim that it passes PractRand (the original RC4 doesn't pass it).
 *
 * 1. Khovayko O., Schelkunov D. RC4OK. An improvement of the RC4 stream
 *    cipher // Cryptology ePrint Archive, Paper 2023/1486.
 *    https://eprint.iacr.org/2023/1486
 * 2. https://github.com/emercoin/rc4ok/blob/main/demo_rc4ok.c
 * 3. Press W.H., Teukolsky S.A., Vetterling W.T., Flannery B.P.
 *    Numerical recipes. The Art of Scientific Computing. Third Edition.
 *    2007. Cambridge University Press. ISBN 978-0-511-33555-6.
 * 4. Sleem L., Couturier R. TestU01 and Practrand: Tools for a randomness
 *    evaluation for famous multimedia ciphers. Multimedia Tools and
 *    Applications, 2020, 79 (33-34), pp.24075-24088. ffhal-02993846f
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief RC4OK PRNG state.
 */
typedef struct {
    uint8_t s[256];
    uint8_t i;
    uint32_t j;
} RC4OKState;


static inline void swap_bytes(uint8_t *a, uint8_t *b)
{
    uint8_t t = *a;
    *a = *b;
    *b = t;
}

static inline uint8_t RC4OKState_get_byte(RC4OKState *obj)
{
    uint8_t *s = obj->s, i = obj->i, j0, u;
    uint32_t j = obj->j;

    i += 11;
    j = (j << 1) | (j >> 31);
    j += s[i];
    j0 = j & 0xFF;
    swap_bytes(&s[i], &s[j0]);
    u = s[i] + s[j0];
    obj->i = i; obj->j = j;
    return s[u];    
}



static void RC4OKState_init(RC4OKState *obj, const uint8_t *key, size_t key_len)
{
    // Pre-initialize the state
    for (int i = 0, j = 0; i < 256; i++) {
        j = (j + 233) & 0xFF;
        obj->s[i] = (uint8_t) j;
    }
    // Initialize the state with the given key
    int j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + obj->s[i] + key[i % key_len]) & 0xFF;
        swap_bytes(&obj->s[i], &obj->s[j]);
    }
    obj->i = obj->s[j ^ 85];
    obj->j = 0;
    // Skip first 256 bytes
    for (int k = 0; k < 256; k++) {
        (void) RC4OKState_get_byte(obj);
    }
}


static inline uint64_t get_bits_raw(void *state)
{
    uint64_t x = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t b = RC4OKState_get_byte(state);
        x = (x << 8) | b;
    }
    return x;
}


static void *create(const CallerAPI *intf)
{
    RC4OKState *obj = intf->malloc(sizeof(RC4OKState));
    uint64_t v = 0x9E3779B97F4A7C15 ^ intf->get_seed64();
    RC4OKState_init(obj, (uint8_t *) &v, 8);
    return (void *) obj;
}


static int run_self_test(const CallerAPI *intf)
{
    const char key[] = "rc4ok-is-the-best";
    const uint8_t out_ref[] = { // 64 bytes
        0x10, 0x4a, 0x1e, 0x8e,  0x59, 0xb3, 0x03, 0x67,
        0x99, 0x33, 0x96, 0xb4,  0x60, 0x60, 0x16, 0x5a,
        0x7f, 0xd9, 0xe2, 0x71,  0xe8, 0x6e, 0x07, 0xf5,
        0xa5, 0x18, 0xee, 0x40,  0x81, 0x96, 0x58, 0x4c,
        0x35, 0x67, 0x50, 0xbd,  0x3f, 0x17, 0x87, 0x40,
        0x6d, 0x0f, 0x06, 0xcd,  0x8a, 0x0e, 0x82, 0x76,
        0x80, 0xba, 0xf8, 0x23,  0x2d, 0xf4, 0x6a, 0xcc,
        0xfa, 0xce, 0x40, 0x1a,  0x95, 0x50, 0xe6, 0x92
    };
    RC4OKState *obj = intf->malloc(sizeof(RC4OKState));
    size_t key_len = 0;
    int is_ok = 1;
    for (int i = 0; key[i] != 0; i++) {
        key_len++;
    }
    RC4OKState_init(obj, (uint8_t *) key, key_len);
    for (int i = 0; i < 64; i++) {
        uint8_t b = RC4OKState_get_byte(obj);
        if (i % 8 == 0) {
            intf->printf("\n");
        }
        intf->printf("%.2X|%.2X ", (int) b, (int) out_ref[i]);
        if (b != out_ref[i]) {
            is_ok = 0;
        }
    }
    intf->printf("\n");
    intf->free(obj);    
    return is_ok;
}

MAKE_UINT32_PRNG("RC4OK", run_self_test)
