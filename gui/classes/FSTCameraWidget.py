#!/usr/bin/env python
from __future__ import print_function, division
from BaseFLIRCameraWidget import BaseFLIRCameraWidget
from PyQt5.QtWidgets import QPushButton, QHBoxLayout

class FSTCameraWidget(BaseFLIRCameraWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(FSTCameraWidget,self).__init__(config,IP,parent)
        
        hbox3 = QHBoxLayout()
        self.state_button = QPushButton("Manual Mode", self)
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
                    self.state_button.setText("Centroid Mode")
                    self.send_to_server("FST.switchCentroid")
                    print("Beginning Centroid Mode")
                else:
                    self.state_button.setText("Plate Solve Mode")
                    self.send_to_server("FST.switchPlateSolve")
                    print("Beginning Plate Solve Mode")
            else:
                self.state_button.setChecked(False)
                self.state_button.setText("Manual Mode")
                print("CAMERA NOT RUNNING")
        else:
            self.state_button.setChecked(False)
            self.state_button.setText("Manual Mode")
            print("CAMERA NOT CONNECTED")

