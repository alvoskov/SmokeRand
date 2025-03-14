#include "smokerand_core.h"
#include "smokerand_bat.h"
#include "smokerand/threads_intf.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <float.h>



#define NTHREADS 6

typedef struct {
    size_t init_pos;
    size_t nsamples;    
    double *z_ary;
    GeneratorInfo *gi;
    CallerAPI *intf;
} HammingThreadState;

static ThreadRetVal THREADFUNC_SPEC hamming_ot_run_test(void *udata)
{
    HammingOtOptions opts = {.mode = HAMMING_OT_BYTES, .nbytes = 100000000};
    HammingThreadState *obj = udata;
    GeneratorState gen = GeneratorState_create(obj->gi, obj->intf);
    for (size_t i = obj->init_pos; i < obj->nsamples; i += NTHREADS) {
        obj->intf->printf("%lu of %lu\n",
            (unsigned long) (i + 1), (unsigned long) obj->nsamples);
        TestResults res = hamming_ot_test(&gen, &opts);
        obj->z_ary[i] = res.x;
    }
    GeneratorState_free(&gen, obj->intf);
    return NULL;
}


double *generate_sample(GeneratorInfo *gi, int nsamples)
{
    HammingThreadState threads[NTHREADS];
    CallerAPI intf = CallerAPI_init_mthr();
    double *z_ary = calloc(nsamples, sizeof(double));
    for (int i = 0; i < NTHREADS; i++) {
        threads[i].init_pos = i;
        threads[i].nsamples = nsamples;
        threads[i].z_ary = z_ary;
        threads[i].gi = gi;
        threads[i].intf = &intf;
    }
    init_thread_dispatcher();
    ThreadObj *handles = calloc(sizeof(ThreadObj), NTHREADS);
    for (int i = 0; i < NTHREADS; i++) {
        handles[i] = ThreadObj_create(hamming_ot_run_test, &threads[i], i + 1);
    }
    // Get data from threads
    for (int i = 0; i < NTHREADS; i++) {
        ThreadObj_wait(&handles[i]);
    }
    // Deallocate array
    free(handles);
    CallerAPI_free();
    return z_ary;
}

int main()
{
    const char *mod_name = "generators/libchacha_avx.dll";
    //const char *mod_name = "generators/libspeck128_avx.dll";
    int nsamples = 100000;

    GeneratorModule mod = GeneratorModule_load(mod_name);
    if (!mod.valid) {
        CallerAPI_free();
        return 1;
    }
    GeneratorInfo *gi = &mod.gen;
    printf("Generator name:    %s\n", gi->name);
    printf("Output size, bits: %d\n", gi->nbits);
    FILE *fp = fopen("dc6.bin", "wb");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open output file 'dc6.m'\n");
        return 1;
    }
    double *z_ary = generate_sample(gi, nsamples);
    fwrite(z_ary, nsamples, sizeof(double), fp);
/*
    fprintf(fp, "z = [");
    for (int i = 0; i < nsamples; i++) {
        if (i != nsamples - 1) {
            fprintf(fp, "%g, ", z_ary[i]);
            if ((i + 1) % 8 == 0) {
                fprintf(fp, "\n");
            }
        } else {
            fprintf(fp, "%g", z_ary[i]);
        }
    }
    fprintf(fp, "]\n");
*/
    fclose(fp);
    free(z_ary);
    GeneratorModule_unload(&mod);
    return 0;
}
