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


class RobotControlWidget(QWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(RobotControlWidget,self).__init__(parent)

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

        button_layout = QHBoxLayout()
        self.zero_button = QPushButton("Zero", self)
        self.zero_button.setFixedWidth(200)
        self.zero_button.clicked.connect(self.zero_button_func)
        button_layout.addWidget(self.zero_button)
        self.level_button = QPushButton("Level", self)
        self.level_button.setFixedWidth(200)
        self.level_button.clicked.connect(self.level_button_func)
        button_layout.addWidget(self.level_button)
        self.stop_button = QPushButton("Stop", self)
        self.stop_button.setFixedWidth(200)
        self.stop_button.clicked.connect(self.stop_button_func)
        button_layout.addWidget(self.stop_button)
        vBoxlayout.addLayout(button_layout)

        vBoxlayout.addSpacing(30)

        hbox1 = QHBoxLayout()
        lbl1 = QLabel('Velocity (mm/s): ', self)
        self.translate_line_edit = QLineEdit("0")
        self.translate_button = QPushButton("Translate", self)
        self.translate_button.clicked.connect(self.translate)
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.translate_line_edit)
        hbox1.addSpacing(20)
        hbox1.addWidget(self.translate_button)
        vBoxlayout.addLayout(hbox1)

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

    def zero_button_func(self):
        self.send_to_server("zero")
        print("Sending 'Zero' command")

    def level_button_func(self):
        self.send_to_server("level")
        print("Sending 'Level' command")

    def stop_button_func(self):
        self.send_to_server("stop")
        print("Sending 'Stop' command")

    def translate(self):
        try:
            velocity = float(self.translate_line_edit.text())
        except:
            self.response_label.append("Gui ERROR: translation speed must be a float")
            return

        self.send_to_server("translate, %f"%velocity)

        print("Sending 'Translate' command with speed {:#.4g} mm/s".format(velocity))



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
