/**
 * @file xtea2.c
 * @details An experimental modification of XTEA block cipher with 128-bit
 * block size and 128-bit key developed by Tom St Denis. Uses 32-bit words.
 * @details
 *
 * Note:
 *
 * - 7 rounds - pass `express` and `brief` battery.
 * - 8 rounds - pass `default` battery.
 *
 * WARNING! No cryptoanalysis of this cipher was found in literature!
 * IT MUST NOT BE USED FOR ENCRYPTION!
 *
 * References:
 * 
 * 1. Tom St Denis. Extended TEA Algorithms. April 20th 1999.
 *    https://tomstdenis.tripod.com/xtea.pdf
 */
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t ctr[4];
    uint32_t key[4];
    uint32_t out[4];
    int pos;
} Xtea2State;


static inline uint32_t xtea2_mix(uint32_t v, uint32_t sum, uint32_t rkey)
{
    return ((v << 4) ^ (v >> 5)) + sum + rotl32(rkey, v & 0x1F);
}

void Xtea2State_block(Xtea2State *obj)
{
    // Load and pre-white the registers
    uint32_t a = obj->ctr[0], b = obj->ctr[1] + obj->key[0];
    uint32_t c = obj->ctr[2], d = obj->ctr[3] + obj->key[1];
    uint32_t sum = 0;
    // Round functions
    for (int i = 0; i < 32; i++) {
        a += xtea2_mix(b, d ^ sum, obj->key[sum & 3]);
        sum += 0x9E3779B9;
        c += xtea2_mix(d, b ^ sum, obj->key[(sum >> 11) & 3]);
        // Rotate plaintext registers
        const uint32_t t = a;
        a = b; b = c; c = d; d = t;
    }
    // Store and post-white the registers
    obj->out[0] = a ^ obj->key[2]; obj->out[1] = b;
    obj->out[2] = c ^ obj->key[3]; obj->out[3] = d;
}


void Xtea2State_init(Xtea2State *obj, const uint32_t *key)
{
    for (size_t i = 0; i < 4; i++) {
        obj->ctr[i] = 0;
        obj->key[i] = key[i];        
    }
    Xtea2State_block(obj);
    obj->pos = 0;
}

static inline uint64_t get_bits_raw(void *state)
{
    Xtea2State *obj = state;
    if (obj->pos == 4) {
        Xtea2State_block(obj);
        if (++obj->ctr[0] == 0) { obj->ctr[1]++; }
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}


static inline void *create(const CallerAPI *intf)
{
    uint32_t key[4];
    Xtea2State *obj = intf->malloc(sizeof(Xtea2State));
    seeds_to_array_u32(intf, key, 4);
    Xtea2State_init(obj, key);
    return obj;
}


/**
 * @brief An internal self-test based on the test vectors obtained
 * from the reference implementation
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t ctr[4] = {0x12345678, 0x87654321, 0x9ABCDEF0, 0x0FEDCBA9};
    static const uint32_t key[4] = {0x243F6A88, 0x85A308D3, 0x13198A2E, 0x03707344};
    static const uint32_t ref[4] = {0xE78E47E4, 0x8EBE5C3B, 0xDA8E629B, 0x9A84D7F9};

    Xtea2State *obj = intf->malloc(sizeof(Xtea2State));
    for (size_t i = 0; i < 4; i++) {
        obj->ctr[i] = ctr[i];
        obj->key[i] = key[i];
    }
    Xtea2State_block(obj);
    int is_ok = 1;
    for (size_t i = 0; i < 4; i++) {
        intf->printf("Out = %" PRIX32 "; ref = %" PRIX32 "\n", obj->out[i], ref[i]);
        if (obj->out[i] != ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}    

MAKE_UINT32_PRNG("XTEA2", run_self_test)
