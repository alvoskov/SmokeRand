#include "smokerand/entropy.h"

int main()
{
    uint64_t seed = 0;
    for (int i = 0; i < 5; i++) {
        seed += call_rdseed();
    }
    if (!seed) {
        printf("rdseed is not implemented: sum is 0x%llX\n",
            (unsigned long long) seed);
        return 1;
    } else {
        printf("rdseed is implemented: sum is 0x%llX\n",
            (unsigned long long) seed);
        return 0;
    }
}
