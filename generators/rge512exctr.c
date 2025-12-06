// Original: https://doi.org/10.5281/zenodo.17713219
// express, brief, default, full, birthday
// PractRand >= 16 TiB
#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

#ifdef __AVX2__
#include "smokerand/x86exts.h"
#endif

#define RGE512EXCTR_PI0 0x243F6A8885A308D3
#define RGE512EXCTR_PI1 0x13198A2E03707344


////////////////////////////////////////////////////
///// Cross-platform (portable) implementation /////
////////////////////////////////////////////////////


typedef struct {
    uint64_t ctr[8];
    uint64_t out[8];
    int pos;
} RGE512ExCtrState;



static inline void RGE512ExCtrState_block(RGE512ExCtrState *obj)
{
    uint64_t *s = obj->out;
    for (int i = 0; i < 8; i++) {
        obj->out[i] = obj->ctr[i];
    }
    for (int i = 0; i < 6; i++) { // 5 rounds is enough to pass "full"
        s[0] += s[1]; s[1] ^= rotl64(s[0], 3);
        s[2] += s[3]; s[3] ^= rotl64(s[2], 12);
        s[4] += s[5]; s[5] ^= rotl64(s[4], 24);    
        s[6] += s[7]; s[7] ^= rotl64(s[6], 48);

        s[5] ^= s[0]; s[0] += rotl64(s[5], 7);
        s[6] ^= s[1]; s[1] += rotl64(s[6], 17);
        s[7] ^= s[2]; s[2] += rotl64(s[7], 23);
        s[4] ^= s[3]; s[3] += rotl64(s[4], 51);
    }
    for (int i = 0; i < 8; i++) {
        obj->out[i] += obj->ctr[i];
    }
}


static void RGE512ExCtrState_init(RGE512ExCtrState *obj, const uint32_t *seed)
{
    obj->ctr[0] = 0;          obj->ctr[1] = 0;
    obj->ctr[2] = RGE512EXCTR_PI0;
    obj->ctr[3] = RGE512EXCTR_PI1;
    obj->ctr[4] = seed[0];    obj->ctr[5] = seed[1];
    obj->ctr[6] = seed[2];    obj->ctr[7] = seed[3];
    RGE512ExCtrState_block(obj);
    obj->pos = 0;
}


static inline uint64_t get_bits_scalar_raw(void *state)
{
    RGE512ExCtrState *obj = state;
    if (obj->pos >= 8) {
        obj->ctr[0]++;
        RGE512ExCtrState_block(obj);
        obj->pos = 0;
    }
    return obj->out[obj->pos++];
}



MAKE_GET_BITS_WRAPPERS(scalar)


static void *create_scalar(const GeneratorInfo *gi, const CallerAPI *intf)
{
    RGE512ExCtrState *obj = intf->malloc(sizeof(RGE512ExCtrState));
    uint32_t seed[4];
    (void) gi;
    seeds_to_array_u32(intf, seed, 4);
    RGE512ExCtrState_init(obj, seed);
    return obj;
}



////////////////////////////////////////
///// AVX2 (vector) implementation /////
////////////////////////////////////////



#define RGE512_NCOPIES 4

typedef union {
    uint64_t u64[RGE512_NCOPIES];
} RGE512Element;


typedef struct {
    RGE512Element ctr[8];
    RGE512Element out[8];
    size_t pos;
} RGE512ExCtrVecState;


#ifdef __AVX2__
static inline __m256i mm256_roti_epi64_def(__m256i in, int r)
{
    return _mm256_or_si256(_mm256_slli_epi64(in, r), _mm256_srli_epi64(in, 64 - r));
}


static inline void rge512exctr_iter0(__m256i *s, int i0, int i1, int shl)
{
    s[i0] = _mm256_add_epi64(s[i0], s[i1]); // s0 += s1
    s[i1] = _mm256_xor_si256(s[i1], mm256_roti_epi64_def(s[i0], shl)); // s1 ^= s0 <<< shl
    
}

static inline void rge512exctr_iter1(__m256i *s, int i0, int i1, int shl)
{
    s[i0] = _mm256_xor_si256(s[i0], s[i1]); // s0 += s1
    s[i1] = _mm256_add_epi64(s[i1], mm256_roti_epi64_def(s[i0], shl)); // s1 ^= s0 <<< shl    
}

#endif

void RGE512ExCtrVecState_block(RGE512ExCtrVecState *obj)
{
#ifdef __AVX2__
    __m256i out[8], x[8];
    for (size_t i = 0; i < 8; i++) {
        out[i] = _mm256_loadu_si256((__m256i *) (void *) &obj->ctr[i].u64[0]);
        x[i] = out[i];
    }
    for (int i = 0; i < 6; i++) {
        rge512exctr_iter0(out, 0, 1,  3);
        rge512exctr_iter0(out, 2, 3,  12);
        rge512exctr_iter0(out, 4, 5,  24);
        rge512exctr_iter0(out, 6, 7,  48);

        rge512exctr_iter1(out, 5, 0,  7);
        rge512exctr_iter1(out, 6, 1,  17);
        rge512exctr_iter1(out, 7, 2,  23);
        rge512exctr_iter1(out, 4, 3,  51);
    }
    for (size_t i = 0; i < 8; i++) {
        out[i] = _mm256_add_epi64(out[i], x[i]);
        _mm256_storeu_si256((__m256i *) (void *) &obj->out[i].u64[0], out[i]);
    }
#else
    (void) obj;
#endif
}


