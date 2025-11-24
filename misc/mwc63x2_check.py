import sympy, math

def calc_mwc_mod(a, b):
    return a * b - 1

def check_mwc_mult(a, b):
    m = calc_mwc_mod(a, b)
    return sympy.isprime(m) and sympy.isprime((m - 1) // 2)

a0, a1, b = 1073100393, 1073735529, 2**32

print("a0 is ok" if check_mwc_mult(a0, b) else "a0 is bad")
print("a1 is ok" if check_mwc_mult(a1, b) else "a1 is bad")
period0 = sympy.n_order(b, calc_mwc_mod(a0, b))
period1 = sympy.n_order(b, calc_mwc_mod(a1, b))
print("Period for mwc0: {0} (2^{1:.3f})".format(period0, math.log2(period0)))
print("Period for mwc1: {0} (2^{1:.3f})".format(period1, math.log2(period1)))

if math.gcd(period0, period1):
    print("Combined generator period: {0} (2^{1:.3f})".format(
        period0 * period1, math.log2(period0) + math.log2(period1)))
