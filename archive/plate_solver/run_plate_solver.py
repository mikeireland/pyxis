"""
Plate solves a given file (from the command line), and returns an AltAz attitude quaternion

Uses Tetra3 and astrometry.net programs to solve
"""
import os, sys
import time
import conversion
import tetra3 as t3
from astropy.io import fits
import subprocess as sp
import scipy.ndimage as nd
import glob
import numpy as np

import matplotlib.pyplot as plt

"""
Function that takes a list of star positions and creates a .axy file for Astrometry.net

INPUTS
filename: filename to write .axy file to
x = x coordinates of stars
y = y coordinates of stars
depth = number of stars to match to
scale_bounds = min/max of FOV of camera in degrees
"""
def writeANxy(filename, x, y, dim=(0,0), depth=10, scale_bounds = (0.,0.)):

    #Create FITS BINTABLE columns
    colX = fits.Column(name='X', format='1D', array=x)
    colY = fits.Column(name='Y', format='1D', array=y)
    cols = [colX, colY]

    #Create the BinTableHDU
    tbhdu = fits.BinTableHDU.from_columns(cols)
    prihdr = fits.Header()

    #Create keywords
    prihdr['AN_FILE'] = ('XYLS', 'Astrometry.net file type')
    prihdr['IMAGEW'] = (dim[0],'image width')
    prihdr['IMAGEH'] = (dim[1],'image height')
    prihdr['ANRUN'] = (True, 'Solve this field!')
    prihdr['ANVERUNI'] = (True, 'Uniformize field during verification')
    prihdr['ANVERDUP'] = (False, 'Deduplicate field during verification')
    prihdr['ANAPPL1'] = (scale_bounds[0]/dim[0]*3600,'scale: arcsec/pixel min')
    prihdr['ANAPPU1'] = (scale_bounds[1]/dim[0]*3600,'scale: arcsec/pixel max')
    prihdr['ANTWEAK'] = (True, 'Tweak: yes please!')
    prihdr['ANTWEAKO'] = (2, 'Tweak order')
    prihdr['ANSOLVED'] = ('./%s.solved'%filename,'solved output file')
    prihdr['ANMATCH']  = ('./%s.match'%filename, 'match output file')
    prihdr['ANRDLS']   = ('./%s.rdls'%filename, 'ra-dec output file')
    prihdr['ANWCS']    = ('./%s.wcs'%filename, 'WCS header output filename')
    prihdr['ANCORR']   = ('./%s.corr'%filename, 'Correspondences output filename')
    prihdr['ANDPL1']   = (1, 'no comment')
    prihdr['ANDPU1']   = (depth, 'no comment')
    extension = '.axy'
    prihdu = fits.PrimaryHDU(header=prihdr)

    #Write to file
    thdulist = fits.HDUList([prihdu, tbhdu])
    thdulist.writeto(filename+extension, overwrite=True)

"""
Convert xy pixel values to r,theta from the centre of the image
r is in pixels
theta is in degrees
"""
def xy_to_rt(coord,c=(720,540)):
    x,y = coord
    r = np.sqrt((x-c[0])**2+(y-c[1])**2)
    theta = np.degrees(np.arctan2(y,x))
    return (r,theta)

def match_error(folder_prefix,mag_only=1):
    corr_fits = fits.open(folder_prefix+'.corr')[1].data
    pict_px = []
    solved_px = []

    for i, obj in enumerate(corr_fits):
        px = (obj[4],obj[5])
        rt_px = xy_to_rt(px)

        match_px = (obj[0],obj[1])
        rt_match_px = xy_to_rt(match_px)

        pict_px.append(rt_match_px)
        solved_px.append(rt_px)

    pict_px = np.array(pict_px)
    solved_px = np.array(solved_px)

    if mag_only:
        deltas = np.abs(pict_px - solved_px)
    else:
        deltas = pict_px - solved_px

    plt.figure(1)
    plt.plot(pict_px[:,0],deltas[:,0]*14.22,"k.")
    plt.xlabel("Radii from centre [px]")
    plt.ylabel("Error in radial coordinate [arcsec]")

    plt.figure(2)
    plt.plot(pict_px[:,0],deltas[:,1]*3600,"k.")
    plt.xlabel("Radii from centre [px]")
    plt.ylabel("Error in theta coordinate [arcsec]")

    print("Median Values (r [arcsec], theta [arcsec])")
    print(np.median(deltas,axis=0)[0]*14.22, np.median(deltas,axis=0)[1]*3600)
    print("Mean Values (r [arcsec], theta [arcsec])")
    print(np.mean(deltas,axis=0)[0]*14.22, np.mean(deltas,axis=0)[1]*3600)
    print("Std Values (r [arcsec], theta [arcsec])")
    print(np.std(deltas,axis=0)[0]*14.22, np.std(deltas,axis=0)[1]*3600)
    plt.show()
    return


