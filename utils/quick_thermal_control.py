"""
Remmber to cose the labjack when done.
LJ.close()
"""
import u3
import numpy as np
import time

gain = 0.1              #In fractional heater/K, which is W/K for 
integral_gain = gain*0.01    #Integral term: W/K/s. Set gain*2*response_time
servo_dt = 0.1
target_T = 35.0
N_print = 20

#-----
def calc_T_double(a0, R0=10, T0=298, B=3920, Vref=5):
    """Given a differential voltage across a Wheatstone bridge, 
    return a temperature. 
    """
    R1 = R0*(Vref - a0)/(Vref + a0)
    T1 = 1/(1.0/T0 + np.log(R1/R0)/B)
    return T1-273
    
def get_T():
    return calc_T_double(LJ.getAIN(6,7))

def set_heater(val):
    if val<0:
        val=0
    if val>1:
        val=1
    LJ.getFeedback( u3.Timer0Config(TimerMode = 0, Value = int(65536*(1-val)) ))
    return

#Set up LabJack.   
try:
    LJ.close()
    print("Closing then re-opening Labjack connection")
except:
    print("Labjack not yet opened - opening now.")

LJ = u3.U3()
LJ.configIO(FIOAnalog = 15+64+128, TimerCounterPinOffset=4, NumberOfTimersEnabled=1)

#61 Hz PWM.
LJ.configTimerClock(TimerClockDivisor=1, TimerClockBase=0)

#Start with the PWM off. On is 0, i.e. backwards. 
LJ.getFeedback( u3.Timer0Config(TimerMode = 0, Value = 65535) )

T_error_int = 0
count = 0
try:
    while(True):
        T_now = get_T()
        T_error = T_now - target_T
        T_error_int += T_error
        heater_val =  0.5 - T_error*gain - integral_gain*servo_dt*T_error_int
        if heater_val>1:
            heater_val=1
        if heater_val<0:
            heater_val=0
        set_heater( heater_val )
        count += 1
        if count==N_print:
            print("{:5.1f}C, {:5.3f}W".format(T_now, heater_val))
            count=0
        time.sleep(servo_dt) 
except KeyboardInterrupt:
    LJ.close()