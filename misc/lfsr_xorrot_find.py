import lfsr_engine as lfsr
import sympy

gen = lfsr.XorGenMaker(64)
abc = list(filter(lambda x: sympy.isprime(x), range(3, 64)))
print(abc)
for a in abc:
    for b in abc:
        print(a, b)
        for c in abc:
            T = gen.make_xorrot_matrix(a, b, c)
            if lfsr.is_full_period(T, False):
                print("=====>", a, b, c)
