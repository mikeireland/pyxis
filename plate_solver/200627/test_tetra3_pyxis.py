"""
This example loads the tetra3 default database and solves for every image in the tetra3/test_data directory.

Note: Requires PIL (pip install Pillow)
"""
import sys
import os
sys.path.append('../../..')
from tetra3 import Tetra3
from PIL import Image
from astropy.io import fits
from glob import glob
import matplotlib.pyplot as plt

# Create instance and load default_database (built with max_fov=12 and the rest as default)
t3 = Tetra3('tetra_db_fov6_mag9.npz')

#Take image
#ac.main()

files = glob('*.fits') # * means all if need specific format then *.csv
impath = max(files, key=os.path.getctime)

print('Solving for image at: ' + str(impath))

hdul = fits.open(str(impath))

img = hdul[0].data
#plt.imshow(img[0].data)

solved = t3.solve_from_image(img,bg_sub_mode="local_mean",sigma_mode="global_root_square",filtsize=21)  # Adding e.g. fov_estimate=11.4, fov_max_error=.1 improves performance
print('Solution: ' + str(solved))
