#!/usr/bin/env python
from __future__ import print_function, division
from client_socket import ClientSocket
import random
import json
from astroquery.simbad import Simbad
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


class TargetWidget(QWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(TargetWidget,self).__init__(parent)

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
        self.line_edit = QLineEdit("DA.")
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

        lbl1 = QLabel('Command to SIMBAD: ', self)
        self.SIMBAD_line_edit = QLineEdit("")
        self.SIMBAD_line_edit.returnPressed.connect(self.send_to_SIMBAD)

        #Next, the info button
        self.SIMBAD_submit_button = QPushButton("Submit", self)
        self.SIMBAD_submit_button.clicked.connect(self.send_to_SIMBAD)

        hbox1 = QHBoxLayout()
        hbox1.addWidget(lbl1)
        hbox1.addWidget(self.SIMBAD_line_edit)
        hbox1.addWidget(self.SIMBAD_submit_button)
        vBoxlayout.addLayout(hbox1)

        vBoxlayout.addSpacing(30)

        self.simbad = Simbad()
        self.simbad.add_votable_fields('ra(d;A)', 'dec(d;D)')
        self.simbad.remove_votable_fields('coordinates')

        Coord_layout = QHBoxLayout()
        lbl = QLabel("New target:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Coord_layout.addWidget(lbl)
        Coord_layout.addStretch()
        self.new_Target_Name = QLabel("Target: ")
        self.new_Target_Name.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Coord_layout.addWidget(self.new_Target_Name)
        Coord_layout.addSpacing(50)
        self.new_RA = QLabel("RA = ")
        self.new_RA.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Coord_layout.addWidget(self.new_RA)
        Coord_layout.addSpacing(50)
        self.new_DEC = QLabel("DEC = ")
        self.new_DEC.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Coord_layout.addWidget(self.new_DEC)
        Coord_layout.addSpacing(50)
        self.Set_Target_button = QPushButton("Set Target", self)
        self.Set_Target_button.setFixedWidth(200)
        self.Set_Target_button.clicked.connect(self.set_target_func)
        Coord_layout.addWidget(self.Set_Target_button)

        vBoxlayout.addLayout(Coord_layout)
        vBoxlayout.addSpacing(30)

        Coord_layout = QHBoxLayout()
        lbl = QLabel("Current target:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Coord_layout.addWidget(lbl)
        Coord_layout.addStretch()
        self.current_Target_Name = QLabel("Target: ")
        self.current_Target_Name.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Coord_layout.addWidget(self.current_Target_Name)
        Coord_layout.addSpacing(50)
        self.current_RA = QLabel("RA = ")
        self.current_RA.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Coord_layout.addWidget(self.current_RA)
        Coord_layout.addSpacing(50)
        self.current_DEC = QLabel("DEC = ")
        self.current_DEC.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Coord_layout.addWidget(self.current_DEC)

        vBoxlayout.addLayout(Coord_layout)


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

    def send_to_SIMBAD(self):
        table = self.simbad.query_object(str(self.SIMBAD_line_edit.text()))
        ra = table["RA_d_A"][0]
        dec = table["DEC_d_D"][0]
        name = table["MAIN_ID"][0]
        self.new_Target_Name.setText(name)
        self.new_RA.setText(ra)
        self.new_DEC.setText(dec)


    def set_target_func(self):
        self.send_to_server("TS.setCoordinates [%s,%s]"%(self.current_RA,self.current_DEC))
        self.current_Target_Name.setText(str(self.new_Target_Name.text()))
        self.current_RA.setText(str(self.new_RA.text()))
        self.current_DEC.setText(str(self.new_DEC.text()))

    def command_enter(self):
        """Parse the LineEdit string and send_to_server
        """
        self.send_to_server(str(self.line_edit.text()))
        

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
        self.line_edit.setText("DA.")

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
