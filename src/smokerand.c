/**
 * @file smokerand.c
 * @brief SmokeRand command line interface.
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/coretests.h"
#include "smokerand/lineardep.h"
#include "smokerand/entropy.h"
#include "smokerand/bat_default.h"
#include "smokerand/bat_brief.h"
#include "smokerand/bat_full.h"
#include "smokerand/extratests.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define SUM_BLOCK_SIZE 1024

/**
 * @brief Keeps PRNG speed measurements results.
 */
typedef struct
{
    double ns_per_call; ///< Nanoseconds per call
    double ticks_per_call; ///< Processor ticks per call
} SpeedResults;

typedef enum {
    speed_uint, ///< Single value (emulates call of PRNG function)
    speed_sum   ///< Sum of values (emulates usage of inline PRNG)
} SpeedMeasurementMode;

/**
 * @brief PRNG speed measurement for uint output.
 * @details Two modes are supported: measurement of one function call
 * and measurement of sum computation by PRNG inside the cycle.
 */
static SpeedResults measure_speed(GeneratorInfo *gen, const CallerAPI *intf,
    SpeedMeasurementMode mode)
{
    void *state = gen->create(intf);
    SpeedResults results;
    double ns_total = 0.0;
    for (size_t niter = 2; ns_total < 0.5e9; niter <<= 1) {
        clock_t tic = clock();
        uint64_t sum = 0;
        uint64_t tic_proc = cpuclock();
        if (mode == speed_uint) {
            for (size_t i = 0; i < niter; i++) {
                sum += gen->get_bits(state);
            }
        } else {
            uint64_t sum = 0;
            for (size_t i = 0; i < niter; i++) {
                sum += gen->get_sum(state, SUM_BLOCK_SIZE);
            }
        }
        uint64_t toc_proc = cpuclock();
        clock_t toc = clock();
        ns_total = 1.0e9 * (toc - tic) / CLOCKS_PER_SEC;
        results.ns_per_call = ns_total / niter;
        results.ticks_per_call = (double) (toc_proc - tic_proc) / niter;
    }
    intf->free(state);
    return results;
}

static void *dummy_create(const CallerAPI *intf)
{
    (void) intf;
    return NULL;
}

static uint64_t dummy_get_bits(void *state)
{
    (void) state;
    return 0;
}

/**
 * @brief Sums some predefined elements in the cycle to measure
 * baseline time for PRNG performance estimation. It intentionally
 * uses a table with numbers to override optimization of compilers
 * (exclusion of cycle, even replacement of cycle for arithmetic
 * progression to formula etc.) Inspired by DOOM PRNG.
 */
static uint64_t dummy_get_sum(void *state, size_t len)
{
    uint64_t data[] = {9338, 34516, 60623, 45281,
        9064,   60090,  62764,  5557,
        44347,  35277,  25712,  20552,
        50645,  61072,  26719,  21307};
    uint64_t sum = 0;
    (void) state;
    for (size_t i = 0; i < len; i++) {
        sum += data[i & 0xF];
    }
    return sum;
}



void battery_speed_test(GeneratorInfo *gen, const CallerAPI *intf,
    SpeedMeasurementMode mode)
{
    GeneratorInfo dummy_gen = {.name = "dummy", .create = dummy_create,
        .get_bits = dummy_get_bits, .get_sum = dummy_get_sum,
        .nbits = gen->nbits, .self_test = NULL};
    SpeedResults speed_full = measure_speed(gen, intf, mode);
    SpeedResults speed_dummy = measure_speed(&dummy_gen, intf, mode);
    size_t block_size = (mode == speed_uint) ? 1 : SUM_BLOCK_SIZE;
    size_t nbytes = block_size * gen->nbits / 8;
    double ns_per_call_corr = speed_full.ns_per_call - speed_dummy.ns_per_call;
    double ticks_per_call_corr = speed_full.ticks_per_call - speed_dummy.ticks_per_call;
    double cpb_corr = ticks_per_call_corr / nbytes;
    double gb_per_sec = (double) nbytes / (1.0e-9 * ns_per_call_corr) / pow(2.0, 30.0);

    printf("Nanoseconds per call:\n");
    printf("  Raw result:                %g\n", speed_full.ns_per_call);
    printf("  For empty 'dummy' PRNG:    %g\n", speed_dummy.ns_per_call);
    printf("  Corrected result:          %g\n", ns_per_call_corr);
    printf("  Corrected result (GB/sec): %g\n", gb_per_sec);
    printf("CPU ticks per call:\n");
    printf("  Raw result:                %g\n", speed_full.ticks_per_call);
    printf("  For empty 'dummy' PRNG:    %g\n", speed_dummy.ticks_per_call);
    printf("  Corrected result:          %g\n", ticks_per_call_corr);
    printf("  Corrected result (cpB):    %g\n\n", cpb_corr);
}

