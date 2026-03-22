# https://www.johndcook.com/blog/2023/01/03/pratt-certificate/
import sympy, random

def pratt_cert(x, level = 1):
    if x < 2**64:        
        return sympy.isprime(x)
    elif not sympy.isprime(x):
        return False
    else:
        mul_all = sympy.primefactors(x - 1)
        print("  " * level, mul_all)
        mul = [p for p in mul_all if p > 2**64]

        is_ok = False
        while not is_ok:
            a = random.randint(2, x - 2)
            print("  " * level, "a = ", a)
            if pow(a, x - 1, x) == 1:
                is_ok = True
                for p in mul_all:
                    f = pow(10, (x - 1) // p, x) != 1
                    is_ok = is_ok and f
                    print("  " * level, p, ": +" if f else ": -")

        if len(mul) == 0:
            return True
        else:
            return list(map(lambda p : pratt_cert(p, level + 1), mul))
        


b = 2**64
a = b - 10**16 - 273*256
m = a*b - 1

print(pratt_cert(m))