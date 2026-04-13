import sympy, math

mwcfp = [
    [64,   4291122658], # Certified
    [128,  4142557070], # Certified
    [256,  4238794375], # Certified
    [512,  4294137855], # Certified
    [1024, 4287999874], # Certified
    [2048, 4273733971], # Certified
    [128,  17741297344439402706], # Certified
    [256,  17873945764845871615], # Certified
    [512,  16996179571824182298], # Certified
    [1024, 18439945329244120106], # Certified
    [2048, 18235832631006504774], # Certified
    [4096, 17633152884372258591]  # Certified
]

for e in mwcfp:
    p, a = e[0], e[1]
    b = 2**p + 2
    m = a*b - 1
    print("--------------------------")
    print(f"b = 2**{p} + 2, a = {a}")
    o = sympy.n_order(a, m)
    print(f"  isprime(m = ab - 1) = {sympy.isprime(m)}", end='')
    print(f"  full_period: {(o - (m - 1)) == 0}, period: {math.log2(o)}")
    print(f"  m = {m}")
    print(f"  factors = ", sympy.factorint(m - 1))
