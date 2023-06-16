#!/usr/bin/env python
from __future__ import print_function, division
import time
import json
import numpy as np
import cv2
import pyqtgraph as pg
from RawWidget import RawWidget

#Import only what we need from PyQt5, or everything from PyQt4. In any case, we'll try
#to keep this back-compatible. Although this floods the namespace somewhat, everything
#starts with a "Q" so there is little chance of getting mixed up.

try:
    from PyQt5.QtWidgets import QWidget, QPushButton, QHBoxLayout, \
        QVBoxLayout, QGridLayout, QLabel, QLineEdit
    from PyQt5.QtCore import QTimer, QPoint, Qt
    from PyQt5.QtGui import QPixmap, QImage, QPainter
except:
    print("Please install PyQt5.")
    raise UserWarning

class FeedLabel(QLabel):
    def __init__(self, img):
        super(FeedLabel, self).__init__()
        self.pixmap = QPixmap(img)
        self.dims = (100,100)

    def paintEvent(self, event):
        size = self.size()
        painter = QPainter(self)
        point = QPoint(0,0)
        scaledPix = self.pixmap.scaled(size, Qt.KeepAspectRatio, transformMode = Qt.FastTransformation)
        # start painting the label from left upper corner
        point.setX((size.width() - scaledPix.width())/2)
        point.setY((size.height() - scaledPix.height())/2)
        painter.drawPixmap(point, scaledPix)
        grid_spacing = 5
        x = point.x()
        y = point.y()
        width = scaledPix.width()
        height = scaledPix.height()
        gridSize_x = grid_spacing*width/self.dims.width()
        gridSize_y = grid_spacing*height/self.dims.height()
        while y <= height+point.y():
            # draw horizontal lines
            painter.drawLine(point.x(), y, width+point.x(), y)
            y += gridSize_y
        while x <= width+point.x():
            # draw vertical lines
            painter.drawLine(x, point.y(), x, height+point.y())
            x += gridSize_x

    def changePixmap(self, img):
        self.dims = img.size()
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

class GDPlotWindow(QWidget):
    def __init__(self):
        super(GDPlotWindow, self).__init__()

        # Label
        self.resize(900, 500)
        self.setWindowTitle("Group Delay Plot Feed")
        hbox = QHBoxLayout()
        self.cam_feed = FeedLabel("assets/camtest1.png")
        hbox.addWidget(self.cam_feed)
        self.setLayout(hbox)

