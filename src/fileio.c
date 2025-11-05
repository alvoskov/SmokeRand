/**
 * @file fileio.c
 * @brief Implementation of PRNG based on reading binary data from stdin.
 * @copyright
 * (c) 2024-2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/fileio.h"
#include "smokerand/specfuncs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define STDIN_COLLECTOR_BUFFER_SIZE 1024

static unsigned long long nbytes_total = 0;

typedef struct {
    uint8_t buffer[STDIN_COLLECTOR_BUFFER_SIZE];
    size_t pos;
} StdinCollector;


static void StdinCollector_fill_buffer(StdinCollector *obj)
{
    size_t nbytes = fread(obj->buffer, 1, STDIN_COLLECTOR_BUFFER_SIZE, stdin);
    nbytes_total += nbytes;
    obj->pos = 0;
    if (nbytes != STDIN_COLLECTOR_BUFFER_SIZE) {
        fprintf(stderr, "Reading from stdin failed");
        exit(EXIT_FAILURE);
    }
}

static void *StdinCollector_create(const GeneratorInfo *gi, const CallerAPI *intf)
{
    StdinCollector *obj = intf->malloc(sizeof(StdinCollector));
    (void) *gi;
    obj->pos = STDIN_COLLECTOR_BUFFER_SIZE;
    set_bin_stdin();
    return (void *) obj;
}

static void StdinCollector_free(void *state, const GeneratorInfo *gi, const CallerAPI *intf)
{
    (void) gi;
    intf->free(state);
}

////////////////////////////////////////
///// Implementaton of 32-bit PRNG /////
////////////////////////////////////////

static inline uint64_t get_bits32_raw(void *state)
{
    StdinCollector *obj = state;
    if (obj->pos >= STDIN_COLLECTOR_BUFFER_SIZE) {
        StdinCollector_fill_buffer(obj);
        obj->pos = 0;
    }
    void *ptr = (void *) (obj->buffer + obj->pos);
    uint32_t x = *( (uint32_t *) ptr );
    obj->pos += sizeof(uint32_t);
    return x;
}

static uint64_t get_sum32(void *state, size_t len)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += get_bits32_raw(state);
    }
    return sum;
}

static uint64_t get_bits32(void *state)
{
    return get_bits32_raw(state);
}

////////////////////////////////////////
///// Implementaton of 64-bit PRNG /////
////////////////////////////////////////

static inline uint64_t get_bits64_raw(void *state)
{
    StdinCollector *obj = state;
    if (obj->pos >= STDIN_COLLECTOR_BUFFER_SIZE) {
        StdinCollector_fill_buffer(obj);
        obj->pos = 0;
    }
    void *ptr = (void *) (obj->buffer + obj->pos);
    uint64_t x = *( (uint64_t *) ptr );
    obj->pos += sizeof(uint64_t);
    return x;    
}

static uint64_t get_sum64(void *state, size_t len)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += get_bits64_raw(state);
    }
    return sum;
}

static uint64_t get_bits64(void *state)
{
    return get_bits64_raw(state);
}


/////////////////////
///// Interface /////
/////////////////////

void StdinCollector_print_report(void)
{
    printf("  Bytes processed: %llu (2^%.2f, 10^%.2f)\n", nbytes_total,
        sr_log2((double) nbytes_total), log10((double) nbytes_total));
}


GeneratorInfo StdinCollector_get_info(StdinCollectorType type)
{
    GeneratorInfo gen = {.name = NULL, .description = NULL, .nbits = 0,
        .create = StdinCollector_create, .free = StdinCollector_free,
        .get_bits = NULL, .get_sum = NULL, .self_test = NULL,
        .parent = NULL};
    if (type == stdin_collector_32bit) {
        gen.name = "stdin32";
        gen.get_bits = get_bits32;
        gen.get_sum = get_sum32;
        gen.nbits = 32;
    } else if (type == stdin_collector_64bit) {
        gen.name = "stdin64";
        gen.get_bits = get_bits64;
        gen.get_sum = get_sum64;
        gen.nbits = 64;
    } else {
        fprintf(stderr, "get_stdin_collector_info: unknown type");
        exit(EXIT_FAILURE);
    }
    return gen;
}
