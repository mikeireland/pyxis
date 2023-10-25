#include <Eigen/Dense>
#include <functional>
#include <cmath>
#include "setup.hpp"
#include "brent.hpp"
#include "globals.h"
#include <vector>
#include <deque>
#include <iostream>
#include <fstream>
#include <complex>
#include <commander/commander.h>
#include <fftw3.h>
#include <pthread.h>

using json = nlohmann::json;
using namespace Eigen;

extern const Cd I(0.0,1.0);

// list of all P2VM Matrices (for each polarisation (2) and wavelength channel (10))
Eigen::Matrix<Cd,3,3> GLOB_SC_P2VM_l[20];

// Array of fluxes of only Dextra input
Eigen::Array<double,20,3> GLOB_SC_FLUX_A = Eigen::Array<double,20,3>::Zero();

// Array of fluxes of only Dextra input
Eigen::Array<double,20,3> GLOB_SC_FLUX_B = Eigen::Array<double,20,3>::Zero();

// Dark value
double GLOB_SC_DARK_VAL = 0;

// Total flux value
double GLOB_SC_TOTAL_FLUX = 0;

// Science camera calibration struct
SC_calibration GLOB_SC_CAL;

pthread_mutex_t GLOB_SC_FLAG_LOCK = PTHREAD_MUTEX_INITIALIZER;

/*
Function to save all the relevant setup and calibration data to a csv file for easy loading
Inputs:
    fileName - filename in which to save the data
Saves the dark value, as well as the individual fluxes of each deputy in which to calculate the P2VMs
*/
void saveData(std::string fileName)
{
	//https://eigen.tuxfamily.org/dox/structEigen_1_1IOFormat.html
	const static IOFormat CSVFormat(FullPrecision, DontAlignCols, "; ", "\n");

	std::ofstream file(fileName);
	if (file.is_open())
	{
	    file << GLOB_SC_DARK_VAL;
	    file << "\n";
		file << GLOB_SC_FLUX_A.format(CSVFormat);
		file << "\n";
		file << GLOB_SC_FLUX_B.format(CSVFormat);
		file.close();
	}
}

/*
Function to read all the relevant setup and calibration data from a CSV file
Inputs:
    fileToOpen- filename in which to openmthe data
Reads the dark value, as well as the individual fluxes of each deputy in which to calculate the P2VMs
*/
void readData(std::string fileToOpen)
{

	// the inspiration for creating this function was drawn from here (I did NOT copy and paste the code)
	// https://stackoverflow.com/questions/34247057/how-to-read-csv-file-and-assign-to-eigen-matrix
	
	// the input is the file: "fileToOpen.csv":
	// a,b,c
	// d,e,f
	// This function converts input file data into the Eigen matrix format
	// the matrix entries are stored in this variable row-wise. For example if we have the matrix:
	// M=[a b c 
	//	  d e f]
	// the entries are stored as matrixEntries=[a,b,c,d,e,f], that is the variable "matrixEntries" is a row vector
	// later on, this vector is mapped into the Eigen matrix format
	std::vector<double> matrixEntries;

	// in this object we store the data from the matrix
	std::ifstream matrixDataFile(fileToOpen);

	// this variable is used to store the row of the matrix that contains commas 
	std::string matrixRowString;

	// this variable is used to store the matrix entry;
	std::string matrixEntry;

    // READ DARK VALUE
    getline(matrixDataFile, matrixRowString);
    GLOB_SC_DARK_VAL = stod(matrixRowString);
    
    // this variable is used to track the number of rows
	int matrixRowNumber = 0;

    // READ FLUX A
	while (matrixRowNumber < 20) // here we read a row by row of matrixDataFile and store every line into the string variable matrixRowString
	{
	    getline(matrixDataFile, matrixRowString);
		std::stringstream matrixRowStringStream(matrixRowString); //convert matrixRowString that is a string to a stream variable.

		while (getline(matrixRowStringStream, matrixEntry, ';')) // here we read pieces of the stream matrixRowStringStream until every comma, and store the resulting character into the matrixEntry
		{
		    matrixEntries.push_back(stod(matrixEntry));   //here we convert the string to complex double and fill in the row vector storing all the matrix entries
		}
		matrixRowNumber++; //update the column numbers
	}

    GLOB_SC_FLUX_A = Eigen::Map<Eigen::Matrix<double,20,3, RowMajor>>(matrixEntries.data(), matrixRowNumber, matrixEntries.size() / matrixRowNumber);

        // this variable is used to track the number of rows
	matrixRowNumber = 0;
	matrixEntries.clear();

    // READ FLUX B
	while (matrixRowNumber < 20) // here we read a row by row of matrixDataFile and store every line into the string variable matrixRowString
	{
	    getline(matrixDataFile, matrixRowString);
		std::stringstream matrixRowStringStream(matrixRowString); //convert matrixRowString that is a string to a stream variable.

		while (getline(matrixRowStringStream, matrixEntry, ';')) // here we read pieces of the stream matrixRowStringStream until every comma, and store the resulting character into the matrixEntry
		{
		    matrixEntries.push_back(stod(matrixEntry));   //here we convert the string to complex double and fill in the row vector storing all the matrix entries
		}
		matrixRowNumber++; //update the column numbers
	}
    GLOB_SC_FLUX_B = Eigen::Map<Eigen::Matrix<double,20,3, RowMajor>>(matrixEntries.data(), matrixRowNumber, matrixEntries.size() / matrixRowNumber);

	// here we convet the vector variable into the matrix and return the resulting object, 
	// note that matrixEntries.data() is the pointer to the first memory location at which the entries of the vector matrixEntries are stored;
	return;

}

