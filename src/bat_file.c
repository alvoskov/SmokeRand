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
#include "smokerand/entropy.h"
#include <stdio.h>
#include <stdlib.h>

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

void TestInfo_init(TestInfo *obj, const char *testname)
{
    obj->args = malloc(sizeof(TestArgument));
    obj->nargs = 0;
    xstrcpy_s(obj->testname, 64, testname);
}

void TestInfo_addarg(TestInfo *obj, const char *name, const char *value)
{
    obj->nargs++;
    obj->args = realloc(obj->args, obj->nargs * sizeof(TestArgument));
    xstrcpy_s(obj->args[obj->nargs - 1].name, 64, name);
    xstrcpy_s(obj->args[obj->nargs - 1].value, 64, value);
}

const char *TestInfo_get_arg_value(const TestInfo *obj, const char *name)
{
    for (size_t i = 0; i < obj->nargs; i++) {
        if (!strcmp(name, obj->args[i].name)) {
            return obj->args[i].value;
        }
    }
    return NULL;
}


long long TestInfo_get_arg_intvalue(const TestInfo *obj, const char *name)
{
    const char *txtvalue = TestInfo_get_arg_value(obj, name);
    long long value;
    if (txtvalue == NULL) {
        return LLONG_MAX;
    }
    if (sscanf(txtvalue, "%lld", &value) != 1) {
        return LLONG_MAX;
    }
    return value;
}


int TestInfo_arg_value_to_code(const TestInfo *obj, const char *name,
    const char **vals, const int *codes, char *errmsg, int *is_ok)
{
    const char *value = TestInfo_get_arg_value(obj, name);
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



void TestInfo_free(TestInfo *obj)
{
    free(obj->args);
}

void TestInfo_print(const TestInfo *obj)
{
    printf("%s ", obj->testname);
    for (size_t i = 0; i < obj->nargs; i++) {
        printf("%s=%s ", obj->args[i].name, obj->args[i].value);
    }
    printf("\n");
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

//////////////////////////////////////////////
///// TestInfoArray class implementation /////
//////////////////////////////////////////////

void TestInfoArray_init(TestInfoArray *obj)
{
    obj->tests = malloc(sizeof(TestInfo));
    obj->ntests = 0;
}

/**
 * @brief test Test info (WILL TAKE OWNERSHIP!)
 */
void TestInfoArray_addtest(TestInfoArray *obj, const TestInfo *test)
{
    obj->ntests++;
    obj->tests = realloc(obj->tests, obj->ntests * sizeof(TestInfo));
    obj->tests[obj->ntests - 1] = *test;
}

void TestInfoArray_free(TestInfoArray *obj)
{
    for (size_t i = 0; i < obj->ntests; i++) {
        TestInfo_free(&obj->tests[i]);
    }
    free(obj->tests);
}

void TestInfoArray_clear(TestInfoArray *obj)
{
    TestInfoArray_free(obj);
    TestInfoArray_init(obj);
}

//////////////////
///// LOADER /////
//////////////////

static TestInfoArray load_tests(const char *filename)
{
    FILE *fp = fopen(filename, "r");    
    char linetxt[512], linenum = 0;
    TestInfoArray tests;
    TestInfoArray_init(&tests);
    if (fp == NULL) {
        fprintf(stderr, "File '%s' not found\n", filename);
        return tests;
    }
    while (fgets(linetxt, 512, fp)) {
        // Erase comments, ignore empty strings
        erase_comments(linetxt, 512);
        linenum++;
        char *token = strtok(linetxt, " \t\r\n");
        if (token == NULL) {
            continue;
        }
        // Parse not empty string
        // a) Test name
        TestInfo testinfo;
        TestInfo_init(&testinfo, token);
        testinfo.linenum = linenum;
        // b) Test arguments
        while ((token = strtok(NULL, " ")) != NULL) {
            char name[64], value[64];
            char *eqpos = strstr(token, "=");
            int i;
            if (eqpos == NULL) {
                fprintf(stderr, "Error in line %d. Token '%s' doesn't contain '='",
                    linenum, token);
                TestInfoArray_clear(&tests);
                TestInfo_free(&testinfo);
                return tests;
            }
            for (i = 0; i < 64 && token[i] != '='; i++) {
                name[i] = token[i];
            }
            name[i] = '\0';
            xstrcpy_s(value, 64, eqpos + 1);
            TestInfo_addarg(&testinfo, name, value);
        }
        TestInfoArray_addtest(&tests, &testinfo);
    }
    fclose(fp);
    return tests;
}


static int parse_bspace_nd(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    long long nbits_per_dim = TestInfo_get_arg_intvalue(obj, "nbits_per_dim");
    if (nbits_per_dim == LLONG_MAX || nbits_per_dim < 1 || nbits_per_dim > 64) {
        return 0;
    }

    long long ndims = TestInfo_get_arg_intvalue(obj, "ndims");
    if (ndims == LLONG_MAX || ndims < 1 || ndims > 64) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid ndims value");
        return 0;
    }

    long long nsamples = TestInfo_get_arg_intvalue(obj, "nsamples");
    if (nsamples == LLONG_MAX || nsamples < 1 || nsamples > (1ll << 30ll)) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid nsamples value");
        return 0;
    }

    long long get_lower = TestInfo_get_arg_intvalue(obj, "get_lower");
    if (nsamples == LLONG_MAX || (get_lower != 0 && get_lower != 1)) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid get_lower value");    
        return 0;
    }
    BSpaceNDOptions *opts = calloc(1, sizeof(BSpaceNDOptions));
    opts->nbits_per_dim = nbits_per_dim;
    opts->ndims = ndims;
    opts->nsamples = nsamples;
    opts->get_lower = get_lower;
    out->name = obj->testname;
    out->run = bspace_nd_test_wrap;
    out->udata = opts;
    out->nseconds = 1;
    out->ram_load = ram_hi;
    return 1;
}

