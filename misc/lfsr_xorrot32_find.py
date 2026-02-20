import lfsr_engine as lfsr
import sympy
import numpy as np

O = lfsr.make_zero_matrix(16)
I = lfsr.make_eye_matrix(16)
L = lfsr.make_shl_matrix(16)
ROL = lfsr.make_rol_matrix(16)

# 1, 3, 5 is good
def make_xorrot16x2(a, b, c):
    A = I + lfsr.gfpow(L, a)
    B = I + lfsr.gfpow(ROL, b) + lfsr.gfpow(ROL, c)

    T = np.vstack((np.hstack((O, A)), np.hstack((I, B))))
    return lfsr.gf2mat_to_list(T)

for a in range(1, 16):
    for b in range(1, 16):
        for c in range(1, 16):
            T = make_xorrot16x2(a, b, c)
            if lfsr.is_full_period(T, False):
                print(a, b, c)





#        x = y;
#        y = x0 ^ (x0 << a) ^ y0 ^ rotl16(y0, b) ^ rotl16(y0, c);
