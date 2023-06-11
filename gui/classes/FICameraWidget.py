#!/usr/bin/env python
from __future__ import print_function, division
from BaseFLIRCameraWidget import BaseFLIRCameraWidget

#Import only what we need from PyQt5, or everything from PyQt4. In any case, we'll try
#to keep this back-compatible. Although this floods the namespace somewhat, everything
#starts with a "Q" so there is little chance of getting mixed up.

try:
    from PyQt5.QtWidgets import QPushButton, QHBoxLayout
except:
    print("Please install PyQt5.")
    raise UserWarning


class FICameraWidget(BaseFLIRCameraWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(FICameraWidget,self).__init__(config, IP, parent)

        hbox3 = QHBoxLayout()
        self.enable_button = QPushButton("Enable Centroiding", self)
        self.enable_button.setCheckable(True)
        self.enable_button.setFixedWidth(200)
        self.enable_button.clicked.connect(self.enable_centroid_loop_button_func)
        hbox3.addWidget(self.enable_button)
        self.sidePanel.addLayout(hbox3)
        
        hbox3 = QHBoxLayout()
        self.state_button = QPushButton("Set Tip/Tilt Mode", self)
        self.state_button.setCheckable(True)
        self.state_button.setFixedWidth(200)
        self.state_button.clicked.connect(self.switch_state_button_func)
        hbox3.addWidget(self.state_button)
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
                self.state_button.setChecked(False)
                self.state_button.setText("Manual Mode")
        else:
            self.run_button.setChecked(False)
            print("CAMERA NOT CONNECTED")


    def switch_state_button_func(self):
        if self.Connect_button.isChecked():
            if self.run_button.isChecked():     
                if self.state_button.isChecked():
                    self.state_button.setText("Set Acquisition Mode")
                    self.send_to_server("FI.enable_tiptiltservo [1]")
                    print("Beginning Tip/Tilt Servo Mode")
                else:
                    self.state_button.setText("Set Tip/Tilt Mode")
                    self.send_to_server("FI.enable_tiptiltservo [0]")
                    print("Beginning Acquisition Mode")
            else:
                self.state_button.setChecked(False)
                print("CAMERA NOT RUNNING")
        else:
            self.state_button.setChecked(False)
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
