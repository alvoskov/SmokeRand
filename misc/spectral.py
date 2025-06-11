# https://github.com/lpareja99/spectral-test-knuth/tree/main
from math import floor, sqrt, pi, factorial, gamma
import operator

def dot(list1, list2):
    """return usual inner product of two lists"""
    return sum(map(operator.mul, list1, list2))

def add(list1, list2):
    """element-wise sum two lists"""
    return list(map(operator.add, list1, list2))

def scalarmult(scalar, list1):
    """scalar multiply a list"""
    return [scalar * elt for elt in list1]

def sub(list1, list2):
    """return element-wise difference of two lists"""
    return add(list1, scalarmult(-1, list2))

def knuth(T,a,m):
    """Return nu_T

    This implenents Knuth's alogrithm beginning on page 102 of his
    book:
         The Art of Computer Programming, 3rd Ed., Vol. 2:
         Seminumerical Algorithms, 1998
    """

    # S1) initialize the values
    t = 2
    h = a
    h_prime = m
    p = 1
    p_prime = 0
    r = a
    s = 1 + a**2

    # S2) Euclidean step
    q = h_prime // h
    u = h_prime - q*h
    v = p_prime - q*p

    while u**2 + v**2 < s:
        s = u**2 + v**2
        h_prime = h
        h = u
        p_prime = p
        p = v
        q = h_prime // h
        u = h_prime - q*h
        v = p_prime - q*p

    # S3)a - compute v2
    u = u - h
    v = v - p
    if u**2 + v**2 < s:
        s = u**2 + v**2

    v2 = sqrt(s)

    if  T == 2:
        return v2

    # S3)b - set up U and V matrices
    U = [[-h, p], [-h_prime, p_prime]]
    V = [[p_prime,h_prime], [-p,-h]] if p_prime <= 0 else [[-p_prime,-h_prime], [p,h]]

    while t < T:
        # S4) advance step
        t = t+1
        r = (a*r) % m

        # add column and row
        for row in U:
            row.append(0)
        U.append([-r] + (t-2)*[0] + [1])

        for row in V:
            row.append(0)
        V.append((t-1)*[0] + [m])

        for i in range(0,t-1):
            q = V[i][0] * r // m #round(V[i][0]*r/m)
            V[i][t-1] = V[i][0]*r - q*m
            U[t-1] = add(U[t-1], scalarmult(q, U[i]))

        s = min(s, dot(U[t-1], U[t-1]))
        k = t-1
        j = 0

        # S5) Transform
        while j != k: # go until j and k match (S6)
            for i in range(t):
                if i != j and abs(2*dot(V[i], V[j])) > dot(V[j], V[j]):
                    q = round(dot(V[i], V[j]) / dot(V[j], V[j]))
                    V[i] = sub(V[i], scalarmult(q, V[j]))
                    U[j] = add(U[j], scalarmult(q, U[i]))
                    s = min(s, dot(U[j], U[j]))
                    k = j

            # S6) Advance j
            j = 0 if j == t - 1 else j + 1

        # S7) Prepare for search
        X = [0]*t
        Y = [0]*t
        k = t-1
        Z = []
        for j in range(t):
            Z.append(floor(sqrt(dot(V[j], scalarmult((s/(m**2)), V[j])))))

        # S8) advance Xk
        while X[k] != Z[k] and k >= 0:
            X[k] += 1
            Y += U[k]

            # S9) Advance k
            k = k+1
            while (k <= t-1): # < or <=
                X[k] = -Z[k]
                Y -= scalarmult(2*Z[k], U[k])
                k = k+1

            s = min(s, dot(Y,Y))

            # S10) Decrease k
            k = k-1

    return sqrt(s)


def v_to_mu(v, t, m):
    return pi**(t/2.0) * v**t / gamma(t / 2 + 1.0) / m

def main():
#    a = [ 23, 129, 262145, 3141592653, 137, 3141592621, 6364136223846793005, 18000_69069_69069_69069,
#        0xffa04e67b3c95d86000000000000000000000000]
#    m = [ 100000001, 2**35, 2**35, 2**35, 256, 10**10,  2**64,               2**128,        
#         0xffa04e67b3c95d85ffffffffffffffffffffffffffffffff]



    # v_t: 1.8446744073709552e+19 1.744674407370955e+19 2676411032.267 2676411032.267 2676411032.267 32998703.18 9922035.902
    # mu_t: 0.0 3.747 0.0 0.0 0.32 0.0 0.064 
    a = [0xf21f494c589bf7150000000000000000]
    m = [0xf21f494c589bf714ffffffffffffffffffffffffffffffff]

    # v_t: 1.8446744073709552e+19 1.344674407370955e+19 4087043909.38 4087043909.38 3747368126.36 40003861.485 8810694.971
    # mu_t: 0.0 2.226 0.0 0.0 3.128 0.0 0.032
    #a = [0xba9c6e7dbb0bf94a0000000000000000]    
    #m = [0xba9c6e7dbb0bf949ffffffffffffffffffffffffffffffff]

    #v_t: 1.8446744073709552e+19 1.8436744073709541e+19 4916603133.166 4916603133.166 4916603133.166 51737149.474 12352338.488
    #mu_t: 0.0 4.184 0.0 0.0 11.635 0.001 0.351
    #a = [0xffdc790d903ed8610000000000000000]
    #m = [0xffdc790d903ed860ffffffffffffffffffffffffffffffff]


    #v_t: 1.8446744073709552e+19 1.8445744073709502e+19 1237359980.491 1237359980.491 1237359980.491 37331716.731 11630233.172
    #mu_t: 0.0 4.188 0.0 0.0 0.003 0.0 0.216
    #a = [0xfffc72815b38bcc70000000000000000]
    #m = [0xfffc72815b38bcc6ffffffffffffffffffffffffffffffff]

    #m = [0xeb520b48cdfcc441ffffffffffffffffffffffffffffffff]    
    #a = [0xeb520b48cdfcc4420000000000000000]

    # A really awful multiplier
    # mu = 1.84e19, 1569, 1569, 1569, ...
    #a = [0x6210000000000000000]
    #m = [0x620ffffffffffffffffffffffffffffffff]


    #a.append(pow(a[0], 2, m[0]));
    #m.append(m[0])
    #a.append(pow(a[0], 4, m[0]))
    #m.append(m[0])
    #print(a)

    #b = 2**24
    #m = b**24 - b**10 + 1
    #a = m - (m - 1) // b
    #a = pow(a, 46, m)

    #a = [a]
    #m = [m]
    #print(a)
    #print(m)

    m = [2**64]
    a = [pow(18000_69069_69069_69069, 2**40, 2**128)]
    print(hex(a[0]))

    m = [2**16]
    a = [62517]
    print(a[0] % 4, hex(a[0]))


    for i in range (len(a)):
        t = range(2, 9)
        v_t = list(map(lambda t: knuth(t,a[i],m[i]), t))
        print("       (v_t)^2:",end=" ")
        print(*map(lambda t: t**2, v_t))
        print("           v_t:",end=" ")
        print(*map(lambda t: round(t, 3), v_t))
        print("           mu_t:",end=" ")
        print(*map(lambda t: round(v_to_mu(t[1], t[0], m[i]), 3), zip(t, v_t)))
        print()
#        print("        mu_t:",end=" ")


main()