void battery_speed(GeneratorInfo *gen, const CallerAPI *intf)
{
    printf("===== Generator speed measurements =====\n");
    printf("----- Speed test for uint generation -----\n");
    battery_speed_test(gen, intf, speed_uint);
    printf("----- Speed test for uint sum generation -----\n");
    battery_speed_test(gen, intf, speed_sum);
}





static inline void TestResults_print(const TestResults obj)
{
    printf("%s: p = %.3g; xemp = %g", obj.name, obj.p, obj.x);
    if (obj.p < 1e-10 || obj.p > 1 - 1e-10) {
        printf(" <<<<< FAIL\n");
    } else {
        printf("\n");
    }
}



void battery_self_test(GeneratorInfo *gen, const CallerAPI *intf)
{
    if (gen->self_test == 0) {
        intf->printf("Internal self-test not implemented\n");
        return;
    }
    intf->printf("Running internal self-test...\n");
    if (gen->self_test(intf)) {
        intf->printf("Internal self-test passed\n");
    } else {
        intf->printf("Internal self-test failed\n");
    }    
}

void print_help()
{
    printf("Usage: smokerand battery generator_lib\n");
    printf(" battery: battery name; supported batteries:\n");
    printf("   - brief\n");
    printf("   - default\n");
    printf("   - full\n");
    printf("   - selftest\n");
    printf("   - stdout\n");
    printf("  generator_lib: name of dynamic library with PRNG that export the functions:\n");
    printf("   - int gen_getinfo(GeneratorInfo *gi)\n");
    printf("\n");
}


typedef struct {
    int nthreads;
    int testid;
    int reverse_bits;
} SmokeRandSettings;


