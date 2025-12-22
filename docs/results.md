# Tests results

A table in this document contains pseudorandom number generators results obtained
from SmokeRand, TestU01 and PractRand test suites. The next grading algorithm
was used:

1. An initial grade was obtained from SmokeRand `full` battery.
2. If PRNG failed `birthday` or `freq` battery - then 1 is subtracted for each
   failure.
3. If the grade is 4.0 and PRNG failed some tests in TestU01 or PractRand
   then 0.5 is subtracted.
4. If PRNG is widely known cryptographic and has no known weakness then
   the grade is 5.
5. If PRNG is an experimental cryptographic - then the grade is 4.
6. If it is not proved that PRNG period in the worst case is greater than 60
   then the grade is 0 (written as `(0)` after empirically obtained grade).

 Algorithm         | Output | express | brief | default | full | cpb  | bday64 | Grade | TestU01 | PractRand 
-------------------|--------|---------|-------|---------|------|------|--------|-------|---------|-----------
 a5rand            | u64    | +       | +     | +       | +    | 0.37 |        | 4(0)  |         | >= 2 TiB
 a5rand32          | u32    | +       | 1/2   | 9       |      |      |        |       | Small   | 4 GiB
 a5randw           | u64    | +       | +     | +       | +    | 0.41 |        | 4     |         | >= 1 TiB
 a5rand32w         | u32    | +       | 0/1   | 1       | 1    |      |        |       | +       | 256 GiB
 aesni128          | u64    | +       | +     | +       | +    | 0.89 | +      | 5     | +il     | >= 32 TiB
 aes128(c99)       | u64    | +       | +     | +       | +    | 6.8  | +      | 5     |         | >= 32 TiB
 alfib             | u64    | 2       | 5     | 6       | 8    | 0.23 | +      | 0     | Small   | 128 MiB
 alfib8x5          | u32    | +       | +     | +       | +    | 3.2  | +      | 4     |         | >= 4 TiB
 alfib64x5         | u64    | +       | +     | +       | +    | 0.42 | +      | 4     |         | >= 8 TiB
 alfib_lux         | u32    | +       | 1     | 1       | 1    | 6.1  | +      | 3.75  | +       | 4 GiB
 alfib_mod         | u32    | +       | +     | +       | +    | 0.50 | +      | 3.5   | +       | 1 TiB
 ara32             | u32    | +       | 1     | 1       | 1    | 0.96 | +      | 2(0)  | +       | 512 MiB
 arxfw8            | u32    | +       | 18    |         |      |      | -(>>10)| 0     | -       | 2 MiB
 arxfw8ex          | u32    | +       | 3/5   | 8       |      |      | -(>>10)| 0     | -/Small | 128 MiB
 arxfw8ex2         | u32    | +       | +     | +       | +    |      |        |       | +       | >= 1 TiB
 arxfw16           | u32    | +       | +     | +       | +    | 2.6  | +      | 3.5(0)| +       | 8 TiB
 arxfw32           | u32    | +       | +     | +       | +    | 0.74 | +      | 4(0)  | +       | ?
 arxfw64           | u32    | +       | +     | +       | +    | 0.38 | +      | 4     |         | >= 2 TiB
 biski8_mul        | u32    | 1       | 19    | 33      | 41   | 2.2  | -(>>10)| 0     | -       | 512 KiB
 biski16_mul       | u32    | +       | 2     | 3       | 6    | 1.6  | -      | 0     | -       | 16 GiB
 biski64_mul       | u64    | +       | +     | +       | +    | 0.18 | +      | 4     |         | >= 2 TiB
 biski8            | u32    | +       | 17    | 35      | 35   | 1.4  | -(>>10)| 0     | -       | 2 MiB
 biski8_alt        | u32    | +       | 17    | 31      | 31   | 1.4  | -(>>10)| 0     | -       | 2 MiB
 biski16           | u32    | +       | +     | +       | 1    | 0.81 | +      | 2(0)  | +       | 1 TiB
 biski16_alt       | u32    | +       | +     | +       | +    | 1.1  | +      | 3.5(0)| +       | 1 TiB
 biski32           | u32    | +       | +     | +       | +    | 0.32 | +      | 3.5(0)| +       | >= 8 TiB
 biski32_alt       | u32    | +       | +     | +       | +    | 0.43 | +      | 4(0)  | +       | >= 16 TiB
 biski64           | u64    | +       | +     | +       | +    | <0.1 | +      | 3     | +il     | >= 32 TiB
 biski64_alt       | u64    | +       | +     | +       | +    | 0.15 | +      | 4     |         | >= 32 TiB(?)
 blabla2           | u64    | +       | +     | +       | +    | 0.37 | +      | 4     | +il     | >= 16 TiB
 blabla4           | u64    | +       | +     | +       | +    | 0.58 | +      | 4     |         | >= 8 TiB
 blabla10          | u64    | +       | +     | +       | +    | 1.2  | +      | 4     |         | >= 16 TiB
 chacha            | u32    | +       | +     | +       | +    | 3.3  | +      | 5     | +       | >= 32 TiB
 chacha_avx        | u32    | +       | +     | +       | +    | 2.4  | +      | 5     | +       | >= 32 TiB
 chacha_avx2       | u32    | +       | +     | +       | +    | 1.0  | +      | 5     | +       | >= 16 TiB
 chacha_ctr32      | u32    | +       | +     | +       | 1    | 2.0  | -(>>10)| 0     | +       | 256 GiB
 cmwc4096          | u32    | +       | +     | +       | +    | 0.43 | +      | 4     | +       | >= 32 TiB
 cmwc4827          | u32    | +       | 1     | 1       | 1    | 0.44 | +      | 2     | +       | 512 MiB
 combo             | u32    | +       | 4     | 6       | 8    | 0.75 | +      | 0     | Small   | 4 GiB
 coveyou64         | u32    | 1       | 3     | 4       | 4    | 0.62 | +      | 0     | Small   | 256 KiB
 cswb4288          | u32    | +       | 1     | 1       | 4/5  | 0.90 | +      | 0     | Crush   | >= 32 TiB
 cswb4288_64       | u64    | +       | 1     | 2       | 4/5  | 0.52 | +      | 0     | +lo/+hi | >= 32 TiB
 cwg64             | u64    | +       | +     | +       | +    | 0.30 | +      | 4     | +lo/+hi | >= 16 TiB
 des-ctr           | u64    | +       | +     | +       | +    | 24   | -      | 3     | +IL     | >= 4 TiB
 drand48           | u32    | 3       | 13    | 21      | 23/24| 0.72 | -      | 0     | -       | 1 MiB
 efiix64x48        | u64    | +       | +     | +       | +    | 0.38 | +      | 4     | +IL     | >= 16 TiB
 isaac             | u32    | +       | +     | +       | +    | 1.6  | +      | 4.5   | +       | >= 16 TiB
 isaac64           | u64    | +       | +     | +       | +    | 0.75 | +      | 4.5   | +       | >= 32 TiB
 jctr32            | u32    | +       | +     | +       | +    | 2.4  | +      | 4     | +       | >= 16 TiB
 jctr32_avx2       | u32    | +       | +     | +       | +    | 0.51 | +      | 4     |         | >= 32 TiB
 jctr64            | u64    | +       | +     | +       | +    | 1.1  | +      | 4     |         | >= 16 TiB
 jctr64_avx2       | u64    | +       | +     | +       | +    | 0.42 | +      | 4     | +IL     | >= 16 TiB
 jkiss             | u32    | +       | +     | +       | +    | 0.80 | +      | 4     | +       | >= 16 TiB 
 jkiss32           | u32    | +       | +     | +       | +    | 0.71 | +      | 4     | +       | >= 16 TiB
 jlkiss64          | u64    | +       | +     | +       | +    | 0.50 | +      | 4     |         | >= 16 TiB
 flea32x1          | u32    | +       | 1     | 1       | 1    | 0.48 | +      | 2     | +       | 4 MiB
 gjrand8           | u32    | +       | 4     | 11      | >=15 | 3.5  | -(>>10)| 0     | Small   | 128 MiB
 gjrand16          | u32    | +       | +     | +       | +    | 2.6  | +      | 4(0)  | +       | 8 TiB
 gjrand32          | u32    | +       | +     | +       | +    | 0.69 | +      | 4(0)  | +       | >= 32 TiB
 gjrand64          | u64    | +       | +     | +       | +    | 0.32 | +      | 4     |         | >= 32 TiB
 gmwc128           | u64    | +       | +     | +       | +    | 0.72 | +      | 4     |         | >= 32 TiB
 hc256             | u32    | +       | +     | +       | +    | 1.1  | +      | 5     | +       | >= 32 TiB
 hicg64_u32        | u32    | 1       | 2     | 3       | 3    | 5.4  | +      | 0     | Small   | 32 MiB
 icg31x2           | u32    | +       | +     | +       | 1    | 87   |        | 2     | Crush   | 8 GiB
 icg64             | u32    | +       | +     | +       | +    | 113  |        |       | >=Crush | >= 1 TiB
 icg64_p2          | u32    | 1       | 2     | 3       | 3/4  | 5.1  | +      | 0     | Small   | 32 MiB
 kiss93            | u32    | 1       | 1     | 3       | 5    | 0.82 | +      | 2.75  | Small   | 1 MiB
 kiss96            | u32    | +       | +     | +       | +    | 0.80 | +      | 4     | +       | >= 32 TiB(?)
 kiss99            | u32    | +       | +     | +       | +    | 1.0  | +      | 4     | +       | >= 32 TiB
 kiss64            | u64    | +       | +     | +       | +    | 0.53 | +      | 4     | +       | >= 32 TiB
 kiss11_32         | u32    | +       | +     | +       | +    | 0.96 | +      | 4     | +       | >= 16 TiB
 kiss11_64         | u64    | +       | +     | +       | +    | 0.60 | +      | 4     |         | >= 32 TiB
 kiss4691          | u32    | +       | +     | +       | +    | 1.1  | +      | 4     | +       | >= 32 TiB
 komirand16        | u32    | 1-7     | 19-20 | 40      |      |      |        | 0     | -       | 64 KiB
 komirand16w       | u32    | 1-4     | 20-21 | 39      |      |      |        | 0     | -       | 16 MiB
 komirand32        | u32    | +       | 1     | 2       | 10   | 0.63 | -(>>10)| 0     | Small   | 2-8 GiB
 komirand32w       | u32    | +       | +     | +       | +    | 1.0  | +      | 4(0)  | +       | >= 2 TiB
 komirand          | u64    | +       | +     | +       | +    | 0.49 | +      | 4(0)  |         | >= 16 TiB
 komirandw         | u64    | +       | +     | +       | +    | 0.52 | +      | 4     |         | >= 8 TiB
 kuzn              | u64    | +       | +     | +       | +    | 17   | +      | 5     | +       | >= 4 TiB
 lcg32prime        | u32    | 1       | 13    | 24      | 26/27| 2.2  | -(>>10)| 0     | -       | 512 MiB
 lcg42             | u32    | 5       | 17    | 34      | 36   | 0.66 | -      | 0     | -       | 16 KiB
 lcg64             | u32    | 1       | 6     | 8       | 11   | 0.40 | +      | 0     | Small   | 16 MiB
 lcg64prime        | u64    | +       | 1     | 1       | 1    | 1.5  | -      | 0     | +-      | >= 32 TiB
 lcg64sc           | u64    | +       | +     | +       | +    |      | -      | 3     |+HI/+LO  | >= 2 TiB
 lcg64sc2          | u32    | +       | +     | +       | +    |      | +      | 4     |         | >= 1 TiB
 lcg96             | u32    | +       | 1     | 1       | 1    | 0.78 | +      | 3     | +       | 32 GiB
 lcg128            | u64    | +       | 1     | 1       | 1    | 0.35 | +      | 3     | +       | 64 GiB
 lcg128_full       | u64    | +       | 1     | 1       | 1    | 0.42 | +      | 3     | +       | 64 GiB
 lcg128_u32_full   | u32    | +       | +     | 1       | 1    | 0.75 | +      | 3     | +       | >= 32 TiB
 lcg69069          | u32    | 6       | 20    | 38/39   | 43/44| 0.38 | -(>>10)| 0     | -       | 2 KiB
 lea128            | u32    | +       | +     | +       | +    | 5.7  | +      | 5     | +       | >= 32 TiB
 lea128_avx        | u32    | +       | +     | +       | +    | 1.2  | +      | 5     | >= Crush| >= 32 TiB
 lfib_par[31+]     | u32    | 1       | 6/7   | 8/0     | 11/12| 0.70 | +      | 0     | -       | 32 MiB
 lfib_par[55+]     | u32    | 1       | 5     | 6       | 8    | 0.51 | +      | 0     | -       | 2 GiB
 lfib_par[55-]     | u32    | 1       | 5     | 6       | 8    | 0.51 | +      | 0     | -       | 2 GiB
 lfib_par[127+]    | u32    | 1       | 4     | 4       | 5    | 0.48 | +      | 0     | -/Small | 512 MiB
 lfib_par[127-]    | u32    | 1       | 4     | 4       | 5    | 0.48 | +      | 0     | -/Small | 512 MiB
 lfib_par[258+]    | u32    | 1       | 4     | 4       | 5    | 0.44 | +      | 0     | Small   | 8 GiB
 lfib_par[258-]    | u32    | 1       | 4     | 4       | 5    | 0.46 | +      | 0     | Small   | 8 GiB
 lfib_par[378+]    | u32    | 1       | 4     | 4       | 5    | 0.46 | +      | 0     | Small   | 32 GiB
 lfib_par[378-]    | u32    | 1       | 4     | 4       | 5    | 0.45 | +      | 0     | Small   | 32 GiB
 lfib_par[607+]    | u32    | 1       | 4     | 4       | 5    | 0.41 | +      | 0     | Small   | 256 GiB
 lfib_par[607-]    | u32    | 1       | 4     | 4       | 5    | 0.40 | +      | 0     | Small   | 256 GiB
 lfib_par[1279+]   | u32    | 1       | 3/4   | 3/4     | 4/5  | 0.40 | +      | 0     | Crush   | 1 TiB
 lfib_par[1279-]   | u32    | 1       | 3/4   | 3/4     | 4/5  | 0.40 | +      | 0     | Crush   | 1 TiB
 lfib_par[2281+]   | u32    | +       | 3     | 3       | 4    | 0.38 | +      | 0     | +       | 8 TiB
 lfib_par[2281-]   | u32    | 0/1     | 3     | 3       | 4    | 0.38 | +      | 0     | +       | 8 TiB
 lfib_par[3217+]   | u32    | +       | 1     | 1       | 1/2  | 0.39 | +      | 2     | +       | 16 TiB
 lfib_par[3217-]   | u32    | +       | 1     | 1       | 2/4  | 0.39 | +      | 2     | +       | 16 TiB
 lfib_par[4423+]   | u32    | +       | 1     | 1       | 1    | 0.45 | +      | 2     | +       | 128 TiB(?)
 lfib_par[4423-]   | u32    | +       | 1     | 1       | 1    | 0.45 | +      | 2     | +       | >= 16 TiB
 lfib_par[9689+]   | u32    | +       | 1     | 1       | 1    | 0.47 | +      | 2     | +       | >= 16 TiB
 lfib_par[9689-]   | u32    | +       | 1     | 1       | 1    | 0.47 | +      | 2     | +       | >= 16 TiB
 lfib_par[19937+]  | u32    | +       | +     | 1       | 1    | 0.46 | +      | 2     | +       | >= 2 TiB
 lfib_par[19937-]  | u32    | +       | +     | 1       | 1    | 0.48 | +      | 2     | +       | >= 16 TiB
 lfib_par[44497+]  | u32    | +       | +     | 1       | 1    | 0.49 | +      | 2     | +       | >= 8 TiB
 lfib_par[44497-]  | u32    | +       | +     | 1       | 1    | 0.49 | +      | 2     | +       | >= 2 TiB
 lfib_par[110503+] | u32    | +       | +     | +       | +    | 0.52 | +      | 4     | +       | >= 2 TiB
 lfib_par[110503-] | u32    | +       | +     | +       | +    | 0.50 | +      | 4     | +       | >= 1 TiB
 lfib4             | u32    | 1       | 1     | 3       | 4    | 0.37 | +      | 3     | +       | 32 MiB
 lfib4_u64         | u32    | +       | +     | +       | +    | 0.34 | +      | 4     | +       | >= 32 TiB
 lfsr113           | u32    | 2       | 3     | 5       | 7    | 1.1  | +      | 2.25  | Small   | 32 KiB 
 lfsr258           | u64    | 2       | 3     | 5       | 7    | 0.75 | +      | 2.25  | Small   | 1 MiB
 lrnd64_255        | u64    | 2       | 5     | 10      | 15   | 0.45 | +      | 0     | Small   | 512 KiB
 lrnd64_1023       | u64    | 2       | 3     | 5       | 7    | 0.44 | +      | 2.25  | Small   | 4 MiB
 lxm_64x128        | u64    | +       | +     | +       | +    | 0.42 | +      | 4     |         | >= 32 TiB
 macmarsa          | u32    | 2       | 12    | 18      | 19   | 0.67 | -(>>10)| 0     | -       | 128 KiB
 magma             | u64    | +       | +     | +       | +    | 25   |        |       | +       | >= 1 TiB
 magma_avx-ctr     | u64    | +       | +     | +       | +    | 7.1  | -      | 3     |         | >= 16 TiB
 magma_avx-cbc     | u64    | +       | +     | +       | +    | 7.1  | +      | 4     |         | >= 2 TiB
 melg607           | u64    | 2       | 3     | 5       | 7    | 0.73 | +      | 2.25  | Small   | 8 MiB
 melg19937         | u64    | +       | 3     | 3       | 3    | 0.73 | +      | 3.25  | Small   | 256 GiB
 melg44497         | u64    | +       | +     | 3       | 3    | 0.75 | +      | 3.25  | Small   | 2 TiB
 minstd            | u32    | 1       | 16    | 32      | 37   | 1.4  | -(>>10)| 0     | -       | 256 KiB
 mixmax_low32      | u32    | +       | +     | +       | +    | 1.7  | +      | 4     | +       | >= 16 TiB
 mlfib17_5         | u32    | +       | +     | +       | +    | 0.48 | +      | 4     | +       | >= 32 TiB
 mt19937           | u32    | +       | 3     | 3       | 3    | 0.59 | +      | 3.25  | Small   | 128 GiB
 mt19937_64        | u64    | +       | 3     | 3       | 3    | 0.45 | +      | 3.25  | Small   | 256 GiB
 mt19937_64_full   | u64    | +       | 3     | 3       | 3    | 0.46 | +      | 3.25  | Small   | 256 GiB
 mtc8              | u32    | 1       | 20/21 | 35      | 39   | 1.9  |        | 0     | -       | 2 MiB
 mtc16             | u32    | +       | +     | +       | +    | 1.3  | +      | 3.5(0)| +       | 512 GiB(stdin32)*
 mtc32             | u32    | +       | +     | +       | +    | 0.39 | +      | 4(0)  | >= Crush| >= 4 TiB
 mtc64             | u64    | +       | +     | +       | +    | 0.21 | +      | 4     |         | >= 16 TiB
 mtc64hi           | u64    | +       | +     | +       | +    | 0.40 | +      | 4     |         | >= 2 TiB
 mrg32k3a          | u32    | +       | +     | +       | +    | 2.5  | +      | 3.5   | +       | 2 TiB
 msws              | u32    | +       | +     | +       | +    | 0.72 | +      | 4     | +       | >= 16 TiB
 msws_ctr          | u64    | +       | +     | +       | +    | 0.37 | +      | 4     |         | >= 8 TiB
 msws64            | u64    | +       | +     | +       | +    | 0.41 | +      | 4     |         | >= 32 TiB
 msws64x           | u64    | +       | +     | +       | +    | 0.50 | +      | 4     |         | >= 32 TiB
 mularx64_r2       | u32    | +       | 1     | 1       | 1    | 1.5  | -      | 1     |         | 1 TiB
 mularx64_u32      | u32    | +       | +     | +       | +    | 1.7  | -      | 3     |         | >= 2 TiB
 mularx128         | u64    | +       | +     | +       | +    | 0.50 | +      | 4     |         | >= 4 TiB
 mularx128_u32     | u32    | +       | +     | +       | +    | 0.95 | +      | 4     |         | >= 32 TiB
 mularx128_str     | u64    | +       | +     | +       | +    | 0.38 | +      | 4     |         | >= 32 TiB
 mularx256         | u64    | +       | +     | +       | +    | 0.67 | +      | 4     |         | >= 32 TiB
 mulberry32        | u32    | +       | 1     | 2/3     | 5    | 0.51 | -(>>10)| 0     | Small   | 512 MiB
 mwc32x            | u32    | +       | 3     | 4       | 8    | 1.5  | -(>>10)| 0     | Small   | 128 MiB
 mwc32xxa8         | u32    | +       | 1     | 4       | 10   | 1.9  | -(>>10)| 0     | Small   | 256 MiB
 mwc40xxa8         | u32    | +       | +     | +       | 1    | 2.1  | -(>>10)| 0     | Crush   | 16 GiB
 mwc48xxa16        | u32    | +       | +     | +       | +    | 1.2  | +      | 4     | +       | 1 TiB
 mwc64             | u32    | +       | 1     | 2       | 4    | 0.37 | -      | 0     | Small   | 1 TiB
 mwc64x            | u32    | +       | +     | +       | +    | 0.53 | +      | 4     | +       | >= 32 TiB
 mwc64_2p58        | u64    | 2       | 9     | 17      | 17   | 0.26 | +      | 0     | -       | 128 KiB
 mwc128            | u64    | +       | +     | +       | +    | 0.30 | +      | 4     | +       | >= 16 TiB
 mwc128x           | u64    | +       | +     | +       | +    | 0.30 | +      | 4     | +       | >= 32 TiB
 mwc128xxa32       | u32    | +       | +     | +       | +    | 0.52 | +      | 4     | +       | >= 32 TiB
 mwc256xxa64       | u64    | +       | +     | +       | +    | 0.26 | +      | 4     |         | >= 32 TiB
 mwc1616           | u32    | 1       | 10/11 | 13/19   | 20   | 0.48 | -      | 0     | -/Small | 16 MiB
 mwc1616p          | u32    | +       | +     | +       | +    | 0.55 | +      | 4     | >= Crush| 16 TiB
 mwc1616x          | u32    | +       | +     | +       | +    | 0.60 | +      | 3.5   | +       | 32 TiB
 mwc3232x          | u64    | +       | +     | +       | +    | 0.30 | +      | 4     |+il      | >= 32 TiB
 mwc4691           | u32    | +       | 1     | 1       | 1    | 0.45 | +      | 2     | +       | 1 GiB
 mwc8222           | u32    | +       | +     | +       | +    | 0.59 | +      | 4     | +       | >= 32 TiB
 mzran13           | u32    | 1       | 4     | 8/9     | 11   | 1.2  | +      | 0     | Small   | 64 KiB
 ncombo            | u32    | 2       | 5     | 8/9     | 11   | 1.4  | +      | 0     | Small   | 64 KiB
 pcg32             | u32    | +       | +     | +       | +    | 0.44 | +      | 3.5   | +       | 32 TiB
 pcg32_dxsm        | u32    | +       | +     | +       | +    | 0.48 | +      | 4     | +       | >= 32 TiB
 pcg32_xsl_rr      | u32    | +       | +     | +       | +    | 0.58 | +      | 4     | +       | 256 GiB
 pcg64             | u64    | +       | +     | +       | +    | 0.28 | -      | 3     | +       | >= 32 TiB
 pcg64_dxsm        | u64    | +       | +     | +       | +    | 0.53 | +      | 4     |         | >= 16 TiB
 pcg64_xsl_rr      | u64    | +       | +     | +       | +    | 0.43 | +      | 4     |         | >= 32 TiB
 philox            | u64    | +       | +     | +       | +    | 1.0  | +      | 4     | +       | >= 32 TiB
 philox2x32        | u32    | +       | +     | +       | +    | 1.6  | -      | 3     | +       | >= 32 TiB
 philox32          | u32    | +       | +     | +       | +    | 1.6  | +      | 4     | +       | >= 32 TiB
 prvhash12c        | u32    | +       | 0/1   | 0/1     |      |      |        | 0     |         | 8 GiB
 prvhash12cw       | u32    | +       | +     | +       |      |      |        |       |         | ?
 prvhash16c        | u32    | +       | +     | +       | +    | 2.4  |        |       |         | 256 GiB
 prvhash16cw       | u32    | +       | +     | +       | +    | 2.4  |        |       |         | ?
 prvhash64c        | u64    | +       | +     | +       | +    | 0.51 | +      | 4(0)  |         | ?
 prvhash64cw       | u64    | +       | +     | +       | +    | 0.41 | +      | 4     |         | >= 16 TiB
 ran               | u64    | +       | +     | +       | +    | 0.43 | +      | 4     |         | >= 32 TiB
 ranq1             | u64    | 1       | 1     | 3       | 6    | 0.32 | -      | 0     |S_lo/+_hi| 512 KiB
 ranq2             | u64    | +       | +     | 1       | 2    | 0.33 | +      | 3.5   |+_lo/+_hi| 2 MiB
 randu             | u32    | 6       | 23    | 41      | 45   | 0.41 | -(>>10)| 0     | -       | 1 KiB
 ranlux++          | u64    | +       | +     | +       | +    | 2.4  | +      | 4     | +       | >= 32 TiB
 ranrot_bi         | u64    | +       | +     | 1       | 2/4  | 0.33 | +      | 0     |>=C(IL)  | 8 GiB
 ranrot32[7/3]     | u32    | +       | 3     | 5/6     | 6    | 0.58 | +      | 0     | Small   | 128 MiB
 ranrot32[17/9]    | u32    | +       | 1     | 2       | 4    | 0.68 | +      | 0     | +       | 1 GiB
 ranrot32[57/13]   | u32    | +       | +     | +       | 1    | 0.74 | +      | 2     | +       | 8 GiB
 ranrot8tiny       | u32    | +       | 8     | 24      | 27   | 2.0  | -(>>10)| 0     | -       | 4 MiB
 ranrot16tiny      | u32    | +       | +     | +       | 1    | 1.0  | -      | 2(0)  | Crush   | 8 GiB
 ranrot32tiny      | u32    | +       | +     | +       | +    | 0.41 | +      | 3(0)  | +       | 2 TiB
 ranrot64tiny      | u64    | +       | +     | +       | +    | 0.21 | +      | 4     | +il     | >= 16 TiB
 ranshi            | u64    | +       | 2     | 7       | 8    | 0.43 | +      | 0     | -(IL)   | 32 KiB
 ranshi_upper32    | u32    | +       | +     | +       | +    | 0.86 | +      | 3.5   | +       | 8 TiB
 ranshi_lower32    | u32    | +       | +     | +       | +    | 0.86 | +      | 4     | +       | >= 32 TiB
 ranval            | u32    | +       | +     | +       | +    | 0.31 | +      | 4(0)  | +       | >= 32 TiB
 ranval64          | u64    | +       | +     | +       | +    | 0.23 | +      | 4(0)  |         | >= 16 TiB
 r250              | u32    | 3       | 7     | 10      | 13   | 0.50 | +      | 0     | -       | 1 MiB
 r1279             | u32    | 2       | 5     | 7       | 10   | 0.47 | +      | 0     | Small   | 64 MiB
 ranlux[0]         | u32    | 1       | 6     | 9       | 12   | 6.8  | N/A    | 0     | -       | 4 MiB
 ranlux[1]         | u32    | +       | +     | 2       | 3    | 13   | N/A    | 0     | Small   | 4 GiB
 ranlux[2]         | u32    | +       | +     | +       | +    | 27   | N/A    |       |         | >= 2 TiB
 rc4               | u32    | +       | +     | +       | +    | 6.0  | +      | 3     | +       | 512 GiB
 rc4ok             | u32    | +       | +     | +       | +    | 6.2  | N/A    | 4.5   | +       | >= 32 TiB
 rge256lite        | u32    | +       | +     | +       | +    | 5.2  | +      | 4(0)  | +       | >= 1 TiB
 rge256ex          | u32    | +       | +     | +       | +    | 0.62 | +      | 4     | +       | >= 16 TiB
 rge256ex-ctr      | u32    | +       | +     | +       | +    | 1.8  |        | 4     | +       | >= 2 TiB
 rge512ex          | u64    | +       | +     | +       | +    | 0.34 | +      | 4     | +(IL)   | >= 8 TiB
 rge512ex-ctr      | u64    | +       | +     | +       | +    | 0.85 | +      | 4     |         | >= 1 TiB
 rge512ex-ctr-avx2 | u64    | +       | +     | +       | +    | 0.39 | +      | 4     |+IL,+H   | >= 16 TiB
 romutrio          | u64    | +       | +     | +       | +    | 0.15 | +      | 4(0)  |         | >= 32 TiB
 rrmxmx            | u64    | +       | +     | +       | +    | 0.14 | -      | 3     |         | >= 16 TiB
 sapparot          | u32    | +       | 2     | 3       | 5    | 0.70 | +      | 0     | Crush   | 8 MiB
 sapparot2         | u32    | +       | +     | +       | +    | 0.42 | +      | 3.5(0)| +       | 2 TiB
 sapparot2_64      | u64    | +       | +     | +       | +    | 0.27 | +      | 4(0)  |         | >= 16 TiB
 sezgin63          | u32    | +       | +     | 1       | 3    | 3.0  | -      | 0     | Crush   | >= 32 TiB
 sfc8              | u32    | +       | 3     | 7       | 14   | 1.9  | -(>>10)| 0     | -       | 128 MiB
 sfc16             | u32    | +       | +     | +       | +    | 0.93 | +      | 3.5(0)| +       | 128 GiB(stdin32)*
 sfc32             | u32    | +       | +     | +       | +    | 0.24 | +      | 4(0)  | +       | >= 16 TiB
 sfc64             | u64    | +       | +     | +       | +    | 0.10 | +      | 4     | +       | >= 16 TiB
 skiss32           | u32    | +       | +     | +       | +    | 1.7  | +      | 4     | +       | >= 16 TiB
 skiss64           | u64    | +       | +     | +       | +    | 0.86 | +      | 4     |         | >= 8 TiB
 smwc16x8          | u32    | +       | +     | +       | +    | 1.2  | +      | 4     | >= Crush| >= 4 TiB
 smwc192bad        | u64    | +       | +     | +       | +    | 0.19 | +      | 4     |         | >= 16 TiB(?)
 speck64_128       | u64    | +       | +     | +       | +    | 6.1  | -      | 3     |         | >= 4 TiB
 speck128          | u64    | +       | +     | +       | +    | 3.8  | +      | 5     | >= Crush| >= 32 TiB
 speck128_avx(full)| u64    | +       | +     | +       | +    | 0.75 | +      | 5     |         | >= 16 TiB
 speck128_avx(r16) | u64    | +       | +     | +       | +    | 0.38 | +      | 4     |         | >= 32 TiB
 splitmix          | u64    | +       | +     | +       | +    | 0.12 | -      | 3     | +       | >= 32 TiB
 splitmix_g1       | u64    | +       | 1     | 1       | 2    | 0.12 | -      | 0.75  |sIL/>=CLH| 8 GiB
 splitmix32        | u32    | +       | 3     | 4       | 5/7  | 0.25 | -(>>10)| 0     | Small   | 1 GiB
 splitmix32cbc     | u32    | +       | +     | +       | +    | 2.1  | -      | 3     | +       | 8 TiB
 sqxor             | u64    | +       | +     | +       | +    | 0.13 | +      | 4     | +       | >= 16 TiB
 sqxor32           | u32    | +       | 2     | 3       | 5    | 0.20 | -(>>10)| 0     | Small   | 16 GiB
 stormdrop         | u32    | +       | +     | +       | 1    | 1.2  | +      | 0     | +       | >= 32 TiB
 stormdrop_old     | u32    | +       | +     | 1       | 2    | 1.4  | +      | 3.5(0)| Small   | 1 MiB
 superduper73      | u32    | 3       | 11    | 17      | 21   | 0.64 | +      | 0     | -       | 32 KiB
 superduper96      | u32    | 1       | 2     | 5/6     | 9/10 | 0.71 | +      | 0     | Small   | 128 KiB
 superduper64      | u64    | 1       | 1     | 3       | 5    | 0.35 | +      | 2.75  | Small   | 512 KiB
 superduper64_u32  | u32    | +       | +     | +       | +    | 0.70 | +      | 4     | +       | >= 32 TiB
 shr3              | u32    | 2       | 17    | 33      | 37(?)| 0.76 | -(>>10)| 0     | -       | 32 KiB
 swb               | u32    | 1       | 6     | 7       | 9    | 3.2  | +      | 0     | Small   | 128 MiB
 swblux[luxury=1]  | u32    | +       | +     | +       | 0/1  | 6.3  | N/A    | 2     | Crush   | 4 TiB
 swblux[luxury=2]  | u32    | +       | +     | +       | +    | 9.1  | N/A    | 4     | +       | >= 8 TiB
 swblarge          | u32    | 1       | 4     | 5       | 8    | 0.56 | +      | 0     | Crush   | 512 GiB
 swbmwc32          | u32    | +       | 1     | 1       | 1    | 0.87 | +      | 0     | Small   | 128 GiB
 swbmwc64          | u64    | +       | +     | +       | +    | 0.42 | +      | 4     |+_lo/+_hi| >= 32 TiB
 swbw              | u32    | +       | 1     | 1       | 1    | 2.8  | +      | 2     | +       | 4 GiB
 taus88            | u32    | 2       | 3     | 5       | 7    | 0.74 | +      | 2.25  | Small   | 32 KiB
 tinymt32          | u32    | 1       | 2     | 4       | 6    | 1.5  | +      | 0     | +       | 4 GiB
 tinymt64          | u64    | 1       | 1     | 2       | 4    | 2.7  | +      | 3     |+_lo/+_hi| 32 GiB
 threefry          | u64    | +       | +     | +       | +    | 1.0  | +      | 4     | +       | >= 32 TiB
 threefry_avx      | u64    | +       | +     | +       | +    | 0.39 | +      | 4     |         | >= 8 TiB
 threefish         | u64    | +       | +     | +       | +    | 4.3  | +      | 5     |         | >= 32 TiB
 threefish_avx     | u64    | +       | +     | +       | +    | 1.3  | +      | 5     |         | >= 8 TiB
 threefish1024     | u64    | +       | +     | +       | +    | 4.4  | +      | 5     | +(il)   | >= 16 TiB
 threefish1024_avx | u64    | +       | +     | +       | +    | 1.3  | +      | 5     | +(il)   | >= 2 TiB
 threefry2x64      | u64    | +       | +     | +       | +    | 1.3  | +      | 4     |         | >= 16 TiB
 threefry2x64_avx  | u64    | +       | +     | +       | +    | 0.45 | +      | 4     |         | >= 32 TiB
 tylo64            | u64    | +       | +     | +       | +    | 0.17 | +      | 4     |>=C(il)  | >= 32 TiB
 ultra             | u32    | +       | +     | +       | 1    | 0.81 | +      | 2     | +       | 4 GiB
 ultra64           | u64    | +       | +     | +       | +    | 0.37 | +      | 4     |+_lo/+_hi| >= 16 TiB
 v3b               | u32    | +       | +     | +       | +    | 0.78 | +      | 4     | +       | >= 32 TiB
 wich1982          | u32    | +       | 5     | 11      | 13   | 2.3  | -      | 0     | -       | 256 GiB
 wich2006          | u32    | +       | +     | +       | +    | 4.6  | +      | 4     | +       | >= 16 TiB
 well1024a         | u32    | 2       | 3     | 5       | 7    | 1.0  | +      | 2.25  | Small   | 64 MiB
 wob2m             | u64    | +       | +     | +       | +    | 0.24 | +      | 4     |+_lo/+_hi| >= 32 TiB
 wyrand            | u64    | +       | +     | +       | +    | 0.12 | +      | 4     |         | >= 32 TiB
 xabc8             | u32    | +       | 8     | 15      | 22   | 3.7  | -(>>10)| 0     | -       | 8 MiB
 xabc16            | u32    | +       | +     | 1       | 1    | 1.6  | +      | 2     | Small   | 64 GiB
 xabc32            | u32    | +       | +     | +       | +    | 0.82 | +      | 4(0)  | +       | 16 TiB
 xabc64            | u64    | +       | +     | +       | +    | 0.40 | +      | 4     |+IL/+H   | 4 TiB
 xkiss8_awc        | u32    | +       | +     | +       | +    | 3.2  | +      | 4     | +       | >= 16 TiB
 xkiss16_awc       | u32    | +       | +     | +       | +    | 1.6  | +      | 4     | +       | >= 32 TiB
 xkiss16sh_awc     | u32    | +       | +     | +       | +    | 2.1  | +      | 3     | +       | 16 GiB
 xkiss32_awc       | u32    | +       | +     | +       | +    | 0.66 | +      | 4     | +       | >= 32 TiB
 xkiss32sh_awc     | u32    | +       | +     | +       | +    | 0.99 | +      | 4     | +       | >= 16 TiB
 xkiss64_awc       | u64    | +       | +     | +       | +    | 0.40 | +      | 4     |         | >= 16 TiB
 xorgens           | u64    | +       | +/1   | 1       | 1    | 0.41 | +      | 3.75  |         | 2 TiB
 xoroshiro32       | u32    | 2       | 14    | 28/29   | 37/39| 1.4  | -(>>10)| 0     | -       | 32 KiB
 xoroshiro32pp     | u32    | +       | 1     | 2       | 6/8  | 1.5  | -(>>10)| 0     | Small   | 256 MiB
 xoroshiro64pp     | u32    | +       | +     | +       | +    | 0.52 | +      | 4     | +       | >= 8 TiB
 xoroshiro64st     | u32    | 1       | 1     | 3       | 5    | 0.51 | -      | 1.75  | Small   | 1 MiB
 xoroshiro64stst   | u32    | +       | +     | +       | +    | 0.61 | -      | 3     |         | >= 32 TiB
 xorshift64        | u64    | 2       | 6     | 12      | 15/16| 0.49 | -      | 0     | -       | 32 KiB
 xorshift64st      | u64    | 1       | 1     | 3       | 5    | 0.48 | -      | 1.75  |S_lo/+_hi| 512 KiB
 xorshift128       | u32    | 2       | 5     | 7/8     | 9    | 0.41 | +      | 0     | -       | 128 KiB
 xorshift128p      | u64    | 1       | 1     | 2       | 3    | 0.26 | +      | 0     |+lo/+hi  | 32 GiB
 xorshift128pp     | u64    | +       | +     | +       | +    | 0.29 | +      | 4     |         | >= 16 TiB
 xorshift128pp_avx | u64    | +       | +     | +       | +    | 0.15 | +      | 4     |         | >= 1 TiB
 xorshift128rp     | u64    | +       | +     | 1       | 3/5  | 0.21 | +      | 0     | Small   | 4 GiB
 xoroshiro128      | u64    | 2       | 3     | 5       | 7    | 0.27 | +      | 2.25  | Small   | 256 KiB
 xoroshiro128aox   | u64    | +       | +     | +       | +    | 0.35 | +      | 4     |+lo/+hi  | >= 32 TiB
 xoroshiro128p     | u64    | 1       | 1     | 2       | 3    | 0.16 | +      | 3.25  |+lo/+hi  | 16 MiB
 xoroshiro128pp    | u64    | +       | +     | +       | +    | 0.26 | +      | 4     |         | >= 32 TiB
 xoroshiro128pp_avx| u64    | +       | +     | +       | +    | 0.16 | +      | 4     |         | >= 1 TiB
 xoroshiro1024st   | u64    | 1       | 1     | 1       | 2    | 0.33 | +      | 3.5   |+lo/+hi  | 128 GiB
 xoroshiro1024stst | u64    | +       | +     | +       | +    | 0.33 | +      | 4     | +       | >= 16 TiB
 xorwow            | u32    | 1       | 3     | 7       | 9    | 0.52 | +      | 0     | Small   | 128 KiB
 xoshiro128p       | u32    | 1       | 1     | 2       | 4    | 0.38 | +      | 3     | +       | 8 MiB
 xoshiro128pp      | u32    | +       | +     | +       | +    | 0.42 | +      | 4     | +       | >= 16 TiB
 xoshiro256p       | u64    | 1       | 1     | 2       | 3    | 0.20 | +      | 3.25  |         | 64 MiB
 xoshiro256pp      | u64    | +       | +     | +       | +    | 0.22 | +      | 4     |         | >= 4 TiB
 xoshiro256stst    | u64    | +       | +     | +       | +    | 0.22 | +      |       |         | ?
 xsh               | u64    | 2       | 9     | 14      | 18   | 0.43 | -      | 0     | -       | 32 KiB
 xtea              | u64    | +       | +     | +       | +    | 27   | -      | 3     | >= Crush| >= 4 TiB
 xtea_avx(ctr)     | u64    | +       | +     | +       | +    | 2.3  | -      | 3     | >= Crush| >= 32 TiB
 xtea_avx(cbc)     | u64    | +       | +     | +       | +    | 2.3  | +      | 4     | >= Crush| >= 8 TiB
 xtea2             | u32    | +       | +     | +       | +    | 12   |        |       | >= Crush| >= 8 TiB
 xtea2_64          | u64    | +       | +     | +       |      | 28   |        |       |         | ?
 xxtea128          | u32    | +       | +     | +       | +    | 18   | +      | 4.5   | >= Crush| >= 1 TiB
 xxtea128_avx      | u32    | +       | +     | +       | +    | 2.7  | +      | 4.5   | >= Crush| >= 32 TiB
 xxtea256          | u32    | +       | +     | +       | +    | 12   | +      | 4.5   | >= Crush| >= 1 TiB
 xxtea256_avx      | u32    | +       | +     | +       | +    | 1.9  | +      | 4.5   | >= Crush| >= 32 TiB
 ziff98            | u32    | +       | 3     | 3       | 3    | 0.47 | +      | 3.25  | Small   | 32 GiB

