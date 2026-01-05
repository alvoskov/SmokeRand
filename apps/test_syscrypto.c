/**
 * @file test_syscrypto.c
 * @brief Runs SmokeRand `speed`, `express`, `brief`, `default` and `full`
 * batteries on OS-level CSPRNG (cryptographically secure pseudorandom number
 * generator).
 *
 * @copyright
 * (c) 2026 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand_core.h"
#include "smokerand_bat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYS_CRYPTO_BUF_SIZE 4096

/**
 * @brief Buffer for OS-level CSPRNG output.
 */
typedef struct {
    uint64_t buf[SYS_CRYPTO_BUF_SIZE]; ///< Buffer.
    size_t pos; /// Current position inside the buffer.
} SysCryptoState;


static uint64_t get_bits(void *state)
{
    SysCryptoState *obj = state;
    if (obj->pos >= SYS_CRYPTO_BUF_SIZE) {
        fill_from_random_device(
            (unsigned char *) obj->buf,
            sizeof(uint64_t) * SYS_CRYPTO_BUF_SIZE
        );
        obj->pos = 0;
    }
    return obj->buf[obj->pos++];
}


static void *gen_create(const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    SysCryptoState *obj = intf->malloc(sizeof(SysCryptoState));    
    obj->pos = 0;
    return obj;
}


static void gen_free(void *state, const GeneratorInfo *info, const CallerAPI *intf)
{
    (void) info;
    intf->free(state);
}

static void print_help()
{
    static const char help_text[] = 
        "Usage:\n"
        "  test_syscrypto bat_name\n"
        "  bat_name is speed, express, brief, default or full\n\n";
    printf(help_text);
}


int main(int argc, char *argv[])
{
    BatteryOptions opts = {
        .test = {.id = TESTS_ALL, .name = NULL},
        .nthreads = 1,
        .report_type = REPORT_FULL,
        .param = NULL
    };
    static const GeneratorInfo gen = {
        .name = "syscrypto",
        .description = "OS-level CSPRNG",
        .nbits = 64,
        .create = gen_create,
        .free = gen_free,
        .get_bits = get_bits,
        .self_test = NULL,
        .get_sum = NULL,
        .parent = NULL
    };

    if (argc != 2) {
        print_help();
        return 0;
    }

    CallerAPI intf = CallerAPI_init();
    if (!strcmp(argv[1], "speed")) {
        battery_speed(&gen, &intf);
    } else if (!strcmp(argv[1], "express")) {
        battery_express(&gen, &intf, &opts);
    } else if (!strcmp(argv[1], "brief")) {
        battery_brief(&gen, &intf, &opts);
    } else if (!strcmp(argv[1], "default")) {
        battery_default(&gen, &intf, &opts);
    } else if (!strcmp(argv[1], "full")) {
        battery_full(&gen, &intf, &opts);
    } else {
        fprintf(stderr, "Unknown battery '%s'\n", argv[1]);
    }
    CallerAPI_free();
    return 0;
}
