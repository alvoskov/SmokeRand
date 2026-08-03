import sympy
n_ary = [32, 64, 128, 256]

def int_to_u64_ary(x):
    txt = "{"
    while x != 0:
        txt += f"0x{x % 2**64:016X},"
        x >>= 64
    txt += "}"
    return txt

for n in n_ary:
    period = 2**n - 1
    factors = sympy.primefactors(period)
    print(f"// 2**{n} - 1 = ", factors)
    m = [period // f for f in factors]
    for mi in m:
        print(int_to_u64_ary(mi) + f" // 0x{mi:X}")