Performance estimation for some 64-bit generators

 Generator                | ising    | bday64   | usphere  | default
--------------------------|----------|----------|----------|----------
 biski64                  |          | 00:03:52 | 00:03:49 | 00:04:01
 sfc64                    | 00:10:56 | 00:03:41 | 00:03:36 | 00:03:42
 xoroshiro128++           | 00:11:04 | 00:03:13 | 00:03:45 | 00:03:30
 wyrand                   | 00:11:01 | 00:03:05 | 00:03:23 | 00:03:25
 mwc256xxa64              | 00:11:28 | 00:04:01 | 00:03:43 | 00:03:31
 tylo64                   | 00:11:11 | 00:03:52 | 00:04:00 | 00:03:29
 wob2m                    | 00:11:48 | 00:03:17 | 00:03:21 | 00:03:43
 pcg64_xsl_rr             | 00:12:01 | 00:04:22 | 00:05:22 | 00:04:38
 speck128/128-vector-full | 00:15:45 | 00:06:53 | 00:06:50 | 00:04:51
 aesni                    | 00:15:43 | 00:08:32 | 00:09:44 | 00:05:08
 speck128/128-scalar      | 00:31:18 | 00:24:56 | 00:23:19 | 00:10:52


Performance estimation for some 32-bit generators

 Generator                | ising    | bday64   | usphere  | default
