import lfsr_engine as lfsr
import sympy

gen = lfsr.XorGenMaker(8)
for a in range(1,8):
    for b in range(1,8):
        for c in range(b + 1, 8):
            T = gen.make_xorrot_matrix(a, b, c)
            if lfsr.is_full_period(T, False):
                print("=====>", a, b, c)


T = gen.make_xorrot_matrix(3, 1, 4)
lfsr.is_full_period(T, True)
