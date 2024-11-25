/**
 * @file swb_shared.c
 * @brief Subtract with borrow PRNG with "luxury levels"
 *
 * 1. https://doi.org/10.1103/PhysRevLett.69.3382
 * 2. https://doi.org/10.1016/0010-4655(90)90033-W
 * 3. https://doi.org/10.1214/aoap/1177005878
 * 4. TAOCP2
 *
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_A 43
#define SWB_B 22

/**
 * @brief SWB generator state (with luxury levels)
 */
typedef struct {    
    uint32_t x[SWB_A + 1];
    uint32_t c;
    int i;
    int j;
    int luxury;
    int pos;
} SwbLuxState;

/**
 * @brief SWB implementation without "luxury level".
 */
static inline uint64_t get_bits_nolux(SwbLuxState *obj)
{
    uint32_t x;
    int64_t xj = obj->x[obj->j], xi = obj->x[obj->i];
    int64_t t = xj - xi - (int64_t) obj->c;
    if (t >= 0) {
        x = (uint32_t) t;
        obj->c = 0;
    } else {
        x = (uint32_t) (t + 0xFFFFFFFB);
        obj->c = 1;
    }
    obj->x[obj->i] = x;
    if (--obj->i == 0) obj->i = SWB_A;
	if (--obj->j == 0) obj->j = SWB_A;
    return x;
}

/**
 * @brief This wrapper implements "luxury levels".
 */
static inline uint64_t get_bits_raw(void *state)
{
    SwbLuxState *obj = state;
    if (++obj->pos == SWB_A) {
        obj->pos = 0;
        for (int i = 0; i < SWB_A * obj->luxury; i++) {
            get_bits_nolux(obj);
        }
    }
    return get_bits_nolux(obj);
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

static void *create(const CallerAPI *intf)
{
    int luxury = get_luxury(intf);
    if (luxury == -1) {
        return NULL;
    }
    intf->printf("SWB(42,22,2^32-5)[luxury=%d]\n", luxury);
    SwbLuxState *obj = intf->malloc(sizeof(SwbLuxState));
    for (int i = 1; i <= SWB_A; i++) {
        obj->x[i] = intf->get_seed32();
    }
    obj->c = 1;
    obj->x[1] |= 1;
    obj->x[2] = (obj->x[2] >> 1) << 1;
    obj->i = SWB_A; obj->j = SWB_B;
    obj->luxury = luxury;
    obj->pos = 0;
    return (void *) obj;
}


MAKE_UINT32_PRNG("SWBLUX", NULL)