int SmokeRandSettings_load(SmokeRandSettings *obj, int argc, char *argv[])
{
    obj->nthreads = 1;
    obj->testid = TESTS_ALL;
    obj->reverse_bits = 0;
    for (int i = 3; i < argc; i++) {
        char argname[32];
        int argval;
        char *eqpos = strchr(argv[i], '=');
        size_t len = strlen(argv[i]);
        if (!strcmp(argv[i], "--threads")) {
            obj->nthreads = get_cpu_numcores();
            printf("%d CPU cores detected\n", obj->nthreads);
            continue;
        }
        if (len < 3 || (argv[i][0] != '-' || argv[i][1] != '-') || eqpos == NULL) {
            printf("Argument '%s' should have --argname=argval layout\n", argv[i]);
            return 1;
        }
        size_t name_len = (eqpos - argv[i]) - 2;
        if (name_len >= 32) name_len = 31;
        memcpy(argname, &argv[i][2], name_len); argname[name_len] = '\0';
        argval = atoi(eqpos + 1);
        if (!strcmp(argname, "nthreads")) {
            obj->nthreads = argval;
            if (argval <= 0) {
                printf("Invalid value of argument '%s'\n", argname);
                return 1;
            }
        } else if (!strcmp(argname, "testid")) {
            obj->testid = argval;
            if (argval <= 0) {
                printf("Invalid value of argument '%s'\n", argname);
                return 1;
            }
        } else if (!strcmp(argname, "reverse-bits")) {
            obj->reverse_bits = argval;
        } else {
            printf("Unknown argument '%s'\n", argname);
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Keeps the sum of Hamming weights
 */
typedef struct {
    unsigned long long count;
    unsigned long long sum;
    double p; ///< Probability of the corresponding tuple
} HammingWeightInfo;


typedef struct {
    GeneratorInfo *gen; ///< Used generator
    void *state; ///< PRNG state
    uint64_t buffer; ///< Buffer with PRNG raw data
    size_t nbytes; ///< Number of bytes returned by the generator
    size_t bytes_left;
} ByteStreamGenerator;


void ByteStreamGenerator_init(ByteStreamGenerator *obj, GeneratorInfo *gen, const CallerAPI *intf)
{
    obj->gen = gen;
    obj->state = gen->create(intf);
    obj->nbytes = gen->nbits / 8;
    obj->bytes_left = 0;
}

inline uint8_t ByteStreamGenerator_getbyte(ByteStreamGenerator *obj)
{
    if (obj->bytes_left == 0) {
        obj->buffer = obj->gen->get_bits(obj->state);
        obj->bytes_left = obj->nbytes;
    }
    uint8_t out = obj->buffer & 0xFF;
    obj->buffer >>= 8;
    obj->bytes_left--;
    return out;
}

inline uint8_t ByteStreamGenerator_getbyte_low1(ByteStreamGenerator *obj)
{
    if (obj->bytes_left == 0) {
        obj->buffer = 0;
        for (int i = 0; i < 64; i++) {
            obj->buffer = (obj->buffer << 1) | (obj->gen->get_bits(obj->state) & 0x1);
        }
        obj->bytes_left = 8;
    }
    uint8_t out = obj->buffer & 0xFF;
    obj->buffer >>= 8;
    obj->bytes_left--;
    return out;
}


void ByteStreamGenerator_free(ByteStreamGenerator *obj, const CallerAPI *intf)
{
    intf->free(obj->gen);
}


/*
static int cmp_doubles(const void *aptr, const void *bptr)
{
    double aval = *((double *) aptr), bval = *((double *) bptr);
    if (aval < bval) return -1;
    else if (aval == bval) return 0;
    else return 1;
}
*/

/*
void battery_hamming(GeneratorInfo *gen, const CallerAPI *intf)
{
    static uint8_t hw_to_triit[] = {0, 0, 0, 0, 1, 2, 2, 2, 2};
    int tuple_size = 9;
    size_t ntuples = (size_t) pow(3.0, tuple_size);
    HammingWeightInfo *info = calloc(ntuples, sizeof(HammingWeightInfo)); // 3^9
    uint8_t *hw = calloc(256, sizeof(uint8_t));
    uint64_t tuple = 0; // 9 4-bit Hamming weights + 1 extra Hamming weight
    uint8_t cur_weight; // Current Hamming weight
    unsigned long long len = 5000000000;

    

    ByteStreamGenerator bs;
    ByteStreamGenerator_init(&bs, gen, intf);
    // Initialize table of Hamming weights
    for (int i = 0; i < 256; i++) {        
        for (int j = 0; j < 8; j++) {
            if (i & (1 << j)) {
                hw[i]++;
            }
        }
        printf("%d ", hw[i]);
    }
    // Pre-fill tuple
    cur_weight = hw[ByteStreamGenerator_getbyte(&bs)];
    for (int i = 0; i < 9; i++) {
        tuple = ((tuple * 3) + hw_to_triit[cur_weight]) % ntuples;
        cur_weight = hw[ByteStreamGenerator_getbyte(&bs)];
    }
    // Generate other overlapping tuples
    for (unsigned long long i = 0; i < len; i++) {
        // Process current tuple
        info[tuple].count++;
        info[tuple].sum += cur_weight;
        // Update tuple
        tuple = ((tuple * 3) + hw_to_triit[cur_weight]) % ntuples;
        cur_weight = hw[ByteStreamGenerator_getbyte(&bs)];
        //printf("%llu\n", tuple);

        //uint64_t tmp = tuple;
        //for (size_t j = 0; j < 9; j++) {
        //    printf("%llu", tmp % 3);
        //    tmp /= 3;
        //}
        //printf("\n");
    }


    double z_max = 0;
    double *z_ary = calloc(ntuples, sizeof(double));
    for (size_t i = 0; i < ntuples; i++) {
        double std = sqrt(2.0) / sqrt(info[i].count);
        double mean = info[i].sum / (double) info[i].count;
        double z = (mean - 4.0) / std;
        z_ary[i] = z;
        if (z_max < fabs(z)) {
            z_max = fabs(z);
        }
//        printf("%g\n", z);
//        printf("%llu|%llu|%g ", info[i].count, info[i].sum, z);
    }
    printf("z_max = %g\n", z_max);


    // Dump tuples to the stdout
    unsigned long long count_total = 0;
    for (size_t i = 0; i < ntuples; i++) {
        count_total += info[i].count;
    }
    for (size_t i = 0; i < ntuples; i++) {
        printf("%6lu, %12.10g, %12.10g,\n", (long) i, (double) info[i].count / (double) count_total, z_ary[i]);
    }


    // Kolmogorov-Smirnov test
    qsort(z_ary, ntuples, sizeof(double), cmp_doubles);
    double D = 0.0;
    for (size_t i = 0; i < ntuples; i++) {        
        double f = 0.5 * (1 + erf(z_ary[i] / sqrt(2.0)));
        double idbl = (double) i;
        double Dplus = (idbl + 1.0) / ntuples - f;
        double Dminus = f - idbl / ntuples;
        if (Dplus > D) D = Dplus;
        if (Dminus > D) D = Dminus;
    }
    double K = sqrt(ntuples) * D + 1.0 / (6.0 * sqrt(ntuples));
    double p = ks_pvalue(K);
    printf("K = %g, p = %g\n", K, p);



    free(info);
    free(hw);
}
*/


// NOTE: PractRand also uses modifications of the test that works only with 1 or 8 lower bits.
// And this modification is especially sensitive to SWB, SWBW, Mulberry32 etc. Probably they
// see longer-range correlations
void battery_hamming(GeneratorInfo *gen, const CallerAPI *intf)
{
    // From PractRand
    // a) Chaotic variant
    //                                0      1      2      3    4    5    6      7      8
    //static uint32_t hw_to_code[] = {0x201, 0x201, 0x201, 0x1, 0x0, 0x0, 0x200, 0x200, 0x201};
    //static uint64_t tuple_mask = 0x1FEFF; // 1 1111 1110 1111 1111

/*
0:0:0
1:1:0
5:2:1
7:3:1
92:4:2
122:5:2
123:6:3
254:7:3
255:8:0


8:1:0
9:2:1
10:2:1 11:3:1 12:2:1 13:3:1 14:3:1 15:4:2 16:1:0 17:2:1 18:2:1 19:3:1 20:2:1 21:3:1 22:3:1 23:4:2 24:2:1 25:3:1 26:3:1 27:4:2 28:3:1 29:4:2 30:4:2
:2 32:1:0 33:2:1 34:2:1 35:3:1 36:2:1 37:3:1 38:3:1 39:4:2 40:2:1 41:3:1 42:3:1 43:4:2 44:3:1 45:4:2 46:4:2 47:5:2 48:2:1 49:3:1 50:3:1 51:4:2 52:3:1 53:4:2 54:4:2 55:5:2 56:3:1 57:4:2 58:4:2 59:5:2 60:4:2 6
2 62:5:2 63:6:3 64:1:0 65:2:1 66:2:1 67:3:1 68:2:1 69:3:1 70:3:1
71:4:2
72:2:1 73:3:1 74:3:1 75:4:2 76:3:1 77:4:2 78:4:2 79:5:2 80:2:1 81:3:1 82:3:1 83:4:2 84:3:1 85:4:2 86:4:2 87:5:2 88:3:1 89:4:2 90:4:2 91
93:5:2 94:5:2 95:6:3 96:2:1 97:3:1 98:3:1 99:4:2 100:3:1 101:4:2 102:4:2 103:5:2 104:3:1 105:4:2 106:4:2 107:5:2 108:4:2 109:5:2 110:5:2 111:6:3 112:3:1 113:4:2 114:4:2 115:5:2 116:4:2 117:5:2 118:5:
9:6:3 120:4:2 121:5:2
124:5:2 125:6:3 126:6:3 127:7:3 128:1:0 129:2:1 130:2:1 131:3:1 132:2:1 133:3:1 134:3:1 135:4:2 136:2:1 137:3:1 138:3:1 139:4:2 140:3:1 141:4:2 142:4:2 143:5:2 144:2:1 1
:1 146:3:1 147:4:2 148:3:1 149:4:2 150:4:2 151:5:2 152:3:1 153:4:2 154:4:2 155:5:2 156:4:2 157:5:2 158:5:2 159:6:3 160:2:1 161:3:1 162:3:1 163:4:2 164:3:1 165:4:2 166:4:2 167:5:2 168:3:1 169:4:2 170:4:2 171:
172:4:2 173:5:2 174:5:2 175:6:3 176:3:1 177:4:2 178:4:2 179:5:2 180:4:2 181:5:2 182:5:2 183:6:3 184:4:2 185:5:2 186:5:2 187:6:3 188:5:2 189:6:3 190:6:3 191:7:3 192:2:1 193:3:1 194:3:1 195:4:2 196:3:1 197:4:2
:4:2 199:5:2 200:3:1 201:4:2 202:4:2 203:5:2 204:4:2 205:5:2 206:5:2 207:6:3 208:3:1 209:4:2 210:4:2 211:5:2 212:4:2 213:5:2 214:5:2 215:6:3 216:4:2 217:5:2 218:5:2 219:6:3 220:5:2 221:6:3 222:6:3 223:7:3 22
1 225:4:2 226:4:2 227:5:2 228:4:2 229:5:2 230:5:2 231:6:3 232:4:2 233:5:2 234:5:2 235:6:3 236:5:2 237:6:3 238:6:3 239:7:3 240:4:2 241:5:2 242:5:2 243:6:3 244:5:2 245:6:3 246:6:3 247:7:3 248:5:2 249:6:3 250:6
51:7:3 252:6:3
253:7:3
*/

    // b) Simplified arranged variant
    //                              0  1  2  3  4  5  6  7  8
    static uint32_t hw_to_code[] = {0, 0, 1, 1, 2, 2, 3, 3, 0};
    //>> binopdf(0:8,8,0.5) * 256
    //                              1  8 28 56 70 56 28  8  1
    static double code_to_prob[] = {10.0/256.0, 84.0/256.0, 126.0/256.0, 36.0/256.0};
    static const uint64_t tuple_mask = 0x3FFFF, code_mask = 0x3, code_nbits = 2; // For 18-bit tuple


    int tuple_size = 9;
    size_t ntuples = (size_t) pow(4.0, tuple_size);
    HammingWeightInfo *info = calloc(ntuples, sizeof(HammingWeightInfo)); // 3^9
    uint8_t *hw = calloc(256, sizeof(uint8_t));
    uint64_t tuple = 0; // 9 4-bit Hamming weights + 1 extra Hamming weight
    uint8_t cur_weight; // Current Hamming weight
    unsigned long long len = 1000ull * 1000ull * 100ull;

    

    ByteStreamGenerator bs;
    ByteStreamGenerator_init(&bs, gen, intf);
    // Initialize table of Hamming weights
    for (int i = 0; i < 256; i++) {        
        for (int j = 0; j < 8; j++) {
            if (i & (1 << j)) {
                hw[i]++;
            }
        }
        printf("%d ", hw[i]);
    }
    // Calculate probabilities of tuples
    // (idea is taken from PractRand)
    for (size_t i = 0; i < ntuples; i++) {
        uint64_t tmp = i;
        info[i].p = 1.0;
        for (int j = 0; j < tuple_size; j++) {
            info[i].p *= code_to_prob[tmp & code_mask];            
            tmp >>= code_nbits;
        }
        //printf("%d %g\n", (int) i, info[i].p);
    }

    // Pre-fill tuple
    cur_weight = hw[ByteStreamGenerator_getbyte_low1(&bs)];
    for (int i = 0; i < 9; i++) {
        tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
        cur_weight = hw[ByteStreamGenerator_getbyte(&bs)];
    }
    // Generate other overlapping tuples
    for (unsigned long long i = 0; i < len; i++) {
        // Process current tuple
        info[tuple].count++;
        info[tuple].sum += cur_weight;
        // Update tuple
        tuple = ((tuple << code_nbits) | hw_to_code[cur_weight]) & tuple_mask;
        cur_weight = hw[ByteStreamGenerator_getbyte_low1(&bs)];
        //printf("%llu\n", tuple);

        //uint64_t tmp = tuple;
        //for (size_t j = 0; j < 9; j++) {
        //    printf("%llu", tmp % 3);
        //    tmp /= 3;
        //}
        //printf("\n");
    }


    double z_max = 0;
    double *z_ary = calloc(ntuples, sizeof(double));
    for (size_t i = 0; i < ntuples; i++) {
        double std = sqrt(2.0) / sqrt(info[i].count);
        double mean = info[i].sum / (double) info[i].count;
        double z = (mean - 4.0) / std;
        z_ary[i] = z;
        if (z_max < fabs(z)) {
            z_max = fabs(z);
        }
//        printf("%g\n", z);
//        printf("%llu|%llu|%g ", info[i].count, info[i].sum, z);
    }
    printf("z_max = %g\n", z_max);


    // Dump tuples to the stdout
    unsigned long long count_total = 0;
    for (size_t i = 0; i < ntuples; i++) {
        count_total += info[i].count;
    }
    // chi2 test
    double chi2emp = 0.0;
    z_max = 0.0;
    for (size_t i = 0; i < ntuples; i++) {
        double Ei = count_total * info[i].p;
        double Oi = info[i].count;
        double dE = Oi - Ei;
        chi2emp += (Oi - Ei) * (Oi - Ei) / Ei;

        double z = fabs(dE) / sqrt( count_total * info[i].p * (1.0 - info[i].p) );
        if (z > z_max) {
            z_max = z;
        }


/*
        printf("%6lu, %12.10g, %12.10g, %12.10g\n",
            (long) i, (double) info[i].count / (double) count_total, z_ary[i], info[i].p);
*/
    }
    printf("chi2emp = %g, p = %g, zmax = %g\n", chi2emp, chi2_pvalue(chi2emp, ntuples - 1), z_max);


    // Kolmogorov-Smirnov test
/*
    qsort(z_ary, ntuples, sizeof(double), cmp_doubles);
    double D = 0.0;
    for (size_t i = 0; i < ntuples; i++) {        
        double f = 0.5 * (1 + erf(z_ary[i] / sqrt(2.0)));
        double idbl = (double) i;
        double Dplus = (idbl + 1.0) / ntuples - f;
        double Dminus = f - idbl / ntuples;
        if (Dplus > D) D = Dplus;
        if (Dminus > D) D = Dminus;
    }
    double K = sqrt(ntuples) * D + 1.0 / (6.0 * sqrt(ntuples));
    double p = ks_pvalue(K);
    printf("K = %g, p = %g\n", K, p);



    free(info);
    free(hw);
*/
}


int main(int argc, char *argv[]) 
{
    if (argc < 3) {
        print_help();
        return 0;
    }
    GeneratorInfo reversed_gen;
    SmokeRandSettings opts;
    if (SmokeRandSettings_load(&opts, argc, argv)) {
        return 1;
    }
    CallerAPI intf = (opts.nthreads == 1) ? CallerAPI_init() : CallerAPI_init_mthr();
    char *battery_name = argv[1];
    char *generator_lib = argv[2];

    GeneratorModule mod = GeneratorModule_load(generator_lib);
    if (!mod.valid) {
        CallerAPI_free();
        return 1;
    }

    printf("Seed generator self-test: %s\n",
        xxtea_test() ? "PASSED" : "FAILED");

    GeneratorInfo *gi = &mod.gen;
    if (opts.reverse_bits) {
        reversed_gen = reversed_generator_set(gi);
        gi = &reversed_gen;
        printf("All tests will be run with the reverse bits order\n");
    }

    printf("Generator name:    %s\n", gi->name);
    printf("Output size, bits: %d\n", gi->nbits);


    if (!strcmp(battery_name, "default")) {
        battery_default(gi, &intf, opts.testid, opts.nthreads);
    } else if (!strcmp(battery_name, "brief")) {
        battery_brief(gi, &intf, opts.testid, opts.nthreads);
    } else if (!strcmp(battery_name, "full")) {
        battery_full(gi, &intf, opts.testid, opts.nthreads);
    } else if (!strcmp(battery_name, "selftest")) {
        battery_self_test(gi, &intf);
    } else if (!strcmp(battery_name, "speed")) {
        battery_speed(gi, &intf);
    } else if (!strcmp(battery_name, "stdout")) {
        GeneratorInfo_bits_to_file(gi, &intf);
    } else if (!strcmp(battery_name, "freq")) {
        battery_blockfreq(gi, &intf);
    } else if (!strcmp(battery_name, "birthday")) {
        battery_birthday(gi, &intf);
    } else if (!strcmp(battery_name, "ising")) {
        battery_ising(gi, &intf);
    } else if (!strcmp(battery_name, "hamming")) {
        battery_hamming(gi, &intf);
    } else {
        printf("Unknown battery %s\n", battery_name);
        GeneratorModule_unload(&mod);
        CallerAPI_free();
        return 1;
    }

    GeneratorModule_unload(&mod);
    CallerAPI_free();
    return 0;
}
           