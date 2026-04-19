import sympy, sys
sys.set_int_max_str_digits(5000)
a = 17183301495294525414
m = a * (2**16384 + 2) - 1
# m - 1 = [2^1] [5^2] * [.....]
print(m)
print(sympy.isprime(m))
o = sympy.n_order(a, m)
print((m - 1) - o)
