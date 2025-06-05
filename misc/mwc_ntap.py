
from sympy import *

b = 2**64
#dmin, dmin_inc = 2**64 - 100000, 99999
dmin, dmin_inc = 5**26, 1000000

for i in range(dmin, dmin + dmin_inc):
    a = (b - i)
    m = a*b**2 - 1
    if isprime(m) and isprime((m - 1) // 2):
        a_lcg = pow(b, -1, m)
        print(i)
        print(hex(a), hex(a_lcg), hex(m))



#define MWC_A2 0xffa04e67b3c95d86