class PlotWindow(QWidget):
    def __init__(self):
        super(PlotWindow, self).__init__()

        # Label
        self.resize(900, 500)
        self.setWindowTitle("V2 Plot Feed")
        hbox = QHBoxLayout()
        self.graphWidget = pg.PlotWidget()
        
        hbox.addSpacing(50)

        vbox = QVBoxLayout()
        hbox2 = QHBoxLayout()
        self.V2_button = QPushButton("V2 per channel", self)
        self.V2_button.setCheckable(True)
        self.V2_button.setFixedWidth(200)
        self.V2_button.clicked.connect(self.set_V2_plot)
        hbox2.addWidget(self.V2_button)
        vbox.addLayout(hbox2)

        hbox2 = QHBoxLayout()
        self.flux_button = QPushButton("Flux", self)
        self.flux_button.setCheckable(True)
        self.flux_button.setFixedWidth(200)
        self.flux_button.clicked.connect(self.set_flux_plot)
        hbox2.addWidget(self.flux_button)
        vbox.addLayout(hbox2)
        
        hbox2 = QHBoxLayout()
        self.V2SNR_button = QPushButton("V2 Total SNR", self)
        self.V2SNR_button.setCheckable(True)
        self.V2SNR_button.setFixedWidth(200)
        self.V2SNR_button.clicked.connect(self.set_V2SNR_plot)
        hbox2.addWidget(self.V2SNR_button)
        vbox.addLayout(hbox2)

        hbox2 = QHBoxLayout()
        self.GD_button = QPushButton("Group Delay", self)
        self.GD_button.setCheckable(True)
        self.GD_button.setFixedWidth(200)
        self.GD_button.clicked.connect(self.set_GD_plot)
        hbox2.addWidget(self.GD_button)
        vbox.addLayout(hbox2)

        hbox.addLayout(vbox)
        
        #Initial plot
        self.set_flux_plot()

        pen = pg.mkPen(width=4)
        self.data_line =  self.graphWidget.plot(self.x, self.y, pen=pen)
        hbox.addWidget(self.graphWidget)
        self.setLayout(hbox)

        
    def set_V2_plot(self):
        self.V2_button.setChecked(True)
        self.flux_button.setChecked(False)
        self.V2SNR_button.setChecked(False)
        self.GD_button.setChecked(False)
        self.func = "getV2array"
        self.funcxlabel = "Channel No."
        self.funcylabel = "V2 Estimate"
        self.graphWidget.setLabel('left', self.funcylabel)
        self.graphWidget.setLabel('bottom', self.funcxlabel)
        self.x = np.arange(20)+1
        self.y = np.zeros(0)
        

    def set_flux_plot(self):
        self.V2_button.setChecked(False)
        self.flux_button.setChecked(True)
        self.V2SNR_button.setChecked(False)
        self.GD_button.setChecked(False)
        self.func = "SC.getFlux"
        self.funcxlabel = "Time"
        self.funcylabel = "Flux"
        self.graphWidget.setLabel('left', self.funcylabel)
        self.graphWidget.setLabel('bottom', self.funcxlabel)
        self.x = np.arange(100)
        self.y = np.zeros(100)

    def set_V2SNR_plot(self):
        self.V2_button.setChecked(False)
        self.flux_button.setChecked(False)
        self.V2SNR_button.setChecked(True)
        self.GD_button.setChecked(False)
        self.func = "SC.getV2SNRestimate"
        self.funcxlabel = "Time"
        self.funcylabel = "V2 SNR"
        self.graphWidget.setLabel('left', self.funcylabel)
        self.graphWidget.setLabel('bottom', self.funcxlabel)
        self.x = np.arange(100)
        self.y = np.zeros(100)

    def set_GD_plot(self):
        self.V2_button.setChecked(False)
        self.flux_button.setChecked(False)
        self.V2SNR_button.setChecked(False)
        self.GD_button.setChecked(True)
        self.func = "SC.getGDestimate"
        self.funcxlabel = "Time"
        self.funcylabel = "Group Delay Estimate"
        self.graphWidget.setLabel('left', self.funcylabel)
        self.graphWidget.setLabel('bottom', self.funcxlabel)
        self.x = np.arange(100)
        self.y = np.zeros(100)
        
 
