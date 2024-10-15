#include "smokerand/smokerand_core.h"
#include "smokerand/lineardep.h"
#include "smokerand/entropy.h"
#include "smokerand/bat_default.h"
#include "smokerand/bat_full.h"
#include <stdio.h>
#include <time.h>
#include <math.h>



static inline void TestResults_print(const TestResults obj)
{
    printf("%s: p = %.3g; xemp = %g", obj.name, obj.p, obj.x);
    if (obj.p < 1e-10 || obj.p > 1 - 1e-10) {
        printf(" <<<<< FAIL\n");
    } else {
        printf("\n");
    }
}



void battery_self_test(GeneratorInfo *gen, const CallerAPI *intf)
{
    if (gen->self_test == 0) {
        intf->printf("Internal self-test not implemented\n");
        return;
    }
    intf->printf("Running internal self-test...\n");
    if (gen->self_test(intf)) {
        intf->printf("Internal self-test passed\n");
    } else {
        intf->printf("Internal self-test failed\n");
    }    
}



static int cmp_ints(const void *aptr, const void *bptr)
{
    uint64_t aval = *((uint64_t *) aptr), bval = *((uint64_t *) bptr);
    if (aval < bval) { return -1; }
    else if (aval == bval) { return 0; }
    else { return 1; }
}



typedef struct {
    size_t n; ///< Number of values
    unsigned int e; ///< Leave only values with zeros in lower (e - 1) bits
} BirthdayOptions;


void birthday_test(GeneratorState *obj, const BirthdayOptions *opts)
{
    uint64_t *x = calloc(opts->n, sizeof(uint64_t));
    if (x == NULL) {
        obj->intf->printf("  Not enough memory (8GiB is required)\n");
        return;
    }
    double lambda = pow(opts->n, 2.0) / pow(2.0, obj->gi->nbits - opts->e + 1.0);
    obj->intf->printf("  lambda = %g\n", lambda);
    obj->intf->printf("  Filling the array with 'birthdays'\n");
    uint64_t mask = (1ull << opts->e) - 1;
    for (size_t i = 0; i < opts->n; i++) {
        uint64_t u;
        do {
            u = obj->gi->get_bits(obj->state);
        } while ((u & mask) != 0);
        x[i] = u;
        if (i % (opts->n / 1000) == 0) {
            obj->intf->printf("\r    %.1f %% completed", 100.0 * i / (double) opts->n);
        }
    }
    obj->intf->printf("\n  Sorting the array\n");
    // qsort is used instead of radix sort to prevent "out of memory" error:
    // 2^30 of u64 is 8GiB of data
    qsort(x, opts->n, sizeof(uint64_t), cmp_ints); // Not radix: to prevent "out of memory"
    obj->intf->printf("  Searching duplicates\n");
    unsigned int ndups = 0;
    for (size_t i = 0; i < opts->n - 1; i++) {
        if (x[i] == x[i + 1])
            ndups++;
    }

    obj->intf->printf("Number of duplicates: %d\n", ndups);
}

void battery_birthday(GeneratorInfo *gen, const CallerAPI *intf)
{
    intf->printf("64-bit birthday test\n");
    BirthdayOptions opts = {.n = 1ull << 30, .e = 7}; // 10
    void *state = gen->create(intf);
    GeneratorState obj = {.gi = gen, .state = state, .intf = intf};
    birthday_test(&obj, &opts);
}

int main(int argc, char *argv[]) 
{
    if (argc < 3) {
        printf("Usage: smokerand battery generator_lib\n");
        printf(" battery: battery name; supported batteries:\n");
        printf("   - default\n");
        printf("   - selftest\n");
        printf("  generator_lib: name of dynamic library with PRNG that export the functions:\n");
        printf("   - int gen_getinfo(GeneratorInfo *gi)\n");
        printf("\n");
        return 0;
    }
    CallerAPI intf = CallerAPI_init();
    char *battery_name = argv[1];
    char *generator_lib = argv[2];


    GeneratorModule mod = GeneratorModule_load(generator_lib);
    if (!mod.valid) {
        return 1;
    }

    printf("Seed generator self-test: %s\n",
        xxtea_test() ? "PASSED" : "FAILED");

    if (!strcmp(battery_name, "default")) {
        battery_default(&mod.gen, &intf);
    } else if (!strcmp(battery_name, "full")) {
        battery_full(&mod.gen, &intf);
    } else if (!strcmp(battery_name, "selftest")) {
        battery_self_test(&mod.gen, &intf);
    } else if (!strcmp(battery_name, "birthday")) {
        battery_birthday(&mod.gen, &intf);
    } else {
        printf("Unknown battery %s\n", battery_name);
        GeneratorModule_unload(&mod);
        return 0;        
    }

    
    GeneratorModule_unload(&mod);
    return 0;
}