static int parse_collisionover(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    int is_ok = parse_bspace_nd(out, obj, errmsg);
    out->run = collisionover_test_wrap;
    return is_ok;
}



static int parse_nbit_words_freq(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    long long bits_per_word = TestInfo_get_arg_intvalue(obj, "bits_per_word");
    if (bits_per_word == LLONG_MAX || bits_per_word < 1 || bits_per_word > 16) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid bits_per_word value");
        return 0;
    }

    long long average_freq = TestInfo_get_arg_intvalue(obj, "average_freq");
    if (average_freq == LLONG_MAX || average_freq < 8 || average_freq > (1ll << 20ll)) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid average_freq value");
        return 0;
    }

    long long nblocks = TestInfo_get_arg_intvalue(obj, "nblocks");
    if (nblocks == LLONG_MAX || nblocks < 16 || nblocks > (1ll << 30ll)) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid nblocks value");
        return 0;
    }
    NBitWordsFreqOptions *opts = calloc(1, sizeof(NBitWordsFreqOptions));
    opts->bits_per_word = bits_per_word;
    opts->average_freq = average_freq;
    opts->nblocks = nblocks;
    out->name = obj->testname;
    out->run = nbit_words_freq_test_wrap;
    out->udata = opts;
    out->nseconds = 1;
    out->ram_load = ram_hi;
    return 1;
}

static int parse_bspace4_8d_decimated(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    long long step = TestInfo_get_arg_intvalue(obj, "step");
    if (step == LLONG_MAX || step < 1 || step > (1ll << 30ll)) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid step value");
        return 0;
    }

    Bspace4x8dDecOptions *opts = calloc(1, sizeof(Bspace4x8dDecOptions));
    opts->step = step;
    out->name = obj->testname;
    out->run = bspace4_8d_decimated_test_wrap;
    out->udata = opts;
    out->nseconds = 1;
    out->ram_load = ram_hi;
    return 1;
}


static int parse_linearcomp(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    long long nbits = TestInfo_get_arg_intvalue(obj, "nbits");
    if (nbits == LLONG_MAX || nbits < 16 || nbits > (1ll << 24ll)) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid nbits value");
        return 0;
    }
    const char *bitpos_descr = TestInfo_get_arg_value(obj, "bitpos");
    long long bitpos;
    if (!strcmp(bitpos_descr, "low")) {
        bitpos = linearcomp_bitpos_low;
    } else if (!strcmp(bitpos_descr, "mid")) {
        bitpos = linearcomp_bitpos_mid;
    } else if (!strcmp(bitpos_descr, "high")) {
        bitpos = linearcomp_bitpos_high;
    } else {
        bitpos = TestInfo_get_arg_intvalue(obj, "bitpos");
        if (bitpos == LLONG_MAX || bitpos < 0 || bitpos > 64) {
            snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid bitpos value");
            return 0;
        }
    }

    LinearCompOptions *opts = calloc(1, sizeof(LinearCompOptions));
    opts->nbits = nbits;
    opts->bitpos = bitpos;
    out->name = obj->testname;
    out->run = linearcomp_test_wrap;
    out->udata = opts;
    out->nseconds = 1;
    out->ram_load = ram_hi;
    return 1;
}


