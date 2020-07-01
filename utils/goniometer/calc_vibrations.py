"""
Slew speed at 16 microsteps:

0.04g semi-amplitude in Y every 32 steps when running 1 step per 250 us means a
frequency of 125 Hz. This is 0.64 microns. 

Tracking speed at 256 microsteps:"

0.003g semi-amplitude in Y, every ~110ms, which is about every 28 steps. This is a
9 micron amplitude! Also some ~0.001g at 250Hz, which is ~4nm. This is the actual movement.

"""
import numpy as np
scale=2**14

tt = np.loadtxt('37500usec_2_track.txt') #Tracking speed
mm = np.loadtxt('1000usec_2_track.txt') #Medium speed

dd = np.loadtxt('2000usec_2_256.txt') #Tracking speed with fine microsteps.
dd = np.loadtxt('2000usec_2_256_2.txt') #Tracking speed with fine microsteps.

dd = np.loadtxt('2000usec_2_256_floor.txt')