--------------------------|----------|----------|----------|----------
 kiss99                   | 00:12:48 |          |          |
 chacha:avx2              |          |          |          |
 chacha:c99               | 00:19:50 |          |          |


Note about `xorshift128+`: an initial rating was 3.25, manually corrected to 0
because the upper 32 bits systematically fail the `hamming_ot_distr` and
`hamming_ot_value` tests from the `full` battery.

Note about `kiss96`: probably it has some subtle artefacts, PractRand 0.94
returned `unusual` and `mildly suspicious` values at 8-32 TiB samples, extra
testing is required.

    rng=RNG_stdin32, seed=unknown
    length= 4 terabytes (2^42 bytes), time= 12681 seconds
      no anomalies in 323 test result(s)

    rng=RNG_stdin32, seed=unknown
    length= 8 terabytes (2^43 bytes), time= 25328 seconds
      Test Name                         Raw       Processed     Evaluation
      [Low8/32]FPF-14+6/16:cross        R=  -2.5  p =1-2.2e-4   unusual
      ...and 330 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 16 terabytes (2^44 bytes), time= 50520 seconds
      Test Name                         Raw       Processed     Evaluation
      [Low8/32]FPF-14+6/16:cross        R=  -2.7  p =1-5.7e-5   mildly suspicious
      ...and 338 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 32 terabytes (2^45 bytes), time= 102780 seconds
      Test Name                         Raw       Processed     Evaluation
      [Low8/32]FPF-14+6/16:cross        R=  -2.7  p =1-3.9e-5   mildly suspicious
      ...and 346 test result(s) without anomalies

