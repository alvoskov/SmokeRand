import math
class MwcPf:
    def __init__(self, a=13115896780146644418, b=2**512 + 2, c=0, x=2**512):
        self.a = a
        self.b = b
        self.c = c
        self.x = x

    def __str__(self):
        txt = ""
        if self.a < 2**32:
            for j in range(round(math.log2(self.b) // 32 + 1)):
                txt += f"0x{(self.x >> (j * 32)) % 2**32:08X} "
            txt += f"; c=0x{self.c:08X}"
        else:
            for j in range(round(math.log2(self.b) // 64 + 1)):
                txt += f"0x{(self.x >> (j * 64)) % 2**64:016X} "
            txt += f"; c=0x{self.c:016X}"
        return txt

    def next(self):
        u = self.a * self.x + self.c
        self.x = u % self.b
        self.c = u // self.b

    def prev(self):
        m = self.a * self.b - 1
        u = (self.b * (self.c*self.b + self.x)) % m
        self.x = u % self.b
        self.c = u // self.b


def generate_test_vectors(gen):
    print("----- Initial state (before zeroland) -----")
    gen.prev()
    print(gen)
    print("----- Zeroland -----")
    gen.next()
    print(gen)
    print("----- Test vectors -----")
    n = 9_999
    for i in range(n + 4):
        gen.next()
        if i >= n:
            print(gen)

generate_test_vectors(MwcPf())
generate_test_vectors(MwcPf(a=3906776790, b=2**256 + 2, c=0, x=2**256))
