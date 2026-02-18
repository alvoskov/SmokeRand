/**
 * @file tf0_64ex.c
 * @details
 * 1. Klimov, A., Shamir, A. (2003) A New Class of Invertible Mappings. In:
 *    Kaliski, B.S., Koç, ç.K., Paar, C. (eds) Cryptographic Hardware and
 *    Embedded Systems - CHES 2002. CHES 2002. Lecture Notes in Computer
 *    Science, vol 2523. Springer, Berlin, Heidelberg.
 *    https://doi.org/10.1007/3-540-36400-5_34
 * 2. Klimov, A., Shamir, A. (2004). Cryptographic Applications of T-Functions.
 *    In: Matsui, M., Zuccherato, R.J. (eds) Selected Areas in Cryptography.
 *    SAC 2003. Lecture Notes in Computer Science, vol 3006. Springer, Berlin,
 *    Heidelberg. https://doi.org/10.1007/978-3-540-24654-1_18
 * 3. Ali Hadipour, Seyed Mahdi Sajadieh, Raheleh Afifi. Jump index in
 *    T-functions for designing a new basic structure of stream ciphers.
 *    Cryptology ePrint Archive, Paper 2020/158. 2020.
 *    https://eprint.iacr.org/2020/158
 * 4. https://doi.org/10.1109/TIT.2006.883624
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG


static inline uint64_t get_bits_raw(Lcg64State *obj)
{
    const uint32_t out = (uint32_t) (obj->x >> 32);
    obj->x += obj->x * obj->x | 0x40000005;
    return out ^ rotl32(out, 7) ^ rotl32(out, 23);
}


static void *create(const CallerAPI *intf)
{
    Lcg64State *obj = intf->malloc(sizeof(Lcg64State));
    obj->x = intf->get_seed64();
    return obj;
}


MAKE_UINT32_PRNG("tf0_64sc", NULL)
