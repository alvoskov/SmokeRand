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
        np.hstack((O, O, I, A)),
        np.hstack((I, O, O, O)),
        np.hstack((O, I, O, O)),
        np.hstack((O, O, I, B)),
    ))
    return lfsr.gf2mat_to_list(T)


for i in range(1, 16):
    for j in range(1, 16):
        print(i, j)
        for k in range(j + 1, 16):
            T = make_xorrot32w8(i, j, k)
            if lfsr.is_full_period(T, False):
                print("=====>", i, j, k)
