#Simple script to plot the GD servo data

import csv
import numpy as np
import matplotlib.pyplot as plt

file = open("GD_servo_data_3.txt")

csvreader = csv.reader(file)

rows = []

for row in csvreader:
    rows.append(row)
    
data = np.array(rows).astype(float)

cum_steps = np.cumsum(data[:,1])

plt.plot(data[:,1]*20/1000)
plt.show()


