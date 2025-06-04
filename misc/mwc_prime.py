# https://www.math.ias.edu/~goresky/pdf/p1-goresky.pdf
from sympy import *

def mwc64():
    a = 0xff676488
    m = a * 2**32 - 1
    a_out = pow(2**32, -1, m)
    print(a, a_out)

mwc64()

n = 4
b, delta = 2**16, 50000
for i in range(1, delta):
    a = b - i
    p = a * b ** n - 1
    p2 = (p - 1) // 2
    if isprime(p) and isprime(p2):
        print("a={a} ({a_hex})".format(a=a, a_hex=hex(a)), isprime(p2))
        a_lcg = pow(b, -1, p)
        print("  a_lcg = ", hex(a_lcg), hex(a), hex(p))

