a, b, r = 61578, 2**16, 1
m = a*b**r - 1
print(m)

state = {'x': 1, 'c': 0}
for i in range(0, 10):
    mul = a*state['x'] + state['c']
    state['x'] = mul % b
    state['c'] = mul // b
    print(hex(state['c']), hex(state['x']))

x = 1
for i in range(0, 10):
    x = a*x % m
    print(hex(x))

#------------------------------------
#a=58764 (0xe58c) True
#  a_lcg =  0xe58c000000000000 0xe58c 0xe58bffffffffffffffff

a, b, r = 59814, 2**16, 4
m = a*b**r - 1
a_lcg = pow(b, -1, m)

state = {'c' : 0, 'x': [0x5000, 0, 0x1234, 0x5678]}
for i in range(0, 32):
    mul = a*state['x'][r - 1] + state['c']
    state['x'].insert(0, mul % b)
    state['x'].pop()
    state['c'] = mul // b
    txt = "{ind:2d} | {c:4x} | ".format(ind = i, c = state['c'])
    for xi in state['x']:
        txt = txt + "{x:4x} ".format(x = xi)
    print(txt)


x = 0x5000_0000_1234_5678
for i in range(0, 32):
    x = (a_lcg * x) % m
    print(hex(x))

print(hex(m), hex(a_lcg))


#---------------------------------------------------------

print("----- 64-bit generator -----")
x = 1
a, b, r = 0xf21f494c589bf715, 2**64, 2

m = a*b**r - 1
a_lcg = pow(b, -1, m)
for i in range(0, 32):
    x = (a_lcg * x) % m
    print(hex(x))
