/**
 * @file ranlux48.c
 * @brief Subtract with borrow PRNG with "luxury levels"
 *
 * 1. https://doi.org/10.1103/PhysRevLett.69.3382
 * 2. https://doi.org/10.1016/0010-4655(90)90033-W
 * 3. https://doi.org/10.1214/aoap/1177005878
 * 4. TAOCP2
 *
 * @copyright
 * (c) 2024-2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_A 24
#define SWB_B 10

#define POW24_M1 0xFFFFFF
#define POW24    0x1000000

/**
 * @brief SWB generator state (with luxury levels)
 */
typedef struct {    
    uint32_t x[SWB_A + 1];
    uint32_t c;
    int i;
    int j;
    int luxury;
    int skip;
    int pos;
} SwbLuxState;

/**
 * @brief SWB implementation without "luxury level".
 */
static inline uint32_t get_bits24_nolux(SwbLuxState *obj)
{
    uint32_t x;
    const int32_t xj = (int32_t) obj->x[obj->j], xi = (int32_t) obj->x[obj->i];
    const int32_t t = xj - xi - (int32_t) obj->c;
    if (t >= 0) {
        x = (uint32_t) t;
        obj->c = 0;
    } else {
        x = (uint32_t) (t + POW24);
        obj->c = 1;
    }
    obj->x[obj->i] = x;
    if (--obj->i == 0) obj->i = SWB_A;
	if (--obj->j == 0) obj->j = SWB_A;
    return x;
}

static inline uint32_t get_bits24(SwbLuxState *obj)
{
    if (++obj->pos == SWB_A) {
        obj->pos = 0;
        for (int i = 0; i < obj->skip; i++) {
            get_bits24_nolux(obj);
        }
    }
    return get_bits24_nolux(obj);
}

/**
 * @brief This wrapper implements "luxury levels".
 */
static inline uint64_t get_bits_raw(SwbLuxState *obj)
{
    uint32_t x = get_bits24(obj) >> 16;
    x |= (get_bits24(obj) & POW24_M1) << 8;
    return x;
}


static int get_luxury(const CallerAPI *intf)
{
    const char *param = intf->get_param();
    if (!intf->strcmp("", param) || !intf->strcmp("1", param)) {
        return 1;
    } else if (!intf->strcmp("0", param)) {
        return 0;
    } else if (!intf->strcmp("2", param)) {
        return 2;
    } else if (!intf->strcmp("3", param)) {
        return 3;
    } else if (!intf->strcmp("4", param)) {
        return 4;
    } else {
        intf->printf("Unknown parameter %s\n", param);
        return -1;
    }
}

static int luxury_to_skip(int luxury)
{
    static const int l_to_s[5] = {0, 24, 73, 199, 365};
    return (luxury < 0 || luxury > 4) ? 0 : l_to_s[luxury];
}

static void *create(const CallerAPI *intf)
{
    const int luxury = get_luxury(intf);
    if (luxury == -1) {
        return NULL;
    }
    SwbLuxState *obj = intf->malloc(sizeof(SwbLuxState));
    for (int i = 1; i <= SWB_A; i++) {
        obj->x[i] = intf->get_seed32() & POW24_M1;
    }
    obj->c = 1;
    obj->x[1] |= 1;
    obj->x[2] = (obj->x[2] >> 1) << 1;
    obj->i = SWB_A; obj->j = SWB_B;
    obj->luxury = luxury;
    obj->pos = 0;
    obj->skip = luxury_to_skip(luxury);
    intf->printf("SWB(24,10,2^24)[luxury=%d;%d,%d]\n",
        luxury, SWB_A, SWB_A + obj->skip);
    return obj;
}


MAKE_UINT32_PRNG("SWBLUX", NULL)
