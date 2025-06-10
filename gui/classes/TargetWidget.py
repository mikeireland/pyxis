#!/usr/bin/env python
from __future__ import print_function, division
from conversion import RaDecDot
from astroquery.simbad import Simbad
from RawWidget import RawWidget
import numpy as np
import json

try:
    from PyQt5.QtWidgets import QPushButton, QHBoxLayout, QLabel, QLineEdit
except:
    print("Please install PyQt5.")
    raise UserWarning


class TargetWidget(RawWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(TargetWidget,self).__init__(config,IP,parent)

        # Line edit to send a command to SIMBAD
        hbox = QHBoxLayout()
        lbl = QLabel('Command to SIMBAD: ', self)
        self.SIMBAD_line_edit = QLineEdit("")
        self.SIMBAD_line_edit.returnPressed.connect(self.send_to_SIMBAD)
        self.SIMBAD_submit_button = QPushButton("Submit", self)
        self.SIMBAD_submit_button.clicked.connect(self.send_to_SIMBAD)
        hbox.addWidget(lbl)
        hbox.addWidget(self.SIMBAD_line_edit)
        hbox.addWidget(self.SIMBAD_submit_button)
        self.full_window.addLayout(hbox)

        self.full_window.addSpacing(30)

        self.simbad = Simbad()
        self.simbad.add_votable_fields('ra(d;A)', 'dec(d;D)')
        self.simbad.remove_votable_fields('coordinates')

        # Layout to show the new target's information
        Coord_layout = QHBoxLayout()
        lbl = QLabel("New target:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Coord_layout.addWidget(lbl)
        Coord_layout.addStretch()
        self.new_Target_Name = QLabel("Target: ")
        self.new_Target_Name.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffffff}")
        Coord_layout.addWidget(self.new_Target_Name)
        Coord_layout.addSpacing(50)
        self.new_RA = QLabel("RA = 0.0")
        self.new_RA.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffffff}")
        Coord_layout.addWidget(self.new_RA)
        Coord_layout.addSpacing(50)
        self.new_DEC = QLabel("DEC = 0.0")
        self.new_DEC.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffffff}")
        Coord_layout.addWidget(self.new_DEC)
        Coord_layout.addSpacing(50)
        self.Set_Target_button = QPushButton("Set Target", self)
        self.Set_Target_button.setFixedWidth(200)
        self.Set_Target_button.clicked.connect(self.set_target_func)
        Coord_layout.addWidget(self.Set_Target_button)
        Coord_layout.addStretch()
        self.full_window.addLayout(Coord_layout)
        self.full_window.addSpacing(10)

        # Show the Alt/Az in degrees
        Coord_layout = QHBoxLayout()
        lbl = QLabel("Alt/Az (degrees):",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Coord_layout.addWidget(lbl)
        Coord_layout.addStretch()
        self.ALT = QLabel("ALT = 0.0")
        self.ALT.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffffff}")
        Coord_layout.addWidget(self.ALT)
        Coord_layout.addSpacing(50)
        self.AZ = QLabel("AZ = 0.0")
        self.AZ.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffffff}")
        Coord_layout.addWidget(self.AZ)
        Coord_layout.addStretch()
        self.full_window.addLayout(Coord_layout)
        self.full_window.addSpacing(10)

        # Show the velocities in rad/s
        Coord_layout = QHBoxLayout()
        lbl = QLabel("Velocities (rad/s):",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Coord_layout.addWidget(lbl)
        Coord_layout.addStretch()
        self.dALTdt = QLabel("dALTdt = 0.0")
        self.dALTdt.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffffff}")
        Coord_layout.addWidget(self.dALTdt)
        Coord_layout.addSpacing(50)
        self.dAZdt = QLabel("dAZdt = 0.0")
        self.dAZdt.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffffff}")
        Coord_layout.addWidget(self.dAZdt)
        Coord_layout.addStretch()
        self.full_window.addLayout(Coord_layout)
        self.full_window.addSpacing(10)
        
        # Baseline update command
        Coord_layout = QHBoxLayout()
        lbl = QLabel("Baseline:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Coord_layout.addWidget(lbl)
        Coord_layout.addStretch()
        lbl = QLabel('Input: ', self)
        self.baseline_line_edit = QLineEdit("1.0")
        self.baseline_line_edit.returnPressed.connect(self.change_baseline)
        self.baseline_submit_button = QPushButton("Configure", self)
        self.baseline_submit_button.clicked.connect(self.change_baseline)
        Coord_layout.addWidget(lbl)
        Coord_layout.addWidget(self.baseline_line_edit)
        Coord_layout.addWidget(self.baseline_submit_button)
        Coord_layout.addStretch()
        
        self.full_window.addLayout(Coord_layout)
        self.full_window.addSpacing(30)

        # Current target (sent to server!)
        Coord_layout = QHBoxLayout()
        lbl = QLabel("Current target:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Coord_layout.addWidget(lbl)
        Coord_layout.addStretch()
        self.current_Target_Name = QLabel("Target: ")
        self.current_Target_Name.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Coord_layout.addWidget(self.current_Target_Name)
        Coord_layout.addSpacing(50)
        self.current_RA = QLabel("RA = 0.0")
        self.current_RA.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Coord_layout.addWidget(self.current_RA)
        Coord_layout.addSpacing(50)
        self.current_DEC = QLabel("DEC = 0.0")
        self.current_DEC.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Coord_layout.addWidget(self.current_DEC)
        Coord_layout.addSpacing(50)
        self.current_baseline = QLabel("Baseline = 0.0")
        self.current_baseline.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Coord_layout.addWidget(self.current_baseline)
        Coord_layout.addStretch()

        self.full_window.addLayout(Coord_layout)
        self.ask_for_status()


    """ Ask for the server's status and update the current target coordinates"""
    def ask_for_status(self):
        response = self.socket.send_command("TS.getCoordinates")
        if (self.socket.connected):

            try:
                response_dict = json.loads(response)
            except:
                return

            self.current_RA.setText(str(response_dict["RA"]))
            self.current_DEC.setText(str(response_dict["DEC"]))

            self.status_light = "assets/green.svg"
            self.svgWidget.load(self.status_light)
            self.status_text = "Socket Connected"
            self.status_label.setText(self.status_text)

        else:
            self.status_light = "assets/red.svg"
            self.status_text = "Socket Not Connected"
            self.status_label.setText(self.status_text)
            self.svgWidget.load(self.status_light)


    """ Send the command to SIMBAD and parse it"""
    def send_to_SIMBAD(self):
        table = self.simbad.query_object(str(self.SIMBAD_line_edit.text()))
        if table is None:
            self.response_label.append("Target not found")
            return
        ra = '{0:.4f}'.format(table["RA_d_A"][0])
        dec = '{0:.4f}'.format(table["DEC_d_D"][0])
        ra_str = "RA = "+ra
        dec_str = "DEC = "+dec
        name = "Target = "+str(table["MAIN_ID"][0])#.split("*")[1]
        self.new_Target_Name.setText(name)
        self.new_RA.setText(ra_str)
        self.new_DEC.setText(dec_str)

        ALT, AZ, dALTdt, dAZdt = RaDecDot(float(ra),float(dec))
        ALT_str = "ALT = " + '{0:.4f}'.format(np.degrees(ALT))
        AZ_str = "AZ = " + '{0:.4f}'.format(np.degrees(AZ))
        dALTdt_str = "dALTdt = " + '{0:.4f}'.format(dALTdt)
        dAZdt_str = "dAZdt = " + '{0:.4f}'.format(dAZdt)
        self.ALT.setText(ALT_str)
        self.AZ.setText(AZ_str)
        self.dALTdt.setText(dALTdt_str)
        self.dAZdt.setText(dAZdt_str)

        return


    """ Function to send the coordinates of the target to the server """
    def set_target_func(self):
        #Edited by Qianhui: check the coordinates > +45deg before sending to the server
        if float(self.new_DEC.text().split('=')[1]) > 45.0:
            self.send_to_server("TS.setCoordinates %s,%s"%(str(float(self.new_RA.text().split('=')[1])),str(float(self.new_DEC.text().split('=')[1]))))
            self.send_to_server('TS.setTargetName "%s"'%self.new_Target_Name.text().split('=')[1])
            self.current_Target_Name.setText(str(self.new_Target_Name.text()))
            self.current_RA.setText(str(self.new_RA.text()))
            self.current_DEC.setText(str(self.new_DEC.text()))
        else:
            self.response_label.append("Target DEC must be > +45 degrees. Please observe a different target.")
        


    """ Function to change the current baseline"""
    def change_baseline(self):
        self.send_to_server("TS.setBaseline %s"%str(float(self.baseline_line_edit.text())))
        baseline = "Baseline = "+str(float(self.baseline_line_edit.text()))
        self.current_baseline.setText(baseline)
