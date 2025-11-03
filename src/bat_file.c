/**
 * @file bat_file.c
 * @brief A custom battery that is loaded from human-readable text file.
 *
 * @copyright (c) 2025 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#include "smokerand/bat_file.h"
#include "smokerand/coretests.h"
#include "smokerand/lineardep.h"
#include "smokerand/hwtests.h"
#include "smokerand/extratests.h"
#include "smokerand/entropy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define ERRMSG_BUF_SIZE 128

typedef struct {
    char name[64];
    char value[64];
} TestArgument;

typedef struct {
    char testname[64];
    TestArgument *args;
    size_t nargs;
    int linenum;
} TestInfo;

typedef struct {
    TestInfo *tests;
    size_t ntests;
} TestInfoArray;

static void xstrcpy_s(char *dest, size_t dlen, const char *src)
{
    size_t i;
    for (i = 0; i < dlen - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static void erase_comments(char *linetxt, size_t len)
{
    for (size_t i = 0; i < len && linetxt[i] != '\0'; i++) {
        if (linetxt[i] == '#') {
            linetxt[i] = '\0';
            break;
        } else if (linetxt[i] < 32) {
            linetxt[i] = ' ';
        }
    }
}

/////////////////////////////////////////
///// TestInfo class implementation /////
/////////////////////////////////////////

void TestInfo_init(TestInfo *obj, const char *testname)
{
    obj->args = malloc(sizeof(TestArgument));
    obj->nargs = 0;
    xstrcpy_s(obj->testname, 64, testname);
}


void TestInfo_destruct(TestInfo *obj)
{
    free(obj->args);
    obj->args = NULL;
    obj->nargs = 0;
}


void TestInfo_addarg(TestInfo *obj, const char *name, const char *value)
{
    obj->nargs++;
    obj->args = realloc(obj->args, obj->nargs * sizeof(TestArgument));
    if (obj->args == NULL) {
        fprintf(stderr, "***** TestInfo_addarg: not enough memory *****");
        exit(1);
    }
    xstrcpy_s(obj->args[obj->nargs - 1].name, 64, name);
    xstrcpy_s(obj->args[obj->nargs - 1].value, 64, value);
}


const char *TestInfo_get_value(const TestInfo *obj, const char *name)
{
    for (size_t i = 0; i < obj->nargs; i++) {
        if (!strcmp(name, obj->args[i].name)) {
            return obj->args[i].value;
        }
    }
    return NULL;
}


static long long txtvalue_to_int(const char *txtvalue)
{
    long long value;
    if (txtvalue == NULL) {
        return LLONG_MAX;
    }
    size_t buflen = strlen(txtvalue) + 1;
    char *stripped = malloc(buflen);
    xstrcpy_s(stripped, buflen, txtvalue);
    for (size_t i = 0, j = 0; i < buflen - 1; i++) {
        while (stripped[j] == '_') j++;
        stripped[i] = stripped[j];
        if (stripped[j++] == '\0') break;
    }
    int ans = sscanf(stripped, "%lld", &value);
    free(stripped);
    return (ans == 1) ? value : LLONG_MAX;
}


long long TestInfo_get_intvalue(const TestInfo *obj, const char *name)
{
    const char *txtvalue = TestInfo_get_value(obj, name);    
    return txtvalue_to_int(txtvalue);
}


static long long TestInfo_get_limited_intvalue(const TestInfo *obj, const char *name,
    long long xmin, long long xmax, char *errmsg)
{
    const char *txtvalue = TestInfo_get_value(obj, name);
    errmsg[0] = '\0';
    if (txtvalue == NULL) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Argument '%s' not found", name);
        return LLONG_MAX;
    }
    const long long value = txtvalue_to_int(txtvalue);
    if (value == LLONG_MAX) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Argument '%s' must be an integer", name);
        return LLONG_MAX;
    } else if (value < xmin || value > xmax) {
        snprintf(errmsg, ERRMSG_BUF_SIZE,
            "'%s' value must be between %lld and %lld (it is %lld)",
            name, xmin, xmax, value);
        return LLONG_MAX;
    } else {
        return value;
    }
}



int TestInfo_value_to_code(const TestInfo *obj, const char *name,
    const char **vals, const int *codes, char *errmsg, int *is_ok)
{
    const char *value = TestInfo_get_value(obj, name);
    if (value == NULL) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Argument '%s' not found\n", name);
        *is_ok = 0;
        return 0;
    }
    for (size_t i = 0; vals[i] != NULL; i++) {
        if (!strcmp(vals[i], value)) {
            *is_ok = 1;
            return codes[i];
        }
    }
    *is_ok = 0;
    snprintf(errmsg, ERRMSG_BUF_SIZE, "Unknown '%s' value '%s'", name, value);
    return 0;
}


void TestInfo_print(const TestInfo *obj)
{
    printf("%s ", obj->testname);
    for (size_t i = 0; i < obj->nargs; i++) {
        printf("%s=%s ", obj->args[i].name, obj->args[i].value);
    }
    printf("\n");
}


//////////////////////////////////////////////
///// TestInfoArray class implementation /////
//////////////////////////////////////////////

void TestInfoArray_init(TestInfoArray *obj)
{
    obj->tests = malloc(sizeof(TestInfo));
    obj->ntests = 0;
}

void TestInfoArray_print(TestInfoArray *obj)
{
    printf("Number of tests: %d\n", (int) obj->ntests);
    for (size_t i = 0; i < obj->ntests; i++) {
        TestInfo_print(&obj->tests[i]);
    }
}

/**
 * @brief Adds test information (`key=value` pairs) to the array. It takes ownership
 * of the given TestInfo structure.
 * @param obj   Array of tests.
 * @param test  Test info: all data pointers will be set to NULL, all data sizes
 *              will be set to 0 to prevent double-free issue.
 */
