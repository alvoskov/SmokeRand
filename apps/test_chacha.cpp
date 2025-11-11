#include <random>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <array>
#include <cinttypes>
#include <cstdio>
#include "smokerand_core.h"
#include "smokerand_bat.h"
#include "chacha.h"


static void *gen_create(const GeneratorInfo *gi, const CallerAPI *intf)
{
    std::array<uint32_t, 8> key{
        0x12345678, 0x12345678, 0x12345678, 0x12345678,
        0x12345678, 0x12345678, 0x12345678, 0x12345678};
    ChaCha20 *gen = new ChaCha20(key, intf->get_seed64());
    (void) gi;
    return gen;
}

static void gen_free(void *state, const GeneratorInfo *info, const CallerAPI *intf)
{
    ChaCha20 *gen = static_cast<ChaCha20 *>(state);
    delete gen;
    (void) info; (void) intf;
}

static uint64_t get_bits(void *state)
{
    std::uniform_int_distribution<uint32_t> int_gen(0, UINT32_MAX);
    ChaCha20 *gen = static_cast<ChaCha20 *>(state);
    return int_gen(*gen);
}


int main()
{
    static GeneratorInfo gen;
    gen.name = "chacha_cpp11";
    gen.description = "chacha in a fancy c++ class";
    gen.nbits = 32;
    gen.create = gen_create;
    gen.free = gen_free;
    gen.get_bits = get_bits;
    gen.self_test = NULL;
    gen.get_sum = NULL;
    gen.parent = NULL;

    CallerAPI intf = CallerAPI_init_mthr();
    battery_speed(&gen, &intf);
    battery_default(&gen, &intf, TESTS_ALL, 8, REPORT_FULL);
    CallerAPI_free();
    return 0;
}
