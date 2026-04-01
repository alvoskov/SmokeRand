print("^^^^^^^^^^^^^^")
c, x, b, a = 0, 2**512, 2**512 + 2, 13115896780146644418
c, x, b, a = 0, 2**256, 2**256 + 2, 3906776790
for i in range(10_000 + 8):
    u = a * x + c
    x, c = u % b, u // b
    if i >= 10_000:
        if x > 2**256:
            for j in range(9):
                print(f"0x{(x >> (j * 64)) % 2**64:016X}", end=' ')
        else:
            for j in range(9):
                print(f"0x{(x >> (j * 32)) % 2**32:08X}", end=' ')
        print("")
