// 
// Hars, L., Petruska, G. Pseudorandom recursions II. J Embedded Systems 2012, 1 (2012). https://doi.org/10.1186/1687-3963-2012-1
// Hars, L., Petruska, G. Pseudorandom Recursions: Small and Fast Pseudorandom Number Generators for Embedded Applications. J Embedded Systems 2007, 098417 (2007). https://doi.org/10.1155/2007/98417
// Hars L. Hardware Bit-Mixers. Cryptology {ePrint} Archive, Paper 2017/084. https://eprint.iacr.org/2017/084}
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


typedef struct {
    uint64_t ctr;
} Xormix64CtrState;


static inline uint64_t get_bits_raw(Xormix64CtrState *obj)
{
    uint64_t x = (obj->ctr += 0x3779884922721DEBU);
    x = (x ^ rotl64(x, 4) ^ rotl64(x, 9)) + 0x49A8D5B36969F969U;
    x = (x ^ rotl64(x, 4) ^ rotl64(x, 9)) + 0x6969F96949A8D5B3U;
    x = (x ^ rotl64(x, 4) ^ rotl64(x, 9));
    return x;
}


static void *create(const CallerAPI *intf)
{
    Xormix64CtrState *obj = intf->malloc(sizeof(Xormix64CtrState));
    obj->ctr = intf->get_seed64();
    for (int i = 0; i < 32; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT64_PRNG("xormix64ctr", NULL)
