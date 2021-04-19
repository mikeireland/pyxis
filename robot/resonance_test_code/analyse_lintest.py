import numpy as np
import matplotlib.pyplot as plt
plt.ion()

file = '/Users/mireland/pyxis/resonance_test_results/lintest/z_axis_2021-04-03_185840.log'
file = '/Users/mireland/pyxis/resonance_test_results/lintest/x_axis_2021-04-03_185742.log'
file = '/Users/mireland/pyxis/resonance_test_results/lintest2/x_axis_2021-04-09_094203.log'
file = '/Users/mireland/pyxis/resonance_test_results/lintest2/yaw_2021-04-09_094545.log'

dt = 0.001              #Sampling rate in seconds
start_ix = 1000         #For final analysis, use this as a starting point.
stop_ix = 5000          #For final analysis, use this as an ending point.
smooth_amount = 1000   #How much to smooth over.
#-----------------------
#Read in the acceleration. Axes are accel0_x, accel0_y, accel0_z, accel1_x, etc.
accels = np.genfromtxt(file, delimiter=',')
time = np.arange(accels.shape[0])*dt

#Remove the average acceleration
accels -= np.mean(accels, axis=0)

#Create a simple integral of acceleration for velocity.
#i.e. v = u + at for each timestep.
vels = np.cumsum(accels, axis=0)*dt

#Remove average velocity
vels -= np.mean(vels, axis=0)

#Simply integrate velocity to give position
dists = np.cumsum(vels, axis=0)*dt

#Remove smoothed version of distance and velocity for analysis.
conv_func = np.ones(smooth_amount)/smooth_amount
vel_hf = np.empty_like(vels)
dist_hf = np.empty_like(dists)
for i in range(accels.shape[1]):
    vel_hf[:,i] = vels[:,i] - np.convolve(vels[:,i], conv_func, mode='same')
    dist_hf[:,i] = dists[:,i] - np.convolve(dists[:,i], conv_func, mode='same')

#Create upper platform pitch, roll and yaw. NB This should probably be via
#a matrix like the lower platform.
up_pitch = (dist_hf[:,14]*0.5 + dist_hf[:,17]*0.5 - dist_hf[:,11])/52e-3
up_pitch_vel = (vel_hf[:,14]*0.5 + vel_hf[:,17]*0.5 - vel_hf[:,11])/52e-3

#Plot versus time
plt.figure(1)
plt.clf()
plt.plot(time, up_pitch*1e3)
plt.xlabel('Time (s)')
plt.ylabel('Pitch (milli-rad)')
plt.tight_layout()

plt.figure(2)
plt.clf()
plt.plot(time, np.degrees(up_pitch_vel)*60*60)
plt.xlabel('Time (s)')
plt.ylabel('Pitch Velocity (arcsec/s)')
plt.tight_layout()

#TODO for James: Use the document "Linear Quadratic Control for the Pyxis robot" 
#to create the x,y,z,pitch,roll,yaw for the lower platform, maybe using matrices, and
#Compare amplitudes and power spectra for upper and lower platforms.