void TestInfoArray_addtest(TestInfoArray *obj, TestInfo *test)
{
    obj->ntests++;
    obj->tests = realloc(obj->tests, obj->ntests * sizeof(TestInfo));
    if (obj->tests == NULL) {
        fprintf(stderr, "***** TestInfoArray_addtest: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    obj->tests[obj->ntests - 1] = *test;
    // Take ownership to prevent double-free
    test->nargs = 0;
    test->args = NULL;
}


void TestInfoArray_destruct(TestInfoArray *obj)
{
    for (size_t i = 0; i < obj->ntests; i++) {
        TestInfo_destruct(&obj->tests[i]);
    }
    free(obj->tests);
}


void TestInfoArray_clear(TestInfoArray *obj)
{
    TestInfoArray_destruct(obj);
    TestInfoArray_init(obj);
}

//////////////////
///// LOADER /////
//////////////////



typedef struct {
    char linetxt[512];
    int linenum;
    int inside_entry;
    TestInfo testinfo;
    TestInfoArray tests;
} ParserState;


static void ParserState_init(ParserState *obj)
{
    obj->linenum = 0;   
    obj->inside_entry = 0;
    TestInfoArray_init(&obj->tests);
    obj->testinfo.nargs = 0;
    obj->testinfo.args = NULL;
}


static void ParserState_destruct(ParserState *obj)
{
    TestInfoArray_clear(&obj->tests);
    TestInfo_destruct(&obj->testinfo);
}


static int ParserState_process_token(ParserState *state, const char *token)
{
    int is_end_token = strcmp(token, "end") == 0;
    if (!state->inside_entry) {
        // Begin addition of a new test
        if (is_end_token) {
            fprintf(stderr, "Error in line %d. 'end' without test name\n",
                state->linenum);
            return 0;
        }
        TestInfo_init(&state->testinfo, token);
        state->testinfo.linenum = state->linenum;
        state->inside_entry = 1;
    } else if (is_end_token) {
        // End of test description
        TestInfoArray_addtest(&state->tests, &state->testinfo);
        state->inside_entry = 0;
    } else {
        char name[64], value[64];
        char *eqpos = strstr(token, "=");
        int i;
        if (eqpos == NULL) {
            fprintf(stderr, "Error in line %d. Token '%s' doesn't contain '='\n",
                state->linenum, token);
            return 0;
        }
        for (i = 0; i < 64 && token[i] != '='; i++) {
            name[i] = token[i];
        }
        name[i] = '\0';
        xstrcpy_s(value, 64, eqpos + 1);
        TestInfo_addarg(&state->testinfo, name, value);
    }
    return 1;
}


static TestInfoArray load_tests(const char *filename)
{
    ParserState state;
    ParserState_init(&state);
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "File '%s' not found\n", filename);
        return state.tests;
    }
    while (fgets(state.linetxt, 512, fp)) {
        // Erase comments, ignore empty strings
        erase_comments(state.linetxt, 512);
        state.linenum++;
        char *token = strtok(state.linetxt, " \t\r\n");
        if (token == NULL) {
            continue;
        }
        // Parse not empty string
        do {
            int is_ok = ParserState_process_token(&state, token);
            if (!is_ok) {
                ParserState_destruct(&state);
                fclose(fp);
                return state.tests;
            }
        } while ((token = strtok(NULL, " \t\r\n")) != NULL);
    }
    // Check if the last entry is closed
    if (state.inside_entry) {
        fprintf(stderr, "Error in line %d: entry '%s' is not closed with 'end'\n",
            state.linenum, state.testinfo.testname);
        ParserState_destruct(&state);
    }
    // Close file and return the battery info
    fclose(fp);
    return state.tests;
}

