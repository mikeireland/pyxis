import os
import numpy as np
import matplotlib.pyplot as plt

def makeTimeSeriesPlot(time_series_array,name,numberstr):
    fig, ax = plt.subplots()
    ax.plot(time_series_array)
    ax.set_xlabel("time (ms)")
    ax.set_ylabel(name)
    ax.grid(visible = True, which = 'major', axis = 'both', color = 'grey')
    fig.savefig(numberstr+"_"+ name + "_time_series.png",dpi = 300)
    plt.close(fig)

if __name__ == "__main__":
    #Import the data from the stabiliser
    stabiliser_state = np.genfromtxt("stabiliser_state.csv",delimiter = ',',skip_header = 1,dtype = np.float64)
    stabiliser_state_names = np.genfromtxt("stabiliser_state.csv",delimiter = ',',max_rows = 1,dtype = str)
    Nsamples, Nvariables = stabiliser_state.shape
    
    time_series_list = stabiliser_state.transpose()
    
                   
    #We produce and time series plots of each variable with an associated name
    try:
        os.mkdir("StabiliserPlots")
    except FileExistsError:
        print("Warning: StabiliserPlots folder already exists, overwriting files")
    
    os.chdir("StabiliserPlots")
    
    for i in range(Nvariables):
        makeTimeSeriesPlot(time_series_list[i],stabiliser_state_names[i],str(i))