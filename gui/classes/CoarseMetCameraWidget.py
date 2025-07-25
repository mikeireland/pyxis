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

        hbox = QHBoxLayout()
        self.align_button = QPushButton("Start Align", self)
        self.align_button.setCheckable(True)
        self.align_button.setFixedWidth(200)
        self.align_button.clicked.connect(self.pupil_align_loop_button_func)
        hbox.addWidget(self.align_button)
        self.sidePanel.addLayout(hbox)

        if config["name"] == "DextraCoarseMet":
            self.deputy = "Dextra"
        elif config["name"] == "SinistraCoarseMet":
            self.deputy = "Sinistra" 
        else:
            self.deputy = "UnknownDeputy"
            print("WARNING: Unknown deputy name, using 'UnknownDeputy'")

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

    """ Function to start the pupil alignment loop """
    def pupil_align_loop_button_func(self):
        if self.Connect_button.isChecked():
            try:
                if self.align_button.isChecked():
                    self.align_button.setText("Stop Align")
                    self.fsm_socket.send_string(f"start_CMalign {self.deputy}")
                    reply = self.fsm_socket.recv_string()
                    self.response_label.append("Asking FSM to start pupil alignment for" + self.deputy)
                    self.response_label.append("FSM reply:"+ str(reply))
                else:
                    self.align_button.setText("Start Align")
                    self.fsm_socket.send_string(f"stop_CMalign {self.deputy}")
                    reply = self.fsm_socket.recv_string()
                    self.response_label.append("Asking FSM to stopping pupil alignment for " + self.deputy)
                    self.response_label.append("FSM reply:"+ str(reply))
            except Exception as e:
                self.response_label.append(f"Error communicating with FSM server: {e}")
                self.align_button.setChecked(False)
        else:
            self.align_button.setChecked(False)
            self.response_label.append("CAMERA NOT CONNECTED")