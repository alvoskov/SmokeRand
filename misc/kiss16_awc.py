import sympy
import math

def rotl16(x, r):
    return ( (x << r) | (x >> (16 - r)) ) % 2**16

print("----- XKISS16/AWC period estimation -----")
m = (2**16)**2 + (2**16) - 1
per = sympy.ntheory.n_order(2**16, m)
print("m: ", m, "log2: ", math.log2(m))
print("Is m prime:", sympy.isprime(m))
print("AWC period: ", per, "log2: ", math.log2(per))

per_full = math.lcm(per, 2**32 - 1, 2**16)
print("Full period: ", per_full, "log2: ", math.log2(per_full))

print("AWC period prime factors: ", sympy.factorint(per))
print("xorshift period prime factors: ", sympy.factorint(2**32 - 1))

print("----- Test vectors generation -----")

AWC_MASK = 2**16 - 1
AWC_SH = 16
WEYL_INC = 0x9E39

weyl = 1234
s = [8765, 4321]
awc_x0, awc_x1, awc_c = 3, 2, 1

u, u_ary = 0, [0, 0]
for i in range(0, 10000):
    for j in range(0, 2):
        # xorshift64 part
        s[1] = s[1] ^ s[0]
        s[0] = rotl16(s[0], 13) ^ s[1] ^ ((s[1] << 5) % 2**16)
        s[1] = rotl16(s[1], 10)
        # AWC (add with carry) part
        t = awc_x0 + awc_x1 + awc_c;
        awc_x1 = awc_x0
        awc_c  = t >> AWC_SH
        awc_x0 = t & AWC_MASK
        # Discrete Weyl sequence part
        weyl = weyl + WEYL_INC
        # Combined output
        u_ary[j] = (awc_x0 + s[0] + s[1] + weyl) % 2**16
    

print(hex((u_ary[0] << 16) | (u_ary[1])))
