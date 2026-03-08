import lfsr_engine as lfsr
gen32 = lfsr.XorGenMaker(32)
gen64 = lfsr.XorGenMaker(64)

print("----- xorshift16 -----")
T = lfsr.XorGenMaker(16).make_xorshift_matrix(7, 9, 8)
print(lfsr.is_full_period(T))

print("----- xorshift32 -----")
T = gen32.make_xorshift_matrix(13, 17, 5)
print(lfsr.is_full_period(T, False))

print("----- xorshift64 -----")
T = gen64.make_xorshift_matrix(13, 17, 43)
print(lfsr.is_full_period(T, False))

print("----- xorrot32 -----")
T = gen32.make_xorrot_matrix(1, 9, 27)
print(lfsr.is_full_period(T, False))

print("----- xorrot64 -----")
T = gen64.make_xorrot_matrix(5, 13, 47)
print(lfsr.is_full_period(T, False))

