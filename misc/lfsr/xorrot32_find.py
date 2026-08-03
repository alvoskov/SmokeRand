import lfsr_engine as lfsr
import sympy, galois
import numpy as np

gen = lfsr.XorGenMaker(32)
abc = range(1,32)
print(abc)
for a in abc:
    for b in abc:
        print(a, b)
        for c in abc:
            if c > b:
                T = gen.make_xorrot_matrix(a, b, c)
                Tout = np.linalg.matrix_power(galois.GF(2)(T), 2**32)
                is_candidate = (Tout == T).all()
                if is_candidate:
                    print("????>", a, b, c)
                    if lfsr.is_full_period(T, False):
                        print("=====>", a, b, c)