"""
Main function to run an image
Takes a FITS image, extracts stars, solves the field and converts into an AltAz
attitude quaternion.

INPUTS:
img_filename = filename of the FITS image to solve
prefix = prefix of all auxillary files (including solution)
output_folder = folder to store auxillary and solution files

OUTPUTS:
An attitude quaternion in AltAz coordinate frame of the image
"""
def run_image(img_filename,prefix,output_folder):

    folder_prefix = output_folder+"/"+prefix

    #Open FITS
    hdul = fits.open(str(img_filename))
    img = hdul[0].data

    #Get list of positions (extract centroids) via Tetra3
    lst = t3.get_centroids_from_image(img,bg_sub_mode="local_mean",sigma_mode="global_root_square",filtsize=15)

    #import pdb; pdb.set_trace()

    #Write .axy file for astrometry.net
    writeANxy(folder_prefix, lst.T[1], lst.T[0], dim=img.T.shape, scale_bounds=(5,6))

    #Run astrometry.net
    print("")
    start_astro = time.perf_counter()
    os.system("astrometry-engine %s.axy -c astrometry.cfg"%folder_prefix)
    end_astro = time.perf_counter()

    #Extract astrometry.net WCS info and print to terminal
    print("\nRESULTS:")
    os.system("wcsinfo %s.wcs| grep -E -w 'ra_center|dec_center|orientation|pixscale|fieldw|fieldh'"%folder_prefix)

    #Extract RA, DEC and POSANGLE from astrometry.net output
    output = sp.getoutput("wcsinfo %s.wcs| grep -E -w 'ra_center|dec_center|orientation'"%folder_prefix)
    [POS,RA,DEC] = [float(s.split(" ")[1]) for s in output.splitlines()]

    #Convert to quaternion
    q = conversion.RaDec2Quat(RA,DEC,POS)

    print(f"\nastrometry.net in {end_astro - start_astro:0.4f} seconds")

    #Check errors?
    #match_error(folder_prefix,mag_only=1)

    #Return the quaternion
    return q

###############################################################################

#Retrieve the filename from command line, checking for the number of arguments
if len(sys.argv) != 3:
    print("Bad number of arguments")
    exit()
else:
    folder = sys.argv[1]
    file_prefix = sys.argv[2]

nx=1440
ny=1080

bytes_pix = 2
filename = folder +'/'+ file_prefix + '.raw'
outname = folder +'/'+ file_prefix + '.fits'

"""
dfiles = glob.glob(folder + 'dark_?.raw')
dmn = np.zeros((ny,nx))
for dfn in dfiles:
    with open(dfn, 'rb') as ff:
        ss = ff.read(nx*ny*bytes_pix)
        ff.close()
        dmn += np.frombuffer(ss, dtype=np.uint16).reshape((ny,nx))

dmn /= len(dfiles)
"""
#Run the main command a few times
for i in range(1):
    tic = time.perf_counter()

    with open(filename, 'rb') as ff:
        ss = ff.read(nx*ny*bytes_pix)
        ff.close()
        im = np.frombuffer(ss, dtype=np.uint16).reshape((ny,nx))
    #im = (im-dmn.astype(np.int16)).astype(np.int16)

    fits.writeto(outname, im, overwrite=True)
    toc = time.perf_counter()

    q = run_image(outname,file_prefix,"output")

    tac = time.perf_counter()
    print(f"Read/Write in {toc - tic:0.4f} seconds")
    print(f"Solve in {tac - toc:0.4f} seconds")
    print(q)


#-------------
