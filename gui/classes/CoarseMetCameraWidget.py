#!/usr/bin/env python
from __future__ import print_function, division
from client_socket import ClientSocket
import time
import random
import json
import numpy as np
import cv2

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
        QVBoxLayout, QGridLayout, QLabel, QLineEdit, QTextEdit, QProgressBar
    from PyQt5.QtCore import QTimer, QPoint, Qt
    from PyQt5.QtGui import QPixmap, QFont, QImage, QPainter
    from PyQt5.QtSvg import QSvgWidget
except:
    print("Please install PyQt5.")
    raise UserWarning

class FeedLabel(QLabel):
    def __init__(self, img):
        super(FeedLabel, self).__init__()
        self.pixmap = QPixmap(img)

    def paintEvent(self, event):
        size = self.size()
        painter = QPainter(self)
        point = QPoint(0,0)
        scaledPix = self.pixmap.scaled(size, Qt.KeepAspectRatio, transformMode = Qt.SmoothTransformation)
        # start painting the label from left upper corner
        point.setX((size.width() - scaledPix.width())/2)
        point.setY((size.height() - scaledPix.height())/2)
        painter.drawPixmap(point, scaledPix)

    def changePixmap(self, img):
        self.pixmap = QPixmap(img)
        self.repaint()

class FeedWindow(QWidget):
    def __init__(self, name):
        super(FeedWindow, self).__init__()

        # Label
        self.resize(900, 500)
        self.setWindowTitle("%s Camera Feed"%name)
        hbox = QHBoxLayout()
        self.cam_feed = FeedLabel("assets/camtest1.png")
        self.binning_flag = 0


        hbox.addWidget(self.cam_feed)
        hbox.addSpacing(50)

        vbox = QVBoxLayout()
        hbox2 = QHBoxLayout()
        self.binning_button = QPushButton("Binning", self)
        self.binning_button.setCheckable(True)
        self.binning_button.setFixedWidth(200)
        self.binning_button.clicked.connect(self.set_binning)
        hbox2.addWidget(self.binning_button)
        vbox.addLayout(hbox2)


        hbox2 = QHBoxLayout()
        self.linear_button = QPushButton("Linear Scaling", self)
        self.linear_button.setCheckable(True)
        self.linear_button.setFixedWidth(200)
        self.linear_button.clicked.connect(self.set_linear_func)
        hbox2.addWidget(self.linear_button)
        vbox.addLayout(hbox2)

        hbox2 = QHBoxLayout()
        self.asinh_button = QPushButton("Asinh Scaling", self)
        self.asinh_button.setCheckable(True)
        self.asinh_button.setFixedWidth(200)
        self.asinh_button.clicked.connect(self.set_asinh_func)
        hbox2.addWidget(self.asinh_button)
        vbox.addLayout(hbox2)

        hbox.addLayout(vbox)
        self.setLayout(hbox)
        self.image_func = self.linear_func

    def set_binning(self):
        if self.binning_button.isChecked():
            self.binning_flag = 1
        else:
            self.binning_flag = 0

    def set_linear_func(self):
        self.linear_button.setChecked(True)
        self.asinh_button.setChecked(False)
        self.image_func = self.linear_func

    def set_asinh_func(self):
        self.asinh_button.setChecked(True)
        self.linear_button.setChecked(False)
        self.image_func = self.asinh_func

    def asinh_func(self,image):
        A = 16
        B = 16
        bias = np.percentile(image[0:3],30)
        noise = 1.48*np.median(image[0:3]-bias)
        scaled = np.arcsinh( A* (image - bias)/noise) + B
        return scaled.astype("uint8")

    def linear_func(self,image):
        return image




