import lfsr_engine as lfsr
import sympy
import numpy as np

n = 64
O = lfsr.make_zero_matrix(n)
I = lfsr.make_eye_matrix(n)
L = lfsr.make_shl_matrix(n)
ROL = lfsr.make_rol_matrix(n)

def make_xorrot256(a, b, c):
    A = I + lfsr.gfpow(L, a)
    B = I + lfsr.gfpow(ROL, b) + lfsr.gfpow(ROL, c)

    T = np.vstack((
        np.hstack((I, O, I, A)),
        np.hstack((I, O, O, O)),
        np.hstack((O, I, O, O)),
        np.hstack((O, O, I, B)),
    ))
    Tout = np.linalg.matrix_power(T, 2**256)
    return lfsr.gf2mat_to_list(T) if (Tout == T).all() else []

for i in range(1, 64, 2):
    for j in range(1, 32):
        print(i, j)
        for k in range(33, 64):
            T = make_xorrot256(i, j, k)
            if len(T) > 0 and lfsr.is_full_period(T, False):
                print("=====>", i, j, k)
