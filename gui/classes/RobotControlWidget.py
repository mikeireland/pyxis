#!/usr/bin/env python
from __future__ import print_function, division
from RawWidget import RawWidget
import json

try:
    from PyQt5.QtWidgets import QPushButton, QHBoxLayout, \
        QVBoxLayout, QLabel, QLineEdit, QGridLayout
    from PyQt5.QtCore import Qt
except:
    print("Please install PyQt5.")
    raise UserWarning


class RobotControlWidget(RawWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(RobotControlWidget,self).__init__(config,IP,parent)

        button_layout = QHBoxLayout()
        self.start_button = QPushButton("Start", self)
        self.start_button.setFixedWidth(200)
        self.start_button.clicked.connect(self.start_button_func)
        button_layout.addWidget(self.start_button)
        self.level_button = QPushButton("Level", self)
        self.level_button.setFixedWidth(200)
        self.level_button.clicked.connect(self.level_button_func)
        button_layout.addWidget(self.level_button)
        self.stop_button = QPushButton("Stop", self)
        self.stop_button.setFixedWidth(200)
        self.stop_button.clicked.connect(self.stop_button_func)
        button_layout.addWidget(self.stop_button)
        self.hard_stop_button = QPushButton("Disconnect", self)
        self.hard_stop_button.setFixedWidth(200)
        self.hard_stop_button.clicked.connect(self.hard_stop_button_func)
        button_layout.addWidget(self.hard_stop_button)

        self.full_window.addLayout(button_layout)

        self.full_window.addSpacing(30)

        # Motor control layout
        content_layout = QVBoxLayout()
        lbl = QLabel("Motor Controls (velocity in units of mm/s or arcsec/s):",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        content_layout.addWidget(lbl)
        content_layout.addSpacing(30)
        config_grid = QGridLayout()
        config_grid.setColumnMinimumWidth(0,150)
        config_grid.setColumnMinimumWidth(1,50)
        config_grid.setColumnMinimumWidth(2,140)
        config_grid.setVerticalSpacing(5)

        
        hbox = QHBoxLayout()
        lbl = QLabel('X Translation', self)
        self.Robot_X_edit = QLineEdit("0.0")
        self.Robot_X_edit.setFixedWidth(120)
        self.Robot_X_L_button = QPushButton("<<", self)
        self.Robot_X_L_button.pressed.connect(lambda: self.move_func(0,-float(self.Robot_X_edit.text())))
        self.Robot_X_L_button.released.connect(self.stop_button_func)
        self.Robot_X_L_button.setFixedWidth(50)
        self.Robot_X_R_button = QPushButton(">>", self)
        self.Robot_X_R_button.pressed.connect(lambda: self.move_func(0,float(self.Robot_X_edit.text())))
        self.Robot_X_R_button.released.connect(self.stop_button_func)
        self.Robot_X_R_button.setFixedWidth(50)
        hbox.addStretch()
        hbox.addWidget(lbl)
        hbox.addSpacing(20)
        hbox.addWidget(self.Robot_X_L_button)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_X_edit)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_X_R_button)
        hbox.addStretch()
        config_grid.addLayout(hbox,0,0)

        # Y Translation
        hbox = QHBoxLayout()
        lbl = QLabel('Y Translation', self)
        self.Robot_Y_edit = QLineEdit("0.0")
        self.Robot_Y_edit.setFixedWidth(120)
        self.Robot_Y_L_button = QPushButton("<<", self)
        self.Robot_Y_L_button.pressed.connect(lambda: self.move_func(1,-float(self.Robot_Y_edit.text())))
        self.Robot_Y_L_button.released.connect(self.stop_button_func)
        self.Robot_Y_L_button.setFixedWidth(50)
        self.Robot_Y_R_button = QPushButton(">>", self)
        self.Robot_Y_R_button.pressed.connect(lambda: self.move_func(1,float(self.Robot_Y_edit.text())))
        self.Robot_Y_R_button.released.connect(self.stop_button_func)
        self.Robot_Y_R_button.setFixedWidth(50)
        hbox.addStretch()
        hbox.addWidget(lbl)
        hbox.addSpacing(20)
        hbox.addWidget(self.Robot_Y_L_button)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_Y_edit)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_Y_R_button)
        hbox.addStretch()
        config_grid.addLayout(hbox,1,0)

        # Z Translation
        hbox = QHBoxLayout()
        lbl = QLabel('Z Translation', self)
        self.Robot_Z_edit = QLineEdit("0.0")
        self.Robot_Z_edit.setFixedWidth(120)
        self.Robot_Z_L_button = QPushButton("<<", self)
        self.Robot_Z_L_button.pressed.connect(lambda: self.move_func(2,-float(self.Robot_Z_edit.text())))
        self.Robot_Z_L_button.released.connect(self.stop_button_func)
        self.Robot_Z_L_button.setFixedWidth(50)
        self.Robot_Z_R_button = QPushButton(">>", self)
        self.Robot_Z_R_button.pressed.connect(lambda: self.move_func(2,float(self.Robot_Z_edit.text())))
        self.Robot_Z_R_button.released.connect(self.stop_button_func)
        self.Robot_Z_R_button.setFixedWidth(50)
        hbox.addStretch()
        hbox.addWidget(lbl)
        hbox.addSpacing(20)
        hbox.addWidget(self.Robot_Z_L_button)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_Z_edit)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_Z_R_button)
        hbox.addStretch()
        config_grid.addLayout(hbox,2,0)

        # Roll movement
        hbox = QHBoxLayout()
        lbl = QLabel('Roll', self)
        lbl.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.Robot_roll_edit = QLineEdit("0.0")
        self.Robot_roll_edit.setFixedWidth(120)
        self.Robot_roll_L_button = QPushButton("<<", self)
        self.Robot_roll_L_button.pressed.connect(lambda: self.move_func(3,-float(self.Robot_roll_edit.text())))
        self.Robot_roll_L_button.released.connect(self.stop_button_func)
        self.Robot_roll_L_button.setFixedWidth(50)
        self.Robot_roll_R_button = QPushButton(">>", self)
        self.Robot_roll_R_button.pressed.connect(lambda: self.move_func(3,float(self.Robot_roll_edit.text())))
        self.Robot_roll_R_button.released.connect(self.stop_button_func)
        self.Robot_roll_R_button.setFixedWidth(50)
        hbox.addStretch()
        hbox.addWidget(lbl)
        hbox.addSpacing(20)
        hbox.addWidget(self.Robot_roll_L_button)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_roll_edit)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_roll_R_button)
        hbox.addStretch()
        config_grid.addLayout(hbox,0,1)

        # Pitch movement
        hbox = QHBoxLayout()
        lbl = QLabel('Pitch', self)
        lbl.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.Robot_pitch_edit = QLineEdit("0.0")
        self.Robot_pitch_edit.setFixedWidth(120)
        self.Robot_pitch_L_button = QPushButton("<<", self)
        self.Robot_pitch_L_button.pressed.connect(lambda: self.move_func(4,-float(self.Robot_pitch_edit.text())))
        self.Robot_pitch_L_button.released.connect(self.stop_button_func)
        self.Robot_pitch_L_button.setFixedWidth(50)
        self.Robot_pitch_R_button = QPushButton(">>", self)
        self.Robot_pitch_R_button.pressed.connect(lambda: self.move_func(4,float(self.Robot_pitch_edit.text())))
        self.Robot_pitch_R_button.released.connect(self.stop_button_func)
        self.Robot_pitch_R_button.setFixedWidth(50)
        hbox.addStretch()
        hbox.addWidget(lbl)
        hbox.addSpacing(20)
        hbox.addWidget(self.Robot_pitch_L_button)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_pitch_edit)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_pitch_R_button)
        hbox.addStretch()
        config_grid.addLayout(hbox,1,1)

        # Yaw movement
        hbox = QHBoxLayout()
        lbl = QLabel('Yaw', self)
        lbl.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.Robot_yaw_edit = QLineEdit("0.0")
        self.Robot_yaw_edit.setFixedWidth(120)
        self.Robot_yaw_L_button = QPushButton("<<", self)
        self.Robot_yaw_L_button.pressed.connect(lambda: self.move_func(5,-float(self.Robot_yaw_edit.text())))
        self.Robot_yaw_L_button.released.connect(self.stop_button_func)
        self.Robot_yaw_L_button.setFixedWidth(50)
        self.Robot_yaw_R_button = QPushButton(">>", self)
        self.Robot_yaw_R_button.pressed.connect(lambda: self.move_func(5,float(self.Robot_yaw_edit.text())))
        self.Robot_yaw_R_button.released.connect(self.stop_button_func)
        self.Robot_yaw_R_button.setFixedWidth(50)
        hbox.addStretch()
        hbox.addWidget(lbl)
        hbox.addSpacing(20)
        hbox.addWidget(self.Robot_yaw_L_button)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_yaw_edit)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_yaw_R_button)
        hbox.addStretch()
        config_grid.addLayout(hbox,2,1)
        
        # Gonometer movement
        hbox = QHBoxLayout()
        lbl = QLabel('Goniometer', self)
        lbl.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.Robot_gon_edit = QLineEdit("0.0")
        self.Robot_gon_edit.setFixedWidth(120)
        self.Robot_gon_L_button = QPushButton("<<", self)
        self.Robot_gon_L_button.pressed.connect(lambda: self.move_func(6,-float(self.Robot_gon_edit.text())))
        self.Robot_gon_L_button.released.connect(self.stop_button_func)
        self.Robot_gon_L_button.setFixedWidth(50)
        self.Robot_gon_R_button = QPushButton(">>", self)
        self.Robot_gon_R_button.pressed.connect(lambda: self.move_func(6,float(self.Robot_gon_edit.text())))
        self.Robot_gon_R_button.released.connect(self.stop_button_func)
        self.Robot_gon_R_button.setFixedWidth(50)
        hbox.addStretch()
        hbox.addWidget(lbl)
        hbox.addSpacing(20)
        hbox.addWidget(self.Robot_gon_L_button)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_gon_edit)
        hbox.addSpacing(10)
        hbox.addWidget(self.Robot_gon_R_button)
        hbox.addStretch()
        config_grid.addLayout(hbox,3,1)

        content_layout.addLayout(config_grid)
        self.full_window.addLayout(content_layout)
        self.full_window.addSpacing(20)


        # #Edited by Qianhui to add a status button and pitch&roll info showing
        status_layout = QHBoxLayout()
        hbox = QHBoxLayout()
        lbl = QLabel("Status",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        hbox.addWidget(lbl)
        status_layout.addLayout(hbox)
        status_layout.addSpacing(20)

        #This is the request status button
        # hbox = QHBoxLayout()
        # self.status_button = QPushButton("Request Status", self)
        # self.status_button.setFixedWidth(200)
        # self.status_button.clicked.connect(self.status_button_func)
        # hbox.addWidget(self.status_button)
        # hbox.setAlignment(Qt.AlignLeft)
        # status_layout.addLayout(hbox)
        # status_layout.addSpacing(20)

        # Pitch and Roll info
        hbox = QHBoxLayout()
        self.pitch = QLabel("Pitch: ")
        self.pitch.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        status_layout.addWidget(self.pitch)
        status_layout.addSpacing(20)
        self.pitchinfo = QLabel("NULL")
        self.pitchinfo.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        hbox.addWidget(self.pitchinfo)
        hbox.addSpacing(50)

        self.roll = QLabel("Roll: ")
        self.roll.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        hbox.addWidget(self.roll)
        hbox.addSpacing(20)
        self.rollinfo = QLabel("NULL")
        self.rollinfo.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        hbox.addWidget(self.rollinfo)
        hbox.addSpacing(50)
        hbox.addStretch()
        status_layout.addLayout(hbox)
        status_layout.addSpacing(20)
        self.full_window.addLayout(status_layout)


        # Save to file button
        hbox = QHBoxLayout()
        lbl = QLabel('Save to file: ', self)
        self.file_line_edit = QLineEdit("")
        self.file_line_edit.returnPressed.connect(self.save_file)
        self.file_submit_button = QPushButton("Submit", self)
        self.file_submit_button.clicked.connect(self.save_file)
        hbox.addWidget(lbl)
        hbox.addWidget(self.file_line_edit)
        hbox.addWidget(self.file_submit_button)
        self.full_window.addLayout(hbox)
        
        self.ask_for_status()

    """ Ask for server status (Currently does nothing!) """
    def ask_for_status(self):
        response = self.socket.send_command("RC.status")

        if (self.socket.connected):
            self.status_light = "assets/green.svg"
            self.svgWidget.load(self.status_light)
            self.status_text = "Socket Connected"
            self.status_label.setText(self.status_text)
            try: 
                self.status_button_func()
            # If the server is not responding, it will throw an exception
            # and we will just ignore it.
            except:
                return

        else:
            self.status_light = "assets/red.svg"
            self.status_text = "Socket Not Connected"
            self.status_label.setText(self.status_text)
            self.svgWidget.load(self.status_light)

    """ UPDATE THESE TO PROPERLY DO THE REQUIRED THINGS! """
        
    """ Request status to the server """
    def status_button_func(self):
        if (self.socket.connected):
            recv = self.send_to_server_with_response("RC.status")
            recv = recv.strip("{}")
            current_pitch = recv.split(",")[0].split(":")[1]
            current_roll = recv.split(",")[1].split(":")[1]
            self.pitchinfo.setText("{:.2f} ".format(float(current_pitch)))
            self.rollinfo.setText("{:.2f} ".format(float(current_roll)))
        else:
            self.pitchinfo.setText("NULL")
            self.rollinfo.setText("NULL")

    """ Level the robot """
    def level_button_func(self):
        self.send_to_server("RC.track [0,0,0,0,0,0,0,0]")
        print("Sending 'Level' command")

    """ Stop the robot """
    def stop_button_func(self):
        self.send_to_server("RC.translate [1,0,0,0,0,0,0,0]")
        print("Sending 'Stop' command")
        
    """ Disconnect the robot """
    def hard_stop_button_func(self):
        self.send_to_server("RC.disconnect")
        print("Sending 'Disconnect' command")
        
    """ Start the robot """
    def start_button_func(self):
        self.send_to_server("RC.start")
        print("Sending 'Start' command")



    """ Save the file """
    def save_file(self):
        filename = str(self.file_line_edit.text())
        self.send_to_server("RC.file [%s]"%filename)
        print("Sending 'Save' command")        

    """ Move the robot along the required axis at a given velocity"""
    def move_func(self,axis,velocity):
        command = ["1","0","0","0","0","0","0","0"]
        command[axis+1] = str(velocity)
        str_command = ",".join(command)
        self.send_to_server("RC.translate [%s]"%str_command)
        print("moving %s at %s"%(axis,velocity))
        return
