AWC_MASK = 2**55 - 1
AWC_SH = 55
WEYL_INC = 0x9E3779B97F4A7C15


weyl = 12345678
xsh  = 87654321
awc_x0, awc_x1, awc_c = 3, 2, 1

u = 0
for i in range(0, 1000000):
    # xorshift64 part
    xsh = (xsh ^ (xsh << 13)) % 2**64
    xsh = (xsh ^ (xsh >> 17)) % 2**64
    xsh = (xsh ^ (xsh << 43)) % 2**64
    # AWC (add with carry) part
    t = awc_x0 + awc_x1 + awc_c;
    awc_x1 = awc_x0
    awc_c  = t >> AWC_SH
    awc_x0 = t & AWC_MASK
    # Discrete Weyl sequence part
    weyl = weyl + WEYL_INC
    # Combined output
    u = (((awc_x0 << 9) ^ awc_x1) + xsh + weyl) % 2**64
    

print(hex(u))
