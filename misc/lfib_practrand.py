import numpy as np
import matplotlib.pyplot as plt
from sklearn.linear_model import LinearRegression


def nbytes_to_text(nbytes):
    mib, gib, tib, pib = 2**20, 2**30, 2**40, 2**50
    if nbytes < gib:
        return "%.2f MiB" % (nbytes / mib)
    elif nbytes < tib:
        return "%.2f GiB" % (nbytes / gib)
    elif nbytes < pib:
        return "%.2f TiB" % (nbytes / tib)
    else:
        return "%.2f PiB" % (nbytes / pib)


mib, gib, tib = 2**20, 2**30, 2**40
nbytes  = np.array([[32*mib, 2*gib, 512*mib, 8*gib, 32*gib, 256*gib, 1*tib, 8*tib, 16*tib, 128*tib]]).transpose()
gensize = np.array([[31,     55,    127,     258,   378,    607,     1279,  2281,  3217, 4423]]).transpose()

regr = LinearRegression()
regr.fit(gensize, nbytes**(1/3))
c = regr.coef_[0]
#print(gensize)
#print(nbytes)
print(c)

gensize_model = np.linspace(10, 4000, 1000)
nbytes_model = (c * gensize_model) ** 3


gensize_ext = np.array([31, 55, 127, 258, 378, 607, 1279,  2281,  3217, 4423, 9689, 19937, 44497, 110503])
nbytes_ext = (c * gensize_ext) ** 3;
for i in range(0, len(gensize_ext)):
    print(gensize_ext[i], nbytes_to_text(nbytes_ext[i]))


plt.loglog(gensize, nbytes, 'bo')
plt.loglog(gensize_model, nbytes_model, 'r-')
#plt.plot(gensize, nbytes**(1/3), 'bo')
#plt.plot(gensize_model, nbytes_model**(1/3), 'r-')
plt.show()

