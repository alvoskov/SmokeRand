import sympy

for a in range(2, 256):
    m = a*(2**16 + 2) - 1
    if sympy.isprime(m) and sympy.n_order(a, m) == m - 1:
        print(a)

print("----------")
for a in range(2, 256):
    m = a*(2**128 + 2) - 1
    if sympy.isprime(m) and sympy.n_order(a, m) == m - 1:
        print(a)