Note about `mt19937` and `philox`: speed significantly depends on gcc optimization settings:
e.g. changing `-O2` to `-O3` speeds up `mt19937` but slows down `philox`; gcc 10.3.0 (tdm64-1).

Note about `mrg32k3a`: it fails the `FPF-14+6/16:cross` test from PractRand at 4 TiB sample.
The failure is systematic and reproducible.

Note about `mtc16`: if its output is processed as `stdin16` by PractRand 0.94 then it
fails after 256 GiB, not after 512 GiB.

Note about `sfc16`: if its output is processed as `stdin16` by PractRand 0.94
then it passes it at >= 1 TiB. But if it is tested as 32-bit PRNG in `stdin32`
mode then it fails PractRand 0.94 at 256 GiB:

    rng=RNG_stdin32, seed=unknown
    length= 64 gigabytes (2^36 bytes), time= 183 seconds
      no anomalies in 263 test result(s)

    rng=RNG_stdin32, seed=unknown
    length= 128 gigabytes (2^37 bytes), time= 362 seconds
      Test Name                         Raw       Processed     Evaluation
      [Low8/32]Gap-16:A                 R=  +6.3  p =  1.7e-4   unusual
      [Low8/32]Gap-16:B                 R=  +8.9  p =  1.7e-7   very suspicious
      ...and 271 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 256 gigabytes (2^38 bytes), time= 699 seconds
      Test Name                         Raw       Processed     Evaluation
      [Low8/32]Gap-16:A                 R= +11.1  p =  1.2e-7   very suspicious
      [Low8/32]Gap-16:B                 R= +22.6  p =  3.2e-19    FAIL !
      ...and 282 test result(s) without anomalies

