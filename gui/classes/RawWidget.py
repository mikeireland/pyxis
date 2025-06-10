#!/usr/bin/env python
from __future__ import print_function, division
from client_socket import ClientSocket
import json

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
        hbox = QHBoxLayout()
        desc = QLabel('Port: %s    Description: %s'%(config["port"],config["description"]), self)
        desc.setStyleSheet("font-weight: bold")
        desc.setFixedHeight(40)
        hbox.setContentsMargins(0, 0, 0, 0)
        hbox.addWidget(desc)
        self.full_window.addLayout(hbox)

        # Command Line
        #First, the command entry box
        lbl = QLabel('Command: ', self)
        self.line_edit = QLineEdit("%s."%self.prefix)
        self.line_edit.returnPressed.connect(self.command_enter)

        #Next, the refresh button
        self.refresh_button = QPushButton("Refresh", self)
        self.refresh_button.clicked.connect(self.refresh_click)

        hbox = QHBoxLayout()
        hbox.addWidget(lbl)
        hbox.addWidget(self.line_edit)
        hbox.addWidget(self.refresh_button)
        self.mainPanel.addLayout(hbox)

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

    """ Basic function to ask the for the server status to see if it's alive
        (and to potentially do things)

        This function should be superceeded by an identically named function inside
        a given GUI class    
    """
    def ask_for_status(self):
        print("NEED TO IMPLEMENT FUNCTION")


    """ Change the IP"""
    def change_ip(self,IP):
        self.socket = ClientSocket(IP=IP, Port=self.port)

    """ What happens when you click the refresh button """
    def refresh_click(self):
        self.ask_for_status()

    """ Parse the LineEdit string and send_to_server """
    def command_enter(self):
        self.send_to_server(str(self.line_edit.text()))

    """ Send a command to the server; don't need to parse response """
    def send_to_server(self, text):
        try:
            response = self.socket.send_command(text)
        except:
            response = "*** Connection Error ***"
        if type(response)==str:
            self.response_label.append(response)
        elif type(response)==bool:
            if response:
                self.response_label.append("Success!")
            else:
                self.response_label.append("Failure!")
        self.line_edit.setText("%s."%self.prefix)


    """ Send a command to the server; parses the response and returns it """
    def send_to_server_with_response(self, text):
        try:
            response = self.socket.send_command(text)
        except:
            response = "*** Connection Error ***"
        if type(response)==str:
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
