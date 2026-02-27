import lfsr_engine as lfsr
import sympy, unittest, galois
import numpy as np

gen64 = lfsr.XorGenMaker(64)
gen32 = lfsr.XorGenMaker(32)
gen16 = lfsr.XorGenMaker(16)

def words_to_bits(words, wordsize=64):
    bits = np.zeros((1, len(words) * wordsize), dtype=np.int32)
    for i in range(len(words)):
        w = words[i]
        for j in range(wordsize):
            bits[0][i*wordsize + (wordsize - 1 - j)] = (w >> j) & 0x1
    return galois.GF(2)(bits)

def bits_to_words(bits, wordsize=64):
    nwords = bits.size // wordsize
    if nwords * wordsize > bits.size:
        nwords += 1
    words = [0] * nwords
    for i in range(bits.size):
        w_ind, w_pos = i // wordsize, wordsize - 1 - i % wordsize
        if bits[0][i] == 1:
            words[w_ind] |= (1 << w_pos)
    return words

def gen_test_vectors(T, Tvec, wordsize=None, period=None, niters = 10_000_000):
    bits = words_to_bits(Tvec, wordsize=wordsize)
    print("Is period full: ", lfsr.is_full_period(T, False))
    Tgf = galois.GF(2)(T)
    Tout = np.linalg.matrix_power(Tgf, period + 1)
    print("Is GFpow ok: ", (Tout == T).all())
    A = np.linalg.matrix_power(Tgf, niters)
    print(list(map(lambda x: hex(x), bits_to_words(bits @ A, wordsize=wordsize))))

def genvec_xorrot256():
    print("----- xorrot256 -----")
    Tvec = [0x123456789ABCDEF, 0xFEDCBA987654321, 0xDEADBEEF, 0xBADF00D]
    bits = words_to_bits(Tvec)
    print(bits)
    print(bits.size)
    words = bits_to_words(bits)
    print(Tvec)
    print(words)
    T = gen64.make_xorrot4w_matrix(3, 8, 37)
    print(lfsr.is_full_period(T, False))

    Tgf = galois.GF(2)(T)
    Tout = np.linalg.matrix_power(Tgf, 2**256)
    print("Is GFpow ok: ", (Tout == T).all())
    A = np.linalg.matrix_power(Tgf, 10_000_000)
    print(list(map(lambda x: hex(x), bits_to_words(bits @ A))))

def genvec_xorrot128():
    print("----- xorrot128 -----")
    T = gen64.make_xorrot2w_matrix(3, 17, 52)
    Tvec = [0x123456789ABCDEF, 0xFEDCBA987654321]
    gen_test_vectors(T, Tvec, wordsize=64, period=2**128-1)

def genvec_xorrot64w32():
    print("----- xorrot64w32 -----")
    Tvec = [0x12345678, 0xFEDCBA98]
    T = gen32.make_xorrot2w_matrix(5, 12, 25)
    gen_test_vectors(T, Tvec, wordsize=32, period=2**64-1)

def genvec_xorrot64w16():
    print("----- xorrot64w16 -----")
    Tvec = [0x1234, 0xFEDC, 0xDEAD, 0xBEEF]
    T = gen16.make_xorrot4w_matrix(3, 7, 13)
    gen_test_vectors(T, Tvec, wordsize=16, period=2**64-1)

def genvec_xorrot128w32():
    print("----- xorrot128w32 -----")
    T = gen32.make_xorrot4w_matrix(9, 4, 17)
    Tvec = [0x12345678, 0xFEDCBA98, 0xDEADBEEF, 0xBADF00D]
    gen_test_vectors(T, Tvec, wordsize=32, period=2**128-1)

def genvec_xorrot32():
    print("----- xorrot32 -----")
    T = gen32.make_xorrot_matrix(1, 9, 27)
    gen_test_vectors(T, [0x12345678], wordsize=32, period=2**32-1)

genvec_xorrot256()
genvec_xorrot128()
genvec_xorrot64w32()
genvec_xorrot64w16()
genvec_xorrot128w32()
genvec_xorrot32()
