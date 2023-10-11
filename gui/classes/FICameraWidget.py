#!/usr/bin/env python
from __future__ import print_function, division
from BaseFLIRCameraWidget import BaseFLIRCameraWidget
import numpy as np
from sliders import FloatSlider

#Import only what we need from PyQt5, or everything from PyQt4. In any case, we'll try
#to keep this back-compatible. Although this floods the namespace somewhat, everything
#starts with a "Q" so there is little chance of getting mixed up.

try:
    from PyQt5.QtWidgets import QWidget, QPushButton, QHBoxLayout, \
        QVBoxLayout, QLabel
    from PyQt5.QtCore import QPoint, Qt
    from PyQt5.QtGui import QPixmap, QPainter
except:
    print("Please install PyQt5.")
    raise UserWarning

class FeedLabel(QLabel):
    def __init__(self, img):
        super(FeedLabel, self).__init__()
        self.pixmap = QPixmap(img)
        self.dims = (100,100)

    def paintEvent(self, event):
        target_1 = QPoint(594,430)
        target_2 = QPoint(632,628)
        offset = QPoint(420,252)

        size = self.size()
        painter = QPainter(self)
        point = QPoint(0,0)
        scaledPix = self.pixmap.scaled(size, Qt.KeepAspectRatio, transformMode = Qt.FastTransformation)
        # start painting the label from left upper corner
        point.setX(int((size.width() - scaledPix.width())/2))
        point.setY(int((size.height() - scaledPix.height())/2))
        painter.drawPixmap(point, scaledPix)
        x = point.x()
        y = point.y()
        width = scaledPix.width()/self.dims.width()
        height = scaledPix.height()/self.dims.height()

        y1 = y + height*(target_1.y() - offset.y())
        x1 = x + width*(target_1.x() - offset.x())
        y2 = y + height*(target_2.y() - offset.y())
        x2 = x + width*(target_2.x() - offset.x())

        painter.drawEllipse(QPoint(x1,y1),3,3)
        painter.drawEllipse(QPoint(x2,y2),3,3)

    def changePixmap(self, img):
        self.dims = img.size()
        self.pixmap = QPixmap(img)
        self.repaint()

class FeedWindow(QWidget):
    def __init__(self, name, contrast_min, contrast_max):
        super(FeedWindow, self).__init__()

        # Label
        self.resize(900, 500)
        self.setWindowTitle("%s Camera Feed"%name)
        hbox = QHBoxLayout()
        self.cam_feed = FeedLabel("assets/camtest1.png")
        self.binning_flag = 0

        mainvbox = QVBoxLayout()

        hbox.addWidget(self.cam_feed)
        hbox.addSpacing(50)

        vbox = QVBoxLayout()
        vbox.addStretch()
        hbox2 = QHBoxLayout()
        self.binning_button = QPushButton("Binning", self)
        self.binning_button.setCheckable(True)
        self.binning_button.setFixedWidth(200)
        self.binning_button.clicked.connect(self.set_binning)
        
        hbox2.addWidget(self.binning_button)
        vbox.addLayout(hbox2)
        vbox.addStretch()


        hbox2 = QHBoxLayout()
        self.linear_button = QPushButton("Linear Scaling", self)
        self.linear_button.setCheckable(True)
        self.linear_button.setFixedWidth(200)
        self.linear_button.clicked.connect(self.set_linear_func)
        hbox2.addWidget(self.linear_button)
        vbox.addLayout(hbox2)
        vbox.addStretch()
        hbox2 = QHBoxLayout()
        self.asinh_button = QPushButton("Asinh Scaling", self)
        self.asinh_button.setCheckable(True)
        self.asinh_button.setFixedWidth(200)
        self.asinh_button.clicked.connect(self.set_asinh_func)
        hbox2.addWidget(self.asinh_button)
        vbox.addLayout(hbox2)
        vbox.addStretch()

        hbox.addLayout(vbox)

        mainvbox.addLayout(hbox)
        mainvbox.addSpacing(20)
        self.contrast = FloatSlider("Contrast", contrast_min, contrast_max, 1.0, mainvbox)

        self.setLayout(mainvbox)
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



class FICameraWidget(BaseFLIRCameraWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(FICameraWidget,self).__init__(config, IP, parent)

        self.numframes_edit.setText("0")
        contrast_min = config["contrast_limits"][0]
        contrast_max = config["contrast_limits"][1]
        self.feed_window = FeedWindow(self.name, contrast_min, contrast_max)


        hbox3 = QHBoxLayout()
        self.enable_button = QPushButton("Enable Centroiding", self)
        self.enable_button.setCheckable(True)
        self.enable_button.setFixedWidth(200)
        self.enable_button.clicked.connect(self.enable_centroid_loop_button_func)
        hbox3.addWidget(self.enable_button)
        self.sidePanel.addLayout(hbox3)
        
        hbox3 = QHBoxLayout()
        self.DextraTT_button = QPushButton("Dextra T/T", self)
        self.DextraTT_button.setCheckable(True)
        self.DextraTT_button.setFixedWidth(200)
        self.DextraTT_button.clicked.connect(self.switch_state_button_func)
        hbox3.addWidget(self.DextraTT_button)
        self.sidePanel.addLayout(hbox3)
        
        hbox3 = QHBoxLayout()
        self.SinistraTT_button = QPushButton("Sinistra T/T", self)
        self.SinistraTT_button.setCheckable(True)
        self.SinistraTT_button.setFixedWidth(200)
        self.SinistraTT_button.clicked.connect(self.switch_state_button_func)
        hbox3.addWidget(self.SinistraTT_button)
        self.sidePanel.addLayout(hbox3)


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
                self.DextraTT_button.setChecked(False)
                self.SinistraTT_button.setChecked(False)
        else:
            self.run_button.setChecked(False)
            print("CAMERA NOT CONNECTED")


    def switch_state_button_func(self):
        if self.Connect_button.isChecked():
            if self.run_button.isChecked():  
                self.socket.client.RCVTIMEO = 60000   
                if self.DextraTT_button.isChecked():
                    if self.SinistraTT_button.isChecked():
                        self.send_to_server("FI.enable_tiptiltservo [3]")
                        print("Beginning Combined Tip/Tilt Servo Mode")
                    else:
                        self.send_to_server("FI.enable_tiptiltservo [1]")
                        print("Beginning Dextra Tip/Tilt Servo Mode")
                elif self.SinistraTT_button.isChecked():
                    self.send_to_server("FI.enable_tiptiltservo [2]")
                    print("Beginning Sinistra Tip/Tilt Servo Mode")
                else:
                    self.send_to_server("FI.enable_tiptiltservo [0]")
                    print("Beginning Acquisition Mode")
                self.socket.client.RCVTIMEO = 3000
            else:
                self.DextraTT_button.setChecked(False)
                self.SinistraTT_button.setChecked(False)
                print("CAMERA NOT RUNNING")
        else:
            self.DextraTT_button.setChecked(False)
            self.SinistraTT_button.setChecked(False)
            print("CAMERA NOT CONNECTED")

    def enable_centroid_loop_button_func(self):
        if self.Connect_button.isChecked():       
            if self.enable_button.isChecked():
                self.enable_button.setText("Disable centroiding")
                self.send_to_server("FI.enable_centroiding [1]")
                print("Enabling Centroiding")
            else:
                self.enable_button.setText("Enable centroiding")
                self.send_to_server("FI.enable_centroiding [0]")
                print("Disabling Centroiding")
        else:
            self.enable_button.setChecked(False)
            print("CAMERA NOT CONNECTED")
