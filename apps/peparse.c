/**
 * @file peparse.c
 * @brief A custom loader of 32-bit PE files (plugins with PRNGs) on such
 * 32-bit platforms as Windows or DOS. Doesn't rely on system loaders.
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/pe32loader.h"
#include "smokerand_core.h"
#include "smokerand_bat.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (sizeof(size_t) != sizeof(uint32_t)) {
        fprintf(stderr, "This program can work only in 32-bit mode\n");
        return 1;
    }
    if (argc < 3) {
        printf("Usage: peparse battery filename");
        return 0;
    }
    const char *filename = argv[2];

    void *handle = dlopen_pe32dos(filename, 0);
    if (handle == NULL) {
        fprintf(stderr, "Error: %s\n", dlerror_pe32dos());
        return 1;
    }

    void *fptr = dlsym_pe32dos(handle, "gen_getinfo");
    if (fptr == NULL) {
        fprintf(stderr, "Cannot find the 'gen_getinfo' function\n");
        return 1;
    }
    GetGenInfoFunc gen_getinfo;
    memcpy(&gen_getinfo, &fptr, sizeof(gen_getinfo));
    //----------------------------------------------------
    const BatteryOptions opts = {
        .test = {.id = TESTS_ALL, .name = NULL}, .nthreads = 1,
        .report_type = REPORT_FULL, .param = NULL
    };
    CallerAPI intf = CallerAPI_init();
    GeneratorInfo gi;
    gen_getinfo(&gi, &intf);
    GeneratorInfo_print(&gi, 1);
    if (!strcmp("express", argv[1])) {
        battery_express(&gi, &intf, &opts);
    } else if (!strcmp("brief", argv[1])) {
        battery_brief(&gi, &intf, &opts);
    } else if (!strcmp("default", argv[1])) {
        battery_default(&gi, &intf, &opts);
    } else if (!strcmp("full", argv[1])) {
        battery_full(&gi, &intf, &opts);
    } else if (!strcmp("selftest", argv[1])) {
        battery_self_test(&gi, &intf);
    } else if (!strcmp("speed", argv[1])) {
        battery_speed(&gi, &intf);
    } else if (strlen(argv[1]) > 0) {
        if (argv[1][0] != '@') {
            fprintf(stderr, "@filename argument must begin with '@'\n");
            return 1;
        }
        battery_file(argv[1] + 1, &gi, &intf, &opts);
    }
    CallerAPI_free();
    //----------------------------------------------------
    dlclose_pe32dos(handle);
    return 0;
}
