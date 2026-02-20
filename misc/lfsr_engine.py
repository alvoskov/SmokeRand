from sympy.polys.matrices import DomainMatrix
import galois, sympy, unittest
import numpy as np
from sympy import GF, Poly
from sympy.abc import x


def gfpow(a, n):
    if n == 1:
        return a.copy()
    else:    
        p = a @ a
        for i in range(n - 2):
            p = p @ a
        return p


def gf2mat_to_list(T):
    return list(map(lambda x: list(map(lambda v: int(v), list(x))), list(T)))


def make_eye_matrix(n):
    I = [[0] * n for _ in range(n)]
    for i in range(n):
        I[i][i] = 1
    return galois.GF(2)(I)

def make_zero_matrix(n):
    I = [[0] * n for _ in range(n)]
    return galois.GF(2)(I)

def make_shr_matrix(n):
    R = [[0] * n for _ in range(n)]
    for i in range(n - 1):
        R[i][i + 1] = 1
    return galois.GF(2)(R)

def make_shl_matrix(n):
    L = [[0] * n for _ in range(n)]
    for i in range(n - 1):
        L[i + 1][i] = 1
    return galois.GF(2)(L)

def make_rol_matrix(n):
    ROL = [[0] * n for _ in range(n)]
    for i in range(n - 1):
        ROL[i + 1][i] = 1
    ROL[0][n - 1] = 1
    return galois.GF(2)(ROL)

def make_ror_matrix(n):
    ROR = [[0] * n for _ in range(n)]
    for i in range(n - 1):
        ROR[i][i + 1] = 1
    ROR[n - 1][0] = 1
    return galois.GF(2)(ROR)


class XorGenMaker:
    def __init__(self, n):
        self.O = make_zero_matrix(n)
        self.I = make_eye_matrix(n)
        self.L = make_shl_matrix(n)
        self.R = make_shr_matrix(n)
        self.ROL = make_rol_matrix(n)

    def make_xorshift_matrix(self, a, b, c):
        #<<a >>b <<c
        I, R, L = self.I, self.R, self. L
        T = ( (I + gfpow(L, a)) @ (I + gfpow(R, b)) @ (I + gfpow(L, c)) )
        return gf2mat_to_list(T)


    def make_xorrot_matrix(self, a, b, c):
        #obj->x ^= obj->x << 1;
        #obj->x ^= rotl32(obj->x, 9) ^ rotl32(obj->x, 27);
        I, L, ROL = self.I, self.L, self.ROL
        T = (I + gfpow(L, a)) @ (I + gfpow(ROL, b) + gfpow(ROL, c))
        return gf2mat_to_list(T)


def is_full_period(T, verbose = True):
    K = GF(2)
    dM = DomainMatrix(T, (len(T), len(T)), K)
    p = dM.charpoly()
    coeffs = list(map(lambda v: int(v), p))
    poly = galois.Poly(coeffs, field=galois.GF(2))
    if verbose:
        for t in T:
            print(t)
        print(Poly(p, x, domain = K))
        print("Our coeffs list: ", coeffs)
        print(poly)
    return poly.is_primitive()


class TestLfsrs(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(TestLfsrs, self).__init__(*args, **kwargs)
        self.gen32 = XorGenMaker(32)
        self.gen64 = XorGenMaker(64)

    def test_xorshift32(self):
        T = self.gen32.make_xorshift_matrix(13, 17, 5)
        self.assertTrue(is_full_period(T, False))

    def test_xorshift32_bad(self):
        T = self.gen32.make_xorshift_matrix(17, 13, 5)
        self.assertFalse(is_full_period(T, False))

    def test_xorshift64(self):
        T = self.gen64.make_xorshift_matrix(13, 17, 43)
        self.assertTrue(is_full_period(T, False))

    def test_xorshift64_bad(self):
        T = self.gen64.make_xorshift_matrix(17, 13, 43)
        self.assertFalse(is_full_period(T, False))

    def test_xorrot32(self):
        T = self.gen32.make_xorrot_matrix(1, 9, 27)
        self.assertTrue(is_full_period(T, False))

    def test_xorrot64(self):
        T = self.gen64.make_xorrot_matrix(5, 13, 47)
        self.assertTrue(is_full_period(T, False))

    def test_xoroshiro128(self):
        """
        https://arxiv.org/pdf/1805.01407
        https://xoroshiro.di.unimi.it/xoroshiro128plus.c

        | I I | * | Ra      O  | = | I + Ra + Sb  Rc |
        | O I |   | I + Sb  Rc |   | I + Sb       Rc |
        
        """
        a, b, c = 24, 16, 37
        L, ROL, I = self.gen64.L, self.gen64.ROL, self.gen64.I
        A = I + gfpow(ROL, a) + gfpow(L, b)
        B = I + gfpow(L, b)
        Rc = gfpow(ROL, c)
        T = np.vstack((
            np.hstack((A, Rc)),
            np.hstack((B, Rc)),
        ))
        T = gf2mat_to_list(T)
        self.assertTrue(is_full_period(T, False))


    def test_xsadd(self):
        sh1, sh2, sh3 = 15, 18, 11
        O, L, R, I = self.gen32.O, self.gen32.L, self.gen32.R, self.gen32.I
        A = (I + gfpow(L, sh1)) @ (I + gfpow(R, sh2))
        B = gfpow(L, sh3)
        T = np.vstack((
            np.hstack((O, O, O, A)),
            np.hstack((I, O, O, O)),
            np.hstack((O, I, O, O)),
            np.hstack((O, O, I, B)),
        ))
        T = gf2mat_to_list(T)
        self.assertTrue(is_full_period(T, False))

    def test_poly(self):
        coeffs = [0] * 608
        coeffs[607] = 1; coeffs[273] = 1; coeffs[0] = 1
        coeffs = coeffs[::-1]
        poly = galois.Poly(coeffs, field=galois.GF(2))
        print(poly)
        self.assertTrue(poly.is_primitive())


if __name__ == '__main__':
    unittest.main()

