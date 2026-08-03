#include "smokerand_core.h"
#include "smokerand_bat.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned int a = 12;
static unsigned int b = 25;
static unsigned int c = 27;


typedef struct {
    uint64_t x;    
} Xorshift64State;

static uint64_t get_bits(void *state)
{
    Xorshift64State *obj = state;
    obj->x ^= obj->x >> a;
    obj->x ^= obj->x << b;
    obj->x ^= obj->x >> c;
    return obj->x;
}

static void *gen_create(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    Xorshift64State *obj = intf->malloc(sizeof(Xorshift64State));
    obj->x = intf->get_seed64();
    return obj;
}

static void gen_free(void *state, const GeneratorInfo *info, const CallerAPI *intf)
{
    (void) info;
    intf->free(state);
}


static int printf_null(const char *format, ...)
{
    (void) format;
    return 0;
}


int main()
{
    const LfsrPeriodOptions opts = {.check_validity = 0};
    static const GeneratorInfo gen = {
        .name = "xorshift64:dynshifts",
        .description = "xorshift64 with dynamic shifts",
        .nbits = 64,
        .create = gen_create,
        .free = gen_free,
        .get_bits = get_bits,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    CallerAPI intf = CallerAPI_init();
    intf.printf = printf_null;
    unsigned int ntriples = 0;
    for (unsigned int ai = 1; ai < 64; ai++) {
        for (unsigned int bi = 1; bi < 64; bi++) {
            for (unsigned int ci = 1; ci < 64; ci++) {
                a = ai; b = bi; c = ci;
                if (a <= c && lfsr_period_test(&gen, &intf, &opts) == LFSR_PERIOD_MAX) {
                    printf("[%2u %2u %2u] ", a, b, c);
                    if (++ntriples % 9 == 0) {
                        printf("\n");
                    }
                }
            }
        }
    }
    printf("\nTotal number of triples: %u", ntriples);

    CallerAPI_free();
    return 0;
}
