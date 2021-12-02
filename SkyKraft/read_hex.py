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
f = open('hex2.txt', 'r')
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
for i in range(len(raw32)):
    raw32[i] = int.from_bytes(raw8[i*4:(i+1)*4], 'little')
    pix[3*i] = reverseBits(raw32[i] % 1024)
    pix[3*i+1] = reverseBits((raw32[i]//1024) % 1024)
    pix[3*i+2] = reverseBits((raw32[i]//1024//1024) % 1024)
    flags[i] = (raw32[i] & 0x40000000)>>30
    clocks[i] = (raw32[i] & 0x80000000)>>31

pix = pix.reshape((150,252))

bad = (clocks[1:]==clocks[:-1])