class CoarseMetCameraWidget(QWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(CoarseMetCameraWidget,self).__init__(parent)

        self.name = config["name"]
        self.port = config["port"]
        self.prefix = config["prefix"]
        self.socket = ClientSocket(IP=IP, Port=self.port)
        self.feed_refresh_time = int(config["feed_refresh_time"]*1000)
        self.compression_param = config["compression_param"]

        #Layout the common elements
        vBoxlayout = QVBoxLayout()
        vBoxlayout.setSpacing(3)

        desc = QLabel('Port: %s    Description: %s'%(config["port"],config["description"]), self)
        desc.setStyleSheet("font-weight: bold")

        #First, the command entry box
        lbl1 = QLabel('Command: ', self)
        self.line_edit = QLineEdit("%s."%self.prefix)
        self.line_edit.returnPressed.connect(self.command_enter)

        #Next, the info button
        self.info_button = QPushButton("Refresh", self)
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

        vbox1.addSpacing(20)

        hbox4 = QHBoxLayout()
        vbox4 = QVBoxLayout()
        #Add controls for taking exposures with the camera
        #Missing COLOFFSET and ROWOFFSET because they don't work.
        #Have: COLBIN=, ROWBIN=, EXPTIME=, DARK
        #First row...
        config_grid = QGridLayout()
        config_grid.setColumnMinimumWidth(1,30)
        config_grid.setVerticalSpacing(10)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Width: ', self)
        self.width_edit = QLineEdit("")
        self.width_edit.setFixedWidth(120)
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.width_edit)
        hbox3.addSpacing(20)
        config_grid.addLayout(hbox3,0,0)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('X Offset: ', self)
        self.xoffset_edit = QLineEdit("")
        self.xoffset_edit.setFixedWidth(120)
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.xoffset_edit)
        hbox3.addSpacing(20)
        config_grid.addLayout(hbox3,1,0)


        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Exposure Time: ', self)
        self.expT_edit = QLineEdit("")
        self.expT_edit.setFixedWidth(120)
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.expT_edit)
        hbox3.addSpacing(20)
        config_grid.addLayout(hbox3,2,0)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Black Level: ', self)
        self.BL_edit = QLineEdit("")
        self.BL_edit.setFixedWidth(120)
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.BL_edit)
        hbox3.addSpacing(20)
        config_grid.addLayout(hbox3,2,1)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Height: ', self)
        self.height_edit = QLineEdit("")
        self.height_edit.setFixedWidth(120)
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.height_edit)
        hbox3.addSpacing(20)
        config_grid.addLayout(hbox3,0,1)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Y Offset: ', self)
        self.yoffset_edit = QLineEdit("")
        self.yoffset_edit.setFixedWidth(120)
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.yoffset_edit)
        hbox3.addSpacing(20)
        config_grid.addLayout(hbox3,1,1)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Gain: ', self)
        self.gain_edit = QLineEdit("")
        self.gain_edit.setFixedWidth(120)
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.gain_edit)
        hbox3.addSpacing(20)
        config_grid.addLayout(hbox3,2,2)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Buffer size: ', self)
        self.buffersize_edit = QLineEdit("")
        self.buffersize_edit.setFixedWidth(120)
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.buffersize_edit)
        hbox3.addSpacing(20)
        config_grid.addLayout(hbox3,0,2)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Frames per save: ', self)
        self.numframes_edit = QLineEdit("0")
        self.numframes_edit.setFixedWidth(120)
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.numframes_edit)
        hbox3.addSpacing(20)
        config_grid.addLayout(hbox3,1,2)


        vbox4.addLayout(config_grid)

        vbox4.addSpacing(20)
        hbox1 = QHBoxLayout()
        lbl1 = QLabel('Save Directory: ', self)
        self.save_dir_line_edit = QLineEdit("./")
        hbox1.addWidget(lbl1)
        hbox1.addSpacing(10)
        hbox1.addWidget(self.save_dir_line_edit)
        hbox1.addSpacing(20)
        self.coadd_flag = 0
        self.coadd_button = QPushButton("Coadd Frames", self)
        self.coadd_button.setCheckable(True)
        self.coadd_button.clicked.connect(self.set_coadd)
        hbox1.addWidget(self.coadd_button)
        vbox4.addLayout(hbox1)

        hbox4.addLayout(vbox4)

        hbox3 = QHBoxLayout()
        hbox3.addSpacing(50)
        self.Connect_button = QPushButton("Connect", self)
        self.Connect_button.setCheckable(True)
        self.Connect_button.setFixedWidth(200)
        self.Connect_button.clicked.connect(self.connect_camera)
        hbox3.addWidget(self.Connect_button)
        hbox3.addSpacing(50)
        vbox2.addLayout(hbox3)

        hbox3 = QHBoxLayout()
        self.run_button = QPushButton("Start Camera", self)
        self.run_button.setCheckable(True)
        self.run_button.setFixedWidth(200)
        self.run_button.clicked.connect(self.run_camera)
        hbox3.addWidget(self.run_button)
        vbox2.addLayout(hbox3)

        vbox1.addLayout(hbox4)

        #vbox2 things
        hbox3 = QHBoxLayout()
        self.Reconfigure_button = QPushButton("Reconfigure", self)
        self.Reconfigure_button.setFixedWidth(200)
        self.Reconfigure_button.clicked.connect(self.reconfigure_camera)
        hbox3.addWidget(self.Reconfigure_button)
        vbox2.addLayout(hbox3)

        self.feed_window = FeedWindow(self.name)

        hbox3 = QHBoxLayout()
        self.Camera_button = QPushButton("Start Feed", self)
        self.Camera_button.setCheckable(True)
        self.Camera_button.setFixedWidth(200)
        self.Camera_button.clicked.connect(self.camera_feed)
        hbox3.addWidget(self.Camera_button)
        vbox2.addLayout(hbox3)
        
        hbox3 = QHBoxLayout()
        self.enable_button = QPushButton("Enable Metrology", self)
        self.enable_button.setCheckable(True)
        self.enable_button.setFixedWidth(200)
        self.enable_button.clicked.connect(self.enable_met_loop_button_func)
        hbox3.addWidget(self.enable_button)
        vbox2.addLayout(hbox3)

        hbox2.addLayout(vbox1)
        hbox2.addLayout(vbox2)
        vBoxlayout.addLayout(hbox2)

        status_layout = QHBoxLayout()
        self.status_light = 'assets/red.svg'
        self.status_text = 'Socket Not Connected'
        self.svgWidget = QSvgWidget(self.status_light)
        self.svgWidget.setFixedSize(20,20)
        self.status_label = QLabel(self.status_text, self)
        status_layout.addWidget(self.svgWidget)
        status_layout.addWidget(self.status_label)

        vBoxlayout.addLayout(status_layout)

        self.setLayout(vBoxlayout)
        self.stimer = QTimer()
        self.auto_updater()
        self.ask_for_status()

    def change_ip(self,IP):
        self.socket = ClientSocket(IP=IP, Port=self.port)

    def ask_for_status(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""
        #As this is on a continuous timer, only do anything if we are
        #connected
        response = self.socket.send_command("%s.status"%self.prefix)
        if (self.socket.connected):
            self.status_light = "assets/green.svg"
            self.svgWidget.load(self.status_light)

            if response == '"Camera Not Connected!"':
                self.Connect_button.setChecked(False)
                self.run_button.setChecked(False)
            elif response == '"Camera Connecting"':
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")
                self.run_button.setChecked(False)
            elif response == '"Camera Reconfiguring"':
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")
                self.run_button.setChecked(False)
            elif response == '"Camera Stopping"':
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")
                self.run_button.setChecked(False)
            elif response == '"Camera Waiting"':
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")
                self.run_button.setChecked(False)
            else:
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")
                self.run_button.setChecked(True)
                self.run_button.setText("Stop Camera")

            self.response_label.append(response)
            self.status_text = "Socket Connected"
            self.status_label.setText(self.status_text)
            self.get_params()

        else:
            self.status_light = "assets/red.svg"
            self.status_text = "Socket Not Connected"
            self.status_label.setText(self.status_text)
            self.svgWidget.load(self.status_light)


    def get_params(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""

        if (self.socket.connected):

            response = self.socket.send_command("%s.getparams"%self.prefix)

            if response.startswith("Error receiving response, connection lost"):
                print(response)
            else:
                response_dict = json.loads(response)
                self.width_edit.setText(str(response_dict["width"]))
                self.height_edit.setText(str(response_dict["height"]))
                self.xoffset_edit.setText(str(response_dict["offsetX"]))
                self.yoffset_edit.setText(str(response_dict["offsetY"]))
                self.expT_edit.setText(str(response_dict["exptime"]))
                self.gain_edit.setText(str(response_dict["gain"]))
                self.BL_edit.setText(str(response_dict["blacklevel"]))
                self.buffersize_edit.setText(str(response_dict["buffersize"]))
                self.save_dir_line_edit.setText(str(response_dict["savedir"]))

    #Function to auto update at a given rate
    def auto_updater(self):
        self.refresh_camera_feed()
        self.stimer.singleShot(self.feed_refresh_time, self.auto_updater)
        return

    def connect_camera(self):

        if self.Connect_button.isChecked():
            # Refresh camera
            self.Connect_button.setText("Disconnect")
            print("Camera is now Connected")
            self.send_to_server("%s.connect"%self.prefix)
            time.sleep(2)
            self.get_params()

        elif self.run_button.isChecked():
            self.Connect_button.setChecked(True)
            print("Can't Disconnect; Camera Running")

        else:
            self.Connect_button.setText("Connect")
            print("Disconnecting Camera")
            self.send_to_server("%s.disconnect"%self.prefix)


    def reconfigure_camera(self):
        if self.Connect_button.isChecked():
            if self.run_button.isChecked():
                print("Camera currently running!")
            else:
                response_dict = {}
                response_dict["width"] = int(self.width_edit.text())
                response_dict["height"] = int(self.height_edit.text())
                response_dict["offsetX"] = int(self.xoffset_edit.text())
                response_dict["offsetY"] = int(self.yoffset_edit.text())
                response_dict["exptime"] = float(self.expT_edit.text())
                response_dict["gain"] = float(self.gain_edit.text())
                response_dict["blacklevel"] = float(self.BL_edit.text())
                response_dict["buffersize"] = int(self.buffersize_edit.text())
                response_dict["savedir"] = self.save_dir_line_edit.text()
                print("Reconfiguring Camera")

                response_str = json.dumps(response_dict)
                self.send_to_server("%s.reconfigure_all [%s]"%(self.prefix,response_str))

        else:
            print("CAMERA NOT CONNECTED")


        return



    def run_camera(self):

        if self.Connect_button.isChecked():
            if self.run_button.isChecked():
                # Refresh camera
                #EXTRACT NUMBER OF FRAMES
                self.run_button.setText("Stop Camera")
                print("Starting Camera")
                num_frames = str(self.numframes_edit.text())
                self.send_to_server("%s.start [%s,%s]"%(self.prefix,num_frames,self.coadd_flag))
            else:
                self.run_button.setText("Start Camera")
                print("Stopping Camera")
                self.send_to_server("%s.stop"%self.prefix)

        else:
            self.run_button.setChecked(False)
            print("CAMERA NOT CONNECTED")

    def camera_feed(self):
        time.sleep(1)
        self.refresh_camera_feed()
        return

    def refresh_camera_feed(self):

        if self.Connect_button.isChecked():
            if self.run_button.isChecked():
                if self.Camera_button.isChecked():
                    # Refresh camera
                    self.feed_window.show()
                    self.Camera_button.setText("Stop Feed")

                    self.get_new_frame()

                else:
                    self.Camera_button.setText("Start Feed")
            else:
                self.Camera_button.setChecked(False)
                #print("CAMERA NOT RUNNING")
        else:
            self.Camera_button.setChecked(False)
            #print("CAMERA NOT CONNECTED")


    def get_new_frame(self):
        #j = random.randint(1, 6)
        #self.feed_window.cam_feed.changePixmap("assets/camtest%s.png"%j)
        response = self.socket.send_command("%s.getlatestimage [%s,%s]"%(self.prefix,self.compression_param,self.feed_window.binning_flag))
        data = json.loads(json.loads(response))
        compressed_data = np.array(data["Image"]["data"], dtype=np.uint8)
        print(len(compressed_data))
        img_data = cv2.imdecode(compressed_data, cv2.IMREAD_UNCHANGED)
        print(img_data.dtype)
        img_data = self.feed_window.image_func(img_data)
        qimg = QImage(img_data.data, data["Image"]["cols"], data["Image"]["rows"], QImage.Format_Grayscale8)
        ##########
        self.feed_window.cam_feed.changePixmap(qimg)

    def set_coadd(self):
        if self.coadd_button.isChecked():
            self.coadd_flag = 1
        else:
            self.coadd_flag = 0

    def info_click(self):
        print(self.name)
        self.ask_for_status()

    def enable_met_loop_button_func(self):
        if self.Connect_button.isChecked():       
            if self.enable_button.isChecked():
                self.Connect_button.setText("Disable metrology")
                self.send_to_server("CM.enableLEDs [1]")
                print("Enabling Coarse Met Loop")
            else:
                self.Connect_button.setText("Enable metrology")
                self.send_to_server("CM.enableLEDs [0]")
                print("Disabling Coarse Met Loop")
        else:
            self.enable_button.setChecked(False)
            print("CAMERA NOT CONNECTED")


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
        self.line_edit.setText("%s."%self.prefix)
