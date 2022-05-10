#!/usr/bin/env python
from __future__ import print_function, division
import sys
import socket
import struct
import numpy as np
import pdb
import time
import pytomlpp
import collections

try:
    try:
        import astropy.io.fits as pyfits
    except:
        import pyfits
    FITS_SAVING=True
except:
    FITS_SAVING=False
try:
    import zmq
except:
    print("Please install zmq, e.g. with 'pip install --user zmq' if you don't have sudo privliges.")
    raise UserWarning

#Import only what we need from PyQt5, or everything from PyQt4. In any case, we'll try
#to keep this back-compatible. Although this floods the namespace somewhat, everything
#starts with a "Q" so there is little chance of getting mixed up.
try:
    from PyQt4.QtGui import *
    from PyQt4.QtCore import *
except:
    try:
        from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QHBoxLayout, \
            QVBoxLayout, QLabel, QLineEdit, QTextEdit, QComboBox, QTabWidget, \
            QCheckBox, QProgressBar
        from PyQt5.QtCore import pyqtSlot, QTimer
        from PyQt5.QtGui import QImage, QPixmap, QFont
    except:
        print("Please install PyQt4 or PyQt5.")
        raise UserWarning

#A hack to allow unicode as a type from python 2 to work in python 3.
#It would be better to typecast everything to str
if (sys.version_info > (3, 0)):
    unicode = str

if len(sys.argv) > 1:
    config = pytomlpp.load(sys.argv[1])
else:
    config = pytomlpp.load("gui_ports_setup.toml")

pyxis_config = config["Pyxis"]
config.pop("Pyxis")
config = collections.OrderedDict({k: config[k] for k in ["Navis","Dextra","Sinistra"]})


#Some constants. Data types for Ian's communication protocol, and the servers
#we'll be commecting to.
DTYPES = {1:float, 2:int, 3:str, 4:bool, 5:"floatimg", 6:"intimg"}

def debug_trace():
  '''Set a tracepoint in the Python debugger that works with Qt.

  This is useful for bugshooting in a Gui environment.'''
  try:
    from PyQt4.QtCore import pyqtRemoveInputHook
  except:
    from PyQt5.QtCore import pyqtRemoveInputHook
  from pdb import set_trace
  pyqtRemoveInputHook()
  set_trace()

class ClientSocket:
    def __init__(self,IP="127.0.0.1",Port="44010"):
        """A socket
        """
        ADS = (IP,Port)
        self.count=0
        try:
            self.context = zmq.Context()
            self.client = self.context.socket(zmq.REQ)
            tcpstring = "tcp://"+IP+":"+Port
            self.client.connect(tcpstring)
            self.client.RCVTIMEO = 20000
            self.connected=True
        except:
            print('ERROR: Could not connect to server. Please check that the server is running and IP is correct.')
            self.connected=False

    def send_command(self, command):
        """Send a command to one of Ian's servers. """

        #If we aren't connected and the user pressed <Enter>, just try to reconnect
        if (self.connected==False) and (len(command)==0):
            try:
                response = self.client.recv()
            except:
                self.count += 1
                return "Could not receive buffered response - connection still lost ({0:d} times).".format(self.count)
            self.connected=True
            return "Connection re-established!"

        #Send a command to the client.
        try:
            self.client.send_string("Req#"+command,zmq.NOBLOCK)
        except:
            self.connected=False
            self.count += 1
            return 'Error sending command, connection lost ({0:d} times).'.format(self.count)

        #Receive the response
        try:
            response = self.client.recv()
        except:
            self.connected=False
            self.count += 1
            return 'Error receiving response, connection lost ({0:d} times)\nPress Enter to reconnect.'.format(self.count)
        try:
            self.connected=True
            #Lets see what data type we have, and support all relevant ones.
            if len(response) > 4:
                data_type = struct.unpack("<I", response[:4])[0]
            if DTYPES[data_type]==str:
                str_response = response[4:].decode(encoding='utf-8')
                return str_response
            if DTYPES[data_type]==bool:
                bool_response = struct.unpack("<I", response[4:8])
                return bool(bool_response)
            elif DTYPES[data_type]=="intimg":
                #For an integer image, data starts with the number of rows and
                #columns, the time of exposure then the exposure time (in s)
                if len(response) > 28:
                    rows_cols = struct.unpack("<II", response[4:12])
                    times = struct.unpack("dd", response[12:28])
                npix = rows_cols[0]*rows_cols[1]
                if len(response) < 28+npix*2:
                    return 'Not enough pixels to unpack!'
                data = struct.unpack("<{:d}H".format(npix), response[28:28+npix*2])
                return [rows_cols, times, np.array(data).reshape(rows_cols)]
            else:
                return 'Unsupported response type'
        except:
            return 'Error parsing response!'


