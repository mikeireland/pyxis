#!/usr/bin/env python
from __future__ import print_function, division
import random
import numpy as np
from BaseFLIRCameraWidget import BaseFLIRCameraWidget
from sliders import FloatSlider

#Import only what we need from PyQt5, or everything from PyQt4. In any case, we'll try
#to keep this back-compatible. Although this floods the namespace somewhat, everything
#starts with a "Q" so there is little chance of getting mixed up.

try:
    from PyQt5.QtWidgets import QWidget, QPushButton, QHBoxLayout, QVBoxLayout, QLabel
    from PyQt5.QtCore import QPoint, Qt
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
        grid_spacing = 44
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
        self.contrast = FloatSlider("Contrast", contrast_min, contrast_max, 1.0, mainvbox, label_widt=40)

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


class temp_cam_widget(BaseFLIRCameraWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(temp_cam_widget,self).__init__(config,IP,parent)

        self.feed_window = FeedWindow(self.name)
        self.camera_feed()


    def refresh_camera_feed(self):
        # Refresh camera
        self.feed_window.show()
        self.Camera_button.setText("Stop Feed")
        self.get_new_frame()

    def get_new_frame(self):
        j = random.randint(1, 6)
        qImg = QImage("assets/camtest%s.png"%j)
        self.feed_window.cam_feed.changePixmap(qImg)
