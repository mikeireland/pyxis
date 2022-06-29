#!/usr/bin/env python
from __future__ import print_function, division
from client_socket import ClientSocket
import random
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


class PowerControlWidget(QWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(PowerControlWidget,self).__init__(parent)

        self.name = config["name"]
        self.port = config["port"]
        self.socket = ClientSocket(IP=IP, Port=self.port)

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
        self.line_edit = QLineEdit("")
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

        LED_layout = QHBoxLayout()
        lbl = QLabel("LED Controls:",self)
        LED_layout.addWidget(lbl)
        LED_layout.addStretch()
        self.startBlink_button = QPushButton("Start Blink", self)
        self.startBlink_button.setFixedWidth(200)
        self.startBlink_button.clicked.connect(self.startBlink_button_func)
        LED_layout.addWidget(self.startBlink_button)
        LED_layout.addSpacing(50)
        self.stopBlink_button = QPushButton("Stop Blink", self)
        self.stopBlink_button.setFixedWidth(200)
        self.stopBlink_button.clicked.connect(self.stopBlink_button_func)
        LED_layout.addWidget(self.stopBlink_button)
        LED_layout.addStretch()
        vBoxlayout.addLayout(LED_layout)

        vBoxlayout.addSpacing(20)

        LED_layout = QHBoxLayout()
        lbl = QLabel("Power System Controls:",self)
        LED_layout.addWidget(lbl)
        LED_layout.addStretch()
        self.getVoltage_button = QPushButton("Get Voltage", self)
        self.getVoltage_button.setFixedWidth(200)
        self.getVoltage_button.clicked.connect(self.getVoltage_func)
        LED_layout.addWidget(self.getVoltage_button)
        LED_layout.addSpacing(50)
        self.voltage = QLabel("0.00 V")
        self.voltage.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ff7e40}")
        LED_layout.addWidget(self.voltage)
        LED_layout.addSpacing(50)
        self.getCurrent_button = QPushButton("Get Current", self)
        self.getCurrent_button.setFixedWidth(200)
        self.getCurrent_button.clicked.connect(self.getCurrent_func)
        LED_layout.addWidget(self.getCurrent_button)
        LED_layout.addSpacing(50)
        self.current = QLabel("0.00 A")
        self.current.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ff7e40}")
        LED_layout.addWidget(self.current)
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

    def change_ip(self,IP):
        self.socket = ClientSocket(IP=IP, Port=self.port)

    def ask_for_status(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""
        command = "INFO"
        #As this is on a continuous timer, only do anything if we are
        #connected
        k = random.randint(0, 1)
        #if (self.socket.connected):
        if k:
            self.status_light = "assets/green.svg"
            self.svgWidget.load(self.status_light)
            response = self.socket.send_command("INFO")
            self.status_text = "Socket Connected"
            self.status_label.setText(self.status_text)
            if type(response)!=str and type(response)!=unicode:
                raise UserWarning("Incorrect INFO response!")
            if response[:5]=='Error':
                print("Error in INFO response from {:s}...".format(self.name))
            else:
                status_list = response.split('\n')
                if len(status_list)<3:
                    status_list = response.split(' ')
                status = {t.split("=")[0].lstrip():t.split("=")[1] for t in status_list if t.find("=")>0}
                #Now deal with the response in a different way for each server.
                self.status_label.setText("Text: {:6.3f} Tset: {:6.3f} Tmc: {:6.3f}".format(\
                    float(status["externalenclosure"]),\
                    float(status["externalenclosure.setpoint"]),\
                    float(status["minichiller.internal"])))
        else:
            self.status_light = "assets/red.svg"
            self.status_text = "Socket Not Connected"
            self.status_label.setText(self.status_text)
            self.svgWidget.load(self.status_light)

    #What happens when you click the info button
    def info_click(self):
        print(self.name)
        self.ask_for_status()
        self.send_to_server("INFO")


    def command_enter(self):
        """Parse the LineEdit string and send_to_server
        """
        self.send_to_server(str(self.line_edit.text()))

    def startBlink_button_func(self):
        self.send_to_server("startBlink")
        print("Sending 'startBlink' command")

    def stopBlink_button_func(self):
        self.send_to_server("stopBlink")
        print("Sending 'stopBlink' command")

    def getVoltage_func(self):
        voltage_str = self.send_to_server_with_response("getVoltage")
        print("Sending 'getVoltage' command")
        try:
            float(voltage_str)
        except:
            return
        self.voltage.setText(voltage_str+" V")


    def getCurrent_func(self):
        current_str = self.send_to_server_with_response("getCurrent")
        print("Sending 'getCurrent' command")
        try:
            float(current_str)
        except:
            return
        self.current.setText(current_str+" A")



    def send_to_server_with_response(self, text):
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
        self.line_edit.setText("")