#define GET_LIMITED_INTVALUE(name, xmin, xmax) \
    long long name = TestInfo_get_limited_intvalue(obj, #name, xmin, xmax, errmsg); \
    if (name == LLONG_MAX) return 0;


static int parse_bspace_nd(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(nbits_per_dim, 1, 64)
    GET_LIMITED_INTVALUE(ndims, 1, 64)
    GET_LIMITED_INTVALUE(nsamples, 1, 1ll << 30ll)
    GET_LIMITED_INTVALUE(get_lower, 0, 1)
    BSpaceNDOptions *opts = calloc(1, sizeof(BSpaceNDOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_bspace_nd: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->nbits_per_dim = (unsigned int) nbits_per_dim;
    opts->ndims = (unsigned int) ndims;
    opts->nsamples = (unsigned long) nsamples;
    opts->get_lower = (int) get_lower;
    out->name = obj->testname;
    out->run = bspace_nd_test_wrap;
    out->udata = opts;
    return 1;
}


static int parse_collover_nd(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(nbits_per_dim, 1, 64)
    GET_LIMITED_INTVALUE(ndims, 1, 64)
    GET_LIMITED_INTVALUE(nsamples, 1, 1ll << 30ll)
    GET_LIMITED_INTVALUE(get_lower, 0, 1)
    GET_LIMITED_INTVALUE(n, 1000, 1ll << 30ll)
    CollOverNDOptions *opts = calloc(1, sizeof(CollOverNDOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_collover_nd: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->nbits_per_dim = (unsigned int) nbits_per_dim;
    opts->ndims = (unsigned int) ndims;
    opts->nsamples = (unsigned long) nsamples;
    opts->n = (unsigned long) n;
    opts->get_lower = (int) get_lower;
    out->name = obj->testname;
    out->run = bspace_nd_test_wrap;
    out->udata = opts;
    return 1;
}


static int parse_collisionover(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    int is_ok = parse_collover_nd(out, obj, errmsg);
    out->run = collisionover_test_wrap;
    return is_ok;
}



static int parse_monobit_freq(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(nvalues, 1024, 1ll << 50ll)
    MonobitFreqOptions *opts = calloc(1, sizeof(MonobitFreqOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_monobit_freq: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->nvalues = nvalues;
    out->name = obj->testname;
    out->run = monobit_freq_test_wrap;
    out->udata = opts;
    return 1;
}



static int parse_nbit_words_freq(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(bits_per_word, 1, 16)
    GET_LIMITED_INTVALUE(average_freq, 8, 1ll << 20ll)
    GET_LIMITED_INTVALUE(nblocks, 16, 1ll << 30ll)
    NBitWordsFreqOptions *opts = calloc(1, sizeof(NBitWordsFreqOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_nbit_words_freq: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->bits_per_word = (unsigned int) bits_per_word;
    opts->average_freq = (unsigned int) average_freq;
    opts->nblocks = (size_t) nblocks;
    out->name = obj->testname;
    out->run = nbit_words_freq_test_wrap;
    out->udata = opts;
    return 1;
}


static int parse_bspace4_8d_decimated(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(step, 1, 1ll << 30ll)
    BSpace4x8dDecimatedOptions *opts = calloc(1, sizeof(BSpace4x8dDecimatedOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_bspace4_8d_decimated: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->step = (unsigned long) step;
    out->name = obj->testname;
    out->run = bspace4_8d_decimated_test_wrap;
    out->udata = opts;
    return 1;
}


static int parse_gap(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(shl, 1, 64)
    GET_LIMITED_INTVALUE(ngaps, 10000, 1ll << 40ll)
    GapOptions *opts = calloc(1, sizeof(GapOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_gap: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->shl = (unsigned int) shl;
    opts->ngaps = ngaps;
    out->name = obj->testname;
    out->run = gap_test_wrap;
    out->udata = opts;
    return 1;
}


static int parse_gap16_count0(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(ngaps, 10000, 1ll << 40ll)
    Gap16Count0Options *opts = calloc(1, sizeof(Gap16Count0Options));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_gap16_count0: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->ngaps = (unsigned long long) ngaps;
    out->name = obj->testname;
    out->run = gap16_count0_test_wrap;
    out->udata = opts;
    return 1;
}


static int parse_hamming_distr(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(nvalues, 65536, 1ll << 50ll)
    GET_LIMITED_INTVALUE(nlevels, 1, 20)
    HammingDistrOptions *opts = calloc(1, sizeof(HammingDistrOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_hamming_distr: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->nvalues = nvalues;
    opts->nlevels = (int) nlevels;
    out->name = obj->testname;
    out->run = hamming_distr_test_wrap;
    out->udata = opts;
    return 1;
}



static int parse_hamming_ot(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(nbytes, 65536, 1ll << 50ll)
    int is_ok;
    const char *m_txt[] = {"values", "bytes", "bytes_low8", "bytes_low1", NULL};
    const int m_codes[] = {HAMMING_OT_VALUES, HAMMING_OT_BYTES,
        HAMMING_OT_BYTES_LOW8, HAMMING_OT_BYTES_LOW1, 0};
    HammingOtMode mode = TestInfo_value_to_code(obj, "mode",
        m_txt, m_codes, errmsg, &is_ok);
    if (!is_ok) {
        return 0;
    }

    HammingOtOptions *opts = calloc(1, sizeof(HammingOtOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_hamming_ot: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->nbytes = nbytes;
    opts->mode = mode;
    out->name = obj->testname;
    out->run = hamming_ot_test_wrap;
    out->udata = opts;
    return 1;
}


static int parse_hamming_ot_long(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(nvalues, 65536, 1ll << 50ll)
    int is_ok;
    const char *ws_txt[] = {"w128", "w256", "w512", "w1024", NULL};
    const int ws_codes[] = {HAMMING_OT_W128, HAMMING_OT_W256,
        HAMMING_OT_W512, HAMMING_OT_W1024, 0};
    HammingOtWordSize ws = TestInfo_value_to_code(obj, "wordsize",
        ws_txt, ws_codes, errmsg, &is_ok);
    if (!is_ok) {
        return 0;
    }
    HammingOtLongOptions *opts = calloc(1, sizeof(HammingOtLongOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_hamming_ot_long: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->nvalues = nvalues;
    opts->wordsize = ws;
    out->name = obj->testname;
    out->run = hamming_ot_long_test_wrap;
    out->udata = opts;
    return 1;
}


static int parse_linearcomp(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(nbits, 8, 1ll << 30ll);
    const char *bitpos_descr = TestInfo_get_value(obj, "bitpos");
    long long bitpos;
    if (!strcmp(bitpos_descr, "low")) {
        bitpos = LINEARCOMP_BITPOS_LOW;
    } else if (!strcmp(bitpos_descr, "mid")) {
        bitpos = LINEARCOMP_BITPOS_MID;
    } else if (!strcmp(bitpos_descr, "high")) {
        bitpos = LINEARCOMP_BITPOS_HIGH;
    } else {
        bitpos = TestInfo_get_limited_intvalue(obj, "bitpos", 0, 64, errmsg);
        if (bitpos == LLONG_MAX) {
            return 0;
        }
    }
    LinearCompOptions *opts = calloc(1, sizeof(LinearCompOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_linearcomp: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->nbits = (size_t) nbits;
    opts->bitpos = (int) bitpos;
    out->name = obj->testname;
    out->run = linearcomp_test_wrap;
    out->udata = opts;
    return 1;
}

static int parse_matrixrank(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(max_nbits, 8, 64)
    GET_LIMITED_INTVALUE(n, 256, 65536)
    if (n % 256 != 0) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "'n' value must be divisible by 256");
        return 0;
    }
    MatrixRankOptions *opts = calloc(1, sizeof(MatrixRankOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_matrixrank: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->n = (size_t) n;
    opts->max_nbits = (unsigned int) max_nbits;
    out->name = obj->testname;
    out->run = matrixrank_test_wrap;
    out->udata = opts;
    return 1;
}

static int parse_mod3(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(nvalues, 100000, 1ll << 50ll)
    Mod3Options *opts = calloc(1, sizeof(Mod3Options));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_mod3: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->nvalues = (unsigned long long) nvalues;
    out->name = obj->testname;
    out->run = mod3_test_wrap;
    out->udata = opts;
    return 1;
}

static int parse_sumcollector(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(nvalues, 100000, 1ll << 50ll)
    SumCollectorOptions *opts = calloc(1, sizeof(SumCollectorOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_sumcollector: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->nvalues = nvalues;
    out->name = obj->testname;
    out->run = sumcollector_test_wrap;
    out->udata = opts;
    return 1;
}


static int parse_ising2d(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(sample_len, 100, 10000000000);
    GET_LIMITED_INTVALUE(nsamples, 3, 100000);
    int is_ok;
    const char *alg_txt[] = {"wolff", "metropolis", NULL};
    const int alg_codes[] = {ISING_WOLFF, ISING_METROPOLIS, 0};
    IsingAlgorithm algorithm = TestInfo_value_to_code(obj, "algorithm",
        alg_txt, alg_codes, errmsg, &is_ok);
    if (!is_ok) {
        return 0;
    }

    Ising2DOptions *opts = calloc(1, sizeof(Ising2DOptions));
    if (opts == NULL) {
        fprintf(stderr, "***** parse_ising2d: not enough memory *****");
        exit(EXIT_FAILURE);
    }
    opts->sample_len = (unsigned long) sample_len;
    opts->nsamples = (unsigned int) nsamples;
    opts->algorithm = algorithm;
    out->name = obj->testname;
    out->run = ising2d_test_wrap;
    out->udata = opts;
    return 1;
}


static int parse_usphere(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    GET_LIMITED_INTVALUE(ndims, 2, 20);
    GET_LIMITED_INTVALUE(npoints, 1000, 1ull << 40ull);
    UnitSphereOptions *opts = calloc(1, sizeof(UnitSphereOptions));
    opts->ndims = (unsigned int) ndims;
    opts->npoints = npoints;
    out->name = obj->testname;
    out->run = unit_sphere_volume_test_wrap;
    out->udata = opts;
    return 1;
}





typedef struct {
    const char *name;
    int (*func)(TestDescription *, const TestInfo *, char *);
} TestFunc;


/**
 * @brief Loads a custom battery from a user-defined text file.
 * @details File format:
 * test_name param1=value1 param2=value2
 */
int battery_file(const char *filename, const GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads, ReportType rtype)
{
    static const TestFunc parsers[] = {
        {"bspace_nd", parse_bspace_nd},
        {"bspace4_8d_decimated", parse_bspace4_8d_decimated},
        {"collisionover", parse_collisionover},
        {"gap", parse_gap},
        {"gap16_count0", parse_gap16_count0},
        {"hamming_distr", parse_hamming_distr},
        {"hamming_ot", parse_hamming_ot},
        {"hamming_ot_long", parse_hamming_ot_long},
        {"ising2d", parse_ising2d},
        {"linearcomp", parse_linearcomp},
        {"matrixrank", parse_matrixrank},
        {"mod3", parse_mod3},
        {"monobit_freq", parse_monobit_freq},
        {"nbit_words_freq", parse_nbit_words_freq},
        {"sumcollector", parse_sumcollector},
        {"usphere", parse_usphere}, {NULL, NULL}
    };
    char errmsg[ERRMSG_BUF_SIZE];
    errmsg[0] = '\0';
    TestInfoArray tests_args = load_tests(filename);
    if (tests_args.ntests == 0) {
        TestInfoArray_destruct(&tests_args);
        return 0;
    }
    size_t ntests = tests_args.ntests;
    TestDescription *tests = calloc(ntests + 1, sizeof(TestDescription));
    if (tests == NULL) {
        fprintf(stderr, "***** battery_file: not enough memory *****");
        TestInfoArray_destruct(&tests_args);
        return 0;
    }

    int is_ok = 1;
    for (size_t i = 0; i < tests_args.ntests; i++) {        
        const char *name = TestInfo_get_value(&tests_args.tests[i], "test");
        const TestInfo *curtest = &tests_args.tests[i];
        int parsed = 0;
        for (const TestFunc *parser = parsers; parser->name != NULL; parser++) {
            if (!strcmp(name, parser->name)) {
                is_ok = parser->func(&tests[i], curtest, errmsg);
                parsed = 1;
            }
        }
        if (!parsed) {
            fprintf(stderr, "Error in line %d. Unknown test '%s'\n", curtest->linenum, name);
            is_ok = 0;
            goto battery_file_freemem;
        }

        if (!is_ok) {
            fprintf(stderr, "Error in line %d. %s\n", curtest->linenum, errmsg);
            goto battery_file_freemem;
        }
    }

    TestsBattery bat;
    bat.name = "CUSTOM";
    bat.tests = tests;
    if (gen != NULL) {
        TestsBattery_run(&bat, gen, intf, testid, nthreads, rtype);
    } else {
        TestsBattery_print_info(&bat);
    }

battery_file_freemem:
    TestInfoArray_destruct(&tests_args);
    for (size_t i = 0; i < ntests; i++) {
        free((void *) tests[i].udata);
    }
    free(tests);
    return is_ok;
}