class ScienceCameraWidget(RawWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(ScienceCameraWidget,self).__init__(config,IP,parent)

        self.feed_refresh_time = int(config["feed_refresh_time"]*1000)
        self.plot_refresh_time = int(config["plot_refresh_time"]*1000)
        self.compression_param = config["compression_param"]

        self.GD_array = np.zeros((config["GD_window_size"],config["num_delays"]),dtype="uint16")
        self.GD_scale = config["GD_scale"]

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
        vbox4.addLayout(hbox1)

        hbox4.addLayout(vbox4)
        self.mainPanel.addLayout(hbox4)

        self.mainPanel.addSpacing(15)
        #################################

        SC_button_grid = QGridLayout()
        SC_button_grid.setColumnMinimumWidth(1,30)
        SC_button_grid.setVerticalSpacing(15)

        self.darks_button = QPushButton("Dark", self)
        self.darks_button.setCheckable(True)
        self.darks_button.setFixedWidth(200)
        self.darks_button.clicked.connect(self.darks_func)
        SC_button_grid.addWidget(self.darks_button,0,0)

        self.flux1_button = QPushButton("Flux 1", self)
        self.flux1_button.setCheckable(True)
        self.flux1_button.setFixedWidth(200)
        self.flux1_button.clicked.connect(self.flux1_func)
        SC_button_grid.addWidget(self.flux1_button,0,1)

        self.flux2_button = QPushButton("Flux 2", self)
        self.flux2_button.setCheckable(True)
        self.flux2_button.setFixedWidth(200)
        self.flux2_button.clicked.connect(self.flux2_func)
        SC_button_grid.addWidget(self.flux2_button,0,2)

        self.target_baseline_button = QPushButton("Target/Baseline Calc", self)
        self.target_baseline_button.setFixedWidth(200)
        self.target_baseline_button.clicked.connect(self.target_baseline_func)
        SC_button_grid.addWidget(self.target_baseline_button,1,0)

        self.p2vm_button = QPushButton("Calc P2VM", self)
        self.p2vm_button.setFixedWidth(200)
        self.p2vm_button.clicked.connect(self.p2vm_func)
        SC_button_grid.addWidget(self.p2vm_button,1,1)

        self.fringe_scan_button = QPushButton("Start Fringe Scan", self)
        self.fringe_scan_button.setCheckable(True)
        self.fringe_scan_button.setFixedWidth(200)
        self.fringe_scan_button.clicked.connect(self.fringe_scan_func)
        SC_button_grid.addWidget(self.fringe_scan_button,1,2)

        self.mainPanel.addLayout(SC_button_grid)

        ####################################

        hbox3 = QHBoxLayout()
        hbox3.addSpacing(50)
        self.Connect_button = QPushButton("Connect", self)
        self.Connect_button.setCheckable(True)
        self.Connect_button.setFixedWidth(200)
        self.Connect_button.clicked.connect(self.connect_camera)
        hbox3.addWidget(self.Connect_button)
        hbox3.addSpacing(50)
        self.sidePanel.addLayout(hbox3)

        hbox3 = QHBoxLayout()
        self.run_button = QPushButton("Start Camera", self)
        self.run_button.setCheckable(True)
        self.run_button.setFixedWidth(200)
        self.run_button.clicked.connect(self.run_camera)
        hbox3.addWidget(self.run_button)
        self.sidePanel.addLayout(hbox3)

        #vbox2 things
        hbox3 = QHBoxLayout()
        self.Reconfigure_button = QPushButton("Reconfigure", self)
        self.Reconfigure_button.setFixedWidth(200)
        self.Reconfigure_button.clicked.connect(self.reconfigure_camera)
        hbox3.addWidget(self.Reconfigure_button)
        self.sidePanel.addLayout(hbox3)

        self.feed_window = FeedWindow(self.name)

        hbox3 = QHBoxLayout()
        self.Camera_button = QPushButton("Start Feed", self)
        self.Camera_button.setCheckable(True)
        self.Camera_button.setFixedWidth(200)
        self.Camera_button.clicked.connect(self.camera_feed)
        hbox3.addWidget(self.Camera_button)
        self.sidePanel.addLayout(hbox3)

        self.GD_window = GDPlotWindow()

        hbox3 = QHBoxLayout()
        self.GD_button = QPushButton("Start GD plotter", self)
        self.GD_button.setCheckable(True)
        self.GD_button.setFixedWidth(200)
        self.GD_button.clicked.connect(self.plot_func)
        hbox3.addWidget(self.GD_button)
        self.sidePanel.addLayout(hbox3)
        
        self.plot_window = PlotWindow()

        hbox3 = QHBoxLayout()
        self.plot_button = QPushButton("Start General plotter", self)
        self.plot_button.setCheckable(True)
        self.plot_button.setFixedWidth(200)
        self.plot_button.clicked.connect(self.plot_func)
        hbox3.addWidget(self.plot_button)
        self.sidePanel.addLayout(hbox3)

        self.feedtimer = QTimer()
        self.GDtimer = QTimer()
        self.plottimer = QTimer()        
        
        self.ask_for_status()
        self.get_params()

    def info_click(self):
        print(self.name)
        self.ask_for_status()
        self.get_params()

    def ask_for_status(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""
        #As this is on a continuous timer, only do anything if we are
        #connected
        response = self.socket.send_command("SC.status")
        if (self.socket.connected):
            self.status_light = "assets/green.svg"
            self.svgWidget.load(self.status_light)

            if response == '"Camera Not Connected!"':
                self.Connect_button.setChecked(False)
                self.Connect_button.setText("Connect")
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
            #self.get_params()

        else:
            self.response_label.append(response)
            self.status_light = "assets/red.svg"
            self.status_text = "Socket Not Connected"
            self.status_label.setText(self.status_text)
            self.svgWidget.load(self.status_light)


    def get_params(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""

        if (self.socket.connected):

            response = self.socket.send_command("SC.getparams")

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


    def connect_camera(self):

        if self.Connect_button.isChecked():
            # Refresh camera
            self.Connect_button.setText("Disconnect")
            print("Camera is now Connected")
            self.send_to_server("SC.connect")
            time.sleep(2)
            self.get_params()

        elif self.run_button.isChecked():
            self.Connect_button.setChecked(True)
            print("Can't Disconnect; Camera Running")

        else:
            self.Connect_button.setText("Connect")
            print("Disconnecting Camera")
            self.send_to_server("SC.disconnect")


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
                self.send_to_server("SC.reconfigure_all [%s]"%response_str)

        else:
            print("CAMERA NOT CONNECTED")

        return


    def darks_func(self):
        if self.Connect_button.isChecked():
            if self.flux1_button.isChecked():
                self.darks_button.setChecked(False)
                print("TURN OFF FLUX1 MODE!")                 
            elif self.flux2_button.isChecked():
                self.darks_button.setChecked(False)
                print("TURN OFF FLUX2 MODE!") 
            elif self.fringe_scan_button.isChecked():
                self.darks_button.setChecked(False)
                print("CURRENTLY FRINGE SCANNING!")                  
            else:
                print("SETTING DARK MODE")
                self.send_to_server("SC.enableDarks [%s]" %int(self.darks_button.isChecked()))
                print(int(self.darks_button.isChecked()))  
        else:
            self.darks_button.setChecked(False)
            print("CAMERA NOT CONNECTED")        
        return

    def flux1_func(self):       
        if self.Connect_button.isChecked():
            if self.darks_button.isChecked():
                self.flux1_button.setChecked(False)
                print("TURN OFF DARK MODE!") 
            elif self.flux2_button.isChecked():
                self.flux1_button.setChecked(False)
                print("TURN OFF FLUX2 MODE!") 
            elif self.fringe_scan_button.isChecked():
                self.flux1_button.setChecked(False)
                print("CURRENTLY FRINGE SCANNING!")     
            else:
                print("SETTING FLUX1 MODE")
                self.send_to_server("SC.enableFluxes [%s]" %int(self.flux1_button.isChecked()))
                print(int(self.flux1_button.isChecked()))  
    
        else:
            self.flux1_button.setChecked(False)
            print("CAMERA NOT CONNECTED")        
        return

    def flux2_func(self):       
        if self.Connect_button.isChecked():
            if self.flux1_button.isChecked():
                self.flux2_button.setChecked(False)
                print("TURN OFF FLUX1 MODE!")                 
            elif self.darks_button.isChecked():
                self.flux2_button.setChecked(False)
                print("TURN OFF DARK MODE!") 
            elif self.fringe_scan_button.isChecked():
                self.flux2_button.setChecked(False)
                print("CURRENTLY FRINGE SCANNING!")     
            else:
                print("SETTING FLUX2 MODE")
                self.send_to_server("SC.enableFluxes [%s]" %int(self.flux2_button.isChecked()*2))
                print(int(self.flux2_button.isChecked()*2))  
        else:
            self.flux2_button.setChecked(False)
            print("CAMERA NOT CONNECTED")        
        return
    
    def fringe_scan_func(self):
        if self.Connect_button.isChecked():
            if self.flux1_button.isChecked():
                self.fringe_scan_button.setChecked(False)
                print("TURN OFF FLUX1 MODE!")                 
            elif self.flux2_button.isChecked():
                self.fringe_scan_button.setChecked(False)
                print("TURN OFF FLUX2 MODE!") 
            elif self.darks_button.isChecked():
                self.fringe_scan_button.setChecked(False)
                print("TURN OFF DARK MODE!")  
            else:  
                if self.fringe_scan_button.isChecked():
                    self.send_to_server("SC.enableFringeScan [1]")
                    print("Starting Fringe Scanning")                 
                else:
                    print("Stopping Fringe Scanning")
                    self.send_to_server("SC.enableFringeScan [0]")
        else:
            self.fringe_scan_button.setChecked(False)
            print("CAMERA NOT CONNECTED")        
        return


    def check_fringe_scan_mode(self):       
        if self.Connect_button.isChecked():
            if self.fringe_scan_button.isChecked():
                response = self.socket.send_command("SC.fringeScanStatus")
                if not response:
                    self.fringe_scan_button.setChecked(False)
                    print("Fringe scan ended")
                else:
                    print("Still fringe scanning")                 
        else:
            self.flux2_button.setChecked(False)
            print("CAMERA NOT CONNECTED")        
        return


    def target_baseline_func(self):
        self.send_to_server("SC.setTargetandBaseline")
        return
    
    def p2vm_func(self):
        self.send_to_server("SC.calcP2VM")
        return

    def run_camera(self):

        if self.Connect_button.isChecked():
            if self.run_button.isChecked():
                # Refresh camera
                #EXTRACT NUMBER OF FRAMES
                self.run_button.setText("Stop Camera")
                print("Starting Camera")
                num_frames = str(self.numframes_edit.text())
                self.send_to_server("SC.start [%s]"%(num_frames))
            else:
                self.run_button.setText("Start Camera")
                print("Stopping Camera")
                self.send_to_server("SC.stop")

        else:
            self.run_button.setChecked(False)
            print("CAMERA NOT CONNECTED")

    def camera_feed(self):
        time.sleep(1)
        self.auto_feed_updater()
        return

    #Function to update feed at a given rate
    def auto_feed_updater(self):
        self.refresh_camera_feed()
        self.feedtimer.singleShot(self.feed_refresh_time, self.auto_feed_updater)
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
                self.Camera_button.setText("Start Feed")
                #print("CAMERA NOT RUNNING")
        else:
            self.Camera_button.setChecked(False)
            self.Camera_button.setText("Start Feed")
            #print("CAMERA NOT CONNECTED")


    def refresh_GD_func(self):
        if self.Connect_button.isChecked():
            if self.run_button.isChecked():
                if self.GD_button.isChecked():
                    # Refresh camera
                    self.GD_window.show()
                    self.GD_button.setText("Stop GD plotter")
                    print("gd")
                    self.get_new_GD()

                else:
                    self.GD_button.setText("Start GD plotter")
            else:
                self.GD_button.setChecked(False)
                self.GD_button.setText("Start GD plotter")
                #print("CAMERA NOT RUNNING")
        else:
            self.GD_button.setChecked(False)
            self.GD_button.setText("Start GD plotter")
            #print("CAMERA NOT CONNECTED")
        return
    

    def plot_func(self):
        time.sleep(1)
        self.auto_plot_updater()
        return

    #Function to update feed at a given rate
    def auto_plot_updater(self):
        self.refresh_plot_func()
        self.refresh_GD_func()
        self.plottimer.singleShot(self.plot_refresh_time, self.auto_plot_updater)
        return
    
    def refresh_plot_func(self):
        self.plot_window.show()
        self.get_new_plot()
        """if self.Connect_button.isChecked():
            if self.run_button.isChecked():
                if self.plot_button.isChecked():
                    # Refresh camera
                    self.plot_window.show()
                    self.plot_button.setText("Stop general plotter")
                    print("v2")

                    self.get_new_plot()

                else:
                    self.plot_button.setText("Start general plotter")
            else:
                self.plot_button.setChecked(False)
                self.plot_button.setText("Start general plotter")
                #print("CAMERA NOT RUNNING")
        else:
            self.plot_button.setChecked(False)
            self.plot_button.setText("Start general plotter")
            #print("CAMERA NOT CONNECTED")
        """
        return

    def get_new_GD(self):
        response = self.socket.send_command("SC.getGDarray")
        data = json.loads(json.loads(response))
        GD_data = np.array(data["GroupDelay"], np.double)
        GD_data = (GD_data*self.GD_scale).astype("uint16")
        temp = np.roll(self.GD_array,1,axis=0)
        temp[0] = GD_data
        self.GD_array = temp
        qimg = QImage(self.GD_array, self.GD_array.shape(0), self.GD_array.shape(1), QImage.Format_Grayscale16)
        ##########
        self.GD_window.cam_feed.changePixmap(qimg)

        
    def get_new_plot(self):
        response = self.socket.send_command(self.plot_window.func)
        if self.plot_window.func == "SC.getV2array":
            data = json.loads(json.loads(response))
            data = np.array(data["V2"], np.double)
            self.plot_window.y = data
        else:
            data = float(response.split[":"][1])
            self.plot_window.x = self.plot_window.x[1:]  # Remove the first y element.
            self.plot_window.x = np.append(self.plot_window.x, self.plot_window.x[-1] + 1)  # Add a new value 1 higher than the last.

            self.plot_window.y = self.plot_window.y[1:]  # Remove the first
            self.plot_window.y = np.append(self.plot_window.y,data)
  
        self.plot_window.data_line.setData(self.plot_window.x, self.plot_window.y)

    def get_new_frame(self):
        #j = random.randint(1, 6)
        #self.feed_window.cam_feed.changePixmap("assets/camtest%s.png"%j)
        response = self.socket.send_command("SC.getlatestimage [%s,%s]"%(self.compression_param,self.feed_window.binning_flag))
        data = json.loads(json.loads(response))
        compressed_data = np.array(data["Image"]["data"], dtype=np.uint8)
        print(len(compressed_data))
        img_data = cv2.imdecode(compressed_data, cv2.IMREAD_UNCHANGED)
        img_data = self.feed_window.image_func(img_data)
        qimg = QImage(img_data.data, data["Image"]["cols"], data["Image"]["rows"], QImage.Format_Grayscale8)
        ##########
        self.feed_window.cam_feed.changePixmap(qimg)
