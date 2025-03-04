/**
 * @file mixmax_shared.c
 * @brief A simplified modification of MIXMAX generator.
 * @details The MIXMAX algorithm was suggested by K.Savidy and G.K. Savidy.
 * The modern C implementation is created by Konstantin Savvidy.
 *
 * References:
 *
 * 1. https://www.gnu.org/software/gsl/
 * 2. https://mixmax.hepforge.org/
 * 3. G.K.Savvidy and N.G.Ter-Arutyunian. On the Monte Carlo simulation
 *    of physical systems. J.Comput.Phys. 97, 566 (1991);
 *    Preprint EPI-865-16-86, Yerevan, Jan. 1986
 * 4. K.Savvidy The MIXMAX random number generator. Comp. Phys. Commun.
 *    196 (2015), pp 161–165 http://dx.doi.org/10.1016/j.cpc.2015.06.003
 *
 * This modification returns lower 32 bits of its 61-bit output. It also
 * contains an internal self-test that is based on the values obtained
 * from not modified code by K.Savvidy. The latest versions of MIXMAX are
 * released under proprietary licenses. This modification is based on
 * the plugin for GSL released under GNU LGPL v3.
 *
 * @copyright The original code is created by Konstantin Savvidy
 * 
 * Simplified modification for SmokeRand:
 *
 * (C) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * The code is released under GNU Lesser General Public License v3.
 */

#include "smokerand/cinterface.h"

PRNG_CMODULE_PROLOG

//#define USE_INLINE_ASM

/* The currently recommended generator is the three-parameter MIXMAX with
N=240, s=487013230256099140, m=2^51+1

   Other newly recommended N are N=8, 17 and 240, 
   as well as the ordinary MIXMAX with N=256 and s=487013230256099064

   Since the algorithm is linear in N, the cost per number is almost independent of N.
 */

#define N 240

typedef struct rng_state_st {
    uint64_t V[N];
    uint64_t sumtot;
    int counter;
} rng_state_t;

// get the N programmatically, useful for checking the value for which the library was compiled
/*
static inline int rng_get_N(void)
{
    return N;
}
*/

uint64_t iterate_raw_vec(uint64_t* Y, uint64_t sumtotOld);


//   FUNCTIONS FOR SEEDING

void seed_spbox(rng_state_t* X, uint64_t seed);    // non-linear method, makes certified unique vectors,  probability for streams to collide is < 1/10^4600
void fill_array(rng_state_t* X, unsigned int n, double *array); // fastest method: set n to a multiple of N (e.g. n=256)
void iterate_and_fill_array(rng_state_t* X, double *array); // fills the array with N numbers


#define BITS  61

/* magic with Mersenne Numbers */

#define M61   2305843009213693951ULL
#define MERSBASE M61 //xSUFF(M61)
#define MOD_PAYNE(k) ((((k)) & MERSBASE) + (((k)) >> BITS) )  // slightly faster than my old way, ok for addition
#define MOD_REM(k) ((k) % MERSBASE )  // latest Intel CPU is supposed to do this in one CPU cycle, but on my machines it seems to be 20% slower than the best tricks
#define MOD_MERSENNE(k) MOD_PAYNE(k)
#define INV_MERSBASE (0x1p-61)
//#define INV_MERSBASE (0.4336808689942017736029811203479766845703E-18)
//const double INV_MERSBASE=(0.4336808689942017736029811203479766845703E-18); // gives "duplicate symbol" error

// Begin of fmodmulM61 implementation
#if defined(__x86_64__)
static inline uint64_t mod128(__uint128_t s)
{
    uint64_t s1;
    s1 = ( (  ((uint64_t)s)&MERSBASE )    + (  ((uint64_t)(s>>64)) * 8 )  + ( ((uint64_t)s) >>BITS) );
    return	MOD_MERSENNE(s1);
}

static inline uint64_t fmodmulM61(uint64_t cum, uint64_t a, uint64_t b)
{
	__uint128_t temp;
	temp = (__uint128_t)a*(__uint128_t)b + cum;
	return mod128(temp);
}

#else // on all other platforms, including 32-bit linux, PPC and PPC64 and all Windows
#define MASK32 0xFFFFFFFFULL
static inline uint64_t fmodmulM61(uint64_t cum, uint64_t s, uint64_t a)
{
    register uint64_t o, ph, pl, ah, al;
    o = s * a;
    ph = s >> 32;
    pl = s & MASK32;
    ah = a >> 32;
    al = a & MASK32;
    o = (o & M61) + ((ph*ah)<<3) + ((ah*pl+al*ph + ((al*pl)>>32))>>29) ;
    o += cum;
    o = (o & M61) + ((o>>61));
    return o;
}
#endif
// End of fmodmulM61 implementation


    
// the charpoly is irreducible for the combinations of N and SPECIAL

