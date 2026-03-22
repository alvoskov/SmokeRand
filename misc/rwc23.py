import sympy

class Rwc23Generator:
    def __init__(self, c=None, x=None, y=None, z=None, a2=1111111464, a3=1111111464, b=2**32):
        self.c, self.x, self.y, self.z = c, x, y, z
        self.a2, self.a3, self.b = a2, a3, b
        self.m = a3*b**3 + a2*b**2 - 1
        self.lcg, self.a_lcg = self.to_lcg(), pow(b, -1, self.m)

    def to_lcg(self):
        b = self.b
        return b**3 * self.c + b**2*(self.x - self.a2*self.z) + b*self.y + self.z

    def check_constants(self):
        print(sympy.isprime(self.m), sympy.n_order(self.b, self.m) / self.m)

    def next(self):
        mul, b = self.a3*self.z + self.a2*self.y + self.c, self.b
        self.z, self.y = self.y, self.x
        self.c, self.x = mul // b, mul % b
        self.lcg = (self.a_lcg * self.lcg) % self.m
        return self.x

    def __str__(self):
        lcg_calc = self.to_lcg()
        txt = f"c=0x{self.c:X} x=0x{self.x:X}, y=0x{self.y:X}, z=0x{self.z:X}, \n"
        txt += f"  lcg={self.lcg:X} lcg_calc={lcg_calc:X}"
        return txt

print("----- 64-bit version -----")
gen64 = Rwc23Generator(1, 12345678, 87654321, 12345,
    a2=12345671234567586, a3=12345671234567586, b=2**64)
#gen64.check_constants() # May be slow
for i in range(10_000_000):
    gen64.next()
gen64.next(); print(gen64)
gen64.next(); print(gen64)
gen64.next(); print(gen64)

print("----- 32-bit version -----")
gen = Rwc23Generator(1, 12345678, 87654321, 12345)
gen.check_constants()
for i in range(10_000_000):
    gen.next()
gen.next(); print(gen)
gen.next(); print(gen)
gen.next(); print(gen)