class VeloceGui(QTabWidget):
    def __init__(self, IP='127.0.0.1', parent=None):
        """The Veloce GUI.

        Parameters
        ----------
        IP: str
            The IP address of the Veloce server as as string
        """
        super(VeloceGui,self).__init__(parent)

        print("Connecting to IP %s"%IP)
        #Set up sockets. Each of the cameras needs a timeout and file number
        self.sockets={}

        #We'll have tabs for different servers
        self.resize(900, 550)
        self.tab_widgets = {}
        vBoxlayouts = {}
        self.line_edits = {}
        self.info_buttons = {}
        self.response_labels = {}
        self.progress_bars = {}
        self.exposure_bars = {}
        self.sub_tab_widgets = {}

        for tab in config:
            self.tab_widgets[tab] = QTabWidget()
            self.sub_tab_widgets[tab] = {}

            for item in config[tab]:
                sub_config = config[tab][item]
                name = sub_config["name"]
                self.sub_tab_widgets[tab][name] = QWidget()
                self.sockets[name] = ClientSocket(IP=IP, Port=sub_config["port"])

                self.line_edits[name] = QLineEdit("")
                self.info_buttons[name] = QPushButton("INFO", self)

                #Layout the common elements
                vBoxlayouts[name] = QVBoxLayout()
                vBoxlayouts[name].setSpacing(3)

                desc = QLabel('Port: %s    Description: %s'%(sub_config["port"],sub_config["description"]), self)

                #First, the command entry box
                lbl1 = QLabel('Command: ', self)
                self.line_edits[name].returnPressed.connect(self.command_enter)

                #Next, the info button
                self.info_buttons[name].clicked.connect(self.info_click)

                hbox2 = QHBoxLayout()
                hbox2.setContentsMargins(0, 0, 0, 0)
                desc.setFixedHeight(40)
                #desc.adjustSize()
                hbox2.addWidget(desc)
                vBoxlayouts[name].addLayout(hbox2)

                hbox1 = QHBoxLayout()
                hbox1.addWidget(lbl1)
                hbox1.addWidget(self.line_edits[name])
                hbox1.addWidget(self.info_buttons[name])
                vBoxlayouts[name].addLayout(hbox1)

                #Next, the response box
                self.response_labels[name] = QTextEdit('[No Sever Response Yet]', self)
                self.response_labels[name].setReadOnly(True)
                self.response_labels[name].setStyleSheet("QTextEdit { background-color : black; color : lime; }")
                self.response_labels[name].setFixedHeight(150)
                vBoxlayouts[name].addWidget(self.response_labels[name])

                if sub_config["gui_type"] == "camera":
                    self.add_camera_gui(name,vBoxlayouts)


        # Complete setup, add status labels and add tabs to the GUI
        self.status_labels = {}

        for tab in config:

            for item in config[tab]:
                sub_config = config[tab][item]
                name = sub_config["name"]
                self.status_labels[name] = QLabel("", self)
                vBoxlayouts[name].addWidget(self.status_labels[name])
                self.sub_tab_widgets[tab][name].setLayout(vBoxlayouts[name])
                self.tab_widgets[tab].addTab(self.sub_tab_widgets[tab][name],sub_config["tab_name"])

            self.addTab(self.tab_widgets[tab],tab)

        #Now show everything, and start status timers.
        self.setWindowTitle("Pyxis Server Gui")
        self.stimer = QTimer()
        self.ask_for_status()


    def add_camera_gui(self,name,vBoxlayouts):
        self.timeouts={name:0}
        self.fnum={name:0}
        bigfont = QFont("Times", 20, QFont.Bold)

        #Add controls for taking exposures with the camera
        #Missing COLOFFSET and ROWOFFSET because they don't work.
        #Have: COLBIN=, ROWBIN=, EXPTIME=, DARK
        #First row...
        hbox1 = QHBoxLayout()
        lbl1 = QLabel('Save Directory: ', self)
        self.save_dir_line_edit = QLineEdit("./")
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.save_dir_line_edit)
        vBoxlayouts[name].addLayout(hbox1)

        #3rd Row...
        hbox1 = QHBoxLayout()
        lbl1 = QLabel('Exp. Time (ms): ', self)
        self.exptime_line_edit = QLineEdit("0")
        self.expose_button = QPushButton("Expose", self)
        self.expose_button.clicked.connect(self.expose_button_click)
        self.readout_button = QPushButton("Save Fits", self)
        self.readout_button.clicked.connect(self.readout_button_click)
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.exptime_line_edit)
        hbox1.addWidget(self.expose_button)
        hbox1.addWidget(self.readout_button)
        vBoxlayouts[name].addLayout(hbox1)

        #Exposure and readout progress bars ...
        self.exposure_bars[name] = QProgressBar(self)
        #self.exposure_bars["Rosso"].setMaximum(1)
        self.progress_bars[name] = QProgressBar(self)
        hbox1 = QHBoxLayout()
        lbl1 = QLabel('Exposure: ', self)
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.exposure_bars[name])
        vBoxlayouts[name].addLayout(hbox1)
        hbox1 = QHBoxLayout()
        lbl1 = QLabel('Readout:  ', self)
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.progress_bars[name])
        vBoxlayouts[name].addLayout(hbox1)


    def get_current_tab(self):
        tab_index = self.currentIndex()
        tab_name, sub_config = list(config.items())[tab_index]
        sub_tab_index = self.tab_widgets[tab_name].currentIndex()
        return list(sub_config.items())[sub_tab_index][1]["name"]


    def ask_for_status(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""
        command = "INFO"
        name = self.get_current_tab()
        #As this is on a continuous timer, only do anything if we are
        #connected
        if (self.sockets[name].connected):
            response = self.sockets[name].send_command("INFO")
            if type(response)!=str and type(response)!=unicode:
                raise UserWarning("Incorrect INFO response!")
            if response[:5]=='Error':
                print("Error in INFO response from {:s}...".format(name))
            else:
                status_list = response.split('\n')
                if len(status_list)<3:
                    status_list = response.split(' ')
                status = {t.split("=")[0].lstrip():t.split("=")[1] for t in status_list if t.find("=")>0}
                #Now deal with the response in a different way for each server.
                #INSERT HERE!!!

        #Restart the timer to ask for status again.
        self.stimer.singleShot(2000, self.ask_for_status)


    def info_click(self):
        print(self.get_current_tab())
        self.send_to_server("INFO")


    def expose_button_click(self):
        """Start an exposure. Note that once the data have been taken, this does not
        collect the data.
        """
        name = self.get_current_tab()
        #Check inputs
        try:
            exptime = float(self.exptime_line_edit.text())
        except:
            self.response_labels[name].setText("Gui ERROR: exposure time must be a float")
            return

        expstring = "EXPOSE EXPTIME={:f}".format(exptime)
        self.timeouts[name] = time.time() + exptime + 45
        try:
            response = self.sockets[name].send_command(expstring)
        except:
            response = "*** Connection Error ***"
        if type(response)==str or type(response)==unicode:
            self.response_labels[name].setText(response)
        elif type(response)==bool:
            if response:
                self.response_labels[name].setText("Exposure Started")
            else:
                self.response_labels[name].setText("Error...")


    def readout_button_click(self):
        """With an exposure complete, read out the data and save as a fits file.
        """
        name=self.get_current_tab()
        try:
            response = self.sockets[name].send_command("READOUT")
        except:
            response = "*** Connection Error ***"
            return
        if type(response)==str or type(response)==unicode:
            self.response_labels[name].setText(response)
            return
        if type(response)!=list:
            self.response_labels[name].setText("Did not receive an image!")
            return
        if FITS_SAVING:
            header = pyfits.Header()
            header['LOCTIME'] = time.ctime(response[1][0])
            header['EXPTIME'] = response[1][1]
            pyfits.writeto(str(self.save_dir_line_edit.text()) + name+ "{:03d}.fits".format(self.fnum[name]),
                response[2], header, clobber=True)
        else:
            print("Need pyfits to actually save the file!")
        self.fnum[name] += 1


    def command_enter(self):
        """Parse the LineEdit string and send_to_server
        """
        name = self.get_current_tab()
        self.send_to_server(str(self.line_edits[name].text()))


    def send_to_server(self, text):
        """Send a command to the server, dependent on the current tab.
        """
        name = self.get_current_tab()
        try:
            response = self.sockets[name].send_command(text)
        except:
            response = "*** Connection Error ***"
        if type(response)==str or type(response)==unicode:
            self.response_labels[name].setText(response)
        elif type(response)==bool:
            if response:
                self.response_labels[name].setText("Success!")
            else:
                self.response_labels[name].setText("Failure!")
        self.line_edits[name].setText("")


app = QApplication(sys.argv)
myapp = VeloceGui(IP=pyxis_config["server_ip"])
myapp.show()
sys.exit(app.exec_())
