#!/usr/bin/env python
from __future__ import print_function, division
from BaseFLIRCameraWidget import BaseFLIRCameraWidget, FeedWindow

try:
    from PyQt5.QtWidgets import QPushButton, QHBoxLayout
    from PyQt5.QtCore import QPoint
except:
    print("Please install PyQt5.")
    raise UserWarning

class CoarseMetCameraWidget(BaseFLIRCameraWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(CoarseMetCameraWidget,self).__init__(config,IP=IP,parent=parent)
        contrast_min = config["contrast_limits"][0]
        contrast_max = config["contrast_limits"][1]

        target_1 = QPoint(594,430)
        target_ls = [target_1]
        offset = QPoint(420,252)

        self.feed_window = FeedWindow(self.name, contrast_min, contrast_max, target_ls, offset)


        self.numframes_edit.setText("0")
        hbox3 = QHBoxLayout()
        self.enable_button = QPushButton("Enable Metrology", self)
        self.enable_button.setCheckable(True)
        self.enable_button.setFixedWidth(200)
        self.enable_button.clicked.connect(self.enable_met_loop_button_func)
        hbox3.addWidget(self.enable_button)
        self.sidePanel.addLayout(hbox3)


    def enable_met_loop_button_func(self):
        if self.Connect_button.isChecked():       
            if self.enable_button.isChecked():
                self.enable_button.setText("Disable metrology")
                self.send_to_server("CM.enableLEDs [1]")
                print("Enabling Coarse Met Loop")
            else:
                self.enable_button.setText("Enable metrology")
                self.send_to_server("CM.enableLEDs [0]")
                print("Disabling Coarse Met Loop")
        else:
            self.enable_button.setChecked(False)
            print("CAMERA NOT CONNECTED")
