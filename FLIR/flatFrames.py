import numpy as np
from astropy.io import fits
import matplotlib.pyplot as plt

#fits_image_filename = fits.util.get_testdata_filepath("data/biasFrames.fits")
fits_ls = [1000,2000,3000,4000,5000]

hdul = fits.open("data/biasFrames.fits", memmap=True)
bias = np.mean(hdul[0].data/64,axis=0)

var_ls = []
sig_ls = []
for name in fits_ls:
    hdul = fits.open("data/flatFrames%s.fits"%name, memmap=True)
    data = hdul[0].data/64 - bias
    med_all = np.median(data)

    n = len(data)

    for i in range(n):
        data[i] = data[i]*med_all/np.median(data[i])


    stdls = []
    meanls = []
    for i in range(n):
        for j in range(0,i):
            print(i,j)
            a = data[i] - data[j]
            std = np.std(a)/np.sqrt(2)
            mean = np.mean(a)

            stdls.append(std)
            meanls.append(mean)

    var_ls.append(np.mean(stdls)**2)
    sig_ls.append(np.mean(data))

    print("Mean = " + str(np.mean(meanls)))
    print("Std of one image = " + str(np.mean(stdls)))

plt.plot(sig_ls,var_ls)