Note about `pcg32`: it fails the TMFn test from PractRand at 64 TiB!

Run 1:

    rng=RNG_stdin32, seed=unknown
    length= 32 terabytes (2^45 bytes), time= 105157 seconds
      Test Name                         Raw       Processed     Evaluation
      TMFn(2+11):wl                     R= +25.4  p~=   2e-8    very suspicious
      ...and 346 test result(s) without anomalies

Run 2:

    rng=RNG_stdin32, seed=unknown
    length= 32 terabytes (2^45 bytes), time= 98476 seconds
      Test Name                         Raw       Processed     Evaluation
      TMFn(2+12):wl                     R= +21.1  p~=   2e-6    mildly suspicious
      ...and 346 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 64 terabytes (2^46 bytes), time= 196881 seconds
      Test Name                         Raw       Processed     Evaluation
      TMFn(2+11):wl                     R= +25.5  p~=   1e-8    very suspicious
      TMFn(2+12):wl                     R= +47.6  p~=   1e-21     FAIL !!
      TMFn(2+13):wl                     R= +20.6  p~=   3e-6    unusual
      ...and 349 test result(s) without anomalies


About `lcg64prime`: it passes BigCrush if upper 32 bits are returned, but
fails it in interleaved mode (fails test N15 `BirthdaySpacings, t = 4`).

