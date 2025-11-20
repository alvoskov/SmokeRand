import numpy as np
import matplotlib.pyplot as plt


state, npoints = 12345, 500
a, b, m, sh, decstep = 69069, 12345, 2**32, 0, 2**0#9
#a, b, m, sh, decstep = 6906969069, 12345, 2**64, 32, 2**20
x = np.zeros(npoints)
y = np.zeros(npoints)
z = np.zeros(npoints)
for i in range(0, npoints):
    for j in range(0, decstep):
        state = (a * state + b) % m
    x[i] = (state >> sh) / (2**32)
    for j in range(0, decstep):
        state = (a * state + b) % m
    y[i] = (state >> sh) / (2**32)
    for j in range(0, decstep):
        state = (a * state + b) % m
    z[i] = (state >> sh) / (2**32)
    
print(x, y)
    

fig = plt.figure()
ax = fig.add_subplot(projection='3d')

ax.scatter(x, y, z, 'bo')
ax.set_xlabel('x')
ax.set_ylabel('y')
ax.set_zlabel('z')
plt.show()

