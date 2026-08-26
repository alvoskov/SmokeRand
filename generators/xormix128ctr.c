// 
// Hars, L., Petruska, G. Pseudorandom recursions II. J Embedded Systems 2012, 1 (2012). https://doi.org/10.1186/1687-3963-2012-1
// Hars, L., Petruska, G. Pseudorandom Recursions: Small and Fast Pseudorandom Number Generators for Embedded Applications. J Embedded Systems 2007, 098417 (2007). https://doi.org/10.1155/2007/98417
// Hars L. Hardware Bit-Mixers. Cryptology {ePrint} Archive, Paper 2017/084. https://eprint.iacr.org/2017/084}
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint32_t key[4];
    uint32_t ctr[4];
    uint32_t out[4];
    int pos;
} Xormix128CtrState;

// 4 rounds (i < 2, no key sch.) - express, brief, default, full
// >= 1 TiB in PractRand
static inline uint64_t get_bits_raw(Xormix128CtrState *obj)
{
    const int sh1 = 19, sh2 = 5;
    if (obj->pos == 4) {
        uint32_t x = obj->ctr[0], y = obj->ctr[1];
        uint32_t z = obj->ctr[2], w = obj->ctr[3];
        for (int i = 0; i < 3; i++) {
            x += rotl32(y ^ z ^ w, sh1) + obj->key[0];
            y += rotl32(z ^ w ^ x, sh2) + obj->key[1];
            z += rotl32(w ^ x ^ y, sh1) + obj->key[2];
            w += rotl32(x ^ y ^ z, sh2) + obj->key[3];

            x += rotl32(y ^ z ^ w, sh2) + obj->key[0];
            y += rotl32(z ^ w ^ x, sh1) + obj->key[2];
            z += rotl32(w ^ x ^ y, sh2) + obj->key[1];
            w += rotl32(x ^ y ^ z, sh1) + obj->key[3];
        }
        obj->out[0] = x; obj->out[1] = y;
        obj->out[2] = z; obj->out[3] = w;
        if (++obj->ctr[0] == 0) obj->ctr[1]++;
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}


void Xormix128CtrState_init(Xormix128CtrState *obj, const uint32_t *key)
{
    // Key mixer    
    uint32_t x = key[0], y = key[1], z = key[2], w = key[3];
    for (int i = 0; i < 64 + 4; i++) {
        const uint32_t x0 = x, w0 = w;
        x = x0 ^ y;
        y = z;
        z = x0 ^ w0;
        w = (x0 << 9) ^ z ^ rotl32(w0, 4) ^ rotl32(w0, 17);
        if (i >= 64) {
            uint32_t out = rotl32(69069U * x0, 5);
            out += (out * out | 0x4005);
            obj->key[i - 64] = out;
        }
    }
    // Reset counter
    obj->ctr[0] = 0; obj->ctr[1] = 0;
    obj->ctr[2] = 0; obj->ctr[3] = 0;
    obj->pos = 4;
}



static void *create(const CallerAPI *intf)
{
    uint32_t k[4];
    Xormix128CtrState *obj = intf->malloc(sizeof(Xormix128CtrState));
    seeds_to_array_u32(intf, k, 4);
    Xormix128CtrState_init(obj, k);
    return obj;
}

MAKE_UINT32_PRNG("xormix128ctr", NULL)
