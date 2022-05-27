#!/usr/bin/env python
from __future__ import print_function, division
from client_socket import ClientSocket
import time
import random

try:
    try:
        import astropy.io.fits as pyfits
    except:
        import pyfits
    FITS_SAVING=True
except:
    FITS_SAVING=False

#Import only what we need from PyQt5, or everything from PyQt4. In any case, we'll try
#to keep this back-compatible. Although this floods the namespace somewhat, everything
#starts with a "Q" so there is little chance of getting mixed up.

try:
    from PyQt5.QtWidgets import QWidget, QPushButton, QHBoxLayout, \
        QVBoxLayout, QLabel, QLineEdit, QTextEdit, QProgressBar
    from PyQt5.QtGui import QPixmap, QFont
    from PyQt5.QtSvg import QSvgWidget
except:
    print("Please install PyQt5.")
    raise UserWarning


class CameraWidget(QWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(CameraWidget,self).__init__(parent)

        self.name = config["name"]
        self.socket = ClientSocket(IP=IP, Port=config["port"])

        #Layout the common elements
        vBoxlayout = QVBoxLayout()
        vBoxlayout.setSpacing(3)

        desc = QLabel('Port: %s    Description: %s'%(config["port"],config["description"]), self)
        desc.setStyleSheet("font-weight: bold")

        #First, the command entry box
        lbl1 = QLabel('Command: ', self)
        self.line_edit = QLineEdit("")
        self.line_edit.returnPressed.connect(self.command_enter)

        #Next, the info button
        self.info_button = QPushButton("INFO", self)
        self.info_button.clicked.connect(self.info_click)

        hbox2 = QHBoxLayout()
        vbox1 = QVBoxLayout()
        vbox2 = QVBoxLayout()

        hbox1 = QHBoxLayout()
        hbox1.setContentsMargins(0, 0, 0, 0)
        desc.setFixedHeight(40)
        #desc.adjustSize()
        hbox1.addWidget(desc)
        vBoxlayout.addLayout(hbox1)

        hbox1 = QHBoxLayout()
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.line_edit)
        hbox1.addWidget(self.info_button)
        vbox1.addLayout(hbox1)

        #Next, the response box
        self.response_label = QTextEdit('[No Server Response Yet]', self)
        self.response_label.setReadOnly(True)
        self.response_label.setStyleSheet("QTextEdit { background-color : black; }")
        self.response_label.setFixedHeight(150)
        vbox1.addWidget(self.response_label)

        self.timeout = 0
        self.fnum = 0

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
        vbox1.addLayout(hbox1)

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
        vbox1.addLayout(hbox1)

        #Exposure and readout progress bars ...
        self.exposure_bar = QProgressBar(self)
        #self.exposure_bars["Rosso"].setMaximum(1)
        self.progress_bar = QProgressBar(self)
        hbox1 = QHBoxLayout()
        lbl1 = QLabel('Exposure: ', self)
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.exposure_bar)
        vbox1.addLayout(hbox1)
        hbox1 = QHBoxLayout()
        lbl1 = QLabel('Readout:  ', self)
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.progress_bar)
        vbox1.addLayout(hbox1)

        #vbox2 things
        self.cam_feed = QLabel(self)
        self.cam_feed.setStyleSheet("padding: 30")
        self.cam_qpix = QPixmap()
        self.get_new_frame()
        vbox2.addWidget(self.cam_feed)
        hbox3 = QHBoxLayout()
        self.Camera_button = QPushButton("Start Feed", self)
        self.Camera_button.setCheckable(True)
        self.Camera_button.setFixedWidth(200)
        self.Camera_button.clicked.connect(self.refresh_camera_feed)
        hbox3.addWidget(self.Camera_button)
        vbox2.addLayout(hbox3)

        hbox2.addLayout(vbox1)
        hbox2.addLayout(vbox2)
        vBoxlayout.addLayout(hbox2)

        status_layout = QHBoxLayout()
        self.status_light = 'assets/green.svg'
        self.status_text = 'STATUS'
        self.svgWidget = QSvgWidget(self.status_light)
        self.svgWidget.setFixedSize(20,20)
        self.status_label = QLabel(self.status_text, self)
        status_layout.addWidget(self.svgWidget)
        status_layout.addWidget(self.status_label)

        vBoxlayout.addLayout(status_layout)

        self.setLayout(vBoxlayout)


    def ask_for_status(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""
        command = "INFO"
        #As this is on a continuous timer, only do anything if we are
        #connected
        k = random.randint(0, 1)
        #if (self.socket.connected):
        if k:
            self.status_light = "assets/green.svg"
            self.svgWidget.load(self.status_light)
            response = self.socket.send_command("INFO")
            self.status_text = "Socket Connected; STATUS:"
            self.status_label.setText(self.status_text)
            if type(response)!=str and type(response)!=unicode:
                raise UserWarning("Incorrect INFO response!")
            if response[:5]=='Error':
                print("Error in INFO response from {:s}...".format(self.name))
            else:
                status_list = response.split('\n')
                if len(status_list)<3:
                    status_list = response.split(' ')
                status = {t.split("=")[0].lstrip():t.split("=")[1] for t in status_list if t.find("=")>0}
                #Now deal with the response in a different way for each server.
                self.status_text = "Text: {:6.3f} Tset: {:6.3f} Tmc: {:6.3f}".format(\
                    float(status["externalenclosure"]),\
                    float(status["externalenclosure.setpoint"]),\
                    float(status["minichiller.internal"]))
                self.status_label.setText(self.status_text)
        else:
            self.status_light = "assets/red.svg"
            self.status_text = "Socket Not Connected"
            self.status_label.setText(self.status_text)
            self.svgWidget.load(self.status_light)


    def refresh_camera_feed(self):

        if self.Camera_button.isChecked():
            # Refresh camera
            self.Camera_button.setText("Stop Feed")
            self.get_new_frame()

        else:
            self.Camera_button.setText("Start Feed")

    def get_new_frame(self):
        j = random.randint(1, 6)
        self.send_to_server("GET FRAME")
        self.cam_qpix.load("assets/camtest%s.png"%j)
        self.cam_feed.setPixmap(self.cam_qpix.scaledToWidth(300))


    def info_click(self):
        print(self.name)
        self.ask_for_status()
        self.send_to_server("INFO")


    def command_enter(self):
        """Parse the LineEdit string and send_to_server
        """
        self.send_to_server(str(self.line_edit.text()))


    def send_to_server(self, text):
        """Send a command to the server, dependent on the current tab.
        """
        try:
            response = self.socket.send_command(text)
        except:
            response = "*** Connection Error ***"
        if type(response)==str or type(response)==unicode:
            self.response_label.append(response)
        elif type(response)==bool:
            if response:
                self.response_label.append("Success!")
            else:
                self.response_label.append("Failure!")
        self.line_edit.setText("")


    def expose_button_click(self):
        """Start an exposure. Note that once the data have been taken, this does not
        collect the data.
        """
        #Check inputs
        try:
            exptime = float(self.exptime_line_edit.text())
        except:
            self.response_label.append("Gui ERROR: exposure time must be a float")
            return

        expstring = "EXPOSE EXPTIME={:f}".format(exptime)
        self.timeout = time.time() + exptime + 45
        try:
            response = self.socket.send_command(expstring)
        except:
            response = "*** Connection Error ***"
        if type(response)==str or type(response)==unicode:
            self.response_label.append(response)
        elif type(response)==bool:
            if response:
                self.response_label.append("Exposure Started")
            else:
                self.response_label.append("Error...")


    def readout_button_click(self):
        """With an exposure complete, read out the data and save as a fits file.
        """
        try:
            response = self.sockets[name].send_command("READOUT")
        except:
            response = "*** Connection Error ***"
            return
        if type(response)==str or type(response)==unicode:
            self.response_label.append(response)
            return
        if type(response)!=list:
            self.response_label.append("Did not receive an image!")
            return
        if FITS_SAVING:
            header = pyfits.Header()
            header['LOCTIME'] = time.ctime(response[1][0])
            header['EXPTIME'] = response[1][1]
            pyfits.writeto(str(self.save_dir_line_edit.text()) + name+ "{:03d}.fits".format(self.fnum[name]),
                response[2], header, clobber=True)
        else:
            print("Need pyfits to actually save the file!")
        self.fnum += 1
