import lfsr_engine as lfsr
import sympy
import numpy as np

# obj->xs ^= obj->xs >> 13;
# obj->xs ^= obj->xs << 17;
# obj->xs ^= obj->xs >> 5;
a, b, c = 13, 17, 5
n = 32
I = lfsr.make_eye_matrix(n)
R = lfsr.make_shr_matrix(n)
L = lfsr.make_shl_matrix(n)
T = ( (I + lfsr.gfpow(R, a)) @ (I + lfsr.gfpow(L, b)) @ (I + lfsr.gfpow(R, c)) )
T = lfsr.gf2mat_to_list(T)
print(T)



lfsr.is_full_period(T, True)

