/**
 * @file lrnd64_shared.c
 * @brief A simple 64-bit LFSR generator. Fails linear complexity
 * and matrix rank tests.
 *
 * References:
 * 1. http://dx.doi.org/10.4203/ccp.95.23
 * 2. https://itprojects.narfu.ru/grid/materials2015/Yacobovskii.pdf
 * 3. Воронюк Михаил Николаевич Математическое моделирование процессов
 *    фильтрации в перколяционных решетках с использованием вычислительных
 *    систем сверхвысокой производительности // Приборостроение. 2013. №5. 
 *    https://cyberleninka.ru/article/n/matematicheskoe-modelirovanie-protsessov-filtratsii-v-perkolyatsionnyh-reshetkah-s-ispolzovaniem-vychislitelnyh-sistem-sverhvysokoy.
 */
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

/**
 * @brief LRnd64 PRNG state.
 */
typedef struct {
    int w_pos[4];
    uint64_t w[16];
} LRnd64State;

/**
 * @brief Creates LFSR example with taking into account
 * limitations on seeds (initial state).
 */
void *create(const CallerAPI *intf)
{
    LRnd64State *obj = intf->malloc(sizeof(LRnd64State));
    for (int i = 0; i < 16; i++) {
        do {
            obj->w[i] = intf->get_seed64();
        } while (obj->w[i] == 0);
    }
    obj->w_pos[0] = 0;
    obj->w_pos[1] = 1;
    obj->w_pos[2] = 2;
    obj->w_pos[3] = 8;
    return obj;
}

static inline uint64_t get_bits_raw(void *state)
{
    LRnd64State *obj = state;
    uint64_t w0 = obj->w[obj->w_pos[0]];
    uint64_t w1 = obj->w[obj->w_pos[1]];
    uint64_t w2 = obj->w[obj->w_pos[2]];
    uint64_t w8 = obj->w[obj->w_pos[3]];
    // b_{j+1024} = b_{j+512} + b_{j+128} + b_{j+8} + b_{j+1}
    uint64_t w16 = w8 ^ w2 ^ ((w0 >> 8) ^ (w1 << 56)) ^ ((w0 >> 1) ^ (w1 << 63));
    obj->w[obj->w_pos[0]] = w16;
    for (int i = 0; i < 4; i++) {
        obj->w_pos[i] = (obj->w_pos[i] + 1) & 0xF;
    }
    return w16;
}


MAKE_UINT64_PRNG("LRND64", NULL)
