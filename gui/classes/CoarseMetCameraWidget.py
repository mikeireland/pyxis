#!/usr/bin/env python
from __future__ import print_function, division
from BaseFLIRCameraWidget import BaseFLIRCameraWidget, FeedWindow

try:
    from PyQt5.QtWidgets import QPushButton, QHBoxLayout
    # from PyQt5.QtCore import QPoint
except:
    print("Please install PyQt5.")
    raise UserWarning


class CoarseMetCameraWidget(BaseFLIRCameraWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(CoarseMetCameraWidget,self).__init__(config,IP=IP,parent=parent)

        # Button to enable the metrology loop
        self.numframes_edit.setText("0")
        hbox = QHBoxLayout()
        self.enable_button = QPushButton("LED flash", self)
        self.enable_button.setCheckable(True)
        self.enable_button.setFixedWidth(200)
        self.enable_button.clicked.connect(self.enable_met_loop_button_func)
        hbox.addWidget(self.enable_button)
        self.sidePanel.addLayout(hbox)

    """ Function to enable the metrology loop """
    def enable_met_loop_button_func(self):
        if self.Connect_button.isChecked():       
            if self.enable_button.isChecked():
                self.enable_button.setText("Disable flash")
                self.send_to_server("CM.enableLEDs 1")
                print("Enabling LED flash")
            else:
                self.enable_button.setText("LED flash")
                self.send_to_server("CM.enableLEDs 0")
                print("Disabling LED flash")
        else:
            self.enable_button.setChecked(False)
            print("CAMERA NOT CONNECTED")
