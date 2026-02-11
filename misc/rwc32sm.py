import sympy

class RwcGenerator:
    def __init__(self, c=None, x=None, y=None, a=[1111111464, 1111111464], b=2**32):
        self.c, self.x, self.y = c, x, y
        self.a, self.b = a, b
        self.m = a[1]*b**2 + a[0]*b - 1
        self.lcg, self.a_lcg = self.to_lcg(), pow(b, -1, self.m)

    def to_lcg(self):
        b = self.b
        return b**2 * self.c + self.y + b*(self.x - self.a[0]*self.y)

    def check_constants(self):
        print(sympy.isprime(self.m), sympy.n_order(self.b, self.m))

    def next(self):
        mul, b = self.a[1]*self.y + self.a[0]*self.x + self.c, self.b
        self.y, self.c, self.x = self.x, mul // b, mul % b
        self.lcg = (self.a_lcg * self.lcg) % self.m
        return self.x

gen = RwcGenerator(1, 12345678, 87654321)
for i in range(10_000_000):
    gen.next();
gen.next();
print(gen.c, gen.x, gen.y, hex(gen.lcg), hex(gen.to_lcg()), hex(gen.x))
gen.next();
print(gen.c, gen.x, gen.y, hex(gen.lcg), hex(gen.to_lcg()), hex(gen.x))
