# a0, a1 = 4005, 3939
a0, a1 = 1073100393, 1073735529
b = 2**32
m0, m1 = a0 * b - 1, a1 * b - 1
mwc0, mwc1 = 0x123DEADBEEF, 0x456CAFEBABE
# MWC iterations
for i in range(0, 1_000_000 - 1):
    mwc0 = (a0 * mwc0) % m0
    mwc1 = (a1 * mwc1) % m1
    out = ((mwc0 % b) + (mwc1 % b) + (mwc0 >> 32) + (mwc1 >> 32)) % b
    #print("{0:X}, {1:X}, {2:X}".format(out, mwc0, mwc1))
    #print("{0}, {1}, {2}".format(out, mwc0, mwc1))

# Output function
out = ((mwc0 % b) + (mwc1 % b) + (mwc0 >> 32) + (mwc1 >> 32)) % b
print(hex(out), out)