/*
Function to add the tricoupler flux to a vector when only one input is injected
Inputs
    data - raw camera frame
    flux_flag - which input is injected? (1 for Dextra, 2 for Sinistra)
Outputs:
    Saves added flux values to GLOB_SC_FLUX_Y (where Y is either A or B depending on the deputy)
    Returns 1 on error (for bad flux_flag value)
*/
int addToFlux(unsigned short* data, int flux_flag){

    // Extract data
    Eigen::Matrix<double, 20, 3> O;
    extractToMatrix(data,O); 

    if (flux_flag == 1){
        GLOB_SC_FLUX_A += O.array();
    } else if (flux_flag == 2){
        GLOB_SC_FLUX_B += O.array();
    } else {
        return 1;
    }
    return 0;
}

/*
Function to measure and save a science camera dark frame. Simply takes the mean of the entire frame
Inputs:
    data - raw science camera data
Outputs:
    Saves the data in GLOB_SC_DARK_VAL
*/
int measureDark(unsigned short* data){
    int size = GLOB_IMSIZE;
    float total = 0;
    for(int i=0;i<size; i++){
        total += data[i];
    }
    GLOB_SC_DARK_VAL = total/size;

    return 0;
}

/*
Functions used to immitate the Scipy minimise scalar function, using the Brent library
*/
// Function to minimise to find x
double delta_func(double a, double b, double t1, double t2, double t3, double x){
    double delta_2 = a - abs(cos(t1)*cos(t2)*sin(t3) + sin(t2)*cos(t3)*exp(I*x));
    double delta_4 = b - abs(cos(t1)*sin(t2)*sin(t3) - cos(t2)*cos(t3)*exp(I*x));
    return delta_2*delta_2 + delta_4*delta_4;
}

class delta_functor : public brent::func_base   //create functor
{
    public:
        double A;
        double B;
        double T1;
        double T2;
        double T3;
        virtual double operator() (double x)
        {
            return delta_func(A,B,T1,T2,T3,x);
        }
        delta_functor(double a,double b, double t1, double t2, double t3) {A=a;B=b;T1=t1;T2=t2;T3=t3;}
};


