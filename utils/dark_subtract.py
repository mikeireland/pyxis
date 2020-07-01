"""
After reading in .raw files, dark subtract and save to fits file. 

Notes on data taken on 26 June, airmass ~4 (~15 degrees above horizon).

Using astrometry.net, parity was *negative*.

HD9163, a G=7 star, around pixel (y,x)=(630,700) is detected at SNR>15, i.e. at least 
70 counts. Theoretically, this should have about 40% efficiency, with the sky also
at around 50% efficiency, i.e. 20% overall, and a 300nm bandpass. This gives 3000 
electrons expected. 

Std of 1.9 ADU in 8 bit mode, where the sky background was 22 ADU above the 
background. 

In 16 bit mode (10 bit readout), around pixel 708, 435, HD9162 (G=8) was detected 
with about 10,000 counts where the readout noise was 220. Assuming 30 ADU/electron,
this is 300 electrons instead of 1200 electrons. 

Similar (but slightly improved) for HD 9266: G=9 should have ~470 electrons, and 
when adding up all flux (i.e. including wings) has ~6000 counts or ~200 electrons. 
So certainly within a factor of ~2.5 of expectations. 

Actions:
- Characterise detector gain and readout noise. Above was based on 30 ADU/electron
in 10 bit mode (where quantization is 64).
- Acquire dark frames with a better background level
- Ensure that data are collected with no gamma correction.
- See if astrometry.net processing time can be <1s. If not, we need our own star 
tracker software based on triangles and not quadrilaterals. 
"""

import astropy.io.fits as pyfits
import numpy as np
import glob
import scipy.ndimage as nd

nx=1440
ny=1080
bytes_pix = 1
file = '/Users/mireland/pyxis/data/200627/stars3.raw'
outname = 'stars3.fits'
dfiles = glob.glob('/Users/mireland/pyxis/data/200627/dark?.raw')

bytes_pix = 2
file = '/Users/mireland/pyxis/data/200627/stars16_1.raw'
outname = 'stars16_1.fits'
dfiles = glob.glob('/Users/mireland/pyxis/data/200627/dark16_?.raw')


#-------------

with open(file, 'rb') as ff:
    ss = ff.read(nx*ny*bytes_pix)
    ff.close()
    if bytes_pix==1:
        im = np.frombuffer(ss, dtype=np.uint8).reshape((ny,nx))
    else:
        im = np.frombuffer(ss, dtype=np.uint16).reshape((ny,nx))
    
dmn = np.zeros((ny,nx))
for dfn in dfiles:
    with open(dfn, 'rb') as ff:
        ss = ff.read(nx*ny*bytes_pix)
        ff.close()
        if bytes_pix==1:
            dmn += np.frombuffer(ss, dtype=np.uint8).reshape((ny,nx))
        else:
            dmn += np.frombuffer(ss, dtype=np.uint16).reshape((ny,nx))
dmn /= len(dfiles)

im = (im-dmn.astype(np.int16)).astype(np.int16)

#Now some basic dark subtraction.
imsub = im-nd.median_filter(im,7)
imsub = np.maximum(imsub,-10)

pyfits.writeto(outname, imsub, overwrite=True)

if bytes_pix==1:
    noise = np.std(np.minimum(imsub,20))
else:
    noise = np.std(np.minimum(imsub,1000))
print("Readout noise: {:.2f} ADU".format(noise))