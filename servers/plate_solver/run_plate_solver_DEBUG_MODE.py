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
    hdul = fits.open(str(config["path_to_data"]+"/"+img_filename))
    img = (hdul[0].data)[0]
    #img = hdul[0].data

    #Get list of positions (extract centroids) via Tetra3
    lst = t3.get_centroids_from_image(img,bg_sub_mode=config["Tetra3"]["bg_sub_mode"],
                                          sigma_mode=config["Tetra3"]["sigma_mode"],
                                          filtsize=config["Tetra3"]["filt_size"])

    if len(lst) == 0:
        print("COULD NOT EXTRACT STARS")
        return (0,np.array([0,0,0]))

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
    if len(sys.argv) == 2:
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

    IP = config["IP"]

    """
    try:
        context = zmq.Context()
        target_socket = context.socket(zmq.REQ)
        tcpstring = "tcp://"+IP+":"+config["target_port"]
        target_socket.connect(tcpstring)
        target_socket.RCVTIMEO = 1000
        print("Connected to target server, port %s"%config["target_port"])
    except:
        print('ERROR: Could not connect to target server. Please check that the server is running and IP is correct.')
    try:
        context = zmq.Context()
        camera_socket = context.socket(zmq.REQ)
        tcpstring = "tcp://"+IP+":"+config["camera_port"]
        camera_socket.connect(tcpstring)
        camera_socket.RCVTIMEO = 1000
        print("Connected to camera, port %s"%config["camera_port"])
    except:
        print('ERROR: Could not connect to camera server. Please check that the server is running and IP is correct.')
    """
    try:
        robot_control_socket = context.socket(zmq.REQ)
        tcpstring = "tcp://"+IP+":"+config["robot_control_port"]
        robot_control_socket.connect(tcpstring)
        robot_control_socket.RCVTIMEO = 1000
        print("Connected to robot")
    except:
        print('ERROR: Could not connect to robot server. Please check that the server is running and IP is correct.')       
    
    print("Beginning loop")
    while(1):

        """
        print("Sending request")

        target_socket.send_string("TS.getCoordinates")
        message = target_socket.recv()
        message = message.decode("utf-8")
        print("Received target server message: %s" % message )

        try:
            result = json.loads(message)
            target = (result["RA"],result["DEC"])
        except:
            print("Bad target format")
        
        print(target)
        
        camera_socket.send_string(config["camera_port_name"]+".getlatestfilename")
    
        #Ask camera for next image
        message = camera_socket.recv()
        print("Received camera message: %s" % message.decode("utf-8").strip('\"') )
        """
        
        #SET TARGET AND FILENAME TO SOLVE
        target = (191.93028656 -59.68877200) #Beta Crux
        filename = "data/test_1234.fits"
        
        if os.path.exists(str(config["path_to_data"]+"/"+filename)):
            print("Filename Exists. Running solver")

            #run image
            flag,angles = run_image(filename,config,target)

            if flag>0:

                # WORK ON ANGLES -> return_message
                return_message = "receive_CST_angles [%s,%s,%s]"%(angles[0],angles[1],angles[2]) #angles

                #Send reply to robot
                print(return_message)
                robot_control_socket.send_string(return_message)
                try:
                    message = robot_control_socket.recv()
                    print("Robot response: %s" % message)
                except:
                    print("Could not communicate with robot")
            else:
                print("ERROR")
        else:
            print("Not a real file")
            time.sleep(1)
        
#-------------
"""

    nx=1440
    ny=1080

    bytes_pix = 2
    filename = folder +'/'+ file_prefix + '.raw'
    outname = folder +'/'+ file_prefix + '.fits'

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


Convert xy pixel values to r,theta from the centre of the image
r is in pixels
theta is in degrees

def xy_to_rt(coord,c=(720,540)):
    x,y = coord
    r = np.sqrt((x-c[0])**2+(y-c[1])**2)
    theta = np.degrees(np.arctan2(y,x))
    return (r,theta)

def match_error(folder_prefix,mag_only=1):
    import matplotlib.pyplot as plt

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
