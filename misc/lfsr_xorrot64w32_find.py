import lfsr_engine as lfsr
import sympy
import numpy as np

n = 32
O = lfsr.make_zero_matrix(n)
I = lfsr.make_eye_matrix(n)
L = lfsr.make_shl_matrix(n)
ROL = lfsr.make_rol_matrix(n)

def make_xorrot64x2(a, b, c):
    A = I + lfsr.gfpow(L, a)
    B = I + lfsr.gfpow(ROL, b) + lfsr.gfpow(ROL, c)

    T = np.vstack((np.hstack((O, A)), np.hstack((I, B))))
    return lfsr.gf2mat_to_list(T)

abc = range(1,32)#[1] + list(filter(lambda x: sympy.isprime(x), range(3, n)))
for a in abc:
    for b in abc:
        print(a, b)
        for c in abc:
            if c > b:
                T = make_xorrot64x2(a, b, c)
                if lfsr.is_full_period(T, False):
                    print("=====>", a, b, c)
