# Pyxis Project Code Manual

## Firmware & Wired Commands

The [firmware.ino](https://gitlab.cecs.anu.edu.au/u5845781/engn4221_pyxis/-/blob/master/firmware/firmware.ino) sketch can be found in the firmware folder in the Pyxis repo. This sketch compiles and can be uploaded to the device.

This firmware handles an arbitrary number of accelerometers whose pins are defined in the ‘AccelerometerReader’ class. This class handles all acceleration and tilt reading and references each accelerometer by their order in the ‘chip_select_pins’ array using an index ‘i’. Given that these accelerometers use SPI, adding more would share the same data pins and simply need unique chip select pins.

The ‘MotorDriver’ class is used to interface directly with the motor driver carriers. This class uses a timer and writes the motor velocities to the carriers at a fixed interrupt. This class takes in body-fixed-frame velocities (such as x, y, yaw) and calculates the forward-kinematics, setting each motor with its correct velocity.

The ‘Controller’ class nests both of the aforementioned classes within it. It serves as an abstraction for the user from the hardware. The controller internally polls the onboard accelerometer and stabilises the tilt and roll of the upper platform. In the current deployment, the controller sits in a loop and waits for the user to input commands through the serial monitor:

‘w’: Add positive x<br>
‘s’: Add negative x<br>
‘a’: Add positive y<br>
‘d’: Add negative y<br>
‘q’: Add anticlockwise yaw<br>
‘e’: Add clockwise yaw<br>
‘z’: Add positive z<br>
‘x’: Add negative z<br>
‘p’: Zero all velocities<br>
‘1’: Toggle upper platform stabilisation<br>
‘2’: Print x, y, z accelerations<br>

Using the arduino serial monitor, the user can send these wired commands to test the unit. This controller class can take similar commands from a network interface, making networking an easy addition to the current stack.


## Resonance Testing

Built into the firmware is a method to initiate an AC sweep test, where sinusoids of increasing frequency are sent to a target motor and the system response measured. This method should not be called manually, and is designed to work with the [collect_data.py](https://gitlab.cecs.anu.edu.au/u5845781/engn4221_pyxis/-/blob/master/resonance_test_code/collect_data.py) method in the ‘data_processing’ directory.

The workflow for a resonance test goes as follows:
1. Open the ‘collect_data.py’ script and change the ‘target_motor’ variable to any of the following: 'Wheel0',  'Wheel1', 'Wheel2', 'Linear0', 'Linear1', 'Linear2'
2. Upload the firmware to the device and ensure the serial monitor in the Arduino software is closed.
3. Run the ‘collect_data.py’ script. It will connect to the serial port and send the device a sweep test command.
4. Wait for ~20 minutes while the sweep test is performed, the software will dump all of the data in a .log file in the ‘resonance_test_results’ directory. It will notify you when finished in the python terminal.

To plot the results from a .log file:
1. Open the [plot_data.py](https://gitlab.cecs.anu.edu.au/u5845781/engn4221_pyxis/-/blob/master/resonance_test_code/plot_data.py) script in the ‘resonance_test_code’ directory.
2. Update the ‘filename’ variable to the name of the .log file you want to plot.
3. Update the ‘target_fourier_axis’ variable to ‘x_pos’, ‘y_pos’, or ‘z_pos’ - depending on which axis you are interested in seeing.
4. Run the ‘plot_data.py’ script. It will generate a bode plot that contains all three axes and 20 fourier transform plots (one for each integer input frequency in the range [1, 20]). These will be saved as .png images in a subdirectory in the [resonance_test_results](https://gitlab.cecs.anu.edu.au/u5845781/engn4221_pyxis/-/tree/master/resonance_test_results) folder. All 21 of these plots will automatically open as Matlab figures.

The bode plot provides a brief overview of the the magnitude of response for each input frequency based on the absolute average magnitude amplitude. The fourier plots show the exact harmonic responses for each input frequency.
