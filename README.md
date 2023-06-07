# pyxis
Control software and utilities for the Pyxis project

Note that there are some other repositories that should also be merged, in particular:
https://github.com/raplonu/commander

# TO INSTALL:
1 Install ubuntu  (minimal installation) (username: pyxisuser, computername: pyxisuser-Nameofcomputer, password: *****)

When prompted, ask Jonah for Pro credentials

2 Run: sudo pro enable realtime-kernel
(requires restart)

3 Run: sudo apt-get install build-essential curl git file pkg-config swig \
       libcairo2-dev libnetpbm10-dev netpbm libpng-dev libjpeg-dev \
       zlib1g-dev libbz2-dev libcfitsio-dev wcslib-dev libavcodec58 libavformat58 \
libswscale5 libswresample3 libavutil56 libusb-1.0-0 \
libpcre2-16-0 libdouble-conversion3 libxcb-xinput0 \
libxcb-xinerama0 libopencv-dev libgsl-dev

4 Download and install Spinnaker

5 Download and install Miniconda (Python 3.9); requires making the file an executable

6 Download Pyxis git repo

7 pip install conan==1.59 cmake numpy scipy astropy qt-material matplotlib pyqt5 pyzmq tomli pytomlpp opencv-python astroquery pyqtgraph

8 make all relevant servers in git repo. Check they compile!

9 Add updated 00-teensy rules file to /dev (file inside pyxis repo)
sudo cp 00-teensy.rules /etc/udev/rules.d/00-teensy.rules

10 Add second user (pyxisuser2)

11 Repeat steps 5->7 for second user

12 ON NAVIS ONLY: run install QHY script on first user