/*
Temp function to create P2VM matrix for a single polarisation and wavelength. In reality, may need a proper calibration function to do this.
Based on the formulation in the Pyxis paper and tricoupler paper (Hansen et al. 2023, JATIS; Hansen et al. 2022, JATIS)
Inputs:
    IMat - 3 x 2 vector of the raw outputs vs inputs of the tricoupler (3 outputs, 2 inputs)
    P2VM - output 3x3 P2VM matrix
Outputs:
    Stores the P2VM matrix in the P2VM variable
*/
int calcP2VMMat(Eigen::Array<double,3,2>& IMat, Eigen::Matrix<Cd,3,3>& P2VM) {

    // Convert from intensity to modulus amplitude
    Eigen::Array<double,3,2> MagE = IMat.sqrt();

    // What are the theta parameters?
    double t1 = acos(MagE(0,0));
    double t2 = atan(MagE(2,0)/MagE(1,0));
    double t3 = asin(-MagE(0,1)/sin(t1));

    // Work out the delta parameter through a Scipy "minimise scalar"-like algorithm
    delta_functor myfunc(MagE(1,1), MagE(2,1), t1, t2, t3);
    double delta;
    brent::local_min(0.0, 2*kPi, 1e-12, myfunc, delta);

    // CKM parameterisation of the transfer matrix
    Eigen::Matrix<Cd,3,3> CKM; 

    CKM(0,0) = cos(t1);
    CKM(0,1) = -sin(t1)*cos(t3);
    CKM(0,2) = -sin(t1)*sin(t3);

    CKM(1,0) = sin(t1)*cos(t2);
    CKM(1,1) = cos(t1)*cos(t2)*cos(t3) - sin(t2)*sin(t3)*exp(I*delta);
    CKM(1,2) = cos(t1)*cos(t2)*sin(t3) + sin(t2)*cos(t3)*exp(I*delta);

    CKM(2,0) = sin(t1)*sin(t2);
    CKM(2,1) = cos(t1)*sin(t2)*cos(t3) + cos(t2)*sin(t3)*exp(I*delta);
    CKM(2,2) = cos(t1)*sin(t2)*sin(t3) - cos(t2)*cos(t3)*exp(I*delta);

    // Work out the estimated complex coherence at four different phases
    Eigen::Matrix<Cd,3,1> g_0, g_pi2, g_pi, g_3pi2 ;
    Eigen::Matrix<Cd,3,1> r, i, f ; // Real, imaginary, flux

    // Calculate the complex coherence vectors
    g_0 = 0.5*(CKM*(Eigen::MatrixXcd(3,1) << 1.0, 0.0, exp(I*0.0)).finished()).cwiseAbs2().real();
    g_pi2 = 0.5*(CKM*(Eigen::MatrixXcd(3,1) << 1.0, 0.0, exp(I*kPi/2.0)).finished()).cwiseAbs2().real();
    g_pi = 0.5*(CKM*(Eigen::MatrixXcd(3,1) << 1.0, 0.0, exp(I*kPi)).finished()).cwiseAbs2().real();
    g_3pi2 = 0.5*(CKM*(Eigen::MatrixXcd(3,1) << 1.0, 0.0, exp(I*3.0*kPi/2.0)).finished()).cwiseAbs2().real();

    // Get the real, imaginary and flux vectors
    r = 0.5*(g_pi - g_0);
    i = 0.5*(g_3pi2 - g_pi2);
    f = 0.25*(g_0 + g_pi2 + g_pi + g_3pi2);

    // V2PM matrix
    Eigen::Matrix<Cd,3,3> V2PM;
    V2PM << r, i, f;

    // Invert to get P2VM
    P2VM = V2PM.inverse();

    return 0;

}