About `lfib_par`: `LFib(4423,2098,+)` passes PractRand even at 128 TiB. However
the observed `unusual` is typical for additive/subtractive lagged Fibonacci
generators prior failure. The failure is expected at 0.25-0.5 PiB.

    rng=RNG_stdin32, seed=unknown
    length= 64 terabytes (2^46 bytes), time= 205527 seconds
      no anomalies in 352 test result(s)

    rng=RNG_stdin32, seed=unknown
    length= 128 terabytes (2^47 bytes), time= 405531 seconds
      Test Name                         Raw       Processed     Evaluation
      [Low1/32]BCFN(2+5,13-0,T)         R=  +9.3  p =  1.6e-4   unusual
      ...and 357 test result(s) without anomalies

About `mwc1616x`: it passes until 32 TiB but it fails the BCFN test at 64 TiB.

Run 1:

    rng=RNG_stdin32, seed=unknown
    length= 8 terabytes (2^43 bytes), time= 28773 seconds
      no anomalies in 331 test result(s)

    rng=RNG_stdin32, seed=unknown
    length= 16 terabytes (2^44 bytes), time= 56248 seconds
      Test Name                         Raw       Processed     Evaluation
      [Low1/32]BCFN(2+6,13-0,T)         R=  -9.1  p =1-7.8e-5   unusual
      ...and 338 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 32 terabytes (2^45 bytes), time= 120904 seconds
      Test Name                         Raw       Processed     Evaluation
      BCFN(2+0,13-0,T)                  R= +10.8  p =  2.6e-5   mildly suspicious
      FPF-14+6/16:all                   R=  +4.8  p =  5.4e-4   unusual
      ...and 345 test result(s) without anomalies

