#!/usr/bin/env python
from __future__ import print_function, division
import time
import json
import numpy as np
import cv2
from RawWidget import RawWidget
from sliders import FloatSlider

try:
    from PyQt5.QtWidgets import QWidget, QPushButton, QHBoxLayout, \
        QVBoxLayout, QGridLayout, QLabel, QLineEdit
    from PyQt5.QtCore import QTimer, QPoint, Qt
    from PyQt5.QtGui import QPixmap, QImage, QPainter
except:
    print("Please install PyQt5.")
    raise UserWarning

class FeedLabel(QLabel):
    def __init__(self, img, point_ls, offset):
        super(FeedLabel, self).__init__()
        self.pixmap = QPixmap(img)
        self.dims = (100,100)
        self.offset = offset
        self.point_ls = point_ls

    def paintEvent(self, event):

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

        if len(self.point_ls > 0):
            for point in self.point_ls:
                y1 = y + height*(point.y() - self.offset.y())
                x1 = x + width*(point.x() - self.offset.x())

                painter.drawEllipse(QPoint(x1,y1),3,3)

    def changePixmap(self, img):
        self.dims = img.size()
        self.pixmap = QPixmap(img)
        self.repaint()

class FeedWindow(QWidget):
    def __init__(self, name, contrast_min, contrast_max, point_ls=[], offset=QPoint(0,0)):
        super(FeedWindow, self).__init__()

        # Label
        self.resize(900, 500)
        self.setWindowTitle("%s Camera Feed"%name)
        hbox = QHBoxLayout()
        self.cam_feed = FeedLabel("assets/camtest1.png",point_ls,offset)
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

        #Default feed window is to bin!
        self.binning_button.setChecked(True)
        self.set_binning()
        
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


class BaseFLIRCameraWidget(RawWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(BaseFLIRCameraWidget,self).__init__(config,IP,parent)

        self.feed_refresh_time = int(config["feed_refresh_time"]*1000)
        self.compression_param = config["compression_param"]
        contrast_min = config["contrast_limits"][0]
        contrast_max = config["contrast_limits"][1]

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
        self.numframes_edit = QLineEdit("1")
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
        self.mainPanel.addLayout(hbox4)

        ############ SIDE PANEL ####################

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

        self.feed_window = FeedWindow(self.name, contrast_min, contrast_max)

        hbox3 = QHBoxLayout()
        self.Camera_button = QPushButton("Start Feed", self)
        self.Camera_button.setCheckable(True)
        self.Camera_button.setFixedWidth(200)
        self.Camera_button.clicked.connect(self.camera_feed)
        hbox3.addWidget(self.Camera_button)
        self.sidePanel.addLayout(hbox3)

        self.feedtimer = QTimer()
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
        response = self.socket.send_command("%s.status"%self.prefix)
        if (self.socket.connected):
            self.status_light = "assets/green.svg"
            self.svgWidget.load(self.status_light)

            if response == '"Camera Not Connected!"':
                self.Connect_button.setChecked(False)
                self.Connect_button.setText("Connect")
                self.run_button.setChecked(False)
                self.run_button.setText("Start Camera")
            elif response == '"Camera Connecting"':
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")
                self.run_button.setChecked(False)
                self.run_button.setText("Start Camera")
            elif response == '"Camera Reconfiguring"':
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")
                self.run_button.setChecked(False)
                self.run_button.setText("Start Camera")
            elif response == '"Camera Stopping"':
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")
                self.run_button.setChecked(False)
                self.run_button.setText("Start Camera")
            elif response == '"Camera Waiting"':
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")
                self.run_button.setChecked(False)
                self.run_button.setText("Start Camera")
            else:
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")
                self.run_button.setChecked(True)
                self.run_button.setText("Stop Camera")

            self.response_label.append(response)
            self.status_text = "Socket Connected"
            self.status_label.setText(self.status_text)
            
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


    def get_new_frame(self):
        #j = random.randint(1, 6)
        #self.feed_window.cam_feed.changePixmap("assets/camtest%s.png"%j)
        response = self.socket.send_command("%s.getlatestimage [%s,%s]"%(self.prefix,self.compression_param,self.feed_window.binning_flag))
        data = json.loads(json.loads(response))
        compressed_data = np.array(data["Image"]["data"], dtype=np.uint8)
        img_data = cv2.imdecode(compressed_data, cv2.IMREAD_UNCHANGED)
        img_data = np.clip(img_data.astype("float")*self.feed_window.contrast.getValue(), 0, 255, img_data)
        img_data = img_data.astype("uint8")
        img_data = self.feed_window.image_func(img_data)
        qimg = QImage(img_data.data, data["Image"]["cols"], data["Image"]["rows"], QImage.Format_Grayscale8)
        ##########
        self.feed_window.cam_feed.changePixmap(qimg)

    def set_coadd(self):
        if self.coadd_button.isChecked():
            self.coadd_flag = 1
        else:
            self.coadd_flag = 0

