#include "smokerand/base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sr_base64_selftest(void)
{
    static const uint32_t x[] = {
        0x00108310, 0x51872092, 0x8b30d38f, 0x41149351,
        0x55976196, 0x9b71d79f, 0x8218a392, 0x59a7a29a,
        0xabb2dbaf, 0xc31cb3d3, 0x5db7e39e, 0xbbf3dfbf};
    static const size_t nwords_max = 12;
    int is_ok = 1;
    for (size_t i = 0; i < nwords_max; i++) {
        char *str = sr_u32_bigendian_to_base64(x, i);
        size_t u32_len;
        uint32_t *u32 = sr_base64_to_u32_bigendian(str, &u32_len);
        if (i != u32_len) {
            fprintf(stderr, "Failure at size = %d: wrong length", (int) i);
            is_ok = 0;
        }
        for (size_t j = 0; j < i; j++) {
            if (x[j] != u32[j]) {
                fprintf(stderr, "Failure at size = %d: wrong values", (int) i);
                is_ok = 0;
            }
        }
        if (i == nwords_max - 1) {
            printf("Base64: %s\n", str);
            printf("Words:  ");
            for (size_t j = 0; j < u32_len; j++) {
                printf("%.8X ", u32[j]);
            }
            printf("\n");
        }
        free(str);
        free(u32);
    }
    return is_ok;
}

int sr_base64_selftest_str()
{
    static const char *str[] = {
        "TheQuickBrownFoxJumpsOverTheLazyDog12345678=",
        "thequickbrownfoxjumpsoverthelazydog90345678=",
        "THEQUICKBROWNFOXJUMPSOVERTHELAZYDOG12345678=",
    };
    int is_ok = 1;
    for (size_t i = 0; i < 3; i++) {
        size_t u32_len;
        uint32_t *u32 = sr_base64_to_u32_bigendian(str[i], &u32_len);
        char *str_dec = sr_u32_bigendian_to_base64(u32, u32_len);
        printf("Input:  %s\nOutput: %s\n", str[i], str_dec);
        if (strcmp(str[i], str_dec)) {
            printf("Failure\n");
            is_ok = 0;
        } else {
            printf("Success\n");
        }        
        free(u32);
        free(str_dec);
    }
    return is_ok;
}

int main()
{
    printf("%d\n", sr_base64_selftest());
    printf("%d\n", sr_base64_selftest_str());
}
