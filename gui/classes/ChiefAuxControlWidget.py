#!/usr/bin/env python
from __future__ import print_function, division
from client_socket import ClientSocket
import random
import json
#Import only what we need from PyQt5, or everything from PyQt4. In any case, we'll try
#to keep this back-compatible. Although this floods the namespace somewhat, everything
#starts with a "Q" so there is little chance of getting mixed up.

try:
    from PyQt5.QtWidgets import QWidget, QPushButton, QHBoxLayout, \
        QVBoxLayout, QGridLayout, QLabel, QLineEdit, QTextEdit
    from PyQt5.QtSvg import QSvgWidget
    from PyQt5.QtCore import Qt, QTimer
except:
    print("Please install PyQt5.")
    raise UserWarning


class ChiefAuxControlWidget(QWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(ChiefAuxControlWidget,self).__init__(parent)

        self.name = config["name"]
        self.port = config["port"]
        self.socket = ClientSocket(IP=IP, Port=self.port)
        self.voltage_limit = config["voltage_limit"]
        self.status_refresh_time = int(config["status_refresh_time"]*1000)

        #Layout the common elements
        vBoxlayout = QVBoxLayout()
        vBoxlayout.setSpacing(3)

        #Description
        desc = QLabel('Port: %s    Description: %s'%(config["port"],config["description"]), self)
        desc.setStyleSheet("font-weight: bold")
        desc.setFixedHeight(40)
        hbox1 = QHBoxLayout()
        hbox1.addWidget(desc)
        vBoxlayout.addLayout(hbox1)

        #First, the command entry box
        lbl1 = QLabel('Command: ', self)
        self.line_edit = QLineEdit("CA.")
        self.line_edit.returnPressed.connect(self.command_enter)

        #Next, the info button
        self.info_button = QPushButton("INFO", self)
        self.info_button.clicked.connect(self.info_click)

        hbox1 = QHBoxLayout()
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.line_edit)
        hbox1.addWidget(self.info_button)
        vBoxlayout.addLayout(hbox1)

        #Next, the response box
        self.response_label = QTextEdit('[No Server Response Yet]', self)
        self.response_label.setReadOnly(True)
        self.response_label.setStyleSheet("QTextEdit { background-color : black; }")
        self.response_label.setFixedHeight(150)
        vBoxlayout.addWidget(self.response_label)

        vBoxlayout.addSpacing(30)
        
        content_layout = QHBoxLayout()
        
        Piezo_layout = QVBoxLayout()
        lbl = QLabel("Actuator Controls:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Piezo_layout.addWidget(lbl)
        Piezo_layout.addSpacing(20)
        config_grid = QGridLayout()
        config_grid.setColumnMinimumWidth(0,150)
        config_grid.setColumnMinimumWidth(1,130)
        config_grid.setColumnMinimumWidth(2,140)
        config_grid.setColumnMinimumWidth(3,150)
        config_grid.setColumnMinimumWidth(4,100)
        config_grid.setVerticalSpacing(5)

        self.Dextra_X_V = config["PiezoStart"]["dextra_X"]
        self.Dextra_Y_V = config["PiezoStart"]["dextra_Y"]
        self.Sinistra_X_V = config["PiezoStart"]["sinistra_X"]
        self.Sinistra_Y_V = config["PiezoStart"]["sinistra_Y"]
        self.SciPiezo_V = config["PiezoStart"]["science"]
        
        self.Dextra_X_dV = config["PiezoSteps"]["dextra_X"]
        self.Dextra_Y_dV = config["PiezoSteps"]["dextra_Y"]
        self.Sinistra_X_dV = config["PiezoSteps"]["sinistra_X"]
        self.Sinistra_Y_dV = config["PiezoSteps"]["sinistra_Y"]
        self.SciPiezo_dV = config["PiezoSteps"]["science"]

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Dextra XY Piezo X (V): ', self)
        self.Dextra_X_lbl = QLabel("", self)
        self.Dextra_X_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #6becfa}")
        self.Dextra_X_edit = QLineEdit(str(self.Dextra_X_V))
        self.Dextra_X_edit.setFixedWidth(120)
        self.Dextra_X_edit.setStyleSheet("color: #6becfa; border-color: #6becfa")
        self.Dextra_X_Set_button = QPushButton("GO", self)
        self.Dextra_X_Set_button.clicked.connect(lambda: self.Piezo_click(0, float(self.Dextra_X_edit.text())))
        self.Dextra_X_Set_button.setFixedWidth(100)
        self.Dextra_X_Set_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa;}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        self.Dextra_X_L_button = QPushButton("<<", self)
        self.Dextra_X_L_button.clicked.connect(lambda: self.Piezo_click(0,self.Dextra_X_V-self.Dextra_X_dV))
        self.Dextra_X_L_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        self.Dextra_X_L_button.setFixedWidth(50)
        self.Dextra_X_R_button = QPushButton(">>", self)
        self.Dextra_X_R_button.clicked.connect(lambda: self.Piezo_click(0,self.Dextra_X_V+self.Dextra_X_dV))
        self.Dextra_X_R_button.setFixedWidth(50)
        self.Dextra_X_R_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        config_grid.addWidget(lbl1,0,0)
        config_grid.addWidget(self.Dextra_X_lbl,0,1)
        config_grid.addWidget(self.Dextra_X_edit,0,2)
        config_grid.addWidget(self.Dextra_X_Set_button,0,3)
        hbox3 = QHBoxLayout()
        hbox3.addWidget(self.Dextra_X_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Dextra_X_R_button)
        config_grid.addLayout(hbox3,0,4)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Dextra XY Piezo Y (V): ', self)
        self.Dextra_Y_lbl = QLabel("", self)
        self.Dextra_Y_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #6becfa}")
        self.Dextra_Y_edit = QLineEdit(str(self.Dextra_Y_V))
        self.Dextra_Y_edit.setFixedWidth(120)
        self.Dextra_Y_edit.setStyleSheet("color: #6becfa; border-color: #6becfa")
        self.Dextra_Y_Set_button = QPushButton("GO", self)
        self.Dextra_Y_Set_button.clicked.connect(lambda: self.Piezo_click(1, float(self.Dextra_Y_edit.text())))
        self.Dextra_Y_Set_button.setFixedWidth(100)
        self.Dextra_Y_Set_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa;}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        self.Dextra_Y_L_button = QPushButton("<<", self)
        self.Dextra_Y_L_button.clicked.connect(lambda: self.Piezo_click(1, self.Dextra_Y_V-self.Dextra_Y_dV))
        self.Dextra_Y_L_button.setFixedWidth(50)
        self.Dextra_Y_L_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        self.Dextra_Y_R_button = QPushButton(">>", self)
        self.Dextra_Y_R_button.clicked.connect(lambda: self.Piezo_click(1, self.Dextra_Y_V+self.Dextra_Y_dV))
        self.Dextra_Y_R_button.setFixedWidth(50)
        self.Dextra_Y_R_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        config_grid.addWidget(lbl1,1,0)
        config_grid.addWidget(self.Dextra_Y_lbl,1,1)
        config_grid.addWidget(self.Dextra_Y_edit,1,2)
        config_grid.addWidget(self.Dextra_Y_Set_button,1,3)
        hbox3 = QHBoxLayout()
        hbox3.addWidget(self.Dextra_Y_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Dextra_Y_R_button)
        config_grid.addLayout(hbox3,1,4)


        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Sinistra XY Piezo X (V): ', self)
        self.Sinistra_X_lbl = QLabel("", self)
        self.Sinistra_X_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #c7f779}")
        self.Sinistra_X_edit = QLineEdit(str(self.Sinistra_X_V))
        self.Sinistra_X_edit.setFixedWidth(120)
        self.Sinistra_X_edit.setStyleSheet("color: #c7f779; border-color: #c7f779")
        self.Sinistra_X_Set_button = QPushButton("GO", self)
        self.Sinistra_X_Set_button.clicked.connect(lambda: self.Piezo_click(2, float(self.Sinistra_X_edit.text())))
        self.Sinistra_X_Set_button.setFixedWidth(100)
        self.Sinistra_X_Set_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        self.Sinistra_X_L_button = QPushButton("<<", self)
        self.Sinistra_X_L_button.clicked.connect(lambda: self.Piezo_click(2, self.Sinistra_X_V-self.Sinistra_X_dV))
        self.Sinistra_X_L_button.setFixedWidth(50)
        self.Sinistra_X_L_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        self.Sinistra_X_R_button = QPushButton(">>", self)
        self.Sinistra_X_R_button.clicked.connect(lambda: self.Piezo_click(2, self.Sinistra_X_V+self.Sinistra_X_dV))
        self.Sinistra_X_R_button.setFixedWidth(50)
        self.Sinistra_X_R_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        config_grid.addWidget(lbl1,2,0)
        config_grid.addWidget(self.Sinistra_X_lbl,2,1)
        config_grid.addWidget(self.Sinistra_X_edit,2,2)
        config_grid.addWidget(self.Sinistra_X_Set_button,2,3)
        hbox3 = QHBoxLayout()
        hbox3.addWidget(self.Sinistra_X_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Sinistra_X_R_button)
        config_grid.addLayout(hbox3,2,4)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Sinistra XY Piezo Y (V): ', self)
        self.Sinistra_Y_lbl = QLabel("", self)
        self.Sinistra_Y_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #c7f779}")
        self.Sinistra_Y_edit = QLineEdit(str(self.Sinistra_Y_V))
        self.Sinistra_Y_edit.setFixedWidth(120)
        self.Sinistra_Y_edit.setStyleSheet("color: #c7f779; border-color: #c7f779")
        self.Sinistra_Y_Set_button = QPushButton("GO", self)
        self.Sinistra_Y_Set_button.clicked.connect(lambda: self.Piezo_click(3, float(self.Sinistra_Y_edit.text())))
        self.Sinistra_Y_Set_button.setFixedWidth(100)
        self.Sinistra_Y_Set_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        self.Sinistra_Y_L_button = QPushButton("<<", self)
        self.Sinistra_Y_L_button.clicked.connect(lambda: self.Piezo_click(3, self.Sinistra_Y_V-self.Sinistra_Y_dV))
        self.Sinistra_Y_L_button.setFixedWidth(50)
        self.Sinistra_Y_L_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        self.Sinistra_Y_R_button = QPushButton(">>", self)
        self.Sinistra_Y_R_button.clicked.connect(lambda: self.Piezo_click(3, self.Sinistra_Y_V+self.Sinistra_Y_dV))
        self.Sinistra_Y_R_button.setFixedWidth(50)
        self.Sinistra_Y_R_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        config_grid.addWidget(lbl1,3,0)
        config_grid.addWidget(self.Sinistra_Y_lbl,3,1)
        config_grid.addWidget(self.Sinistra_Y_edit,3,2)
        config_grid.addWidget(self.Sinistra_Y_Set_button,3,3)
        hbox3 = QHBoxLayout()
        hbox3.addWidget(self.Sinistra_Y_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Sinistra_Y_R_button)
        config_grid.addLayout(hbox3,3,4)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Science Piezo (V): ', self)
        self.SciPiezo_lbl = QLabel("", self)
        self.SciPiezo_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #f47cfc}")
        self.SciPiezo_edit = QLineEdit(str(self.SciPiezo_V))
        self.SciPiezo_edit.setFixedWidth(120)
        self.SciPiezo_edit.setStyleSheet("color: #f47cfc; border-color: #f47cfc")
        self.SciPiezo_Set_button = QPushButton("GO", self)
        self.SciPiezo_Set_button.clicked.connect(lambda: self.Piezo_click(4, float(self.SciPiezo_edit.text())))
        self.SciPiezo_Set_button.setFixedWidth(100)
        self.SciPiezo_Set_button.setStyleSheet("QPushButton {color: #f47cfc; border-color: #f47cfc}"
                                             "QPushButton:pressed {color: #000000; background-color: #f47cfc}")
        self.SciPiezo_L_button = QPushButton("<<", self)
        self.SciPiezo_L_button.clicked.connect(lambda: self.Piezo_click(4, self.SciPiezo_V-self.SciPiezo_dV))
        self.SciPiezo_L_button.setFixedWidth(50)
        self.SciPiezo_L_button.setStyleSheet("QPushButton {color: #f47cfc; border-color: #f47cfc}"
                                             "QPushButton:pressed {color: #000000; background-color: #f47cfc}")
        self.SciPiezo_R_button = QPushButton(">>", self)
        self.SciPiezo_R_button.clicked.connect(lambda: self.Piezo_click(4, self.SciPiezo_V+self.SciPiezo_dV))
        self.SciPiezo_R_button.setFixedWidth(50)
        self.SciPiezo_R_button.setStyleSheet("QPushButton {color: #f47cfc; border-color: #f47cfc}"
                                             "QPushButton:pressed {color: #000000; background-color: #f47cfc}")
        config_grid.addWidget(lbl1,4,0)
        config_grid.addWidget(self.SciPiezo_lbl,4,1)
        config_grid.addWidget(self.SciPiezo_edit,4,2)
        config_grid.addWidget(self.SciPiezo_Set_button,4,3)
        hbox3 = QHBoxLayout()
        hbox3.addWidget(self.SciPiezo_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.SciPiezo_R_button)
        config_grid.addLayout(hbox3,4,4)

        hbox3 = QHBoxLayout()
        lbl2 = QLabel('SDC Steps: ', self)
        lbl4 = QLabel('SDC Frequency (Hz): ', self)
        self.SDC_lbl = QLabel("", self)
        self.SDC_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #f47cfc}")
        self.FineStage_numSteps_edit = QLineEdit("0")
        self.FineStage_numSteps_edit.setFixedWidth(120)
        self.FineStage_numSteps_edit.setStyleSheet("color: #ff7f69; border-color: #ff7f69")
        self.FineStage_freq_edit = QLineEdit("0")
        self.FineStage_freq_edit.setFixedWidth(120)
        self.FineStage_freq_edit.setStyleSheet("color: #ff7f69; border-color: #ff7f69")
        self.FineStage_Set_button = QPushButton("GO", self)
        self.FineStage_Set_button.clicked.connect(lambda: self.FineStage_click(float(self.FineStage_numSteps_edit.text()),float(self.FineStage_freq_edit.text())))
        self.FineStage_Set_button.setFixedWidth(100)
        self.FineStage_Set_button.setStyleSheet("QPushButton {color: #ff7f69; border-color: #ff7f69}"
                                             "QPushButton:pressed {color: #000000; background-color: #ff7f69}")
        self.FineStage_Home_button = QPushButton("HOME", self)
        self.FineStage_Home_button.clicked.connect(self.FineStage_home)
        self.FineStage_Home_button.setFixedWidth(100)
        self.FineStage_Home_button.setStyleSheet("QPushButton {color: #ff7f69; border-color: #ff7f69}"
                                             "QPushButton:pressed {color: #000000; background-color: #ff7f69}")
        config_grid.addWidget(lbl2,5,0)
        config_grid.addWidget(lbl4,6,0)
        config_grid.addWidget(self.SDC_lbl,5,1,2,1)        
        config_grid.addWidget(self.FineStage_numSteps_edit,5,2)
        config_grid.addWidget(self.FineStage_freq_edit,6,2)
        config_grid.addWidget(self.FineStage_Set_button,5,3,2,1)
        config_grid.addWidget(self.FineStage_Home_button,5,4,2,1)
        
        Piezo_layout.addLayout(config_grid)
        Piezo_layout.setAlignment(Qt.AlignTop)
        
        content_layout.addLayout(Piezo_layout,1)
        content_layout.addSpacing(50)

        power_master_layout = QVBoxLayout()

        power_layout = QHBoxLayout()
        lbl = QLabel("Power System Controls:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        power_layout.addWidget(lbl)
        
        #power_layout.addSpacing(50)
        power_master_layout.addLayout(power_layout)
        power_master_layout.addSpacing(20)
        power_layout = QHBoxLayout()
        
        self.getPower_button = QPushButton("Request Status", self)
        self.getPower_button.setFixedWidth(200)
        self.getPower_button.clicked.connect(self.ask_for_status)
        #self.getPower_button.setAlignment(Qt.AlignLeft)
        #power_layout.addStretch()
        power_layout.addWidget(self.getPower_button)
        power_layout.setAlignment(Qt.AlignLeft)
        
        power_master_layout.addLayout(power_layout)
        power_master_layout.addSpacing(20)
        
        power_layout = QHBoxLayout()
        self.PC = QLabel("PC: ")
        self.PC.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        power_master_layout.addWidget(self.PC)
        power_master_layout.addSpacing(20)
        self.PCvoltage = QLabel("0.00 V")
        self.PCvoltage.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        power_layout.addWidget(self.PCvoltage)
        power_layout.addSpacing(50)
        self.PCcurrent = QLabel("0.00 A")
        self.PCcurrent.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        power_layout.addWidget(self.PCcurrent)
        power_layout.addSpacing(50)
        # Complete setup, add status labels and indicators
        self.power_status_light = 'assets/red.svg'
        self.power_status_text = 'Voltage getting low'
        self.power_svgWidget = QSvgWidget(self.power_status_light)
        self.power_svgWidget.setFixedSize(20,20)
        self.power_status_label = QLabel(self.power_status_text, self)
        power_layout.addWidget(self.power_svgWidget)
        power_layout.addWidget(self.power_status_label)
        power_layout.addStretch()
        power_master_layout.addLayout(power_layout)
        power_master_layout.addSpacing(20)

        power_layout = QHBoxLayout()
        self.Motor = QLabel("Motor: ")
        self.Motor.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        power_master_layout.addWidget(self.Motor)
        power_master_layout.addSpacing(20)
        self.Motorvoltage = QLabel("0.00 V")
        self.Motorvoltage.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        power_layout.addWidget(self.Motorvoltage)
        power_layout.addSpacing(50)
        self.Motorcurrent = QLabel("0.00 A")
        self.Motorcurrent.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        power_layout.addWidget(self.Motorcurrent)
        power_layout.addStretch()
        power_master_layout.addLayout(power_layout)
        
        power_master_layout.setAlignment(Qt.AlignTop)
        
        content_layout.addLayout(power_master_layout,1)
        #content_layout.setAlignment(Qt.AlignTop)

        vBoxlayout.addLayout(content_layout)

        # Complete setup, add status labels and indicators
        status_layout = QHBoxLayout()
        self.status_light = 'assets/red.svg'
        self.status_text = 'Socket Not Connected'
        self.svgWidget = QSvgWidget(self.status_light)
        self.svgWidget.setFixedSize(20,20)
        self.status_label = QLabel(self.status_text, self)
        status_layout.addWidget(self.svgWidget)
        status_layout.addWidget(self.status_label)

        vBoxlayout.addLayout(status_layout)

        self.setLayout(vBoxlayout)

        self.stimer = QTimer()
        self.auto_updater()

        #self.Piezo_click(0, self.Dextra_X_V)
        #self.Piezo_click(1, self.Dextra_Y_V)
        #self.Piezo_click(2, self.Sinistra_X_V)
        #self.Piezo_click(3, self.Sinistra_Y_V)
        #self.Piezo_click(4, self.SciPiezo_V)

        self.ask_for_status()

    def auto_updater(self):
        self.ask_for_status()
        self.stimer.singleShot(self.status_refresh_time, self.auto_updater)
        return

    def change_ip(self,IP):
        self.socket = ClientSocket(IP=IP, Port=self.port)

    def ask_for_status(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""
        #As this is on a continuous timer, only do anything if we are
        #connected
        response = self.send_to_server_with_response("CA.requestStatus")
        if (self.socket.connected):
            self.status_light = "assets/green.svg"
            self.svgWidget.load(self.status_light)
            self.response_label.append(response)
            self.status_text = "Socket Connected"
            self.status_label.setText(self.status_text)

            try:
                status_dict = json.loads(response)
            except:
                return

            self.PCvoltage.setText("{:.2f} V".format(status_dict["PC_voltage"]))
            self.PCcurrent.setText("{:.2f} A".format(status_dict["PC_current"]))
            self.Motorvoltage.setText("{:.2f} V".format(status_dict["Motor_voltage"]))
            self.Motorcurrent.setText("{:.2f} A".format(status_dict["Motor_current"]))

            self.Dextra_X_lbl.setText("{:.2f} V, {:.2f} um".format(status_dict["Dextra_X_V"],status_dict["Dextra_X_um"]))
            self.Dextra_Y_lbl.setText("{:.2f} V, {:.2f} um".format(status_dict["Dextra_Y_V"],status_dict["Dextra_Y_um"]))
            self.Sinistra_X_lbl.setText("{:.2f} V, {:.2f} um".format(status_dict["Sinistra_X_V"],status_dict["Sinistra_X_um"]))
            self.Sinistra_Y_lbl.setText("{:.2f} V, {:.2f} um".format(status_dict["Sinistra_Y_V"],status_dict["Sinistra_Y_um"]))
            self.SciPiezo_lbl.setText("{:.2f} V, {:.2f} um".format(status_dict["Science_V"],status_dict["Science_um"]))
            self.SDC_lbl.setText("{} step, {:.2f} um".format(status_dict["SDC_step_count"],0.02*status_dict["SDC_step_count"]))

            if status_dict["PC_voltage"] > self.voltage_limit:
                self.power_status_light = "assets/green.svg"
                self.power_svgWidget.load(self.power_status_light)
                self.power_status_text = "Voltage good"
                self.power_status_label.setText(self.power_status_text)
            else:
                self.power_status_light = "assets/red.svg"
                self.power_status_text = "Voltage getting low"
                self.power_status_label.setText(self.power_status_text)
                self.power_svgWidget.load(self.power_status_light)    


        else:
            self.status_light = "assets/red.svg"
            self.status_text = "Socket Not Connected"
            self.status_label.setText(self.status_text)
            self.svgWidget.load(self.status_light)

    #What happens when you click the info button
    def info_click(self):
        print(self.name)
        self.ask_for_status()

        
    def Piezo_click(self,index,voltage):

        if index == 4:
            response = self.send_to_server_with_response("CA.moveSciPiezo [%s]"%(voltage))
        elif index < 4:
            response = self.send_to_server_with_response("CA.moveTipTiltPiezos [%s,%s]"%(index, voltage))
        else:
            print("INDEX OUT OF RANGE")
            return 

        try:
            response_dict = json.loads(response)
            V = response_dict["voltage"]
            pos = response_dict["position"]
            if index == 0:
                self.Dextra_X_V = V
                self.Dextra_X_lbl.setText("{:.2f} V, {:.2f} um".format(V,pos))
            elif index == 1:
                self.Dextra_Y_V = V
                self.Dextra_Y_lbl.setText("{:.2f} V, {:.2f} um".format(V,pos))
            elif index == 2:
                self.Sinistra_X_V = V
                self.Sinistra_X_lbl.setText("{:.2f} V, {:.2f} um".format(V,pos))
            elif index == 3:
                self.Sinistra_Y_V = V
                self.Sinistra_Y_lbl.setText("{:.2f} V, {:.2f} um".format(V,pos))
            elif index == 4:
                self.SciPiezo_V = V
                self.SciPiezo_lbl.setText("{:.2f} V, {:.2f} um".format(V,pos))
            else:
                print("INDEX OUT OF RANGE")
        except:
                return
            
        return
        
    def FineStage_click(self,numSteps,freq):
        period = int(1e6/freq)
        self.send_to_server("CA.moveSDC [%s,%s]"%(numSteps,period))
        return
    
    def FineStage_home(self):
        self.send_to_server("CA.homeSDC")
        return

    def command_enter(self):
        """Parse the LineEdit string and send_to_server
        """
        self.send_to_server(str(self.line_edit.text()))
        
    def connect_server(self):

        if self.Connect_button.isChecked():
            # Refresh camera
            self.Connect_button.setText("Disconnect")
            print("Server is now Connected")
            self.send_to_server("CA.connect")

        else:
            self.Connect_button.setText("Connect")
            print("Disconnecting Server")
            self.send_to_server("CA.disconnect")

     


    def send_to_server_with_response(self, text):
        """Send a command to the server, dependent on the current tab.
        """
        try:
            response = self.socket.send_command(text)
        except:
            response = "*** Connection Error ***"
        if type(response)==str or type(response)==unicode:
            try:
                response_dict = json.loads(response)
                if "message" in response_dict.keys():
                    self.response_label.append(response_dict["message"])
                else:
                    self.response_label.append(response)
            except:
                self.response_label.append(response)
        elif type(response)==bool:
            if response:
                self.response_label.setText("Success!")
            else:
                self.response_label.setText("Failure!")
        self.line_edit.setText("CA.")

        return response

    def send_to_server(self, text):
        """Send a command to the server, dependent on the current tab.
        """
        try:
            response = self.socket.send_command(text)
        except:
            response = "*** Connection Error ***"
        if type(response)==str or type(response)==unicode:
            self.response_label.append(response)
        elif type(response)==bool:
            if response:
                self.response_label.setText("Success!")
            else:
                self.response_label.setText("Failure!")
        self.line_edit.setText("CA.")
