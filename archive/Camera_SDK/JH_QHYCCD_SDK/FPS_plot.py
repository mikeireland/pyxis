import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib

data = pd.read_csv("~/Documents/pyxis/Camera_SDKs/JH_QHYCCD_SDK/data/FPS_Test.csv")

array = np.array(data).T
bool_arr = array[4] < 2000
array = array[:,bool_arr]

plt.scatter(array[0,:]/1000,array[4,:],c=array[1,:],cmap="viridis",norm=matplotlib.colors.LogNorm())
#plt.xscale("log")
x = np.linspace(1,100,10000)
plt.plot(x,1000/x,"k-",label="Exposure Time Limited")
plt.xlabel("Exposure Time (ms)")
plt.ylabel("Frame Rate (Hz)")
cbar = plt.colorbar()
cbar.set_label('Image Dimensions (px)', rotation=270)
plt.title("FLIR FPS Response")
plt.legend()

plt.show()



