// Function to calculate pixel positions
// Calculate P2VM matrix
// Calibrate wavelength scale

#include <Eigen/Dense>
#include <functional>
#include <cmath>
#include "setup.hpp"
#include "brent.hpp"
#include "globals.h"

extern const Cd I(0.0,1.0);

Eigen::Matrix<Cd,3,3> GLOB_SC_P2VM_l[20];
int GLOB_SC_P2VM_SIGNS[20];
Eigen::Array<double,20,3> GLOB_SC_FLUX_A = Eigen::Array<double,20,3>::Zero();
Eigen::Array<double,20,3> GLOB_SC_FLUX_B = Eigen::Array<double,20,3>::Zero();
double GLOB_SC_DARK_VAL = 0;

SC_calibration GLOB_SC_CAL;

int addToFlux(unsigned short* data, int flux_flag){

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

int measureDark(unsigned short* data){
    int size = GLOB_IMSIZE;
    float total;
    for(int i=0;i<size; i++){
        total += data[i];
    }
    GLOB_SC_DARK_VAL = total/size;

    return 0;
}

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



// Temp function to create P2VM matrix. In reality, need a proper calibration function to do this.
int calcP2VMMat(Eigen::Array<double,3,2>& IMat, int sign, Eigen::Matrix<Cd,3,3>& P2VM) {

    Eigen::Array<double,3,2> MagE = IMat.sqrt();
    double t1 = acos(MagE(0,0));
    double t2 = atan(MagE(2,0)/MagE(1,0));
    double t3 = asin(-MagE(0,1)/sin(t1));

    double delta_2;
    double delta_4;

    delta_functor myfunc(MagE(1,1), MagE(2,1), t1, t2, t3);

    double delta;
    brent::local_min(0.0, 2*kPi, 1e-12, myfunc, delta);

    delta *= sign;

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

    Eigen::Matrix<Cd,3,1> g_0, g_pi2, g_pi, g_3pi2 ;
    Eigen::Matrix<Cd,3,1> r, i, f ;

    g_0 = 0.5*(CKM*(Eigen::MatrixXcd(3,1) << 1.0, 0.0, exp(I*0.0)).finished()).cwiseAbs2().real();
    g_pi2 = 0.5*(CKM*(Eigen::MatrixXcd(3,1) << 1.0, 0.0, exp(I*kPi/2.0)).finished()).cwiseAbs2().real();
    g_pi = 0.5*(CKM*(Eigen::MatrixXcd(3,1) << 1.0, 0.0, exp(I*kPi)).finished()).cwiseAbs2().real();
    g_3pi2 = 0.5*(CKM*(Eigen::MatrixXcd(3,1) << 1.0, 0.0, exp(I*3.0*kPi/2.0)).finished()).cwiseAbs2().real();

    r = 0.5*(g_pi - g_0);
    i = 0.5*(g_3pi2 - g_pi2);
    f = 0.25*(g_0 + g_pi2 + g_pi + g_3pi2);

    Eigen::Matrix<Cd,3,3> V2PM;

    V2PM << r, i, f;

    P2VM = V2PM.transpose().inverse();

    return 0;

}

int calcP2VMmain(){
    int ret_val;
    for(int k=0;k<20;k++){
        Eigen::Array<double,3,2> Imat;
        Imat.col(0) = (GLOB_SC_FLUX_A.row(k)/(GLOB_SC_FLUX_A.row(k)).sum()).transpose();
        Imat.col(1) = (GLOB_SC_FLUX_B.row(k)/(GLOB_SC_FLUX_B.row(k)).sum()).transpose();
        ret_val = calcP2VMMat(Imat, GLOB_SC_P2VM_SIGNS[k], GLOB_SC_P2VM_l[k]);
    }
    return 0;
}

void extractToMatrix(unsigned short* data, Eigen::Matrix<double, 20, 3> & O) {
    
     // From the frame, extract pixel positions into O(utput) matrix
     for(int k=0;k<10;k++){
        O.row(k) << data[GLOB_SC_CAL.pos_p1_A,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[0]+k]+
                    data[GLOB_SC_CAL.pos_p1_A+1,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[0]+k]- 
                    GLOB_SC_DARK_VAL,

                    data[GLOB_SC_CAL.pos_p1_B,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[1]+k]+
                    data[GLOB_SC_CAL.pos_p1_B+1,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[1]+k]-
                    GLOB_SC_DARK_VAL,

                    data[GLOB_SC_CAL.pos_p1_C,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[2]+k]+
                    data[GLOB_SC_CAL.pos_p1_C+1,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[2]+k]-
                    GLOB_SC_DARK_VAL;

        O.row(k+10) << data[GLOB_SC_CAL.pos_p2_A,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[3]+k]+
                       data[GLOB_SC_CAL.pos_p2_A+1,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[3]+k]-
                       GLOB_SC_DARK_VAL,

                       data[GLOB_SC_CAL.pos_p2_B,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[4]+k]+
                       data[GLOB_SC_CAL.pos_p2_B+1,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[4]+k]-
                       GLOB_SC_DARK_VAL,

                       data[GLOB_SC_CAL.pos_p2_C,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[5]+k]+
                       data[GLOB_SC_CAL.pos_p2_C+1,GLOB_SC_CAL.pos_wave+GLOB_SC_CAL.wave_offset[5]+k]-
                       GLOB_SC_DARK_VAL;
    }   
}

// WAVELENGTHS: 0.6063 0.6186 0.6316 0.6454 0.66 0.6755 0.6918 0.7092 0.7277 0.7473

// Function to get the reference pixel positions.
// xref is location of 660nm (offset of -4 if red end on the right, or +5 if on the left)
// for 630nm, offset of -2 for red end on the right, or +7 if on the left
// yref is location of first row

int setPixelPositions(int xref, int yref) {

    GLOB_SC_CAL.pos_wave = xref - 4;

    GLOB_SC_CAL.pos_p1_A = yref;
    GLOB_SC_CAL.pos_p2_A = yref + 8;
    GLOB_SC_CAL.pos_p1_B = yref + 14;
    GLOB_SC_CAL.pos_p2_B = yref + 22;
    GLOB_SC_CAL.pos_p1_C = yref + 28;
    GLOB_SC_CAL.pos_p2_C = yref + 36;

    double temp_waves [] = {0.6063, 0.6186, 0.6316, 0.6454, 0.66, 0.6755, 0.6918, 0.7092, 0.7277, 0.7473};

    for(int k=0;k<10;k++){
        GLOB_SC_CAL.wavelengths[k] = temp_waves[k];
    }

    return 0;
}
