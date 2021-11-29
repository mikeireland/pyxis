import numpy as np
#import matplotlib.pyplot as plt

infile0='bitstream0.txt'
infile1='bitstream1.txt'

def read_bitstream_01(file):
    """Lets encapsulate the reading of the bistream so that 
    altarnative formats can be supported.
    """
    d = f.read()[:-1]
    return d

def complement(achar):
    """Convenience function to complement bits that are recorded as
    text characters '0' and '1'"""
    if achar=='0':
        return '1'
    return '0'

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
for i in range(1,2047):
    Z = bits[i-1]
    Y = bits[i]
    X = bits[i+1]
    
    #Count Transitions
    if X != Y:
        C += 1
    
    #The following code only works if there is a 
    #stream of 0s and provides the starting point...
    if (AdjustFrequency==1):
        if (X=='0') and (Y=='1') and (Z=='0'):
            P = 128
        if (X=='1') and (Y=='0') and (Z=='1'):
            P = 128 + 256
    
        if i==255:
            F = C
            print("Frequency: {:d}/512".format(F))
            print("Initial Phase: {:d}".format(P))
            D = (256-F)/2
            AdjustFrequency=0
    else:
        #Record the 0s...
        if (P>D) and (P<=256-D):
            RecordedBits += complement(Y)

        if (P > 256-D) and (P<256+D):
            #We seem to be lucky that the waveform is asymmetrical in just the right
            #direction... (!)
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

print("Number of phase corrections: {:d}".format(ncorr))
print("Number of apparent XOR errors: {:d}".format(nError))
print(RecordedBits)


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
    if (P>D) and (P<=256-D):
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