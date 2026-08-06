import sympy
n_ary = [512, 1024, 320, 32, 64, 96, 128, 160, 192, 256]

def int_to_u64_ary(x):
    txt = "{"
    while x != 0:
        txt += f"0x{x % 2**64:016X}, "
        x >>= 64
    txt += "}"
    return txt

def test_factors(factors, period):
    fprod = 1
    for f in factors:
        fprod *= f
    print(fprod == period)
    assert fprod == period


for n in n_ary:
    period = 2**n - 1
    if n == 512:
        # https://www.alpertron.com.ar/ECM.HTM
        factors = [3, 5, 17, 257, 641, 65537, 274177, 6700417, 67280421310721, 1238926361552897, 59649589127497217, 5704689200685129054721, 93461639715357977769163558199606896584051237541638188580280321]
        test_factors(factors, period)
    elif n == 1024:
        factors = [3, 5, 17, 257, 641, 65537, 274177, 2424833, 6700417, 67280421310721, 1238926361552897, 59649589127497217, 5704689200685129054721, 7455602825647884208337395736200454918783366342657, 93461639715357977769163558199606896584051237541638188580280321, 741640062627530801524787141901937474059940781097519023905821316144415759504705008092818711693940737]
        test_factors(factors, period)
    else:
        factors = sympy.primefactors(period)
    print(f"// 2**{n} - 1 = ", factors)
    m = [period // f for f in factors]
    for mi in m:
        print("    {" + int_to_u64_ary(mi) + f"}}, // 0x{mi:X}")