static void RGE512ExCtrVecState_init(RGE512ExCtrVecState *obj, const uint32_t *seed)
{
    for (size_t i = 0; i < RGE512_NCOPIES; i++) {
        obj->ctr[0].u64[i] = i;          obj->ctr[1].u64[i] = 0;
        obj->ctr[2].u64[i] = RGE512EXCTR_PI0;
        obj->ctr[3].u64[i] = RGE512EXCTR_PI1;
        obj->ctr[4].u64[i] = seed[0];    obj->ctr[5].u64[i] = seed[1];
        obj->ctr[6].u64[i] = seed[2];    obj->ctr[7].u64[i] = seed[3];
    }
    RGE512ExCtrVecState_block(obj);
    obj->pos = 0;
}


static inline void RGE512ExCtrVecState_inc_counter(RGE512ExCtrVecState *obj)
{
    for (size_t i = 0; i < RGE512_NCOPIES; i++) {
        obj->ctr[0].u64[i] += RGE512_NCOPIES;
    }
}



static inline uint64_t get_bits_vector_raw(void *state)
{
    RGE512ExCtrVecState *obj = state;
    size_t i = (obj->pos & 0x7), j = (obj->pos >> 3);    
    uint64_t x = obj->out[i].u64[j];
    obj->pos++;
    if (obj->pos == 8 * RGE512_NCOPIES) {
        RGE512ExCtrVecState_inc_counter(obj);
        RGE512ExCtrVecState_block(obj);
        obj->pos = 0;
    }
    return x;
}


MAKE_GET_BITS_WRAPPERS(vector)


static void *create_vector(const GeneratorInfo *gi, const CallerAPI *intf)
{
    RGE512ExCtrVecState *obj = intf->malloc(sizeof(RGE512ExCtrVecState));
    uint32_t seed[4];
    (void) gi;
    seeds_to_array_u32(intf, seed, 4);
    RGE512ExCtrVecState_init(obj, seed);
    return obj;
}


//////////////////////
///// Interfaces /////
//////////////////////

static inline void *create(const CallerAPI *intf)
{
    intf->printf("Not implemented\n");
    return NULL;
}

static int run_self_test(const CallerAPI *intf)
{
    int is_ok = 1;
#ifdef __AVX2__
    uint32_t seed[4];
    seeds_to_array_u32(intf, seed, 4);
    RGE512ExCtrState    *obj_sc  = intf->malloc(sizeof(RGE512ExCtrState));
    RGE512ExCtrVecState *obj_vec = intf->malloc(sizeof(RGE512ExCtrVecState));
    RGE512ExCtrState_init(obj_sc, seed);
    RGE512ExCtrVecState_init(obj_vec, seed);
    for (long i = 0; i < 1000000; i++) {
        const uint64_t u_sc = get_bits_scalar_raw(obj_sc);
        const uint64_t u_vec = get_bits_vector_raw(obj_vec);
        if (u_sc != u_vec) {
            is_ok = 0;
        }
    }
    intf->free(obj_sc);
    intf->free(obj_vec);
#endif
    (void) intf;
    return is_ok;
}

static const char description[] =
"RGE512ex-ctr: an experimental counter based generator based on the ARX mixer.\n"
"The next param values are supported:\n"
"  c99  - portable version, default. Performance is around 0.9 cpb.\n"
"  avx2 - AVX2 version. Performance is around 0.4 cpb.\n";


int EXPORT gen_getinfo(GeneratorInfo *gi, const CallerAPI *intf)
{
    const char *param = intf->get_param();
    gi->description = description;
    gi->nbits = 64;
    gi->create = default_create;
    gi->free = default_free;
    gi->self_test = run_self_test;
    gi->parent = NULL;
    if (!intf->strcmp(param, "c99") || !intf->strcmp(param, "")) {
        gi->name = "RGE512ex-ctr:c99";
        gi->create = create_scalar;
        gi->get_bits = get_bits_scalar;
        gi->get_sum = get_sum_scalar;
    } else if (!intf->strcmp(param, "avx2")) {
        gi->name = "RGE512ex-ctr:avx2";
        gi->create = create_vector;
        gi->get_bits = get_bits_vector;
        gi->get_sum = get_sum_vector;
#ifndef __AVX2__
        intf->printf("Not implemented\n");
        return 0;
#endif
    } else {
        gi->name = "RGE512ex-ctr:unknown";
        gi->get_bits = NULL;
        gi->get_sum = NULL;
        return 0;
    }
    return 1;
}
