// https://github.com/danielcota/biski64
// passes `default`, vulnerable to hamming_distr xor test (128-bit blocks)
/*
Hamming weights distribution test (histogram)
  Sample size, values:     137438953472 (2^37.00 or 10^11.14)
  Blocks analysis results
        bits |        z          p |    z_xor      p_xor
          32 |   -0.642       0.74 |   -0.182      0.572
          64 |   -0.072      0.529 |   -0.126       0.55
         128 |   -2.410      0.992 |   -0.669      0.748
         256 |    0.809      0.209 |   -1.344      0.911
         512 |    0.778      0.218 |   -0.304       0.62
        1024 |    1.068      0.143 |    1.316     0.0941
        2048 |    1.235      0.109 |   -0.930      0.824
        4096 |   -0.349      0.636 |    0.482      0.315
        8192 |    0.828      0.204 |   -0.803      0.789
       16384 |   -1.058      0.855 |   -0.180      0.572
  Final: z =  -2.410, p = 0.992
*/

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

typedef struct {
    uint32_t loop_mix;
    uint32_t mix;
    uint32_t ctr;
} Biski32State;


// biski64 generator function
static inline uint64_t get_bits_raw(void *state)
{
    Biski32State *obj = state;

    uint32_t output = obj->mix + obj->loop_mix;
    uint32_t old_loop_mix = obj->loop_mix;
    obj->loop_mix = obj->ctr ^ obj->mix;
    obj->mix = (obj->mix ^ rotl32(obj->mix, 8)) + rotl32(old_loop_mix, 20);
    obj->ctr += 0x99999999;
    return output;
}


static void *create(const CallerAPI *intf)
{
    Biski32State *obj = intf->malloc(sizeof(Biski32State));
    obj->loop_mix = intf->get_seed32();
    obj->mix = intf->get_seed32();
    obj->ctr = intf->get_seed32();
    return obj;
}

MAKE_UINT32_PRNG("biski32_alt", NULL)
