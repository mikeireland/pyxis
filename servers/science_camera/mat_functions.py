import numpy as np
from scipy.optimize import minimize_scalar as ms


def find_CKM_angles(Imat):
    #Finds the CKM angles given a 3x2 intensity coupling matrix
    #Axis 0 = Outputs, Axis 1 = Inputs
    MagE = np.sqrt(Imat)
    t1 = np.arccos(MagE[0,0])
    t2 = np.arctan(MagE[2,0]/MagE[1,0])
    t3 = np.arcsin(-MagE[0,1]/np.sin(t1)) #SHOULD BE NEGATIVE!!!

    def myfunc(x):
        delta_2 = MagE[1,1] - np.abs(np.cos(t1)*np.cos(t2)*np.sin(t3) + np.sin(t2)*np.cos(t3)*np.exp(1j*x))
        delta_4 = MagE[2,1] - np.abs(np.cos(t1)*np.sin(t2)*np.sin(t3) - np.cos(t2)*np.cos(t3)*np.exp(1j*x))
        return delta_2**2 + delta_4**2

    delta = ms(myfunc,bounds=(0,2*np.pi), method='bounded')
    return t1,t2,t3,delta.x


def make_CKM_mat(t1,t2,t3,delta):
    #Makes the CKM matrix from the four given angles
    a11 = np.cos(t1)
    a12 = -np.sin(t1)*np.cos(t3)
    a13 = -np.sin(t1)*np.sin(t3)
    a21 = np.sin(t1)*np.cos(t2)
    a22 = np.cos(t1)*np.cos(t2)*np.cos(t3) - np.sin(t2)*np.sin(t3)*np.exp(1j*delta)
    a23 = np.cos(t1)*np.cos(t2)*np.sin(t3) + np.sin(t2)*np.cos(t3)*np.exp(1j*delta)
    a31 = np.sin(t1)*np.sin(t2)
    a32 = np.cos(t1)*np.sin(t2)*np.cos(t3) + np.cos(t2)*np.sin(t3)*np.exp(1j*delta)
    a33 = np.cos(t1)*np.sin(t2)*np.sin(t3) - np.cos(t2)*np.cos(t3)*np.exp(1j*delta)
    return np.array([[a11,a12,a13],[a21,a22,a23],[a31,a32,a33]])


def find_final_mat(Imat,I1,I2,dphase):
    #Makes the CKM matrix, while trying to break the degeneracy in delta

    #Takes the flux ratio matrix Imat, as well as two fringe intensity measurements
    #I1 and I2, separated by a phase of dphase

    #Imat axis 0 = Inputs, axis 2 = Outputs

    Imat[0]/=np.sum(Imat[0])
    Imat[1]/=np.sum(Imat[1])
    I1_frac = I1/np.sum(I1)
    I2_frac = I2/np.sum(I2)

    t1,t2,t3,delta = find_CKM_angles23(Imat.T)
    mata = make_CKM_mat(t1,t2,t3,delta)
    matb = make_CKM_mat(t1,t2,t3,-delta)

    def find_phase(x,mat,I):
        E = 1/np.sqrt(2)*np.array([1,0,np.exp(1j*x)])
        Eout = np.matmul(mat,E)
        Iout = np.abs(Eout)**2
        diff = I - Iout
        return np.sum(np.abs(diff))

    phi_a1 = ms(find_phase,bounds=(0,2*np.pi), args=(mata,I1_frac),method='bounded').x
    phi_a2 = ms(find_phase,bounds=(0,2*np.pi), args=(mata,I2_frac),method='bounded').x
    phi_b1 = ms(find_phase,bounds=(0,2*np.pi), args=(matb,I1_frac),method='bounded').x
    phi_b2 = ms(find_phase,bounds=(0,2*np.pi), args=(matb,I2_frac),method='bounded').x

    print(phi_a1,phi_a2,phi_b1,phi_b2)

    diff_a = (phi_a2-phi_a1)#%(2*np.pi)
    diff_b = (phi_b2-phi_b1)#%(2*np.pi)

    #residual_a = np.abs(diff_a - dphase)
    #residual_b = np.abs(diff_b - dphase)

    print(diff_a,diff_b)


    #if residual_a > residual_b:
        #return matb
    #else:
        #return mata


def make_P2VM_CKM(ckm_mat):
    #makes the P2VM matrix from the CKM matrix
    def g(phi):
        return 1/2.*np.abs(np.matmul(ckm_mat,np.array([1,0,np.exp(1j*phi)])))**2

    r = 1/2.*(g(np.pi) - g(0))
    i = 1/2.*(g(3*np.pi/2) - g(np.pi/2))
    f = 1/4.*(g(0) + g(np.pi/2) + g(np.pi) + g(3*np.pi/2))

    V2PM = np.array([r,i,f]).T
    print(V2PM)
    return np.linalg.inv(V2PM)
