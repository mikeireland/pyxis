import numpy as np
from scipy.optimize import minimize_scalar as ms

#PARAMS
""" Old params 
glass = "nsf11"
prism_angle = 30 #deg
focus_lens = 7.6*1e-3 #microns
central_wavelength = 0.66 #Microns
central_angle = 27.35
pix_size = 3.45
n_channels = 10
"""
""" New params """
glass = "bk7"
prism_angle = 45 #deg
focus_lens = 15*1e-3 #microns
central_wavelength = 0.66 #Microns
central_angle = 35.4
pix_size = 6.5
n_channels = 13



def nglass(l, glass='sio2'):
    """Refractive index of fused silica and other glasses. Note that C is
    in microns^{-2}

    Parameters
    ----------
    l: wavelength
    """
    try:
        nl = len(l)
    except:
        l = [l]
        nl=1
    l = np.array(l)
    if (glass == 'sio2'):
        B = np.array([0.696166300, 0.407942600, 0.897479400])
        C = np.array([4.67914826e-3,1.35120631e-2,97.9340025])
    elif (glass == 'bk7'):
        B = np.array([1.03961212,0.231792344,1.01046945])
        C = np.array([6.00069867e-3,2.00179144e-2,1.03560653e2])
    elif (glass == 'nf2'):
        B = np.array( [1.39757037,1.59201403e-1,1.26865430])
        C = np.array( [9.95906143e-3,5.46931752e-2,1.19248346e2])
    elif (glass == 'nsf11'):
        B = np.array([1.73759695E+00,   3.13747346E-01, 1.89878101E+00])
        C = np.array([1.31887070E-02,   6.23068142E-02, 1.55236290E+02])
    elif (glass == 'ncaf2'):
        B = np.array([0.5675888, 0.4710914, 3.8484723])
        C = np.array([0.050263605,  0.1003909,  34.649040])**2
    elif (glass == 'mgf2'):
        B = np.array([0.48755108,0.39875031,2.3120353])
        C = np.array([0.04338408,0.09461442,23.793604])**2
    elif (glass == 'npk52a'):
        B = np.array([1.02960700E+00,1.88050600E-01,7.36488165E-01])
        C = np.array([5.16800155E-03,1.66658798E-02,1.38964129E+02])
    elif (glass == 'psf67'):
        B = np.array([1.97464225E+00,4.67095921E-01,2.43154209E+00])
        C = np.array([1.45772324E-02,6.69790359E-02,1.57444895E+02])
    elif (glass == 'npk51'):
        B = np.array([1.15610775E+00,1.53229344E-01,7.85618966E-01])
        C = np.array([5.85597402E-03,1.94072416E-02,1.40537046E+02])
    elif (glass == 'nfk51a'):
        B = np.array([9.71247817E-01,2.16901417E-01,9.04651666E-01])
        C = np.array([4.72301995E-03,1.53575612E-02,1.68681330E+02])
    elif (glass == 'si'): #https://refractiveindex.info/?shelf=main&book=Si&page=Salzberg
        B = np.array([10.6684293,0.0030434748,1.54133408])
        C = np.array([0.301516485,1.13475115,1104])**2
    #elif (glass == 'zns'): #https://refractiveindex.info/?shelf=main&book=ZnS&page=Debenham
    #    B = np.array([7.393, 0.14383, 4430.99])
    #    C = np.array([0, 0.2421, 36.71])**2
    elif (glass == 'znse'): #https://refractiveindex.info/?shelf=main&book=ZnSe&page=Connolly
        B = np.array([4.45813734,0.467216334,2.89566290])
        C = np.array([0.200859853,0.391371166,47.1362108])**2
    elif (glass == 'noa61'):
        n = 1.5375 + 8290.45/(l*1000)**2 - 2.11046/(l*1000)**4
        return n
    else:
        print("ERROR: Unknown glass {0:s}".format(glass))
        raise UserWarning
    n = np.ones(nl)
    for i in range(len(B)):
            n += B[i]*l**2/(l**2 - C[i])
    return np.sqrt(n)

def snell(i,n1,n2):
    return np.degrees(np.arcsin(n1/n2*np.sin(np.radians(i))))

def refraction_angle(i,wave):
    glass_n = nglass(wave,glass)
    r_1 = snell(i,1,glass_n)
    i_2 = prism_angle - r_1
    return snell(i_2,glass_n,1)

def pixel_location(i,wave,px):
    a = refraction_angle(i,wave)
    pixel = a*focus_lens/(px*1e-6)

def wavelength_from_px(d_px,px_size):
    d_angle = d_px*(px_size*1e-6)/focus_lens
    a_central = refraction_angle(central_angle,central_wavelength)
    my_angle = a_central + np.degrees(d_angle)
    return ms(lambda x: np.abs(my_angle-refraction_angle(central_angle,x))).x

#Red_direction is either -1 or +1 that determines the direction the wavelength scale
#should go in. +1 = Red is in the positive direction (on the right)
#-1 = Red is in the negative direction (on the left)
def make_wavelength_scale(num_channels,red_direction=1):
    centre = num_channels//2
    pixel_scale = np.zeros(num_channels)
    for i in range(num_channels):
        #Red is on the right
        d_px = red_direction*(centre-i)
        pixel_scale[i] = wavelength_from_px(d_px,pix_size)
    return pixel_scale

print(make_wavelength_scale(n_channels))
