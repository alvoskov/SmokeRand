import sympy

class RwcGenerator:
    def __init__(self, c=12345, x=[1234, 5678, 8765, 4321],
                                a=[2111111111, 1492, 1776, 5115], b=2**32):
        self.c, self.x, self.a, self.b = c, x, a, b
        self.m, self.r = -1, len(x)
        for i in range(self.r):
            self.m += a[i] * b ** (self.r - i)
        self.lcg, self.a_lcg = self.to_lcg(), pow(b, -1, self.m)

    def to_lcg(self):
        b, r = self.b, self.r
        lcg = b**r * self.c
        for j in range(r):
            lcg += self.x[j] * b**j
        for k in range(1, r):
            for i in range(1, k + 1):
                lcg -= b**k * self.a[r - i] * self.x[k - i]
        return lcg

    def check_constants(self):
        print(sympy.isprime(self.m), sympy.n_order(self.b, self.m) / self.m)

    def next(self):
        mul = self.c + sum(map(lambda x: x[0]*x[1], zip(self.a, self.x)))
        self.c, self.x = mul // self.b, self.x[1:] + [mul % self.b]
        self.lcg = (self.a_lcg * self.lcg) % self.m
        return self.x[-1]

    def __str__(self):
        lcg_calc = self.to_lcg()
        txt = f"c=0x{self.c:X} x = "
        for xi in self.x:
            txt += f"0x{xi:X} "
        txt += f"\n  lcg={self.lcg:X} lcg_calc={lcg_calc:X}"
        return txt


def run_generator(gen):
    for i in range(100_000):
        gen.next()
    gen.next(); print(gen)
    gen.next(); print(gen)
    gen.next(); print(gen)
    gen.next(); print(gen)


print("----- MWC-FP -----")
genlarge = RwcGenerator(c=0, x=[2**256], a=[16166422908038702740], b=2**256 + 1)
print(hex(genlarge.lcg), hex(genlarge.c), hex(genlarge.x[0]))
for i in range(5):
    genlarge.next()
    print(hex(genlarge.lcg), hex(genlarge.c), hex(genlarge.x[0]))
    genlarge.next()
    print(hex(genlarge.lcg), hex(genlarge.c), hex(genlarge.x[0]))

#print(genlarge.check_constants())

"""
print("----- 64-bit huge RWC -----")
genlarge = RwcGenerator(c=1, x=list(range(256)), a = [0xbd4e5d1a2d5969e5, 1] + ([0] * 254), b=2**64)
#gen.check_constants()
run_generator(genlarge)
"""

"""
print("----- 64-bit Mother-of-All -----")
gen64 = RwcGenerator(c=12345, x=[1234, 5678, 8765, 4321], a = [18000690696906969069, 6906969069, 11471147, 9005090050], b=2**64)
gen64.check_constants()
run_generator(gen64)

print("----- 32-bit Mother-of-All -----")
gen = RwcGenerator()
gen.check_constants()
run_generator(gen)

print("----- 32-bit RWC for Excel -----")
gen48 = RwcGenerator(c=12345, x=[12345, 87654321, 12345678], a = [29386, 29386, 0], b=2**32)
gen48.check_constants()
run_generator(gen48)

print("----- 64-bit large RWC -----")
genlarge = RwcGenerator(c=1, x=list(range(64)), a = [0x6b2b8accbeb42f68, 1] + ([0] * 62), b=2**64)
gen.check_constants()
run_generator(genlarge)

"""