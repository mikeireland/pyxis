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
        QVBoxLayout, QLabel, QLineEdit, QTextEdit
    from PyQt5.QtSvg import QSvgWidget
except:
    print("Please install PyQt5.")
    raise UserWarning


class DepAuxControlWidget(QWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(DepAuxControlWidget,self).__init__(parent)

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
        self.line_edit = QLineEdit("DA.")
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

        LED_layout = QHBoxLayout()
        lbl = QLabel("LED Controls:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        LED_layout.addWidget(lbl)
        LED_layout.addStretch()
        self.LEDOn_button = QPushButton("LED On", self)
        self.LEDOn_button.setFixedWidth(200)
        self.LEDOn_button.clicked.connect(self.LEDOn_button_func)
        LED_layout.addWidget(self.LEDOn_button)
        LED_layout.addSpacing(50)
        self.LEDOff_button = QPushButton("LED Off", self)
        self.LEDOff_button.setFixedWidth(200)
        self.LEDOff_button.clicked.connect(self.LEDOff_button_func)
        LED_layout.addWidget(self.LEDOff_button)
        LED_layout.addStretch()
        vBoxlayout.addLayout(LED_layout)

        vBoxlayout.addSpacing(20)

        LED_layout = QHBoxLayout()
        lbl = QLabel("Power System Controls:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        LED_layout.addWidget(lbl)
        LED_layout.addStretch()
        self.getPower_button = QPushButton("Get Power", self)
        self.getPower_button.setFixedWidth(200)
        self.getPower_button.clicked.connect(self.getPower_func)
        LED_layout.addWidget(self.getPower_button)
        LED_layout.addSpacing(50)
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
        
        vBoxlayout.addLayout(LED_layout)

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
        response = self.socket.send_command("DA.status")
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


    def command_enter(self):
        """Parse the LineEdit string and send_to_server
        """
        self.send_to_server(str(self.line_edit.text()))
        
    def connect_server(self):

        if self.Connect_button.isChecked():
            # Refresh camera
            self.Connect_button.setText("Disconnect")
            print("Server is now Connected")
            self.send_to_server("DA.connect")

        else:
            self.Connect_button.setText("Connect")
            print("Disconnecting Server")
            self.send_to_server("DA.disconnect")

    def LEDOn_button_func(self):
        self.send_to_server("DA.LEDOn")
        print("Sending 'LEDOn' command")

    def LEDOff_button_func(self):
        self.send_to_server("DA.LEDOff")
        print("Sending 'LEDOff' command")

    def getPower_func(self):
        power_str = self.send_to_server_with_response("DA.reqpower")
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
        self.line_edit.setText("")

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
        self.line_edit.setText("DA.")