static int parse_hamming_ot(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    long long nbytes = TestInfo_get_arg_intvalue(obj, "nbytes");
    if (nbytes == LLONG_MAX || nbytes < 65536 || nbytes > (1ll << 40ll)) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid nbytes value");
        return 0;
    }

    int is_ok;
    const char *m_txt[] = {"values", "bytes", "bytes_low8", "bytes_low1", NULL};
    const int m_codes[] = {hamming_ot_values, hamming_ot_bytes,
        hamming_ot_bytes_low8, hamming_ot_bytes_low1, 0};
    HammingOtMode mode = TestInfo_arg_value_to_code(obj, "mode",
        m_txt, m_codes, errmsg, &is_ok);
    if (!is_ok) {
        return 0;
    }

    HammingOtOptions *opts = calloc(1, sizeof(HammingOtOptions));
    opts->nbytes = nbytes;
    opts->mode = mode;
    out->name = obj->testname;
    out->run = hamming_ot_test_wrap;
    out->udata = opts;
    out->nseconds = 1;
    out->ram_load = ram_hi;
    return 1;
}


static int parse_hamming_ot_long(TestDescription *out, const TestInfo *obj, char *errmsg)
{
    long long nvalues = TestInfo_get_arg_intvalue(obj, "nvalues");
    if (nvalues == LLONG_MAX || nvalues < 65536 || nvalues > (1ll << 40ll)) {
        snprintf(errmsg, ERRMSG_BUF_SIZE, "Invalid nvalues value");
        return 0;
    }
    int is_ok;
    const char *ws_txt[] = {"w128", "w256", "w512", "w1024", NULL};
    const int ws_codes[] = {hamming_ot_w128, hamming_ot_w256,
        hamming_ot_w512, hamming_ot_w1024, 0};
    HammingOtWordSize ws = TestInfo_arg_value_to_code(obj, "wordsize",
        ws_txt, ws_codes, errmsg, &is_ok);
    if (!is_ok) {
        return 0;
    }
    HammingOtLongOptions *opts = calloc(1, sizeof(HammingOtLongOptions));
    opts->nvalues = nvalues;
    opts->wordsize = ws;
    out->name = obj->testname;
    out->run = hamming_ot_long_test_wrap;
    out->udata = opts;
    out->nseconds = 1;
    out->ram_load = ram_hi;
    return 1;
}




/**
 * @brief Loads a custom battery from a user-defined text file.
 * @details File format:
 * test_name param1=value1 param2=value2
 */
int battery_file(const char *filename, GeneratorInfo *gen, CallerAPI *intf,
    unsigned int testid, unsigned int nthreads)
{
    char errmsg[ERRMSG_BUF_SIZE];
    errmsg[0] = '\0';
    TestInfoArray tests_args = load_tests(filename);
    if (tests_args.ntests == 0) {
        TestInfoArray_free(&tests_args);
        return 0;
    }
    size_t ntests = tests_args.ntests;
    TestDescription *tests = calloc(ntests + 1, sizeof(TestDescription));

    int is_ok = 1;
    for (size_t i = 0; i < tests_args.ntests; i++) {        
        const char *name = TestInfo_get_arg_value(&tests_args.tests[i], "test");
        const TestInfo *curtest = &tests_args.tests[i];
        if (!strcmp(name, "bspace_nd")) {
            is_ok = parse_bspace_nd(&tests[i], curtest, errmsg);
        } else if (!strcmp(name, "bspace4_8d_decimated")) {
            is_ok = parse_bspace4_8d_decimated(&tests[i], curtest, errmsg);
        } else if (!strcmp(name, "collisionover")) {
            is_ok = parse_collisionover(&tests[i], curtest, errmsg);
        } else if (!strcmp(name, "hamming_ot")) {
            is_ok = parse_hamming_ot(&tests[i], curtest, errmsg);
        } else if (!strcmp(name, "hamming_ot_long")) {
            is_ok = parse_hamming_ot_long(&tests[i], curtest, errmsg);
        } else if (!strcmp(name, "linearcomp")) {
            is_ok = parse_linearcomp(&tests[i], curtest, errmsg);
        } else if (!strcmp(name, "nbit_words_freq")) {    
            is_ok = parse_nbit_words_freq(&tests[i], curtest, errmsg);
        } else {
            fprintf(stderr, "Error in line %d. Unknown test '%s'\n", curtest->linenum, name);
            is_ok = 0;
            goto battery_file_freemem;
        }

        if (!is_ok) {
            fprintf(stderr, "Error in line %d. %s\n", curtest->linenum, errmsg);
            goto battery_file_freemem;
        }
    }

    TestsBattery bat = {.name = "CUSTOM", .tests = tests};
    if (gen != NULL) {
        TestsBattery_run(&bat, gen, intf, testid, nthreads);
    } else {
        TestsBattery_print_info(&bat);
    }

battery_file_freemem:
    TestInfoArray_free(&tests_args);
    for (size_t i = 0; i < ntests; i++) {
        free((void *) tests[i].udata);
    }
    free(tests);
    return is_ok;
}

