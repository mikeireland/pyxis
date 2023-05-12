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
        QVBoxLayout, QGridLayout, QLabel, QLineEdit, QTextEdit, QFrame
    from PyQt5.QtSvg import QSvgWidget
    from PyQt5.QtCore import Qt
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
        
        self.Connect_button = QPushButton("Connect", self)
        self.Connect_button.setCheckable(True)
        self.Connect_button.clicked.connect(self.connect_server)

        hbox1 = QHBoxLayout()
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.line_edit)
        hbox1.addWidget(self.info_button)
        hbox1.addWidget(self.Connect_button)
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
        lbl1 = QLabel('Dextra XY Piezo X: ', self)
        self.Dextra_X_lbl = QLabel(str(self.Dextra_X_V)+" V", self)
        self.Dextra_X_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #6becfa}")
        self.Dextra_X_edit = QLineEdit(str(self.Dextra_X_V))
        self.Dextra_X_edit.setFixedWidth(120)
        self.Dextra_X_edit.setStyleSheet("color: #6becfa; border-color: #6becfa")
        self.Dextra_Set_button = QPushButton("GO", self)
        self.Dextra_Set_button.clicked.connect(lambda: self.Dextra_XY_click(float(self.Dextra_X_edit.text()), float(self.Dextra_Y_edit.text())))
        self.Dextra_Set_button.setFixedWidth(100)
        self.Dextra_Set_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa;}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        self.Dextra_X_L_button = QPushButton("<<", self)
        self.Dextra_X_L_button.clicked.connect(lambda: self.Dextra_XY_click(self.Dextra_X_V-self.Dextra_X_dV,self.Dextra_Y_V))
        self.Dextra_X_L_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        self.Dextra_X_L_button.setFixedWidth(50)
        self.Dextra_X_R_button = QPushButton(">>", self)
        self.Dextra_X_R_button.clicked.connect(lambda: self.Dextra_XY_click(self.Dextra_X_V+self.Dextra_X_dV,self.Dextra_Y_V))
        self.Dextra_X_R_button.setFixedWidth(50)
        self.Dextra_X_R_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        config_grid.addWidget(lbl1,0,0)
        config_grid.addWidget(self.Dextra_X_lbl,0,1)
        config_grid.addWidget(self.Dextra_X_edit,0,2)
        config_grid.addWidget(self.Dextra_Set_button,0,3,2,1)
        hbox3 = QHBoxLayout()
        hbox3.addWidget(self.Dextra_X_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Dextra_X_R_button)
        config_grid.addLayout(hbox3,0,4)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Dextra XY Piezo Y: ', self)
        self.Dextra_Y_lbl = QLabel(str(self.Dextra_Y_V)+" V", self)
        self.Dextra_Y_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #6becfa}")
        self.Dextra_Y_edit = QLineEdit(str(self.Dextra_Y_V))
        self.Dextra_Y_edit.setFixedWidth(120)
        self.Dextra_Y_edit.setStyleSheet("color: #6becfa; border-color: #6becfa")
        self.Dextra_Y_L_button = QPushButton("<<", self)
        self.Dextra_Y_L_button.clicked.connect(lambda: self.Dextra_XY_click(self.Dextra_X_V, self.Dextra_Y_V-self.Dextra_Y_dV))
        self.Dextra_Y_L_button.setFixedWidth(50)
        self.Dextra_Y_L_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        self.Dextra_Y_R_button = QPushButton(">>", self)
        self.Dextra_Y_R_button.clicked.connect(lambda: self.Dextra_XY_click(self.Dextra_X_V, self.Dextra_Y_V+self.Dextra_Y_dV))
        self.Dextra_Y_R_button.setFixedWidth(50)
        self.Dextra_Y_R_button.setStyleSheet("QPushButton {color: #6becfa; border-color: #6becfa}"
                                             "QPushButton:pressed {color: #000000; background-color: #6becfa}")
        config_grid.addWidget(lbl1,1,0)
        config_grid.addWidget(self.Dextra_Y_lbl,1,1)
        config_grid.addWidget(self.Dextra_Y_edit,1,2)
        hbox3 = QHBoxLayout()
        hbox3.addWidget(self.Dextra_Y_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Dextra_Y_R_button)
        config_grid.addLayout(hbox3,1,4)


        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Sinistra XY Piezo X: ', self)
        self.Sinistra_X_lbl = QLabel(str(self.Sinistra_X_V)+" V", self)
        self.Sinistra_X_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #c7f779}")
        self.Sinistra_X_edit = QLineEdit(str(self.Sinistra_X_V))
        self.Sinistra_X_edit.setFixedWidth(120)
        self.Sinistra_X_edit.setStyleSheet("color: #c7f779; border-color: #c7f779")
        self.Sinistra_Set_button = QPushButton("GO", self)
        self.Sinistra_Set_button.clicked.connect(lambda: self.Sinistra_XY_click(float(self.Sinistra_X_edit.text()), float(self.Sinistra_Y_edit.text())))
        self.Sinistra_Set_button.setFixedWidth(100)
        self.Sinistra_Set_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        self.Sinistra_X_L_button = QPushButton("<<", self)
        self.Sinistra_X_L_button.clicked.connect(lambda: self.Sinistra_XY_click(self.Sinistra_X_V-self.Sinistra_X_dV,self.Sinistra_Y_V))
        self.Sinistra_X_L_button.setFixedWidth(50)
        self.Sinistra_X_L_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        self.Sinistra_X_R_button = QPushButton(">>", self)
        self.Sinistra_X_R_button.clicked.connect(lambda: self.Sinistra_XY_click(self.Sinistra_X_V+self.Sinistra_X_dV,self.Sinistra_Y_V))
        self.Sinistra_X_R_button.setFixedWidth(50)
        self.Sinistra_X_R_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        config_grid.addWidget(lbl1,2,0)
        config_grid.addWidget(self.Sinistra_X_lbl,2,1)
        config_grid.addWidget(self.Sinistra_X_edit,2,2)
        config_grid.addWidget(self.Sinistra_Set_button,2,3,2,1)
        hbox3 = QHBoxLayout()
        hbox3.addWidget(self.Sinistra_X_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Sinistra_X_R_button)
        config_grid.addLayout(hbox3,2,4)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Sinistra XY Piezo Y: ', self)
        self.Sinistra_Y_lbl = QLabel(str(self.Sinistra_Y_V)+" V", self)
        self.Sinistra_Y_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #c7f779}")
        self.Sinistra_Y_edit = QLineEdit(str(self.Sinistra_Y_V))
        self.Sinistra_Y_edit.setFixedWidth(120)
        self.Sinistra_Y_edit.setStyleSheet("color: #c7f779; border-color: #c7f779")
        self.Sinistra_Y_L_button = QPushButton("<<", self)
        self.Sinistra_Y_L_button.clicked.connect(lambda: self.Sinistra_XY_click(self.Sinistra_X_V, self.Sinistra_Y_V-self.Sinistra_Y_dV))
        self.Sinistra_Y_L_button.setFixedWidth(50)
        self.Sinistra_Y_L_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        self.Sinistra_Y_R_button = QPushButton(">>", self)
        self.Sinistra_Y_R_button.clicked.connect(lambda: self.Sinistra_XY_click(self.Sinistra_X_V, self.Sinistra_Y_V+self.Sinistra_Y_dV))
        self.Sinistra_Y_R_button.setFixedWidth(50)
        self.Sinistra_Y_R_button.setStyleSheet("QPushButton {color: #c7f779; border-color: #c7f779}"
                                             "QPushButton:pressed {color: #000000; background-color: #c7f779}")
        config_grid.addWidget(lbl1,3,0)
        config_grid.addWidget(self.Sinistra_Y_lbl,3,1)
        config_grid.addWidget(self.Sinistra_Y_edit,3,2)
        hbox3 = QHBoxLayout()
        hbox3.addWidget(self.Sinistra_Y_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Sinistra_Y_R_button)
        config_grid.addLayout(hbox3,3,4)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Science Piezo: ', self)
        self.SciPiezo_lbl = QLabel(str(self.SciPiezo_V)+" V", self)
        self.SciPiezo_lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #f47cfc}")
        self.SciPiezo_edit = QLineEdit(str(self.SciPiezo_V))
        self.SciPiezo_edit.setFixedWidth(120)
        self.SciPiezo_edit.setStyleSheet("color: #f47cfc; border-color: #f47cfc")
        self.SciPiezo_Set_button = QPushButton("GO", self)
        self.SciPiezo_Set_button.clicked.connect(lambda: self.SciPiezo_click(float(self.SciPiezo_edit.text())))
        self.SciPiezo_Set_button.setFixedWidth(100)
        self.SciPiezo_Set_button.setStyleSheet("QPushButton {color: #f47cfc; border-color: #f47cfc}"
                                             "QPushButton:pressed {color: #000000; background-color: #f47cfc}")
        self.SciPiezo_L_button = QPushButton("<<", self)
        self.SciPiezo_L_button.clicked.connect(lambda: self.SciPiezo_click(self.SciPiezo_V-self.SciPiezo_dV))
        self.SciPiezo_L_button.setFixedWidth(50)
        self.SciPiezo_L_button.setStyleSheet("QPushButton {color: #f47cfc; border-color: #f47cfc}"
                                             "QPushButton:pressed {color: #000000; background-color: #f47cfc}")
        self.SciPiezo_R_button = QPushButton(">>", self)
        self.SciPiezo_R_button.clicked.connect(lambda: self.SciPiezo_click(self.SciPiezo_V+self.SciPiezo_dV))
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
        lbl1 = QLabel('Fringe Tracking Stage: ', self)
        lbl2 = QLabel('Number of Steps: ', self)
        lbl3 = QLabel('Direction: ', self)
        lbl4 = QLabel('Frequency (Hz): ', self)
        self.FineStage_numSteps_edit = QLineEdit("0")
        self.FineStage_numSteps_edit.setFixedWidth(120)
        self.FineStage_numSteps_edit.setStyleSheet("color: #ff7f69; border-color: #ff7f69")
        self.FineStage_direction_edit = QLineEdit("0")
        self.FineStage_direction_edit.setFixedWidth(120)
        self.FineStage_direction_edit.setStyleSheet("color: #ff7f69; border-color: #ff7f69")
        self.FineStage_freq_edit = QLineEdit("0")
        self.FineStage_freq_edit.setFixedWidth(120)
        self.FineStage_freq_edit.setStyleSheet("color: #ff7f69; border-color: #ff7f69")
        self.FineStage_Set_button = QPushButton("GO", self)
        self.FineStage_Set_button.clicked.connect(lambda: self.FineStage_click(float(self.FineStage_numSteps_edit.text()),float(self.FineStage_direction_edit.text()),float(self.FineStage_freq_edit.text())))
        self.FineStage_Set_button.setFixedWidth(100)
        self.FineStage_Set_button.setStyleSheet("QPushButton {color: #ff7f69; border-color: #ff7f69}"
                                             "QPushButton:pressed {color: #000000; background-color: #ff7f69}")
        config_grid.addWidget(lbl1,5,0,3,1)
        config_grid.addWidget(lbl2,5,1)
        config_grid.addWidget(lbl3,6,1)
        config_grid.addWidget(lbl4,7,1)
        config_grid.addWidget(self.FineStage_numSteps_edit,5,2)
        config_grid.addWidget(self.FineStage_direction_edit,6,2)
        config_grid.addWidget(self.FineStage_freq_edit,7,2)
        config_grid.addWidget(self.FineStage_Set_button,5,3,3,1)
        
        Piezo_layout.addLayout(config_grid)
        Piezo_layout.setAlignment(Qt.AlignTop)
        
        content_layout.addLayout(Piezo_layout,1)
        content_layout.addSpacing(50)

        LED_master_layout = QVBoxLayout()

        LED_layout = QHBoxLayout()
        lbl = QLabel("Power System Controls:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        LED_layout.addWidget(lbl)
        
        #LED_layout.addSpacing(50)
        LED_master_layout.addLayout(LED_layout)
        LED_master_layout.addSpacing(20)
        LED_layout = QHBoxLayout()
        
        self.getPower_button = QPushButton("Get Power", self)
        self.getPower_button.setFixedWidth(200)
        self.getPower_button.clicked.connect(self.getPower_func)
        #self.getPower_button.setAlignment(Qt.AlignLeft)
        #LED_layout.addStretch()
        LED_layout.addWidget(self.getPower_button)
        LED_layout.setAlignment(Qt.AlignLeft)
        
        LED_master_layout.addLayout(LED_layout)
        LED_master_layout.addSpacing(20)
        
        LED_layout = QHBoxLayout()
        self.voltage = QLabel("0.00 V")
        self.voltage.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        LED_layout.addWidget(self.voltage)
        LED_layout.addSpacing(50)
        self.current = QLabel("0.00 A")
        self.current.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        LED_layout.addWidget(self.current)
        LED_layout.addSpacing(50)
        # Complete setup, add status labels and indicators
        self.LED_status_light = 'assets/red.svg'
        self.LED_status_text = 'Voltage getting low'
        self.LED_svgWidget = QSvgWidget(self.LED_status_light)
        self.LED_svgWidget.setFixedSize(20,20)
        self.LED_status_label = QLabel(self.LED_status_text, self)
        LED_layout.addWidget(self.LED_svgWidget)
        LED_layout.addWidget(self.LED_status_label)
        LED_layout.addStretch()
        LED_master_layout.addLayout(LED_layout)
        
        LED_master_layout.setAlignment(Qt.AlignTop)
        
        content_layout.addLayout(LED_master_layout,1)
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
        self.ask_for_status()

    def change_ip(self,IP):
        self.socket = ClientSocket(IP=IP, Port=self.port)

    def ask_for_status(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""
        #As this is on a continuous timer, only do anything if we are
        #connected
        response = self.socket.send_command("CA.status")
        if (self.socket.connected):
            self.status_light = "assets/green.svg"
            self.svgWidget.load(self.status_light)

            if response == '"Server Not Connected!"':
                self.Connect_button.setText("Connect")
                self.Connect_button.setChecked(False)
            else:
                self.Connect_button.setChecked(True)
                self.Connect_button.setText("Disconnect")

            self.response_label.append(response)
            self.status_text = "Socket Connected"
            self.status_label.setText(self.status_text)

        else:
            self.status_light = "assets/red.svg"
            self.status_text = "Socket Not Connected"
            self.status_label.setText(self.status_text)
            self.svgWidget.load(self.status_light)

    #What happens when you click the info button
    def info_click(self):
        print(self.name)
        self.ask_for_status()
        
    def Dextra_XY_click(self,voltageX,voltageY):
        if self.Connect_button.isChecked():
            response = self.send_to_server_with_response("CA.tiptiltD [%s,%s]"%(voltageX,voltageY))
            try:
                response_dict = json.loads(response)
                self.Dextra_X_V = response_dict["X_voltage"]
                self.Dextra_Y_V = response_dict["Y_voltage"]
                self.Dextra_X_lbl.setText("{:.1f} V".format(self.Dextra_X_V))
                self.Dextra_Y_lbl.setText("{:.1f} V".format(self.Dextra_Y_V))
                
            except:
                return
            
        else:
            print("Server Not Connected")
            self.response_label.append("Server Not Connected")
        return

    def Sinistra_XY_click(self,voltageX,voltageY):
        if self.Connect_button.isChecked():
            response = self.send_to_server_with_response("CA.tiptiltS [%s,%s]"%(voltageX,voltageY))
            try:
                response_dict = json.loads(response)
                self.Sinistra_X_V = response_dict["X_voltage"]
                self.Sinistra_Y_V = response_dict["Y_voltage"]
                self.Sinistra_X_lbl.setText("{:.1f} V".format(self.Sinistra_X_V))
                self.Sinistra_Y_lbl.setText("{:.1f} V".format(self.Sinistra_Y_V))
                
            except:
                return
            
        else:
            print("Server Not Connected")
            self.response_label.append("Server Not Connected")
        return
        
    def SciPiezo_click(self,voltage):
        if self.Connect_button.isChecked():
            response = self.send_to_server_with_response("CA.scipiezo [%s]"%(voltage))
            try:
                response_dict = json.loads(response)
                self.SciPiezo_V = response_dict["voltage"]
                self.SciPiezo_lbl.setText("{:.2f} V".format(self.SciPiezo_V))
                
            except:
                return
            
        else:
            print("Server Not Connected")
            self.response_label.append("Server Not Connected")
        return
        
    def FineStage_click(self,numSteps,direction,freq):
        if self.Connect_button.isChecked():
            self.send_to_server("CA.finestage [%s,%s,%s]"%(numSteps,direction,freq))
        else:
            print("Server Not Connected")
            self.response_label.append("Server Not Connected")
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


    def getPower_func(self):
        power_str = self.send_to_server_with_response("CA.reqpower")
        print("Sending 'getPower' command")
        try:
            power_dict = json.loads(power_str)
        except:
            return
        self.voltage.setText("{:.2f} V".format(power_dict["voltage"]))
        self.current.setText("{:.2f} A".format(power_dict["current"]))
        
        if power_dict["voltage"] > self.voltage_limit:
            self.LED_status_light = "assets/green.svg"
            self.LED_svgWidget.load(self.LED_status_light)
            self.LED_status_text = "Voltage good"
            self.LED_status_label.setText(self.LED_status_text)
        else:
            self.LED_status_light = "assets/red.svg"
            self.LED_status_text = "Voltage getting low"
            self.LED_status_label.setText(self.LED_status_text)
            self.LED_svgWidget.load(self.LED_status_light)            


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
