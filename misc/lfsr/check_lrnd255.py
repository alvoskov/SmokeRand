import lfsr_engine as lfsr
import sympy
import numpy as np

n = 64
O = lfsr.make_zero_matrix(n)
I = lfsr.make_eye_matrix(n)
L = lfsr.make_shl_matrix(n)
R = lfsr.make_shr_matrix(n)

def make_lrnd255():
    A = lfsr.gfpow(R, 1)  + lfsr.gfpow(R, 4)  + lfsr.gfpow(R, 8)  + lfsr.gfpow(R, 32)
    B = lfsr.gfpow(L, 63) + lfsr.gfpow(L, 60) + lfsr.gfpow(L, 56) + lfsr.gfpow(L, 32)
    T = np.vstack((
        np.hstack((O, O, O, A)),
        np.hstack((I, O, O, B)),
        np.hstack((O, I, O, O)),
        np.hstack((O, O, I, O)),
    ))
    return lfsr.gf2mat_to_list(T)


T = make_lrnd255()
print("Is full period: ", lfsr.is_full_period(T, True))
