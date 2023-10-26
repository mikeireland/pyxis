#!/usr/bin/env python
from __future__ import print_function, division
from BaseFLIRCameraWidget import BaseFLIRCameraWidget

try:
    from PyQt5.QtWidgets import QPushButton, QHBoxLayout
except:
    print("Please install PyQt5.")
    raise UserWarning


class FICameraWidget(BaseFLIRCameraWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(FICameraWidget,self).__init__(config, IP, parent)

        self.numframes_edit.setText("0")

        # Button to enable centroiding
        hbox = QHBoxLayout()
        self.enable_button = QPushButton("Enable Centroiding", self)
        self.enable_button.setCheckable(True)
        self.enable_button.setFixedWidth(200)
        self.enable_button.clicked.connect(self.enable_centroid_loop_button_func)
        hbox.addWidget(self.enable_button)
        self.sidePanel.addLayout(hbox)
        
        # Button to start Dextra tip/tilt servoing
        hbox = QHBoxLayout()
        self.DextraTT_button = QPushButton("Dextra T/T", self)
        self.DextraTT_button.setCheckable(True)
        self.DextraTT_button.setFixedWidth(200)
        self.DextraTT_button.clicked.connect(self.switch_state_button_func)
        hbox.addWidget(self.DextraTT_button)
        self.sidePanel.addLayout(hbox)
        
        # Button to start Sinistra tip/tilt servoing
        hbox = QHBoxLayout()
        self.SinistraTT_button = QPushButton("Sinistra T/T", self)
        self.SinistraTT_button.setCheckable(True)
        self.SinistraTT_button.setFixedWidth(200)
        self.SinistraTT_button.clicked.connect(self.switch_state_button_func)
        hbox.addWidget(self.SinistraTT_button)
        self.sidePanel.addLayout(hbox)

        self.feed_window.binning_button.setChecked(False)
        self.feed_window.set_binning()

    """ Edited function to run the camera"""
    def run_camera(self):
        if self.Connect_button.isChecked():
            if self.run_button.isChecked():
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

    """ Function to determine which, if any, tip/tilt servos should be run (based on the buttons pressed)"""
    def switch_state_button_func(self):
        if self.Connect_button.isChecked():
            if self.run_button.isChecked():  
                self.socket.client.RCVTIMEO = 60000   
                if self.DextraTT_button.isChecked():
                    if self.SinistraTT_button.isChecked():
                        # RUN BOTH SERVOS
                        self.send_to_server("FI.enable_tiptiltservo [3]")
                        print("Beginning Combined Tip/Tilt Servo Mode")
                    else:
                        # RUN ONLY DEXTRA SERVO
                        self.send_to_server("FI.enable_tiptiltservo [1]")
                        print("Beginning Dextra Tip/Tilt Servo Mode")
                elif self.SinistraTT_button.isChecked():
                    # RUN ONLY SINISTRA SERVO
                    self.send_to_server("FI.enable_tiptiltservo [2]")
                    print("Beginning Sinistra Tip/Tilt Servo Mode")
                else:
                    # DON'T RUN THE SERVOS
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

    """ Function to enable centroiding (NOT SERVO!) """
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
