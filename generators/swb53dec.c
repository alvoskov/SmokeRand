#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#define SWB_R 65
#define SWB_S 5
#define SWB_DECIM 3
#define TWO_M53 0x1.0p-53

typedef struct {    
    double x[SWB_R];
    double c;
    size_t pos;
} Swb64State;


static inline uint64_t get_bits_raw(Swb64State *obj)
{
    if (obj->pos == SWB_R) {
        for (int ii = 0; ii < SWB_DECIM; ii++) {
            for (int i = 0; i < SWB_S; i++) {
                obj->x[i] = obj->x[i] - obj->x[i + (SWB_R - SWB_S)] - obj->c;
                if (obj->x[i] < 0.0) {
                    obj->x[i] += 1.0; obj->c = TWO_M53;
                } else {
                    obj->c = 0.0;
                }
            }
            for (int i = SWB_S; i < SWB_R; i++) {
                obj->x[i] = obj->x[i - SWB_S] - obj->x[i] - obj->c;
                if (obj->x[i] < 0.0) {
                    obj->x[i] += 1.0; obj->c = TWO_M53;
                } else {
                    obj->c = 0.0;
                }
            }
            obj->pos = 0;
        }
    }
    return (uint64_t) (4294967296.0 * obj->x[obj->pos++]);
}


static void *create(const CallerAPI *intf)
{
    Swb64State *obj = intf->malloc(sizeof(Swb64State));
    for (size_t i = 0; i < SWB_R; i++) {
        obj->x[i] = TWO_M53 * (double) (intf->get_seed64() >> 11);
    } 
    obj->c = (obj->x[0] == 0.0) ? TWO_M53 : 0.0;
    obj->pos = SWB_R;
    return obj;
}

MAKE_UINT32_PRNG("SWB53DEC", NULL)
