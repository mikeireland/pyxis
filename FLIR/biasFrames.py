import numpy as np
from astropy.io import fits
import matplotlib.pyplot as plt

#fits_image_filename = fits.util.get_testdata_filepath("data/biasFrames.fits")

hdul = fits.open("data/flats_gain_15/biasFrames.fits", memmap=True)
data = hdul[0].data/64

n = len(data)

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

print("Mean = " + str(np.mean(meanls)))
print("Std of one image = " + str(np.mean(stdls)))





