/**
 * @file swb53dec.c
 * @brief Subtract-with-borrow generators that work directly with doubles
 * (treated as 53-bit integers).
 * @details
 *
 *  Generator | Dec.level | TestU01                   | PractRand 0.96
 * -----------|-----------|---------------------------|----------------
 *  13-2-dec  | 3         | SmallCrush/Crush/BigCrush | >= 4 TiB
 *  18-12-dec | 6         |                           |
 *  83-60-dec | 1         | SmallCrush/Crush/BigCrush | 1 TiB
 *  83-60-dec | 2         | SmallCrush/Crush          | >= 8 TiB
 *  83-60-dec | 3?(sr)    |                           |
 *
 * Dec.level - minimal decimation level to pass the `full` battery
 *
 * Note: 83-60-dec with dec.level 1 fails the BCFN test in PractRand 0.96
 * at 2 TiB sample.
 *
 *    class Swb64:
 *        def __init__(self):
 *            self.r, self.s = 13, 2
 *            self.x = [0] * self.r
 *            self.x[0] = 2**53 - 1
 *            self.c = 1
 *
 *        def next(self):
 *            xj, xi = self.x[self.r - self.s], self.x[0]
 *            d = xj - xi - self.c
 *            xn = d % 2**53
 *            self.c = 1 if d < 0 else 0
 *            self.x = self.x[1:] + [xn]
 *            return xn >> (53 - 32)
 *
 *    swb = Swb64()
 *
 *    for i in range(1_000_000):
 *        swb.next()
 *
 *    for i in range(32):
 *        print(i, hex(swb.x[swb.r - swb.s]), hex(swb.x[0]), hex(swb.c), "|", hex(swb.next()))
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_R 83
#define SWB_S 60
#define SWB_DECIM 3
//#define SWB_DECIM 0
#define TWO_M53 0x1.0p-53


typedef struct {    
    double x[SWB_R];
    double c;
    size_t pos;
    int nblocks;
} Swb53State;


static inline double swb_double(double x, double y, double c_in, double *c_out)
{
    const double delta = x - y - c_in;
    if (delta < 0.0) {
        *c_out = TWO_M53;
        return delta + 1.0;
    } else {
        *c_out = 0.0;
        return delta;
    }
}

static void *create(const CallerAPI *intf)
{
    Swb53State *obj = intf->malloc(sizeof(Swb53State));
    for (size_t i = 0; i < SWB_R; i++) {
        obj->x[i] = TWO_M53 * (double) (intf->get_seed64() >> 11);
    } 
    obj->c = (obj->x[0] == 0.0) ? TWO_M53 : 0.0;
    obj->pos = SWB_R;
    obj->nblocks = SWB_DECIM + 1;
    return obj;
}


static inline uint64_t get_bits_raw(Swb53State *obj)
{
    if (obj->pos == SWB_R) {
        for (int ii = 0; ii < obj->nblocks; ii++) {
            for (int i = 0; i < SWB_S; i++) {
                obj->x[i] = swb_double(obj->x[i + (SWB_R - SWB_S)], obj->x[i], obj->c, &obj->c);
            }
            for (int i = SWB_S; i < SWB_R; i++) {
                obj->x[i] = swb_double(obj->x[i - SWB_S], obj->x[i], obj->c, &obj->c);
            }
            obj->pos = 0;
        }
    }
    return (uint64_t) (0x1.0p32 * obj->x[obj->pos++]);
}


/**
 * @brief An internal self-test based on the `SWB(2**53,2,13)` generator.
 * Uses reference values obtained by the Python 3.x script.
 */
static int run_self_test(const CallerAPI *intf)
{
    static const uint32_t u_ref[16] = { // For r=13, s=2
        0xe82c96c8, 0x8ad0d99c, 0x4d632f9f, 0xa5adae45,
        0xfba29ed7, 0x0a68d910, 0x44a09584, 0x4f025879,
        0xa3dfa2b9, 0xa52a4316, 0xbc3565c4, 0x74af23e9,
        0xbbb1910c, 0x8c828d21, 0x30e0b76f, 0x3f1f5d81
    };

    Swb53State *obj = create(intf);
    for (size_t i = 0; i < SWB_R; i++) {
        obj->x[i] = 0.0;
    }
    obj->x[0] = 1.0 - TWO_M53;
    obj->x[SWB_R - SWB_S] = 0.0;
    obj->c = TWO_M53;
    obj->nblocks = 1; // Disable decimation

    for (long i = 0; i < 1000000; i++) {
        (void) get_bits_raw(obj);
    }
    int is_ok = 1;
    for (size_t i = 0; i < 16; i++) {
        const uint32_t u = (uint32_t) get_bits_raw(obj);
        intf->printf("%8.8lX %8.8lX\n",
            (unsigned long) u, (unsigned long) u_ref[i]);
        if (u != u_ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}



MAKE_UINT32_PRNG("SWB53DEC", run_self_test)
