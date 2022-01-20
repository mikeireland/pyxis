import numpy as np
from astropy.io import fits
import matplotlib.pyplot as plt

fits_ls = [1000,2000,3000,4000,5000,6000,7000,8000,9000,10000]#,11000,12000,13000,14000,15000]

hdul = fits.open("data/biasFrames.fits", memmap=True)
ADC = 10
factor = 2**(16-ADC) 
bias_data = hdul[0].data/factor

n = len(bias_data)

bias_std_ls = []
bias_mean_ls = []
for i in range(n):
    for j in range(0,i):
        a = bias_data[i] - bias_data[j]
        std = np.std(a)/np.sqrt(2)
        mean = np.mean(a)
        bias_std_ls.append(std)
        bias_mean_ls.append(mean)

bias = np.mean(bias_data,axis=0)

var_ls = []
flux_ls = []
for name in fits_ls:
    hdul = fits.open("data/flatFrames_%s.fits"%name, memmap=True)
    data = hdul[0].data/factor - bias
    med_all = np.median(data)

    n = len(data)

    for i in range(n):
        data[i] = data[i]*med_all/np.median(data[i])

    stdls = []
    for i in range(n):
        for j in range(0,i):
            a = data[i] - data[j]
            std = np.std(a)/np.sqrt(2)
            stdls.append(std)

    var_ls.append(np.mean(stdls)**2)
    flux_ls.append(np.mean(data))

    print("Std of one image = " + str(np.mean(stdls)))

plt.plot(flux_ls,var_ls)

m,c = np.polyfit(flux_ls[1:6],var_ls[1:6],1)
plt.plot(flux_ls, m*np.array(flux_ls) + c)
plt.xlabel("Signal (mean of frame)")
plt.ylabel("Variance")

print("\nMean Bias = " + str(np.mean(bias_mean_ls)))
print("Std of one bias image = " + str(np.mean(bias_std_ls)))
print("Flat plot data: Slope = %s, Intercept = %s" %(m,c))
print("Gain = " + str(1./m))
print("Read out noise from bias = " + str(np.mean(bias_std_ls)/m))
print("Read out noise from intercept = " + str(np.sqrt(c/m**2)))

plt.show()
