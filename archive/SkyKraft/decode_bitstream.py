"""
During Zeros

phase=ZYX
phase=001, 470
phase=011, 43
phase=010, 128, we are P=0.25
phase=110, 213
phase=100, 298, 
phase=101, 384, we are P=0.75

"""

import numpy as np
#import matplotlib.pyplot as plt

THREE_ZERO_PHASE_SET=-1
infile0='bitstream0.txt'
infile1='bitstream1.txt'
infile0='bitstream_newboard_on_frame_boundary.txt'; THREE_ZERO_PHASE_SET=700
infile0='bitstream_oldboard_on_frame_boundary.txt' ; THREE_ZERO_PHASE_SET=500
#infile0='bitstream_2.0V.txt'
#infile0='bitstream_2.0V_not_all_zeros.txt'; THREE_ZERO_PHASE_SET=600

def complement(achar):
    """Convenience function to complement bits that are recorded as
    text characters '0' and '1'"""
    if achar=='0':
        return '1'
    return '0'
    
def read_bitstream_01(file):
    """Lets encapsulate the reading of the bistream so that 
    altarnative formats can be supported.
    """
    d = list(f.read()[:-1])
    d_out = ''
    for achar in d:
        d_out += complement(achar)
    return d_out

#For the first file, we just find the frequency.
with open(infile0,'r') as f:
    bits = read_bitstream_01(f)
    
F=int(160) #A starting value...
P=int(0)
X=int(0)
C=int(0)
Ps = np.zeros(2048, dtype=int)
ncorr=0
nError=0
RecordedBits = ''
AdjustFrequency=1
AdjustPhase=0
for i in range(1,2046):
    Z = bits[i-1]
    Y = bits[i]
    X = bits[i+1]
    
    #Count Transitions
    if X != Y:
        C += 1
        
    if (i==THREE_ZERO_PHASE_SET) or (AdjustFrequency==1):
        if (X=='0'):
            if (Y=='1') and (Z=='0'):
                P = 128
            elif (Y=='1') and (Z=='1'):
                P = 213
            elif (Y=='0') and (Z=='1'):
                P=298
            elif (Y=='0') and (Z=='0'):
                P=384
        if (X=='1'):
            if (Y=='0') and (Z=='1'):
                P = 384
            elif (Y=='0') and (Z=='0'):
                P=470
            elif (Y=='1') and (Z=='0'):
                P=43
            elif (Y=='1') and (Z=='1'):
                P = 128
            #else:
            #    raise UserWarning("Invalid data!")
        if (AdjustFrequency==0):
            print(i, len(RecordedBits)//12, len(RecordedBits) % 12, P,  "**********")
    
    #The following code only works if there is a 
    #stream of 0s and provides the starting point...
    if (AdjustFrequency==1):
        if i==255:
            F = C
            print("Frequency: {:d}/512".format(F))
            print("Initial Phase: {:d}".format(P))
            D = (256-F)/2
            AdjustFrequency=0
    else:
        #Record the 0s...
        if (P>256-D-F) and (P<=256-D):
            RecordedBits += complement(Y)

        if (P > 256-D) and (P<256+D):
            #We seem to be lucky that the waveform is asymmetrical in just the right
            #direction... (!)
            if (X == Z):
                print("{:d} Error. XYZ= {:s}{:s}{:s}. P={:d}".format(i,X,Y,X,P))
                if (X!=Y):
                    if (P<256):
                        P=F
                    else:
                        P=512-F
                elif i > THREE_ZERO_PHASE_SET:
                    import pdb; pdb.set_trace()
                nError += 1
            #If before a transition, the transition has already occurred, 
            #adjust phase
            elif ((P < 256) and (Y==X)) or ((P > 256) and (Y==Z)):
                ncorr += 1
                print("{:d} {:d} {:s} {:s} {:s} Phase Error={:3d}".format(i, P, X, Y, Z, 256-P))
                P += (256-P)*3//2
            else:
                print(i, P, X,Y,Z)
    Ps[i]=P
    P = (P+F) % 512

print("Number of phase corrections: {:d}".format(ncorr))
print("Number of apparent XOR errors: {:d}".format(nError))

for i in range(len(RecordedBits)//12):
    print(RecordedBits[i*12:np.minimum((i+1)*12, len(RecordedBits))])


if (False):
    #Finally, do the same for the real data.
    #For the first file, we just find the frequency.
    with open(infile1,'r') as f:
        bits = read_bitstream_01(f)
    ncorr=0
    nError=0
    P=350 #Trial and error - this results in the minimum transition corrections.
    RecordedBits1 = ''
    for i in range(1,2047):
        Z = bits[i-1]
        Y = bits[i]
        X = bits[i+1]
        if (P>256-D-F) and (P<=256-D):
            RecordedBits1 += complement(Y)

        if (P > 256-D) and (P<256+D):
            #We seem to be lucky that the waveform is asymmetrical in just the right
            #direction... (!) There are errors recorded without bits being wrong.
            if (X == Z):
                print("{:d} Error. XYZ= {:s}{:s}{:s}. P={:d}".format(i,X,Y,X,P))
                nError += 1
            #If before a transition, the transition has already occurred, 
            #adjust phase
            if ((P < 256) and (Y==X)) or ((P > 256) and (Y==Z)):
                ncorr += 1
                print("{:d} Phase Correction={:3d}".format(i, 256-P))
                P += (256-P)*3//2
        Ps[i]=P
        P = (P+F) % 512    
    print(ncorr)

    print("Number of phase corrections: {:d}".format(ncorr))
    print("Number of apparent XOR errors: {:d}".format(nError))
    print(RecordedBits1)

    #RecordedBits1 is either...
    #101011111110 (383)
    #101111111010 (509)
    #111111101010 (1013)