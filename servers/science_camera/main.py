import numpy as np
import mat_functions as mf
from astropy.io import fits
import matplotlib.pyplot as plt

def get_array(filename):
    hdul = fits.open(filename)
    data = hdul[0].data/2**4
    return data

dark = get_array("dark_0.fits")
d = np.mean(dark)

def extract(array,xref,yref):
    pos_wave = xref - 4

    pos_p1_A = yref
    pos_p2_A = yref + 8
    pos_p1_B = yref + 14
    pos_p2_B = yref + 22
    pos_p1_C = yref + 28
    pos_p2_C = yref + 36

    ret_mat = np.zeros((20,3))

    for i in range(10):
        p1A = array[pos_p1_A,pos_wave+i]+array[pos_p1_A+1,pos_wave+i]
        p1B = array[pos_p1_B,pos_wave+i-1]+array[pos_p1_B+1,pos_wave+i-1]
        p1C = array[pos_p1_C,pos_wave+i-1]+array[pos_p1_C+1,pos_wave+i-1]
        p2A = array[pos_p2_A,pos_wave+i-1]+array[pos_p2_A+1,pos_wave+i-1]
        p2B = array[pos_p2_B,pos_wave+i-1]+array[pos_p2_B+1,pos_wave+i-1]
        p2C = array[pos_p2_C,pos_wave+i-1]+array[pos_p2_C+1,pos_wave+i-1]
        ret_mat[i] = [p1A,p1B,p1C]
        ret_mat[i+10] = [p2A,p2B,p2C]
    return ret_mat


flux1 = np.sum(get_array("flux1_0.fits")-d,axis=0)
flux2 = np.sum(get_array("flux2_0.fits")-d,axis=0)

f1 = extract(flux1,46,40)
f2 = extract(flux2,46,40)

fringes1 = get_array("fringes1_0.fits")[50]-d
fringes2 = get_array("fringes2_n4_0.fits")[50]-d

fr1 = extract(fringes1,46,40)
fr2 = extract(fringes2,46,40)

nf1 = (f1.T/np.sum(f1,axis=1)).T
nf2 = (f2.T/np.sum(f2,axis=1)).T

def Imat(i):
    return np.array([nf1[i],nf2[i]])

def check_delta(i):
    I = Imat(i)
    mf.find_final_mat(I,fr1[i],fr2[i],0)
    return

def P2VM(i):
    I = Imat(i)
    t1,t2,t3,delta = mf.find_CKM_angles(I.T)
    ckm_mat = mf.make_CKM_mat(t1,t2,t3,delta)
    return mf.make_P2VM_CKM(ckm_mat)
