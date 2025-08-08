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
import glob
import numpy as np
import pytomlpp
import glob
import zmq
import json, sep

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
Takes a FITS image, extracts stars, solves the field and converts into Euler angles.

INPUTS:
    img_filename = filename of the FITS image to solve
    config - configuration file
    target - target Ra and Dec
    offset - offset to reference pixel

OUTPUTS:
    Error code (1 if successful)
    Euler angles in AltAz coordinate frame of the image
"""
def run_image(img_filename,config,target,offset):
    start_time = time.perf_counter()

    prefix = os.path.basename(img_filename).split('.', 1)[0]

    folder_prefix = config["output_folder"]+"/"+prefix

    # #Open FITS
    # hdul = fits.open(str(config["path_to_data"]+"/"+img_filename))
    # img = (hdul[0].data)[0]
    # #img = hdul[0].data
    
    img = fits.getdata(str(config["path_to_data"]+"/"+img_filename))[0].astype(np.float32)
    img -= np.median(img, axis=1, keepdims=True)
    #img = nd.median_filter(img, size=(5, 5))
    # fits.writeto(str(config["path_to_data"]+"/"+img_filename.replace(".fits", "_tmp.fits") ), img, overwrite=True)

    # #Get list of positions (extract centroids) via Tetra3
    # lst = t3.get_centroids_from_image(img,bg_sub_mode=config["Tetra3"]["bg_sub_mode"],
    #                                       sigma_mode=config["Tetra3"]["sigma_mode"],
    #                                       filtsize=config["Tetra3"]["filt_size"])

    #Edited by Qianhui: Get a list of positions via Source Extractor instead of Tetra3
    bkg = sep.Background(img)
    img_sub = img - bkg
    filter_list = config["SExtractor"]["filter_kernel"]
    
    lst = sep.extract(img_sub, 
                      thresh=config["SExtractor"]["thresh"], 
                      err=bkg.globalrms, 
                      minarea=config["SExtractor"]["minarea"], 
                      filter_kernel=np.array(filter_list), 
                      deblend_cont=config["SExtractor"]["deblend_cont"])
    print("Found %d sources"%len(lst))
    # sort the list of sources based on the flux
    flux = lst['flux']
    sort_indices = np.argsort(flux)[::-1] 
    lst = lst[sort_indices]
    
    # If no extraction, return error
    if len(lst) == 0:
        print("COULD NOT EXTRACT STARS")
        return (0,np.array([0,0,0]))

    #Write .axy file for astrometry.net
    writeANxy(folder_prefix, lst['x'], lst['y'], dim=img.T.shape,
              depth=config["Astrometry"]["depth"],
              scale_bounds=(config["Astrometry"]["FOV_min"],
                            config["Astrometry"]["FOV_max"]),
              ref_pix=(config["Astrometry"]["ref_pix_x"]+offset[0],
                       config["Astrometry"]["ref_pix_y"]+offset[1]),
              est_pos_flag=config["Astrometry"]["estimate_position"]["flag"],
              est_pos=(config["Astrometry"]["estimate_position"]["ra"],
                       config["Astrometry"]["estimate_position"]["dec"],
                       config["Astrometry"]["estimate_position"]["rad"]))

    #remove previous results if they exist
    if (os.path.exists("%s.wcs"%folder_prefix)):
        os.remove("%s.wcs"%folder_prefix)
    
    #Run astrometry.net
    os.system("./astrometry/solver/astrometry-engine %s.axy -c astrometry.cfg"%folder_prefix)

    #If failed solve, return error
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

    #Convert to angles
    angles = conversion.diffRaDec2AltAz(RA,DEC,POS,target[0],target[1])
    end_time = time.perf_counter()
    print(f"\nCompleted in {end_time - start_time:0.4f} seconds")

    print(f"Angles: Az={angles[0]}, Alt={angles[1]}, PosAng={angles[2]}")

    return (1,angles)

"""
Function to convert the tip/tilt pixel offsets into a star tracker pixel offset for the plate solver.
Inputs:
    index - 1 = Dextra, 2 = Sinistra
    current_radec - tuple of (ra, dec) of previous position. 
                    Note that the estimates start of with 0,0, and so we will only guess the offset once we solve
                    and know the current ra/dec (i.e elevation)
    offset - the corrective differential pixel displacement from the tip/tilt camera
Output:
    the differential pixel offset on the star tracker camera

