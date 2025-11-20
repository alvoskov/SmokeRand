#include "smokerand/apidefs.h"
#include "smokerand/core.h"
#include <stdio.h>

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


BatteryExitCode EXPORT battery_func(const GeneratorInfo *gen,
    CallerAPI *intf, unsigned int testid, unsigned int nthreads,
    ReportType rtype)
{
    (void) testid; (void) nthreads; (void) rtype;
    GeneratorState obj = GeneratorState_create_x(gen, intf);
    unsigned long long npoints = 100000;

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
    sum /= npoints;
    intf->printf("Mean = %.10f\n", sum);
    GeneratorState_destruct_x(&obj);
    return BATTERY_PASSED;
}
