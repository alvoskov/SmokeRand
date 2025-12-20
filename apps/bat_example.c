/**
 * @file bat_example.c
 * @brief A simple example of custom battery implemented as a plugin, i.e.
 * shared object / dynamic library.
 * @copyright
 * (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/plugindefs.h"
#include <stdio.h>
#include <stdlib.h>

static GeneratorState GeneratorState_create_x(const GeneratorInfo *gi,
    const CallerAPI *intf)
{
    GeneratorState obj;
    obj.gi = gi;
    obj.state = gi->create(gi, intf);
    obj.intf = intf;
    if (obj.state == NULL) {
        fprintf(stderr,
            "Cannot create an example of generator '%s' with parameter '%s'\n",
            gi->name, intf->get_param());
        exit(EXIT_FAILURE);
    }
    return obj;
}


/**
 * @brief Destructor for the generator state: deallocates all internal
 * buffers but not the GeneratorState itself.
 */
static void GeneratorState_destruct_x(GeneratorState *obj)
{
    obj->gi->free(obj->state, obj->gi, obj->intf);
}


/**
 * @brief Battery entry point.
 */
BatteryExitCode EXPORT battery_func(const GeneratorInfo *gen,
    const CallerAPI *intf, const BatteryOptions *opts)
{
    GeneratorState obj = GeneratorState_create_x(gen, intf);
    const unsigned long long npoints = 100000;

    double sum = 0.0;
    if (gen->nbits == 32) {
        for (unsigned long long i = 0; i < npoints; i++) {
            sum += (double) gen->get_bits(obj.state) / (double) UINT32_MAX;
        }
    } else {
        for (unsigned long long i = 0; i < npoints; i++) {
            sum += (double) gen->get_bits(obj.state) / (double) UINT64_MAX;
        }
    }
    sum /= (double) npoints;
    intf->printf("Mean = %.10f\n", sum);
    intf->printf("Test id:           %u\n", opts->test.id);
    intf->printf("Test name:         %s\n",
        (opts->test.name != NULL) ? opts->test.name : "(none)");
    intf->printf("Battery parameter: %s\n", opts->param);
    intf->printf("Number of threads: %u\n", opts->nthreads);
    GeneratorState_destruct_x(&obj);
    return BATTERY_PASSED;
}