Run 2:

    rng=RNG_stdin32, seed=unknown
    length= 16 terabytes (2^44 bytes), time= 52634 seconds
      Test Name                         Raw       Processed     Evaluation
      BCFN(2+0,13-0,T)                  R=  +9.1  p =  2.0e-4   unusual
      BCFN(2+28,13-8,T)                 R= +30.0  p =  1.5e-8   unusual
      BCFN(2+29,13-9,T)                 R= +32.8  p =  1.7e-8   unusual
      ...and 336 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 32 terabytes (2^45 bytes), time= 106496 seconds
      Test Name                         Raw       Processed     Evaluation
      BCFN(2+0,13-0,T)                  R=  +8.9  p =  2.7e-4   unusual
      BCFN(2+3,13-0,T)                  R= +10.5  p =  3.9e-5   unusual
      BCFN(2+21,13-3,T)                 R= +15.4  p =  4.1e-7   unusual
      BCFN(2+27,13-7,T)                 R= +26.9  p =  1.0e-8   unusual
      BCFN(2+28,13-8,T)                 R= +31.5  p =  6.1e-9   unusual
      BCFN(2+29,13-8,T)                 R= +40.0  p =  4.2e-11  suspicious
      BCFN(2+30,13-9,T)                 R= +43.4  p =  7.2e-11  suspicious
      BCFN(2+31,13-9,T)                 R= +44.0  p =  5.2e-11  suspicious
      FPF-14+6/16:all                   R=  +5.9  p =  4.7e-5   mildly suspicious
      ...and 338 test result(s) without anomalies
    
    rng=RNG_stdin32, seed=unknown
    length= 64 terabytes (2^46 bytes), time= 213220 seconds
      Test Name                         Raw       Processed     Evaluation
      BCFN(2+0,13-0,T)                  R= +15.5  p =  8.1e-8   very suspicious
      BCFN(2+1,13-0,T)                  R= +11.4  p =  1.2e-5   mildly suspicious
      BCFN(2+2,13-0,T)                  R=  +9.4  p =  1.4e-4   unusual
      BCFN(2+3,13-0,T)                  R=  +9.2  p =  1.8e-4   unusual
      BCFN(2+4,13-0,T)                  R= +14.0  p =  5.0e-7   suspicious
      BCFN(2+5,13-0,T)                  R= +11.7  p =  8.6e-6   mildly suspicious
      BCFN(2+6,13-0,T)                  R= +12.9  p =  1.9e-6   mildly suspicious
      BCFN(2+7,13-0,T)                  R= +11.9  p =  6.5e-6   mildly suspicious
      BCFN(2+8,13-0,T)                  R= +13.6  p =  7.9e-7   mildly suspicious
      BCFN(2+10,13-0,T)                 R= +11.3  p =  1.4e-5   unusual
      BCFN(2+12,13-0,T)                 R= +12.2  p =  4.8e-6   unusual
      BCFN(2+19,13-1,T)                 R= +15.0  p =  1.6e-7   unusual
      BCFN(2+20,13-2,T)                 R= +19.4  p =  1.5e-9   suspicious
      BCFN(2+21,13-3,T)                 R= +28.5  p =  2.7e-13   VERY SUSPICIOUS
      BCFN(2+22,13-3,T)                 R= +23.9  p =  4.3e-11  very suspicious
      BCFN(2+23,13-4,T)                 R= +31.9  p =  7.2e-14   VERY SUSPICIOUS
      BCFN(2+24,13-5,T)                 R= +39.2  p =  1.6e-15    FAIL
      BCFN(2+25,13-5,T)                 R= +38.1  p =  4.3e-15    FAIL
      BCFN(2+26,13-6,T)                 R= +44.1  p =  1.5e-15    FAIL
      BCFN(2+27,13-6,T)                 R= +46.9  p =  1.6e-16    FAIL
      BCFN(2+28,13-7,T)                 R= +62.9  p =  1.5e-19    FAIL !
      BCFN(2+29,13-8,T)                 R= +80.5  p =  2.2e-21    FAIL !
      BCFN(2+30,13-8,T)                 R= +74.2  p =  9.1e-20    FAIL !
      BCFN(2+31,13-9,T)                 R= +91.5  p =  1.1e-21    FAIL !
      DC6-9x1Bytes-1                    R=  +7.0  p =  2.6e-3   unusual
      FPF-14+6/16:all                   R= +11.3  p =  3.9e-10   VERY SUSPICIOUS
      ...and 326 test result(s) without anomalies


