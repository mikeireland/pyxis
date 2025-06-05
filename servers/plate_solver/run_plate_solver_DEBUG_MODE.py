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
import tomli
import glob
import zmq
import json

"""
Function that takes a list of star positions and creates a .axy file for Astrometry.net

INPUTS
filename: filename to write .axy file to
x = x coordinates of stars
y = y coordinates of stars
depth = number of stars to match to
scale_bounds = min/max of FOV of camera in degrees
"""

def writeANxy(filename, x, y, dim, depth=10, scale_bounds = (0.,0.), ref_pix = (None,None), est_pos_flag = 0, est_pos = (0,0,10)):

    if ref_pix == (None,None):
        ref_pix = (dim[0]//2+0.5,dim[1]//2+0.5)

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
    #prihdr['ANSOLVED'] = ('./%s.solved'%filename,'solved output file')
    #prihdr['ANMATCH']  = ('./%s.match'%filename, 'match output file')
    #prihdr['ANRDLS']   = ('./%s.rdls'%filename, 'ra-dec output file')
    prihdr['ANWCS']    = ('./%s.wcs'%filename, 'WCS header output filename')
    #prihdr['ANCORR']   = ('./%s.corr'%filename, 'Correspondences output filename')
    prihdr['ANDPL1']   = (1, 'no comment')
    prihdr['ANDPU1']   = (depth, 'no comment')
    prihdr['ANCRPIX1']   = (ref_pix[0], 'set the WCS x reference point to the given position')
    prihdr['ANCRPIX2']   = (ref_pix[1], 'set the WCS y reference point to the given position')
    if est_pos_flag:
        prihdr['ANERA']   = (est_pos[0], 'RA center estimate (deg)')
        prihdr['ANEDEC']   = (est_pos[1], 'Dec center estimate (deg)')
        prihdr['ANERAD']   = (est_pos[2], 'Search radius from estimated posn (deg)')
    extension = '.axy'
    prihdu = fits.PrimaryHDU(header=prihdr)

    #Write to file
    thdulist = fits.HDUList([prihdu, tbhdu])
    thdulist.writeto(filename+extension, overwrite=True)


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
def run_image(img_filename,config,target):
    start_time = time.perf_counter()

    prefix = os.path.basename(img_filename).split('.', 1)[0]

    folder_prefix = config["output_folder"]+"/"+prefix

    #Open FITS
    #hdul = fits.open(str(config["path_to_data"]+"/"+img_filename))
    #img = (hdul[0].data)[0].astype(np.float32)
    # Edited by Qianhui medians subtraction to remove stripes 
    #img = img - np.median(img, axis=1, keepdims=True)  
    #Update the fits file with the medians subtracted image
    #!!! This won't work because the inital data was uint16, and the subtraction will make it float32
    #hdul[0].data[0] = img
    #hdul.writeto(str(config["path_to_data"]+"/"+img_filename.replace(".fits", "_tmp.fits") ), overwrite=True)
    
    img = fits.getdata(str(config["path_to_data"]+"/"+img_filename))[0].astype(np.float32)
    img -= np.median(img, axis=1, keepdims=True)
    #img = nd.median_filter(img, size=(5, 5))
    fits.writeto(str(config["path_to_data"]+"/"+img_filename.replace(".fits", "_tmp.fits") ), img, overwrite=True)

    #Get list of positions (extract centroids) via Tetra3
    lst = t3.get_centroids_from_image(img,bg_sub_mode=config["Tetra3"]["bg_sub_mode"], #!!!Question: there is subtract background here, even though the processed images are not saved
                                          sigma_mode=config["Tetra3"]["sigma_mode"],
                                          filtsize=config["Tetra3"]["filt_size"], sigma=2.0)

    if len(lst) == 0:
        print("COULD NOT EXTRACT STARS")
        return (0,np.array([0,0,0]))
    else:
        print("Extracted %d stars"%len(lst))

    #Write .axy file for astrometry.net
    writeANxy(folder_prefix, lst.T[1], lst.T[0], dim=img.T.shape,
              depth=config["Astrometry"]["depth"],
              scale_bounds=(config["Astrometry"]["FOV_min"],
                            config["Astrometry"]["FOV_max"]),
              ref_pix=(config["Astrometry"]["ref_pix_x"],
                       config["Astrometry"]["ref_pix_y"]),
              est_pos_flag=config["Astrometry"]["estimate_position"]["flag"],
              est_pos=(config["Astrometry"]["estimate_position"]["ra"],
                       config["Astrometry"]["estimate_position"]["dec"],
                       config["Astrometry"]["estimate_position"]["rad"]))

    #remove previous results if they exist
    if (os.path.exists("%s.wcs"%folder_prefix)):
        os.remove("%s.wcs"%folder_prefix)
    
    #Run astrometry.net
    os.system("./astrometry/solver/astrometry-engine %s.axy -c astrometry.cfg"%folder_prefix)

    if not(os.path.exists("%s.wcs"%folder_prefix)):
        print("DID NOT SOLVE")
        config["Astrometry"]["estimate_position"]["flag"] = 0
        return (0,np.array([0,0,0]))

    #Extract astrometry.net WCS info and print to terminal
    print("\nRESULTS:")
    os.system("./astrometry/util/wcsinfo %s.wcs| grep -E -w 'ra_center|dec_center|crval0|crval1|orientation|pixscale|fieldw|fieldh'"%folder_prefix)

    #Extract RA, DEC and POSANGLE from astrometry.net output
    output = sp.getoutput("./astrometry/util/wcsinfo %s.wcs| grep -E -w 'crval0|crval1|orientation'"%folder_prefix)
    [RA,DEC,POS] = [float(s.split(" ")[1]) for s in output.splitlines()]
    print(RA,DEC)
    config["Astrometry"]["estimate_position"]["ra"] = RA
    config["Astrometry"]["estimate_position"]["dec"] = DEC
    config["Astrometry"]["estimate_position"]["flag"] = 1

    #Convert to quaternion
    angles = conversion.diffRaDec2AltAz(RA,DEC,POS,target[0],target[1])
    end_time = time.perf_counter()
    print(f"\nCompleted in {end_time - start_time:0.4f} seconds")

    print(f"Angles: Alt={angles[1]}, Az={angles[0]}, PosAng={angles[2]}")

    return (1,angles)

def get_image(input_folder):
    list_of_files = glob.glob(input_folder+'/*.fits') # * means all if need specific format then *.csv
    latest_file = max(list_of_files, key=os.path.getctime)
    return latest_file

###############################################################################

if __name__ == "__main__":

    #Retrieve the filename from command line, checking for the number of arguments
    if len(sys.argv) == 3:
        config_file = sys.argv[1]
        image_name = sys.argv[2]
    elif len(sys.argv) == 2:
        config_file = sys.argv[1]
    elif len(sys.argv) == 1:
        config_file = "Sinistra_astrometry.toml"
        if os.path.exists(config_file):
            print("Using config "+config_file)
        else:
            print("Default config not found. Exiting")
            exit()
    else:
        print("Bad number of arguments")
        exit()

    with open(config_file, "rb") as f:
        config = tomli.load(f)
     
    print("Beginning loop")
    # while(1):      
        #SET TARGET AND FILENAME TO SOLVE
    target = (191.93028656, -59.68877200) #Beta Crux
    filename = "data/"+str(image_name)
    
    if os.path.exists(str(config["path_to_data"]+"/"+filename)):
        print("Filename Exists. Running solver")

        #run image
        flag,angles = run_image(filename,config,target)

        if flag>0:
            # WORK ON ANGLES -> return_message
            return_message = "receive_CST_angles %s,%s,%s"%(angles[0],angles[1],angles[2]) #angles
            #Send reply to robot
            print(return_message)
        else:
            print("ERROR")
    else:
        print("Not a real file...")
        print("Attempted to open: %s"%(str(config["path_to_data"]+"/"+filename)))
        # time.sleep(1)
        
