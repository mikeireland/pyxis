"""
A test program to read in the accelerometers when output as text.

Linear actuator: At a 3ms period, there is a 0.008g amplitude acceleration. This
corresponds to an 18nm oscillation. Definitely seems smooth enough.
"""
import numpy as np
import matplotlib.pyplot as plt
plt.ion()
lsb_g = 16000.0

dd = np.loadtxt('25hz_250us_5x.txt')
nt = dd.shape[0]
ts = np.arange(nt)*250e-6
plt.clf()
accel = dd[:,1]/lsb_g*9.8
accel -= np.mean(accel)
gconv = np.exp(-(np.arange(21)-10)**2/5**2/2)
gconv /= np.sum(gconv)
accel = np.convolve(accel, gconv, mode='same')

#2*pi*n_amp*USEC*2 = 25e-6*2*np.pi*128*2 = 0.0402

plt.plot(ts, accel)
plt.plot(ts, -0.125*np.sin(ts/0.0402*2*np.pi))
plt.xlabel('Time (s)')
plt.ylabel('Accel (m/s^2)')