#include "smokerand_core.h"
#include "smokerand/cinterface.h"
#include "../generators/hc256.c"

int main()
{
    CallerAPI intf = CallerAPI_init();
    GeneratorInfo gi;
    gen_getinfo(&gi);
    GeneratorInfo_bits_to_file(&gi, &intf);
    CallerAPI_free();
    return 0;
}
