import lfsr_engine as lfsr
import sympy
import numpy as np

n = 32
O = lfsr.make_zero_matrix(n)
I = lfsr.make_eye_matrix(n)
L = lfsr.make_shl_matrix(n)
R = lfsr.make_shr_matrix(n)

def make_xorshift3w(a, b, c):
    A = (I + lfsr.gfpow(L, a)) @ (I + lfsr.gfpow(R, b))
    B = I + lfsr.gfpow(R, c)

    T = np.vstack((
        np.hstack((O, O, A)),
        np.hstack((I, O, O)),
        np.hstack((O, I, B))
    ))
    Tout = np.linalg.matrix_power(T, 2**(3*n))
    return lfsr.gf2mat_to_list(T) if (Tout == T).all() else []

for i in range(1, n):
    print(i)
    for j in range(1, n):
        for k in range(1, n):
            T = make_xorshift3w(i, j, k)
            if len(T) > 0 and lfsr.is_full_period(T, False):
                print("=====>", i, j, k)