#if (N==17)
#define SPECIALMUL 36 // m=2^36+1, other valid possibilities are m=2^13+1, m=2^19+1, m=2^24+1
#define SPECIAL 0
    
#elif (N==240)
#define SPECIALMUL 51   // m=2^51+1 and a SPECIAL=487013230256099140
#define SPECIAL 487013230256099140ULL
#define MOD_MULSPEC(k) fmodmulM61( 0, SPECIAL , (k) )
    
#elif (N==44851)
#define SPECIALMUL 0
#define SPECIAL -3
#define MOD_MULSPEC(k) MOD_MERSENNE(3*(MERSBASE-(k)))


#else
#warning Not a verified N, you are on your own!
#define SPECIALMUL 58
#define SPECIAL 0
    
#endif // list of interesting N for modulus M61 ends here


static inline uint64_t get_next(rng_state_t* X)
{
    int i = X->counter;
    if (i <= (N-1)) {
        X->counter++;
        return X->V[i];
    } else {
        X->sumtot = iterate_raw_vec(X->V, X->sumtot);
        X->counter=2;
        return X->V[1];
    }
}

#if 0	
static inline double get_next_float(rng_state_t *X)
{
    /* cast to signed int trick suggested by Andrzej Görlich     */
    int64_t Z = (int64_t) get_next(X);
    double F;
#if defined(__GNUC__) && (__GNUC__ < 5) && (!defined(__ICC)) && defined(__x86_64__) && defined(__SSE2_MATH__) && defined(USE_INLINE_ASM)
/* using SSE inline assemly to zero the xmm register, just before int64 -> double conversion,
   not really necessary in GCC-5 or better, but huge penalty on earlier compilers 
 */
   __asm__  __volatile__("pxor %0, %0; "
                        :"=x"(F)
                        );
#endif
    F=Z;
    return F*INV_MERSBASE;
}
#endif

#undef N

enum {
    N = 240
};


//////////////////////////////////////////////////////////////////////////////////

int iterate(rng_state_t* X)
{
	X->sumtot = iterate_raw_vec(X->V, X->sumtot);
	return 0;
}

#if (SPECIALMUL!=0)
inline uint64_t MULWU (uint64_t k){ return (( (k)<<(SPECIALMUL) & M61) | ( (k) >> (BITS-SPECIALMUL))  )  ;}
#elif (SPECIALMUL==0)
inline uint64_t MULWU (uint64_t k){ return 0;}
#else
#error SPECIALMUL not defined
#endif

static uint64_t modadd(uint64_t foo, uint64_t bar)
{
#if (defined(__x86_64__) || defined(__i386__)) &&  defined(__GNUC__) && defined(USE_INLINE_ASM)
    uint64_t out;
    /* Assembler trick suggested by Andrzej Görlich     */
    __asm__ ("addq %2, %0; "
             "btrq $61, %0; "
             "adcq $0, %0; "
             :"=r"(out)
             :"0"(foo), "r"(bar)
             );
    return out;
#else
    return MOD_MERSENNE(foo+bar);
#endif
}

uint64_t iterate_raw_vec(uint64_t* Y, uint64_t sumtotOld)
{
	// operates with a raw vector, uses known sum of elements of Y
	int i;
#if (SPECIAL!=0)
    uint64_t temp2 = Y[1];
#endif
	uint64_t  tempP, tempV;
    Y[0] = ( tempV = sumtotOld);
    uint64_t sumtot = Y[0], ovflow = 0; // will keep a running sum of all new elements
	tempP = 0;              // will keep a partial sum of all old elements
	for (i=1; i<N; i++){
#if (SPECIALMUL!=0)
        uint64_t tempPO = MULWU(tempP);
        tempP = modadd(tempP,Y[i]);
        tempV = MOD_MERSENNE(tempV + tempP + tempPO); // edge cases ?
#else
        tempP = modadd(tempP , Y[i]);
        tempV = modadd(tempV , tempP);
#endif
        Y[i] = tempV;
		sumtot += tempV; if (sumtot < tempV) {ovflow++;}
	}
#if (SPECIAL!=0)
    temp2 = MOD_MULSPEC(temp2);
    Y[2] = modadd( Y[2] , temp2 );
    sumtot += temp2; if (sumtot < temp2) {ovflow++;}
#endif
	return MOD_MERSENNE(MOD_MERSENNE(sumtot) + (ovflow <<3 ));
}

