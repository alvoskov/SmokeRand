import lfsr_engine as lfsr
import sympy
import numpy as np

n = 64
O = lfsr.make_zero_matrix(n)
I = lfsr.make_eye_matrix(n)
L = lfsr.make_shl_matrix(n)
ROL = lfsr.make_rol_matrix(n)

def make_xorrot64x2(a, b, c):
    A = I + lfsr.gfpow(L, a)
    B = I + lfsr.gfpow(ROL, b) + lfsr.gfpow(ROL, c)
    T = np.vstack((np.hstack((I, A)), np.hstack((I, B))))
    Tout = np.linalg.matrix_power(T, 2**128)
    return lfsr.gf2mat_to_list(T) if (Tout == T).all() else []

abc = range(1,64)#list(filter(lambda x: sympy.isprime(x), range(3, 64)))
for a in range(11,64,2):
    for b in range(1,32):
        print(a, b)
        for c in range(32,64):
            if c > b:
                T = make_xorrot64x2(a, b, c)
                if len(T) > 0 and lfsr.is_full_period(T, False):
                    print("=====>", a, b, c)
