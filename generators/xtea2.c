// https://tomstdenis.tripod.com/xtea.pdf
// ??? _0B_WHi5GiyND7BqO8PGKyh75r4nHVA/CTDldAmsns4hNQ0=
#include "smokerand/cinterface.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t ctr[4];
    uint32_t key[4];
    uint32_t out[4];
    int pos;
} Xtea2State;


void Xtea2State_block(Xtea2State *obj)
{
    // Load and pre-white the registers
    uint32_t a = obj->ctr[0], b = obj->ctr[1] + obj->key[0];
    uint32_t c = obj->ctr[2], d = obj->ctr[3] + obj->key[1];
    uint32_t sum = 0;
    // Round functions
    for (int i = 0; i < 32; i++) {
        a += ((b << 4) ^ (b >> 5)) + (d ^ sum) + rotl32(obj->key[sum & 3], b & 0x1F);
        sum += 0x9E3779B9;
        c += ((d << 4) ^ (d >> 5)) + (b ^ sum) + rotl32(obj->key[(sum >> 11) & 3], d & 0x1F);
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