About `mwc1616p`: fails PractRand at 32 TiB

    rng=RNG_stdin32, seed=unknown
    length= 8 terabytes (2^43 bytes), time= 25996 seconds
      no anomalies in 331 test result(s)

    rng=RNG_stdin32, seed=unknown
    length= 16 terabytes (2^44 bytes), time= 48592 seconds
      Test Name                         Raw       Processed     Evaluation
      BCFN(2+0,13-0,T)                  R= +16.8  p =  1.6e-8   very suspicious
      DC6-9x1Bytes-1                    R=  +9.2  p =  4.9e-4   mildly suspicious
      ...and 337 test result(s) without anomalies

    rng=RNG_stdin32, seed=unknown
    length= 32 terabytes (2^45 bytes), time= 102491 seconds
      Test Name                         Raw       Processed     Evaluation
      BCFN(2+0,13-0,T)                  R= +29.1  p =  4.4e-15    FAIL !
      DC6-9x1Bytes-1                    R=  +9.5  p =  3.5e-4   mildly suspicious
      ...and 345 test result(s) without anomalies

Sensitivity of dieharder is lower than TestU01 and PractRand:

- Failed dieharder: lcg69069, lcg32prime, minstd, randu, shr3, xsh, drand48, lfib31
- Passed dieharder: lcg64, lfib(55,24,+,up32), lfib(607,203,+,up32), swb, xorwow

Sensitivity of ENT for 1 GiB sample (at 4 GiB it fails on Win32 due to `long`
and/or `int` data types overflow) is much lower than `express` battery. The most
sensitive test from ENT is the byte frequency test. It also includes entropy
test, arithmetic mean, Monte-Carlo pi and serial correlation tests. But they
are less sensitive, e.g. entropy test catches only randu.

- Failed ENT: randu, lcg69069, drand48, shr3
- Passes ENT: lcg32prime, lcg64, lfib31, swb

# Notes about TMFn test from PractRand 0.94

It is used in PractRand and catches LCGs with power of 2 modulo, i.e.
\f$ m = 2^{32}\f$, \f$ m = 2^{64}\f$, \f$ m = 2^{128}\f$. It is sensitive
enough to detect 128-bit LCG with truncated lower 64 bits but doesn't detect
96-bit PRNG.

 Truncated bits | Fails at
----------------|------------------------------------
 32             | 32 MiB
 40             | 256 MiB
 48             | 2 GiB
 50             | 2 GiB
 52             | 4 GiB
 54             | 8 GiB
 56             | 8 GiB
 58             | 16 GiB
 60             | 16 GiB
 64             | 64 GiB
 65 = 64 + 1    | 64 GiB; 64 GiB; 64 GiB
 66 = 64 + 2    | 128 GiB; 128 GiB
 67 = 64 + 3    | 128 GiB
 68 = 64 + 4    | 128 GiB; 128 GiB; 128 GiB
 69 = 64 + 5    | 256 GiB; 256 GiB
 70 = 64 + 6    | 256 GiB; 256 GiB; 256 GiB
 71 = 64 + 7    | 256 GiB
 72 = 64 + 8    | 512 GiB; 512 GiB; 512 GiB; 512 GiB
 74 = 64 + 10   | 512 GiB; 512 GiB
 76 = 64 + 12   | 1 TiB
 77 = 64 + 13   | 1 TiB
 78 = 64 + 14   | NOTE: doesn't fail at 32 TiB!
 79 = 64 + 15   | NOTE: doesn't fail at 1 TiB!

Notes from PractRand 0.94 author:

https://sourceforge.net/p/pracrand/discussion/366935/thread/eb8d7f3e06/

TMFn is tripple-mirror-frequency, n-level, I think. I'm not thrilled with it,
but LCGs were doing entirely too well on my other tests, so I decided to use
something that could do a reasonable job of spotting power-of-2-modulus LCGs
without wasting too many cycles. It looks for really obvious patterns in three
symmetrically spaced small sets of bits (that is, it draws a set of N bits,
and another set of N bits from M bits later in the stream, and then a third set
from 2xM bits later in the stream, concatonates those sets of bits, and does
a chi-squared test directly on the distribution of those values... where N is
like 3 and M varies, but IIRC is generally rather large and probably a power
of 2), at a variety of different spacings (powers of 2 IIRC... large ones I
think). It skips most of the data IIRC, on a very simple hardwired scheme
(I think it only looks at the first 64 bits out of each kilobyte or something
like that) to avoid wasting too many cycles on this. Hopefully someday I'll get
rid of this when I have a test I like better that does a reasonable job on p2m
LCGs. 2+2 is... the log2 of the number of kilobytes of spacing it's looking at
I think? Mostly it catches LCGs, though some other stuff shows up once
in a while.

Birthday spacings test with decimation is much more sensitive. If there is no
decimation it consumes 4096 * 4 * 8 = 131072 bytes = 2^17 bytes for generator
with 32-bit output. Samples sizes in the case of decimation:

 Decimation            | Sample size* | Detects
-----------------------|--------------|-----------------------------------------------
 \f$2^{6}\f$  (64)     | 8 MiB        | 64-bit LCG with truncation of lower 32 bits.
 \f$2^{12}\f$ (4096)   | 512 MiB      | 128-bit LCG with truncation of lower 64 bits.
 \f$2^{18}\f$ (262144) | 32 GiB       | 128-bit LCG with truncation of lower 96 bits.
 \f$2^{24}\f$          | 2 TiB        | Upper 8 bits from 128-bit LCG.

* For 32-bit output; for 64-bit output it is doubled

It means that `bspace4_8d_dec` is about 100-1000 times more sensitive than TMFn
test from PractRand 0.94 at least for 128-bit LCGs. It is especially true if at
least lower 64 bits are truncated.

Approximation in GNU Octave:

    tr = [32 64 96 128]'
    s = [8*2^20 512*2^20 32*2^30 2*2^40]'
    y = log2(s)
    b = X \ y

The obtained approximation for sample size (32-bit output) n and number of
truncated bits t:

    n = 2^(17 + 3*t/16) => t = (log2(n) - 17) * 16 / 3

So for 1 PiB t = 176, for 8 PiB t = 192 that correspond to the 256-bit LCG that
retuns upper 64 bits. Of course, 1 PiB is only a minimalistic lower boundary
for a general purpose PRNG that is used to draw a border between seriously
flawed and barely usable LCGs with \f$ m = 2 ^ k \f$ modulo.


# Other notes and TO-DO lists

Examples of false failures in SmokeRand 0.42 in gap16_count0 test (fixed in 0.43):

smokerand.exe brief generators/chacha.dll --seed=_01_UU3t9pAb3d5FYNSe6nbg3ew3LMZtRMkA4p84wYkBr60= --testname=gap16_count0
smokerand.exe brief generators/aes128.dll --seed=_01_9FvOZLUeS9/Pl7c9rZBvJlLmKK85Oo8qGfpwgl4GGvA= --testname=gap16_count0
smokerand.exe brief generators/speck128.dll --testname=gap16_count0 --seed=_01_iQNhejvPDb2ImAIwDMyWe+ZnqFFX3riJn0aorb3XvWc=
