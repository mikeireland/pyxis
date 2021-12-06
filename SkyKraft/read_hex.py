"""
Issues:
1) In "hex.txt" there is a missing pixel on the second row.
2) In "hex.txt" on the third row, there are 3 missing pixels (one block), but 8
clock errors. This can only be an FPGA thing (timing...)
3) In "hex2.txt" there are 4 clock errors and a 3 pixel shift in the first row, 
and a 3+1 pixel shift in the second row. 

*** i.e. in both cases, the second row is missing a pixel! 
Means that the entire row is not always read and synchronisation
is needed at the start... ***

4) In "hex2.txt", there are 2 missing pixels in the 3rd row!

*** An N-bit jump seems to happen sometimes (e.g. N=4), and is
made more common if the bitstream has a greater mix of 1s and 0s.
Does this relate to having >4 bits per cycle in our algorithm??? ***

TO CHECK: 2.2V versus 2V! 
"""
import numpy as np
import matplotlib.pyplot as plt


def reverseBits(num,bitSize=10):   
     #return num
     # convert number into binary representation 
     # output will be like bin(10) = '0b10101' 
     binary = bin(num) 
     # skip first two characters of binary 
     # representation string and reverse 
     # remaining string and then append zeros 
     # after it. binary[-1:1:-1]  --> start 
     # from last character and reverse it until 
     # second last character from left 
     reverse = binary[-1:1:-1] 
     reverse = reverse + (bitSize - len(reverse))*'0'
    
     # converts reversed binary string into integer 
     return int(reverse,2) 

chars = ''
l = '\n'
f = open('hex17.txt', 'r')
while len(l) > 0:
    l = f.readline()
    chars += l[:-1]

raw8 = np.zeros(len(chars)//2, dtype=np.uint8)
raw32 = np.zeros(len(chars)//8, dtype=np.uint32)
flags = np.zeros(len(raw32), dtype=np.uint8)
clocks = np.zeros(len(raw32), dtype=np.uint8)
for i in range(len(raw8)):
    raw8[i] = int(chars[i*2:(i+1)*2],16)
pix = np.zeros(len(raw32)*3, dtype=np.uint16)
oldclock = 2
skippix=0
for i in range(len(raw32)):
    raw32[i] = int.from_bytes(raw8[i*4:(i+1)*4], 'little')
    flags[i] = (raw32[i] & 0x40000000)>>30
    clocks[i] = (raw32[i] & 0x80000000)>>31
    #if clocks[i]==oldclock:
    #    skippix += 3
    #    print(i)
    oldclock = clocks[i]
    if skippix +3*(i+1) >= len(raw32)*3:
        break
    pix[3*i + skippix] = reverseBits(raw32[i] % 1024)
    pix[3*i+1 + skippix] = reverseBits((raw32[i]//1024) % 1024)
    pix[3*i+2 + skippix] = reverseBits((raw32[i]//1024//1024) % 1024)
    
pix2 = pix[:150*249]
pix2 = pix2.reshape((150,249))

pix = pix.reshape((150,252))

bad = (clocks[1:]==clocks[:-1])
bad = np.concatenate(([0],bad))
bad = bad.reshape((150,252//3))
flags2 = flags[:150*249//3]
flags2 = flags2.reshape((150,249//3))
flags = flags.reshape((150,252//3))
clocks = clocks.reshape((150,252//3))