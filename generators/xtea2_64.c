/**
 * @brief xtea2_64.c
 * @details An experimental modification of XTEA block cipher with 128-bit
 * block size and 256-bit key developed by Alex Pukall. Uses 64-bit words.
 *
 * Note:
 *
 * - 5 rounds - pass `express` battery.
 * - 7 rounds - pass `brief` and `default` batteries.
 *
 * WARNING! No cryptoanalysis of this cipher was found in literature!
 * IT MUST NOT BE USED FOR ENCRYPTION!
 *
 * References:
 *
 * 1. https://alexpukall.github.io/xtea/xtea2.txt
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint64_t ctr[2];
    uint64_t out[2];
    uint64_t key[4];
    unsigned int pos;
} Xtea2x64State;


static inline uint64_t xtea2_64_mix(uint64_t v, uint64_t sum, uint64_t rkey)
{
    return (((v << 14) ^ (v >> 15)) + v) ^ (sum + rkey);
}


void Xtea2x64State_block(Xtea2x64State *obj)
{
    static const uint64_t delta = 0x9E3779B97F4A7C15;
    uint64_t a = obj->ctr[0], b = obj->ctr[1], sum = 0;
    for (int i = 0; i < 64; i++) {
        a += xtea2_64_mix(b, sum, obj->key[sum & 3]);
        sum += delta;
        b += xtea2_64_mix(a, sum, obj->key[(sum >> 23) & 3]);
    }
    obj->out[0] = a; obj->out[1] = b;
}


void Xtea2x64State_init(Xtea2x64State *obj, const uint64_t *key)
{
    for (int i = 0; i < 4; i++) {
        obj->key[i] = key[i];
    }
    obj->ctr[0] = 0; obj->ctr[1] = 0;
    Xtea2x64State_block(obj);
    obj->pos = 0;
}


static void *create(const CallerAPI *intf)
{
    uint64_t key[4];
    Xtea2x64State *obj = intf->malloc(sizeof(Xtea2x64State));
    for (int i = 0; i < 4; i++) {
        key[i] = intf->get_seed64();
    }
    Xtea2x64State_init(obj, key);
    return obj;    
}


static uint64_t get_bits_raw(Xtea2x64State *obj)
{
    if (obj->pos >= 2) {
        obj->ctr[0]++;
        Xtea2x64State_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}

int run_self_test(const CallerAPI *intf)
{
    static const uint64_t ref[2] = {0x5A569C159FA954C8, 0x58C5CD4DF3FF55A8};
    Xtea2x64State *obj = intf->malloc(sizeof(Xtea2x64State));
    obj->key[0] = 0; obj->key[1] = 0; obj->key[2] = 0; obj->key[3] = 1;
    obj->ctr[0] = 0; obj->ctr[1] = 1;
    Xtea2x64State_block(obj);
    int is_ok = 1;
    for (int i = 0; i < 2; i++) {
        intf->printf("Out = %llX; ref = %llX\n",
            (unsigned long long) obj->out[i], (unsigned long long) ref[i]);
        if (obj->out[i] != ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}

MAKE_UINT64_PRNG("XTEA2_64", run_self_test)