"""
def tt_to_plate(index,current_radec,offset):

    #Ensure we have an estimate of the ra/dec first, or else the elevation will be wrong!
    if current_radec != (0,0):
        del_x,del_y = offset

        tt_to_beam = 3.35 #1px to arcseconds
        plate_scale = 14.2
        
        #Convert t/t pixel offsets to angles
        beam_angles = np.array([del_x,del_y])*tt_to_beam
        
        #get elevation (and azimuth)
        e,a = conversion.toAltAz_rad(current_radec[0],current_radec[1])
        
        #Coordinate transform FIX SIGNS HERE!!!!
        if index == 1:
            # Dextra matrix
            M = np.array([[np.cos(e)**2, 5], [np.cos(e)*np.sin(e) + 5*np.cos(e)**2, 1]])
        elif index == 2:
            # Sinistra matrix
            M = np.array([[np.cos(e)**2, 5], [np.cos(e)*np.sin(e) + 5*np.cos(e)**2, 1]])
        
        # Invert matrix
        inv_M = np.linalg.inv(M)
        plate_angles = inv_M @ beam_angles #Angles on the star tracker
        pixel_plate_angles = plate_angles/plate_scale #convert to pixel offset
        
        return pixel_plate_angles
    
    else:
        return (0,0) # Return no offset if we haven't solved yet!


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

    # Load config file
    with open(config_file, "rb") as f:
        config = pytomlpp.load(f)

    IP = config["IP"]

    #Make output folder
    output_dir = config["output_folder"]
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    """
    Attempt to connect to all relevant servers.
    If fails, it should restart and try again.
    This does not always works.
    Best practice: ensure all other servers are running, then start/reboot this script.
    """
    error_flag = 1
    context = zmq.Context()
    while error_flag == 1:
        error_flag = 0
        try:
            target_socket = context.socket(zmq.REQ)
            tcpstring = "tcp://"+config["target_IP"]+":"+config["target_port"]
            target_socket.connect(tcpstring)
            target_socket.RCVTIMEO = 1000

            target_socket.send_string("TS.status")
            message = target_socket.recv()
            print("Connected to target server, port %s"%config["target_port"])
        except:
            print('ERROR: Could not connect to target server. Please check that the server is running and IP is correct.')
            error_flag = 1
            target_socket.setsockopt(zmq.LINGER,0)
            target_socket.close()

        #Camera server
        try:
            camera_socket = context.socket(zmq.REQ)
            tcpstring = "tcp://"+IP+":"+config["camera_port"]
            camera_socket.connect(tcpstring)
            camera_socket.RCVTIMEO = 1000

            camera_socket.send_string(config["camera_port_name"]+".status")
            message = camera_socket.recv()
            print("Connected to camera, port %s"%config["camera_port"])
        except:
            print('ERROR: Could not connect to camera server. Please check that the server is running and IP is correct.')
            error_flag = 1
            camera_socket.setsockopt(zmq.LINGER,0)
            camera_socket.close()

        #Robot server    
        try:
            robot_control_socket = context.socket(zmq.REQ)
            tcpstring = "tcp://"+IP+":"+config["robot_control_port"]
            robot_control_socket.connect(tcpstring)
            robot_control_socket.RCVTIMEO = 1000

            robot_control_socket.send_string("RC.status")
            message = robot_control_socket.recv()
            print("Connected to robot")
        except:
            print('ERROR: Could not connect to robot server. Please check that the server is running and IP is correct.')       
            error_flag = 1
            robot_control_socket.setsockopt(zmq.LINGER,0)
            robot_control_socket.close()

        #Tip Tilt Server
        if config["platesolver_index"] > 0:
            try:
                fibre_injection_socket = context.socket(zmq.REQ)
                tcpstring = "tcp://"+config["NavisIP"]+":"+config["tiptilt_port"]
                fibre_injection_socket.connect(tcpstring)
                fibre_injection_socket.RCVTIMEO = 1000

                fibre_injection_socket.send_string("FI.status")
                message = fibre_injection_socket.recv()
                print("Connected to fibre injection")
            except:
                print('ERROR: Could not connect to fibre injection server. Please check that the server is running and IP is correct.')       
                error_flag = 1
                fibre_injection_socket.setsockopt(zmq.LINGER,0)
                fibre_injection_socket.close()
        
        if error_flag == 1:
            print("Server connection failed. Waiting 5 sec before reattempting")
            time.sleep(5)
            print("Retrying connections")


    # MAIN LOOP
    print("Connected to all ports. Beginning plate solving loop")
    while(1):

        #Get target coordinates
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

        # Retrieve tip/tilt offset adjustments if desired
        if config["platesolver_index"] > 0:
            if config["platesolver_index"] == 1:
                fibre_injection_socket.send_string(config["FI.getDiffPosition [1]"])
            elif config["platesolver_index"] == 2:
                fibre_injection_socket.send_string(config["FI.getDiffPosition [2]"])
            message = fibre_injection_socket.recv()
            message = message.decode("utf-8")
            print("Received fibre injection server message: %s" % message )

            try:
                result = json.loads(message)
                raw_offset = (result["X"],result["Y"])
                
                offset = tt_to_plate(config["platesolver_index"],
                                    (config["Astrometry"]["estimate_position"]["ra"],
                                     config["Astrometry"]["estimate_position"]["dec"]),
                                     raw_offset)
                
            except:
                print("Bad target format")
                offset = (0,0)
        else:
            offset = (0,0)
        
        camera_socket.send_string(config["camera_port_name"]+".getlatestfilename")
    
        #Ask camera for next image
        message = camera_socket.recv()
        print("Received camera message: %s" % message.decode("utf-8").strip('\"') )

        # WORK ON MESSAGE -> FILENAME
        filename = message.decode("utf-8").strip('\"') 
        
        if os.path.exists(str(config["path_to_data"]+"/"+filename)):
            print("Filename Exists. Running solver")

            #run image
            flag,angles = run_image(filename,config,target,offset)

            if flag>0:

                # WORK ON ANGLES -> return_message
                return_message = "RC.receive_ST_angles %s,%s,%s"%(angles[0],angles[1],angles[2]) #angles

                #Send reply to robot
                print(return_message)
                try:
                    robot_control_socket.send_string(return_message) #Edited by Qianhui: moved this line here to avoid failing to send the command
                    message = robot_control_socket.recv()
                    print("Robot response: %s" % message)
                except:
                    print("Could not communicate with robot")
            else:
                print("ERROR in run_image, could not solve")
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
