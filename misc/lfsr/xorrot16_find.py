import lfsr_engine as lfsr
import sympy

gen = lfsr.XorGenMaker(16)
abc = range(1,16)
print(abc)
for a in abc:
    print(a)
    for b in abc:
        for c in abc:
            if c > b:
                T = gen.make_xorrot_matrix(a, b, c)
                if lfsr.is_full_period(T, False):
                    print("=====>", a, b, c)
