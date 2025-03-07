/**
 * @file superduper64_body.h
 * @brief An implementation of 64-bit combined "Super Duper" PRNG
 * by G. Marsaglia.
 * @details 
 *
 * https://groups.google.com/g/comp.sys.sun.admin/c/GWdUThc_JUg/m/_REyWTjwP7EJ
 *
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */

/**
 * @brief SuperDuper64 PRNG state
 */
typedef struct {
    uint64_t lcg;
    uint64_t xs;
} SuperDuper64State;

static inline uint64_t superduper64_get_bits(void *state)
{
    SuperDuper64State *obj = state;
    obj->lcg = 6906969069 * obj->lcg + 1234567;
    obj->xs ^= (obj->xs << 13);
    obj->xs ^= (obj->xs >> 17);
    obj->xs ^= (obj->xs << 43);
    return obj->lcg + obj->xs;
}

static inline void *superduper64_create(const CallerAPI *intf)
{
    SuperDuper64State *obj = intf->malloc(sizeof(SuperDuper64State));
    obj->lcg = intf->get_seed64();
    do {
        obj->xs = intf->get_seed64();
    } while (obj->xs == 0);
    return (void *) obj;
}
