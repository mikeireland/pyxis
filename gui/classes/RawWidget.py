#!/usr/bin/env python
from __future__ import print_function, division
from client_socket import ClientSocket
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


class RawWidget(QWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(RawWidget,self).__init__(parent)

        self.name = config["name"]
        self.port = config["port"]
        self.prefix = config["prefix"]
        self.socket = ClientSocket(IP=IP, Port=self.port)

        #Layout the common elements
        self.full_window = QVBoxLayout()
        self.full_window.setSpacing(3)

        self.mainHBox = QHBoxLayout()
        self.mainPanel = QVBoxLayout()
        self.sidePanel = QVBoxLayout()
        #Description
        desc = QLabel('Port: %s    Description: %s'%(config["port"],config["description"]), self)
        desc.setStyleSheet("font-weight: bold")
        desc.setFixedHeight(40)
        hbox1 = QHBoxLayout()
        hbox1.setContentsMargins(0, 0, 0, 0)
        hbox1.addWidget(desc)
        self.full_window.addLayout(hbox1)

        #First, the command entry box
        lbl1 = QLabel('Command: ', self)
        self.line_edit = QLineEdit("%s."%self.prefix)
        self.line_edit.returnPressed.connect(self.command_enter)

        #Next, the info button
        self.info_button = QPushButton("Refresh", self)
        self.info_button.clicked.connect(self.info_click)

        hbox1 = QHBoxLayout()
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.line_edit)
        hbox1.addWidget(self.info_button)
        self.mainPanel.addLayout(hbox1)

        #Next, the response box
        self.response_label = QTextEdit('[No Server Response Yet]', self)
        self.response_label.setReadOnly(True)
        self.response_label.setStyleSheet("QTextEdit { background-color : black; }")
        self.response_label.setFixedHeight(150)
        self.mainPanel.addWidget(self.response_label)
        self.mainPanel.addSpacing(30)
        # Complete setup, add status labels and indicators
        self.mainHBox.addLayout(self.mainPanel)
        self.mainHBox.addLayout(self.sidePanel)
        self.full_window.addLayout(self.mainHBox)

        status_layout = QHBoxLayout()
        self.status_light = 'assets/red.svg'
        self.status_text = 'Socket Not Connected'
        self.svgWidget = QSvgWidget(self.status_light)
        self.svgWidget.setFixedSize(20,20)
        self.status_label = QLabel(self.status_text, self)
        status_layout.addWidget(self.svgWidget)
        status_layout.addWidget(self.status_label)

        self.full_window.addLayout(status_layout)

        self.setLayout(self.full_window)


    def ask_for_status(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""
        print("NEED TO IMPLEMENT FUNCTION")

    def change_ip(self,IP):
        self.socket = ClientSocket(IP=IP, Port=self.port)

    #What happens when you click the info button
    def info_click(self):
        print(self.name)
        self.ask_for_status()

    def command_enter(self):
        """Parse the LineEdit string and send_to_server
        """
        self.send_to_server(str(self.line_edit.text()))


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
                self.response_label.append("Success!")
            else:
                self.response_label.append("Failure!")
        self.line_edit.setText("%s."%self.prefix)


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
                self.response_label.append("Success!")
            else:
                self.response_label.append("Failure!")
        return response
