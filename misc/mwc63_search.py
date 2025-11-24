from sympy import *
import math

base = 2**32
offset = 2**30
#offset = 12_000
for a in range(offset, offset - 10_000, -1):
    m = a*base - 1
    period_max = (m - 1) // 2
    if isprime(m) and isprime(period_max):
        o = n_order(base, m)
        print("a = {0}, period = {1} (2^{2}) | {3}".format(a, o, math.log2(o), period_max - o))
