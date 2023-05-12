import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

data = pd.read_csv('x_res_2.csv')

for i in ['0', '1', '2', '3', '4', '5']:
    for ax in ['x', 'y', 'z']:
        plt.plot(data['accelerometer'+i+'_'+ax].to_numpy())
        plt.show()
