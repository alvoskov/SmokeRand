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
    }
   
    GenGetInfoFunc gen_getinfo = dlsym_pe32dos(handle, "gen_getinfo");
    if (gen_getinfo == NULL) {
        fprintf(stderr, "Cannot find the 'gen_getinfo' function\n");
        return 1;
    }
    //----------------------------------------------------
    CallerAPI intf = CallerAPI_init();
    GeneratorInfo gi;
    gen_getinfo(&gi, &intf);
    GeneratorInfo_print(&gi, 1);
    if (!strcmp("express", argv[1])) {
        battery_express(&gi, &intf, TESTS_ALL, 1, REPORT_FULL);
    } else if (!strcmp("brief", argv[1])) {
        battery_brief(&gi, &intf, TESTS_ALL, 1, REPORT_FULL);
    } else if (!strcmp("default", argv[1])) {
        battery_default(&gi, &intf, TESTS_ALL, 1, REPORT_FULL);
    } else if (!strcmp("full", argv[1])) {
        battery_full(&gi, &intf, TESTS_ALL, 1, REPORT_FULL);
    } else if (!strcmp("selftest", argv[1])) {
        battery_self_test(&gi, &intf);
    } else if (!strcmp("speed", argv[1])) {
        battery_speed(&gi, &intf);
    } else if (strlen(argv[1]) > 0) {
        if (argv[1][0] != '@') {
            fprintf(stderr, "@filename argument must begin with '@'\n");
            return 1;
        }
        battery_file(argv[1] + 1, &gi, &intf, TESTS_ALL, 1, REPORT_FULL);
    }
    CallerAPI_free();
    //----------------------------------------------------
    dlclose_pe32dos(handle);
    return 0;
}
