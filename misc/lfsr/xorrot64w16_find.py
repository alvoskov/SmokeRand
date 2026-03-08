import lfsr_engine as lfsr
import sympy
import numpy as np

n = 16
O = lfsr.make_zero_matrix(n)
I = lfsr.make_eye_matrix(n)
L = lfsr.make_shl_matrix(n)
ROL = lfsr.make_rol_matrix(n)

def make_xorrot32w8(a, b, c):
    A = I + lfsr.gfpow(L, a)
    B = I + lfsr.gfpow(ROL, b) + lfsr.gfpow(ROL, c)

    T = np.vstack((
        np.hstack((I, O, I, A)),
        np.hstack((I, O, O, O)),
        np.hstack((O, I, O, O)),
        np.hstack((O, O, I, B)),
    ))
    Tout = np.linalg.matrix_power(T, 2**64)
    return lfsr.gf2mat_to_list(T) if (Tout == T).all() else []


ijk = range(1, 16)
for i in ijk:
    print(i)
    for j in ijk:
        for k in ijk:
            if k > j:
                T = make_xorrot32w8(i, j, k)
                if len(T) > 0 and lfsr.is_full_period(T, False):
                    print("=====>", i, j, k)
