import sympy, random
b = 2**64 + 2

for i in range(10000000):
    #a = random.randint(2**63 + 1, 2**64 - 1)
    a = random.randint(2**31 + 1, 2**32 - 1)
    #a = 16166422908038702740
    m = a*b - 1
    if sympy.isprime(m):
        print(a)
        #print(hex(b), hex(pow(a, -1, m)))
        f = sympy.factorint((m - 1) // 2, limit=100_000)
        if all([sympy.isprime(x) for x in f]):
            o = sympy.n_order(a, m)
            if o == m - 1:
                print("WOW!", a)                
                break
            else:
                print("====>", o, o / m)
