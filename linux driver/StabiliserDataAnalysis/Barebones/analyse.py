import os
import time
import numpy as np
from scipy.fft import rfft
import matplotlib.pyplot as plt

dt = 0.001

def main():
    file_dir = os.getcwd()

    #Import the data from the stabiliser
    stabiliser_state = np.genfromtxt("stabiliser_state.csv",delimiter = ',',skip_header = 1,dtype = np.float64)
    stabiliser_state_names = np.genfromtxt("stabiliser_state.csv",delimiter = ',',max_rows = 1,dtype = str)
    Nsamples, Nvariables = stabiliser_state.shape
    
    time_series_list = stabiliser_state.transpose()
    
                   
    #We produce time series plots of each variable with an associated name
    try:
        os.mkdir("StabiliserPlots")
    except FileExistsError:
        print("Warning: StabiliserPlots folder already exists, overwriting files")
    
    os.chdir("StabiliserPlots")
    
    for i in range(Nvariables):
        makeTimeSeriesPlot(time_series_list[i],stabiliser_state_names[i],str(i))

    
    #We produce power spectrums of the accelerometer data and store the associated plots
    os.chdir(file_dir)
    #We produce time series plots of each variable with an associated name
    try:
        os.mkdir("FourierPlots")
    except FileExistsError:
        print("Warning: FourierPlots folder already exists, overwriting files")

    os.chdir("FourierPlots")

    for i in range(12,30):
        makePowerSpectrumPlot(time_series_list[i],stabiliser_state_names[i],str(i))

    #We produce the integrals of accelerometer data and store the associated plots
    os.chdir(file_dir)
    #We produce time series plots of each variable with an associated name
    try:
        os.mkdir("FirstIntegralPlots")
    except FileExistsError:
        print("Warning: FirstIntegralPlots folder already exists, overwriting files")

    os.chdir("FirstIntegralPlots")

    for i in range(12,30):
        integral_series = computeIntegralSeries(time_series_list[i]-time_series_list[i+24])
        makeTimeSeriesPlot(integral_series,stabiliser_state_names[i]+"_FI",str(i))

    os.chdir(file_dir)
    #We produce time series plots of each variable with an associated name
    try:
        os.mkdir("SecondIntegralPlots")
    except FileExistsError:
        print("Warning: SecondIntegralPlots folder already exists, overwriting files")

    os.chdir("SecondIntegralPlots")

    for i in range(12,30):
        integral_series = computeIntegralSeries(time_series_list[i]-time_series_list[i+24])
        integral_series = computeIntegralSeries(integral_series)
        makeTimeSeriesPlot(integral_series,stabiliser_state_names[i]+"_SI",str(i))



def makeTimeSeriesPlot(time_series_array,name,numberstr):
    fig, ax = plt.subplots()
    ax.plot(time_series_array)
    ax.set_xlabel("time (ms)")
    ax.set_ylabel(name)
    ax.grid(b = True, which = 'major', axis = 'both', color = 'grey')
    fig.savefig(numberstr+"_"+ name + "_time_series.png",dpi = 300)
    plt.close(fig)

def makePowerSpectrumPlot(time_series_array,name,numberstr):
    fourier_array = rfft(time_series_array)
    f_spacing = 1000/fourier_array.shape[0]
    f_array = f_spacing*np.array([i for i in range(fourier_array.shape[0])],dtype = np.float64)

    fig, ax = plt.subplots()
    ax.plot(f_array,np.log10(np.abs(fourier_array)**2))
    ax.set_xlim([0,500])    
    ax.set_xlabel("Frequency (Hz)")
    ax.set_ylabel(name)
    ax.grid(b = True, which = 'major', axis = 'both', color = 'grey')
    fig.savefig(numberstr+"_"+ name + "_fourier_plot.png",dpi = 300)
    plt.close(fig)



def computeIntegralSeries(time_series_array):
    buffer_array = np.zeros_like(time_series_array)

    for i in range(1,time_series_array.shape[0]):
        buffer_array[i] = buffer_array[i-1]+dt*time_series_array[i] 
        #the factor of dt corresponds to all of our time intervals being 1ms
    return buffer_array

if __name__ == "__main__":
    main()    

