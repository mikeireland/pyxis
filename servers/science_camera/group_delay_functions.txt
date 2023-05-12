//OpenCV used to extract pixels into a 2x(Nx3) matrices (FUNCTION)

// Create 2 cube arrays of N x (3x3) matrices
// Callibrate these cube matrices into PV2M (FUNCTION)

// Result is 2 matrices of size Nx3 (FUNCTION)
    //for i in N:
       // result[i] = cube1.slice(i)*data1[i]

//Convert to coherence (FUNCTION)
// Result is (2x) N size complex vectors
// for i in N:
    //result[i] = 1/result[i,2]*(result[i,0] + i*result[i,1])


//Find delay
//Generate delay matrix beforehand:
// MxN matrix defined by exp(i*2*np.pi*trial_delays[M]/wavelengths[N]) (FUNCTION)
//Then matrix multiplication of MxN matrix with coherences (FUNCTION)

//Finally, find the x relating to the maximum of the |x|^2 of the above result. This is the group delay (FUNCTION)

// Send group delay to robot

//Possibly also phase delay tracking???

//Server functions:
//1. Callibrate PV2M
//2. Calculate trial delays
//3. Callibrate wavelength scale
//4. Find pixel positions
//5. Save callibration data (1->4)
//6. Request group delay
//7. Request group delay envelope (for waterfall plot)