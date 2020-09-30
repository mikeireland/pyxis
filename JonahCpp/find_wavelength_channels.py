import numpy as np
import matplotlib.pyplot as plt


prism_angle = 30
glass = "nsf11"
f_cam = 7.6
pix_scale = 6.5e-3
first_wav = 0.6
final_wav = 0.75

test_wav_spacing = 0.00001


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
    elif (glass == 'f2'):
        B = np.array( [1.345,2.091e-1,9.374e-1])
        C = np.array( [9.977e-3,4.705e-2,1.119e2])
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
    return np.degrees(np.arcsin(n1*np.sin(np.radians(i))/n2))


def dispersion_angle(wav_a,wav_b):

    n_a = nglass(wav_a,glass=glass)[0]
    n_b = nglass(wav_b,glass=glass)[0]

    prism_half_angle = prism_angle/2
    air_half_angles = snell(prism_half_angle,np.array([n_a,n_b]),1)
    #import pdb; pdb.set_trace()
    deviation = 2*air_half_angles
    wavelength_diff = deviation[0]-deviation[1]
    return np.radians(wavelength_diff)

def pixel_spacing(wav_a,wav_b):
    return dispersion_angle(wav_a,wav_b)*f_cam/pix_scale

def find_next_wavelength(wav_a):
    wavelengths = np.arange(wav_a,final_wav,test_wav_spacing)
    pix_width = 0
    i = 0
    while pix_width < 1:
        pix_width = pixel_spacing(wav_a,wavelengths[i])
        #print(pix_width)
        i += 1
    return wavelengths[i]

def find_previous_wavelength(wav_a):
    wavelengths = np.arange(first_wav,wav_a,test_wav_spacing)
    pix_width = pixel_spacing(first_wav,wav_a)
    i = 0
    while pix_width > 1:
        pix_width = pixel_spacing(wavelengths[i],wav_a)
        i += 1
    return wavelengths[i]

def find_wavelengths(cali_wav,pixel):
    spectrum = []
    spectrum.append({"Wavelength":cali_wav,"pixel":pixel})

    current_wav = cali_wav
    current_pix = pixel
    spacing_to_end = pixel_spacing(cali_wav,final_wav)
    while spacing_to_end > 1:
        next_wav = find_next_wavelength(current_wav)
        new_pix = current_pix+1
        spectrum.append({"Wavelength":next_wav,"pixel":new_pix})
        spacing_to_end = pixel_spacing(next_wav,final_wav)
        current_wav = next_wav
        current_pix = new_pix

    current_wav = cali_wav
    current_pix = pixel
    spacing_to_start = pixel_spacing(first_wav,cali_wav)
    while spacing_to_start > 1:
        prev_wav = find_previous_wavelength(current_wav)
        new_pix = current_pix-1
        spectrum.append({"Wavelength":prev_wav,"pixel":new_pix})
        spacing_to_start = pixel_spacing(first_wav,prev_wav)
        current_wav = prev_wav
        current_pix = new_pix

    sorted_spectrum = sorted(spectrum, key=lambda k: k['Wavelength'])

    return sorted_spectrum
