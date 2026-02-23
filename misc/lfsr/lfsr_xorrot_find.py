import lfsr_engine as lfsr
import sympy, galois
import numpy as np


gen = lfsr.XorGenMaker(64)
#abc = list(filter(lambda x: sympy.isprime(x), range(3, 64)))
#print(abc)
for a in range(1,64,2):
    print(a)
    for b in range(1,32):
        for c in range(32,64):
            T = gen.make_xorrot_matrix(a, b, c)
            Tgf = galois.GF(2)(T)
            Tpow = np.linalg.matrix_power(Tgf, 2**64)
            if (Tpow == Tgf).all() and lfsr.is_full_period(T, False):
                print("=====>", a, b, c)