/*
Function to calculate the P2VM matrix for each polarisation (2) and wavlength channel (10). Uses the relevant flux ratios
Also saves the calibration fluxes and dark used to calculate the matrices to a file
Saves each one as an element in the GLOB_SC_P2VM_l list
Inputs:
    P2VM_file - filename of the saved calibration data
*/
int calcP2VMmain(std::string P2VM_file){
    int ret_val;
    for(int k=0;k<20;k++){
        Eigen::Array<double,3,2> Imat;
        // Get ratios
        Imat.col(0) = (GLOB_SC_FLUX_A.row(k)/(GLOB_SC_FLUX_A.row(k)).sum()).transpose();
        Imat.col(1) = (GLOB_SC_FLUX_B.row(k)/(GLOB_SC_FLUX_B.row(k)).sum()).transpose();
        ret_val = calcP2VMMat(Imat, GLOB_SC_P2VM_l[k]);
    }
    saveData(P2VM_file); // Save data
    return 0;
}

/*
Function to read in the calibration arrays and calculate the P2VMs from a file
Inputs:
    P2VM_file - filename of the saved calibration data
*/
int readP2VMmain(std::string P2VM_file){
    int ret_val;
    readData(P2VM_file);
    calcP2VMmain(P2VM_file);
    return 0;
}

/*
Function to extract the relevant pixel values from a raw frame and sort them into a usable array.
Relies on the GLOB_SC_CAL struct for where the relevant pixels are.
Inputs: 
    data - raw science camera frame
    O - array to save data to. Consists of a 20x3 matrix, with the rows being each of the 2 polarisations and 10
        wavelengths, and the 3 columns being the 3 tricoupler outputs.
Outputs:
    Saves the data to the matrix O
    Also saves total flux to GLOB_SC_TOTAL_FLUX
*/
void extractToMatrix(unsigned short* data, Eigen::Matrix<double, 20, 3> & O) {
     
     // From the frame, extract pixel positions into O(utput) matrix
     // Will add two consecutive rows together
     for(int k=0;k<10;k++){
        O.row(k) << data[GLOB_SC_CAL.pos_p1_A*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[0]+k]+
                    data[(GLOB_SC_CAL.pos_p1_A+1)*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[0]+k]- 
                    2*GLOB_SC_DARK_VAL,

                    data[GLOB_SC_CAL.pos_p1_B*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[1]+k]+
                    data[(GLOB_SC_CAL.pos_p1_B+1)*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[1]+k]-
                    2*GLOB_SC_DARK_VAL,

                    data[GLOB_SC_CAL.pos_p1_C*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[2]+k]+
                    data[(GLOB_SC_CAL.pos_p1_C+1)*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[2]+k]-
                    2*GLOB_SC_DARK_VAL;

        O.row(k+10) << data[GLOB_SC_CAL.pos_p2_A*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[3]+k]+
                       data[(GLOB_SC_CAL.pos_p2_A+1)*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[3]+k]-
                       2*GLOB_SC_DARK_VAL,

                       data[GLOB_SC_CAL.pos_p2_B*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[4]+k]+
                       data[(GLOB_SC_CAL.pos_p2_B+1)*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[4]+k]-
                       2*GLOB_SC_DARK_VAL,

                       data[GLOB_SC_CAL.pos_p2_C*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[5]+k]+
                       data[(GLOB_SC_CAL.pos_p2_C+1)*GLOB_WIDTH+GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[5]+k]-
                       2*GLOB_SC_DARK_VAL;
    }

    pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
    GLOB_SC_TOTAL_FLUX = O.sum();
    pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
    
}

