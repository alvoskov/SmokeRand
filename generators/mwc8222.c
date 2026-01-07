/**
 * @file mwc8222.c
 * @brief A multiply-with-carry generator MWC8222 by G.Marsaglia also known
 * as MWC256.
 * @details It is a typical MWC generator with a large 256 lag designed for
 * 32-bit computers like 80386. It seems that there were several versions with
 * several different multipliers, we use one from [1-3].
 *
 * References:
 *
 * 1. https://www.doornik.com/research.html
 * 2. http://school.anhb.uwa.edu.au/personalpages/kwessen/shared/Marsaglia03.html
 * 3. https://groups.google.com/g/comp.lang.c/c/qZFQgKRCQGg
 * 4. https://groups.google.com/g/sci.math/c/k3kVM8KwR-s/m/jxPdZl8XWZkJ
 *
 * @copyright The MWC8222 algorithm was developed by G. Marsaglia.
 *
 * Adaptation for SmokeRand:
 *
 * (c) 2025-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief MWC8222 PRNG state.
 */
typedef struct {
    uint32_t x[256]; ///< Generated values
    uint32_t c; ///< Carry
    uint8_t pos; ///< Current position in the buffer
} Mwc8222State;


static void Mwc8222State_init(Mwc8222State *obj, uint64_t seed)
{
    obj->pos = 255;
    obj->c = 362436; // Must be less than 809430660
    for (int i = 0; i < 256; i++) {
        obj->x[i] = (uint32_t) (pcg_bits64(&seed) >> 32);
    }
}

static inline uint64_t get_bits_raw(Mwc8222State *obj)
{
    static const uint64_t a = 809430660ULL;
    const uint64_t t = a * obj->x[++obj->pos] + obj->c;
    obj->c = (uint32_t) (t >> 32);
    obj->x[obj->pos] = (uint32_t) t;
    return obj->x[obj->pos];
}

static void *create(const CallerAPI *intf)
{
    Mwc8222State *obj = intf->malloc(sizeof(Mwc8222State));
    Mwc8222State_init(obj, intf->get_seed64());
    return obj;
}

MAKE_UINT32_PRNG("Mwc8222", NULL)
