import sympy, sys, math

def check_generator(a, m):
    print(m)
    m_isprime = sympy.isprime(m)
    print(f"Is prime: {m_isprime}")
    o = sympy.n_order(a, m)
    is_fullperiod = (m - 1) == o
    print(f"Is full period: {is_fullperiod}")
    print(f"Period: 2^{math.log2(o)}")
    assert(m_isprime)
    assert(is_fullperiod)

sys.set_int_max_str_digits(5000)

#Iter 2748416
a = 3649336883
m = a * (2**8192 + 2) - 1
# m - 1 = [2^2] * [.....]
check_generator(a, m)

# Iter 1826816
a = 17183301495294525414
m = a * (2**16384 + 2) - 1
# m - 1 = [2^1] [5^2] * [.....]
check_generator(a, m)
