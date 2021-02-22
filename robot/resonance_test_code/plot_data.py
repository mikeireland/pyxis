import os
from os.path import dirname, abspath
import shutil
import numpy as np
import matplotlib.pyplot as plt

# Configuration variables
# ===============================================
# filename: name of the log file
#
# target_fourier_axis: determines which axis' position is plotted
#       'x_pos'
#       'y_pos'
#       'z_pos'
#
filename = 'x_axis_2021-02-22_160744.log'
target_fourier_axis = 'z_pos'

# ===============================================
filename = dirname(dirname(abspath(__file__))) + '/resonance_test_results/' + filename 

# Parse the serial data into dicts
raw_data = {}
with open(filename) as file:
    
    entry = {}
    reading = False
    
    for line in file:
        
        line = line.strip()
        
        if line == 'SEND START;':
            reading = True
            entry = {}
            
        elif reading:
            
            if line.startswith('f;'):
                entry['f'] = float(line.split(';')[1])
                
            elif line[0] in ('t', 'x', 'y', 'z'):
                vals = line.split(';')
                var = vals[0];
                vals = list(map(float, vals[1:-1]))
                entry[var] = vals
                
            elif line == 'SEND END;':
                reading = False
                raw_data[entry['f']] = entry
                
            else: raise Exception('BAD INPUT!')

name_prefix = filename.split("/")[-1].split("_")[0]
results_directory = filename.split(".")[0] + "_" + target_fourier_axis

if os.path.isdir(results_directory):
    shutil.rmtree(results_directory)
os.mkdir(results_directory) 

def integrate(derivs, ts):
    
    derivs -= np.mean(derivs)
    
    assert(len(derivs) == len(ts))
    vals = [0]
    for i in range(len(derivs)-1):
        vals.append(vals[-1] + derivs[i]*(ts[i+1]-ts[i]))
    return vals

# Convert from accelerations to positions
for f, entry in raw_data.items():
    
    entry['t'] = (np.array(entry['t']) - entry['t'][0]) / 1000000
    
    entry['x_vel'] = integrate(entry['x'], entry['t'])
    entry['x_pos'] = integrate(entry['x_vel'], entry['t'])
    entry['y_vel'] = integrate(entry['y'], entry['t'])
    entry['y_pos'] = integrate(entry['y_vel'], entry['t'])
    entry['z_vel'] = integrate(entry['z'], entry['t'])
    entry['z_pos'] = integrate(entry['z_vel'], entry['t'])
    
plot_f = 10.0

fs, xs, ys, zs = [], [], [], []
for f, entry in raw_data.items():
    fs.append(f)
    xs.append(np.mean(np.abs(entry['x_pos'])))
    ys.append(np.mean(np.abs(entry['y_pos'])))
    zs.append(np.mean(np.abs(entry['z_pos'])))
fs = np.array(fs)
xs = np.array(xs)
ys = np.array(ys)
zs = np.array(zs)


# Generate a bode plot of fundamental frequency responses
bode_magnitudes = []
bode_phases = []
bode_frequencies = []

for f in fs:
        dt_average = np.mean(np.array(raw_data[f]['t'])[1:] - np.array(raw_data[f]['t'])[:-1])
        vals = np.fft.fft(raw_data[f][target_fourier_axis]) * 2 * 1e6
        vals /= int(len(vals))
        freq = np.fft.fftfreq(len(vals), d=dt_average)
        freq = [x for x in freq if x >= 0]
        vals = vals[:len(freq)]
        
        # Filter out anything below fundamental
        threshold = 0
        for i_2, f_2 in enumerate(freq):
            if f_2 > f*0.9:
                threshold = i_2
                break
            
        vals = vals[threshold:]
        freq = freq[threshold:]   
        bode_frequencies.append(f)
        
        peak_index = 0
        for peak_index, f_2 in enumerate(freq):
            if f_2 >= f:
                break
            
        peak_sum = np.sum(vals[peak_index-2:peak_index+2])
        bode_magnitudes.append(np.abs(peak_sum))
        bode_phases.append(np.angle(peak_sum))

fig, axs = plt.subplots(2)
fig.suptitle(f'Bode Plot of Target "{name_prefix}" in {target_fourier_axis}\n Note: input amplitude decreased as 1/f')
axs[0].plot(bode_frequencies, bode_magnitudes)
axs[0].set(ylabel='Magnitude [microns]')
axs[1].plot(bode_frequencies, bode_phases)
axs[1].set(xlabel='Frequency [Hz]', ylabel='Phase [rad]')
plt.savefig(f'{results_directory}/Bode_plot.png')


# Generate a total of 20 fourier plots for each integer frequency
for f in fs:
    if f == int(f):

        dt_average = np.mean(np.array(raw_data[f]['t'])[1:] - np.array(raw_data[f]['t'])[:-1])
        vals = np.fft.fft(raw_data[f][target_fourier_axis]) * 2 * 1e6
        vals /= int(len(vals))
        freq = np.fft.fftfreq(len(vals), d=dt_average)
        freq = [x for x in freq if x >= 0]
        vals = vals[:len(freq)]
        plt.figure(f'Fourier Plot for {f} Hz')
        plt.title(f'Fourier Plot of Target "{name_prefix}" in {target_fourier_axis}\nfrequency = {f} Hz')
        plt.xscale("log")
        
        # Filter out anything below fundamental
        threshold = 0
        for i_2, f_2 in enumerate(freq):
            if f_2 > f*0.9:
                threshold = i_2
                break
            
        vals = vals[threshold:]
        freq = freq[threshold:]
        plt.plot(freq, np.abs(vals))
        plt.xlabel('Frequency [Hz]')
        plt.ylabel('Magnitude (microns)')
        plt.savefig(f'{results_directory}/fourier_plot_{f}.png')
        plt.show()


