import lfsr_engine as lfsr
import sympy, galois
import numpy as np

def dandelion_find(n):
    gen = lfsr.XorGenMaker(n)
    for a in range(1,n):
        print(a)
        for b in range(1,n):
            T = gen.make_dandelion_matrix(a, b)
            Tgf = galois.GF(2)(T)
            Tpow = np.linalg.matrix_power(Tgf, 2**(2*n))
            if (Tpow == Tgf).all() and lfsr.is_full_period(T, False):
                print("=====>", a, b)


print("----- 16-bit versions search -----")
dandelion_find(16)
# (5, 3) (7, 6)

print("----- 32-bit versions search -----")
dandelion_find(32)
# (10, 7)

print("----- 64-bit versions search -----")
dandelion_find(64)
# (7, 4) (37, 26)
