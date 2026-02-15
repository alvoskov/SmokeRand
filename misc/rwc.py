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
            txt += f"0x{hex(xi)} "
        txt += f"\n  lcg={self.lcg:X} lcg_calc={lcg_calc:X}"
        return txt


gen64 = RwcGenerator(c=12345, x=[1234, 5678, 8765, 4321], a = [18000690696906969069, 6906969069, 11471147, 9005090050], b=2**64)
gen64.check_constants()

for i in range(1_000_000):
    gen64.next()
gen64.next(); print(gen64)
gen64.next(); print(gen64)
gen64.next(); print(gen64)
gen64.next(); print(gen64)


gen = RwcGenerator()
gen.check_constants()

for i in range(1_000_000):
    gen.next()
gen.next(); print(gen)
gen.next(); print(gen)
gen.next(); print(gen)
gen.next(); print(gen)



"""
    sum_lo  = unsigned_mul128(18000690696906969069U, obj->x[0], &sum_hi); // x(n-4)
    prod_lo = unsigned_mul128(6906969069U,           obj->x[1], &prod_hi); // x(n-3)
    unsigned_add128(&sum_hi, &sum_lo, prod_lo); sum_hi += prod_hi;
    prod_lo = unsigned_mul128(11471147U,             obj->x[2], &prod_hi); // x(n-2)
    unsigned_add128(&sum_hi, &sum_lo, prod_lo); sum_hi += prod_hi;
    prod_lo = unsigned_mul128(9005090050U,           obj->x[3], &prod_hi); // x(n-1)
    unsigned_add128(&sum_hi, &sum_lo, prod_lo); sum_hi += prod_hi;    
"""