/*
Function to set the reference pixel positions.
The following central wavelengths for each channel have been calculated 
using snells law and the given QHY camera setup

WAVELENGTHS: 0.6063 0.6186 0.6316 0.6454 0.66 0.6755 0.6918 0.7092 0.7277 0.7473
Inputs:
    xref - location of the anchoring wavelength (the leftmost wavelength used)
           This is calculated using an offset from a reference wavelength:
           for 630nm, set the xref offset to be -7 if red end on the left, or +2 if on the right
           for 660nm, set the xref offset to be -5 if red end on the left, or +4 if on the right
    yref - location of first row
Outputs:
    Saves all positions to the GLOB_SC_CAL struct
*/
int setPixelPositions(int xref, int yref) {

    GLOB_SC_CAL.pos_wave = xref - 7; // Set reference wavelength pixel. Change offset for a different wavelength!

    // Set the position of each row (first row only! It will sum the row below too!)
    GLOB_SC_CAL.pos_p1_A = yref;
    GLOB_SC_CAL.pos_p2_A = yref + 8;
    GLOB_SC_CAL.pos_p1_B = yref + 14;
    GLOB_SC_CAL.pos_p2_B = yref + 22;
    GLOB_SC_CAL.pos_p1_C = yref + 28;
    GLOB_SC_CAL.pos_p2_C = yref + 36;

    //double temp_waves [] = {0.6063, 0.6186, 0.6316, 0.6454, 0.66, 0.6755, 0.6918, 0.7092, 0.7277, 0.7473};
    double temp_waves [] = {0.7473, 0.7277, 0.7092, 0.6918, 0.6755, 0.66, 0.6454, 0.6316, 0.6186, 0.6063};

    for(int k=0;k<10;k++){
        GLOB_SC_CAL.wavelengths[k] = temp_waves[k];
    }

    return 0;
}

/*
THE FOLLOWING FUNCTIONS ARE AN ALTERNATIVE WAY TO DO A FRINGE SCAN USING FFTs

double GLOB_SC_SCAN_FFT_LS[6][6];

static double * scan_fft_in;
static fftw_complex * scan_fft_out;
static fftw_plan scan_fft_plan;

int init_fringe_scan(){

    scan_fft_in = (double*) fftw_malloc(sizeof(double) * 10);
    scan_fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (6));
    scan_fft_plan = fftw_plan_dft_r2c_1d(10, scan_fft_in, scan_fft_out, FFTW_MEASURE);

    return 0;
}


int fringeScan(unsigned short* data){

    Eigen::Matrix<double, 20, 3> O;
   
    extractToMatrix(data,O);
    std::cout << O << std::endl;
    double power_spec;
    std::complex<double> temp;    
   
    for (int k=0;k<6;k++){
        std::vector<double> temp_data(O.col(k/2).data()+(10*k%2), O.col(k/2).data()+(10*k%2)+10);

        std::copy(temp_data.begin(), temp_data.end(), scan_fft_in);
        fftw_execute(scan_fft_plan);    
               
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        for (int l=0;l<6;l++){
            temp = std::complex<double>(scan_fft_out[l][0], scan_fft_out[l][1]);
            power_spec = std::abs( std::pow(temp, 2) );
            GLOB_SC_SCAN_FFT_LS[k][l] = power_spec;
        }
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
    }
   
    //CALC FFT OVER

    return 0;
}

int fringeScanAndSave(unsigned short* data){

    Eigen::Matrix<double, 20, 3> O;
    json j;
    extractToMatrix(data,O);
    std::cout << O << std::endl;
    double power_spec;
    std::complex<double> temp;    
   
    for (int k=0;k<6;k++){
        std::vector<double> temp_data(O.col(k/2).data()+(10*k%2), O.col(k/2).data()+(10*k%2)+10);

        std::copy(temp_data.begin(), temp_data.end(), scan_fft_in);
        fftw_execute(scan_fft_plan);    
               
        pthread_mutex_lock(&GLOB_SC_FLAG_LOCK);
        for (int l=0;l<6;l++){
            temp = std::complex<double>(scan_fft_out[l][0], scan_fft_out[l][1]);
            power_spec = std::abs( std::pow(temp, 2) );
            GLOB_SC_SCAN_FFT_LS[k][l] = power_spec;
        }
        std::vector<double> vec (GLOB_SC_SCAN_FFT_LS[k], GLOB_SC_SCAN_FFT_LS[k]+6);
        j["FFT"][k] = vec;
        pthread_mutex_unlock(&GLOB_SC_FLAG_LOCK);
    }
    std::string s = j.dump();
    std::ofstream myfile;
    myfile.open ("fringe_scan.txt",std::ios_base::app);
    myfile << s << "\n";
    myfile.close();
   
    //CALC FFT OVER

    return 0;
}

*/