/**
 * @brief Return an array of n random numbers uniformly distributed in (0,1]
 */
void fill_array(rng_state_t* X, unsigned int n, double *array)
{
    unsigned int i,j;
    const int M = N - 1;
    for (i=0; i<(n/M); i++){
        iterate_and_fill_array(X, array+i*M);
    }
    unsigned int rem=(n % M);
    if (rem) {
        iterate(X);
        for (j=0; j< (rem); j++){
            array[M*i+j] = (int64_t)X->V[j] * (double)(INV_MERSBASE);
        }
        // Needed to continue with single fetches from the exact spot,
        // but if you only use fill_array to get numbers then it is not necessary
        X->counter = j;
    } else {
        X->counter = N;
    }
}

void iterate_and_fill_array(rng_state_t* X, double *array)
{
    uint64_t *Y = X->V;
    uint64_t tempP, tempV;
#if (SPECIAL != 0)
    uint64_t temp2 = Y[1];
#endif
    Y[0] = (tempV = X->sumtot);
    uint64_t sumtot = Y[0], ovflow = 0; // will keep a running sum of all new elements
    tempP = 0;             // will keep a partial sum of all old elements
    for (int i = 1; i < N; i++) {
#if (SPECIALMUL!=0)
        uint64_t tempPO = MULWU(tempP);
        tempP = modadd(tempP,Y[i]);
        tempV = MOD_MERSENNE(tempV + tempP + tempPO); // edge cases ?
#else
        tempP = MOD_MERSENNE(tempP + Y[i]);
        tempV = MOD_MERSENNE(tempV + tempP);
#endif
        Y[i] = tempV;
        sumtot += tempV; if (sumtot < tempV) {ovflow++;}
        array[i-1] = (int64_t)tempV * (double)(INV_MERSBASE);
    }
#if (SPECIAL!=0)
    temp2 = MOD_MULSPEC(temp2);
    Y[2] = modadd( Y[2] , temp2 );
    sumtot += temp2; if (sumtot < temp2) {ovflow++;}
#endif
    X->sumtot = MOD_MERSENNE(MOD_MERSENNE(sumtot) + (ovflow <<3 ));
}

/**
 * @brief Non-linear seeding method, makes certified unique vectors,
 * probability for streams to collide is < 1/10^4600.
 * @details It is based on a 64-bit LCG from Knuth line 26, in combination
 * with a bit swap is used to seed
 */
void seed_spbox(rng_state_t* X, uint64_t seed)
{ 
    const uint64_t MULT64=6364136223846793005ULL; 
    int i;
    uint64_t sumtot=0,ovflow=0;
    
    if (seed == 0){
        seed = 0xDEADBEEF;
    }
	
    uint64_t l = seed;

    for (i=0; i < N; i++){
        l*=MULT64; l = (l << 32) ^ (l>>32);
        X->V[i] = l & MERSBASE;
        sumtot += X->V[(i)]; if (sumtot < X->V[(i)]) {ovflow++;}
    }
    X->counter = N;  // set the counter to N if iteration should happen right away
    X->sumtot = MOD_MERSENNE(MOD_MERSENNE(sumtot) + (ovflow <<3 ));
}



//////////////////////////////////////////////////////////////////////////////////





static inline uint64_t get_bits_raw(void *state)
{
    rng_state_t *obj = state;
    uint64_t x = get_next(obj);
    return x & 0xFFFFFFFF;
}

static void *create(const CallerAPI *intf)
{
    rng_state_t *obj = intf->malloc(sizeof(rng_state_t));
	seed_spbox(obj, 123);
    return (void *) obj;
}

static int run_self_test(const CallerAPI *intf)
{
    static uint32_t x_ref[] = {
        0x6C0050AE, 0x0AB5041E, 0xDA6DC23B, 0x34C19D00,
        0xFEA3375E, 0xAC87062B, 0xA1204107, 0x743FA216,
        0xA4E9F6B9, 0xD72CE425, 0xC1E0F655, 0x43274FE8,
        0x53C11860, 0xF47A5FA0, 0x620F338D, 0x297C5553
    };

    int is_ok = 1;
    rng_state_t *obj = create(intf);
    for (int i = 0; i < 16; i++) {
        uint32_t x = (uint32_t) get_bits_raw(obj);
        intf->printf("%.8llX|%.8llX\n", x, x_ref[i]);
        if (x != x_ref[i]) {
            is_ok = 0;
        }
    }
    intf->free(obj);
    return is_ok;
}


MAKE_UINT32_PRNG("MIXMAX", run_self_test)
