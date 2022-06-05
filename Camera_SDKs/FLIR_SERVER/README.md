# FLIR/ZABER C++ Software
Drivers and Programs for interfacing with various hardware.

## Layout

- bin - Binary files/programs
- build - intermediate object files used by make
- config - place to store TOML config files
- data - place to store output csv/FITS files
- lib - contains a few dependencies
- src - source code:
    - top level contains scripts/main programs
    - src/include contains modules and class files for the various programs and hardware
- top level python scripts for camera characterisation
- "find_wavelength_channels" is a python script that will calculate the positions of wavelength channels for the beam combiner based on a callibration wavelength.

## Current files

The current programs support FLIR cameras and Zaber actuators. See the file "config/defaultConfig.toml" for an example on how to setup the configuration of these devices.

### C++ Programs:

- testCam - Tests a FLIR camera by making it taking a number of frames and saving it to a FITS file
- testZaber - Tests a Zaber actuator with a few commands including moving to a position and moving at a velocity
- testFPS - Runs a camera at different ROI dimensions and exposure times to produce a CSV that can show the FPS response to these two values
- testFlats - Takes a number of flats at different exposure times. Use the Spinview GUI to test that the current camera view is truly a flat before using.
- testLock - Experimental program to make a FLIR camera and Zaber actuator work together to lock on to fringes. Not fully tested and may not work.
- testLength - Experimental program to estimate the extra dispersion length for an AC Coupler. Barely tested.
- testTracking - Experimental program to do fringe tracking with an AC Coupler. Barely tested

#### *See Jonah Hansen for more details/explanations.*
