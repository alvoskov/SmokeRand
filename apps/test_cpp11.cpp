/**
 * @file test_cpp11.cpp
 * @brief Mersenne Twister from C++ standard library.
 */
#include <random>
#include "smokerand_core.h"
#include "smokerand_bat.h"


template <class G>
class GeneratorWrapper : public GeneratorInfo
{
public:

    static void *Create(const GeneratorInfo *info, const CallerAPI *intf)
    {
        (void) info;
        std::seed_seq sd {intf->get_seed64(), intf->get_seed64()};
        G *obj = new G(sd);
        return obj;
    }

    static void Free(void *state, const GeneratorInfo *info, const CallerAPI *intf)
    {
        G *obj = (G *) state;
        delete obj;
        (void) info;
        (void) intf;
    }

    static uint64_t GetBits(void *state)
    {
        G *obj = (G *) state;
        return (*obj)();
    }

    GeneratorWrapper();
};


template <class G> GeneratorWrapper<G>::GeneratorWrapper()
{
    this->name = "MT19937";
    this->description = "Mersenne Twister";
    this->nbits = 32;
    this->create = GeneratorWrapper<G>::Create;
    this->free = GeneratorWrapper<G>::Free;
    this->get_bits = GeneratorWrapper<G>::GetBits;
    this->self_test = nullptr;
    this->get_sum = nullptr;
    this->parent = NULL;
}



int main()
{                                     
    BatteryOptions bat_opts;
    bat_opts.testid = TESTS_ALL;
    bat_opts.nthreads = 4;
    bat_opts.report_type = REPORT_FULL;
    bat_opts.param = NULL;
    CallerAPI intf = CallerAPI_init_mthr();
    GeneratorWrapper<std::mt19937> tw;
    //GeneratorWrapper<std::knuth_b> tw;

    printf("TWISTER!\n");
    GeneratorState state = GeneratorState_create(&tw, &intf);

    LinearCompOptions opts;
    opts.nbits = 50000;
    opts.bitpos = LINEARCOMP_BITPOS_MID;
    TestResults res = linearcomp_test(&state, &opts);
    printf("p = %g x = %g\n", res.p, res.x);

    battery_brief(&tw, &intf, &bat_opts);


    GeneratorState_destruct(&state);



    CallerAPI_free();
    return 0;
}
