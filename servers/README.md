README

##### INSTALLATION #####

Installation requirements for servers:
- Spinnaker
- CFITSIO
- OpenCV
- Commander
- qhyccd SDK
- astrometry.net


To install commander: follow readme inside commander folder (located in above directory). May require installation of cmake.

To install qhyccd SDK: run "./install_QHY.sh" inside the servers/libs/camera folder
To uninstall qhyccd SDK: run "./uninstall_QHY.sh" inside the servers/libs/camera folder

To install astrometry.net: run "make" inside the servers/plate_solver folder
To uninstall astrometry.net: run "make clean" inside the servers/plate_solver folder

##### BUILDING #####

To build server programs, use "./master_build.sh", which enters every subdirectory and executes "./configure.sh" and "./build.sh" if present. This builds the executables, with build files in "build/" and binary executables in "bin/". Individual servers can be made separately by entering the subdirectory and running the previously mentioned scripts.

To clean server programs, run "./master_clean.sh", which enters every subdirectory and removes the "build/" and "bin/" folders. NOTE ANYTHING INSIDE THESE FOLDERS WILL BE DELETED, SO BE CAREFUL! Individual servers can be cleaned by just deleting the build and bin folders in the relevant subdirectory.

Shared camera library files (FLIR/QHY) are stored in servers/libs/camera

##### USING #####

For the camera based servers, all servers inherit the relevant FLIR or QHY class (struct), and utilise global variables to communicate between the camera and server threads. To add unique functions for the server thread (i.e commands to the server), simply add the function as a member of the derived struct, and then register it with commander. 

If a callback function is needed after taking every image, and should be done by the camera (not server) thread (eg calculating distance, estimating group delay), this can be implemented through passing said function to the struct constructor. This is performed after *every* image is taken. The function should accept "unsigned short*" (the data array) and return an "int" (error code). Note that the function cannot return any meaningful result (eg distance), and so this must be passed to some registered global variable and then shared with the server thread. 

Servers are initialised through running the binary with an argument to the path of the configuration file. If none is given, it will attempt to load a default one (which may or may not function depending on where the binary is being run from). 

The python script "python_zmq_client.py" can be used to send commands to the server for testing.
