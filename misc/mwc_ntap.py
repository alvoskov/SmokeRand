
from sympy import *

b = 2**64
#dmin, dmin_inc = 2**64 - 100000, 99999

b = 2**32
dmin, dmin_inc = 2**32 - 100000, 99999 # For bad multiplier
dmin, dmin_inc = 1000000, 100000 # For good multiplier



b = 2**8
dmin, dmin_inc = 1, 250


b = 2**16
dmin, dmin_inc = 1, 65535

#dmin, dmin_inc = 5**26, 1000000

for i in range(dmin, dmin + dmin_inc):
    a = (b - i)
    m = a*b**8 - 1
    if isprime(m) and isprime((m - 1) // 2):
        a_lcg = pow(b, -1, m)
        print(i)
        print(a, hex(a), hex(a_lcg), hex(m))



#define MWC_A2 0xffa04e67b3c95d86