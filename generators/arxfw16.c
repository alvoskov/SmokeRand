/**
 * @details
 *
 * WARNING! The minimal guaranteed period is only 2^16, the average period is
 * small is only about 2^47, bad seeds are theoretically possible. Usage of
 * this generator for statistical, scientific and engineering computations is
 * strongly discouraged!
 *
 * It was made as a toy generator to make a scaled down version of arxfw64 PRNG
 * suitable for 16-bit processors such as Intel 8086.
 *
 *
 *
 *
 *Version with ++:
 *TestU01: BigCrush
 *SmokeRand: full
 *64-bit birthday paradox test: passes
 *
 *rng=RNG_stdin32, seed=unknown
 *length= 2 terabytes (2^41 bytes), time= 7651 seconds
 *  no anomalies in 313 test result(s)
 *
 *rng=RNG_stdin32, seed=unknown
 *length= 4 terabytes (2^42 bytes), time= 14449 seconds
 *  Test Name                         Raw       Processed     Evaluation
 *  Gap-16:A                          R=  -4.8  p =1-5.9e-4   unusual
 *  ...and 322 test result(s) without anomalies
 *
 *rng=RNG_stdin32, seed=unknown
 *length= 8 terabytes (2^43 bytes), time= 27521 seconds
 *  Test Name                         Raw       Processed     Evaluation
 *  FPF-14+6/16:all                   R=  -7.5  p =1-7.3e-7   suspicious
 *  ...and 330 test result(s) without anomalies
 *
 * Version with Weyl
 *
 *rng=RNG_stdin32, seed=unknown
 *length= 8 terabytes (2^43 bytes), time= 27994 seconds
 *  Test Name                         Raw       Processed     Evaluation
 *  FPF-14+6/16:all                   R=  -9.8  p =1-3.4e-9    VERY SUSPICIOUS
 *  ...and 330 test result(s) without anomalies
 *
 *rng=RNG_stdin32, seed=unknown
 *length= 16 terabytes (2^44 bytes), time= 54881 seconds
 *  Test Name                         Raw       Processed     Evaluation
 *  FPF-14+6/16:(9,14-0)              R=  -7.1  p =1-2.8e-6   unusual
 *  FPF-14+6/16:all                   R= -17.5  p =1-8.3e-17    FAIL !
 *  ...and 337 test result(s) without anomalies
 */
#include "smokerand/cinterface.h"
#include "smokerand/int128defs.h"
#include <inttypes.h>

PRNG_CMODULE_PROLOG

/**
 * @brief arxfw16 PRNG state.
 */
typedef struct {
    uint16_t a;
    uint16_t b;
    uint16_t w;
} Arxfw16State;

static inline uint16_t get_bits16(Arxfw16State *obj)
{
    static const uint16_t inc = 0x9E37;
    uint16_t a = obj->a, b = obj->b;
    b += (uint16_t) obj->w;
    a += (uint16_t) ( rotl16(b, 3)  ^ rotl16(b, 8) ^ b);
    b ^= (uint16_t) ( rotl16(a, 15) + rotl16(a, 8) + a );
    obj->a = b;
    obj->b = a;
    obj->w += inc;
    return obj->a ^ obj->b;
}

static inline uint64_t get_bits_raw(Arxfw16State *state)
{
    const uint32_t a = get_bits16(state);
    const uint32_t b = get_bits16(state);
    return a | (b << 16);
}


static void *create(const CallerAPI *intf)
{
    Arxfw16State *obj = intf->malloc(sizeof(Arxfw16State));
    uint64_t seed = intf->get_seed64();
    obj->a = (uint16_t) seed;
    obj->b = (uint16_t) (seed >> 16);
    obj->w = (uint16_t) (seed >> 32);
    // Warmup
    for (int i = 0; i < 8; i++) {
        (void) get_bits_raw(obj);
    }
    return obj;
}

MAKE_UINT32_PRNG("arxfw16", NULL)
