#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    int ind;
    uint8_t a;
    uint8_t b;
    uint8_t x[8];
} Xorrot64w8ArxState;


static inline uint8_t get_bits8(Xorrot64w8ArxState *obj)
{
    const int ind7 = obj->ind;
    obj->ind = (obj->ind + 1) & 0x7;
    const int ind0 = obj->ind;
    const uint8_t x0 = obj->x[ind0], x7 = obj->x[ind7];
    // ARX-FW mixer part: it is simplified because it is driven by LFSR
    uint8_t a = obj->a, b = obj->b;
    b = (uint8_t) (b + x0); // LFSR injector
    a = (uint8_t) (a + (rotl8(b, 1) ^ rotl8(b, 4) ^ b));
    obj->a = b; obj->b = a;
    const uint8_t out = (uint8_t) (a ^ b);
    // replace x6 and x7
    const uint8_t x0x7 = (uint8_t) (x0 ^ x7);
    obj->x[ind7] = x0x7;
    obj->x[ind0] = (uint8_t) ((x0 << 5) ^ x0x7 ^ rotl8(x7, 2) ^ rotl8(x7, 3));
    return out;
}


static inline uint64_t get_bits_raw(void *state)
{
    uint32_t out = get_bits8(state);
    out |= (uint32_t)get_bits8(state) << 8;
    out |= (uint32_t)get_bits8(state) << 16;
    out |= (uint32_t)get_bits8(state) << 24;
    return out;
}


static void *create(const CallerAPI *intf)
{
    Xorrot64w8ArxState *obj = intf->malloc(sizeof(Xorrot64w8ArxState));
    obj->ind = 0;
    uint64_t seed = intf->get_seed64();
    if (seed == 0) {
        seed = 0x123456789ABCDEFU;
    }
    for (size_t i = 0; i < 8; i++) {
        obj->x[i] = (uint8_t) (seed >> (i * 8));
    }
    obj->a = (uint8_t) (obj->x[0] + obj->x[1]);
    obj->b = (uint8_t) (obj->x[2] + obj->x[3]);
    return obj;
}


/**
 * @brief An internal self-test based on test vectors independently
 * generated in Python 3.x scripts (see `misc/lfsr/xorrot_gentestvec.py`).
 * These generators are based on explicit matrix arithmetics in GF(2).
 */
static int run_self_test(const CallerAPI *intf)
{
    const uint64_t x_ref[8] = {0x79, 0xa0, 0x88, 0x2D, 0xF7, 0xF7, 0xA8, 0xfB};
    Xorrot64w8ArxState obj = {.x = {8, 1, 2, 3, 4, 5, 6, 7}, .ind = 0};
    
    for (long i = 0; i < 10000000 / 4; i++) {
        (void) get_bits_raw(&obj);
    }
    int is_ok = 1;
    for (int i = 0; i < 8; i++) {
        const int ind = (obj.ind + i + 1) % 8;
        intf->printf("x_out = %llX; x_ref = %llX\n",
            (unsigned int) obj.x[ind], (unsigned int) x_ref[i]);
        if (obj.x[ind] != x_ref[i]) {
            is_ok = 0;
        }
    }
    return is_ok;
}


MAKE_UINT32_PRNG("xorrot64w8arx", run_self_test)


