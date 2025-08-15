#!/usr/bin/env python
from __future__ import print_function, division
from RawWidget import RawWidget
import json

try:
    from PyQt5.QtWidgets import QPushButton, QHBoxLayout, \
        QVBoxLayout, QLabel, QLineEdit, QGridLayout
    from PyQt5.QtCore import Qt, QTimer
except:
    print("Please install PyQt5.")
    raise UserWarning


class RobotControlWidget(RawWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(RobotControlWidget,self).__init__(config,IP,parent)
        self.level_mode = False
        self.setFocusPolicy(Qt.StrongFocus)
        self.refresh_button.setText("Status") #Change the refresh button to status button
        self.refresh_button.clicked.disconnect()  # Remove previous connections
        self.refresh_button.clicked.connect(self.print_status)

        button_layout = QHBoxLayout()
        self.start_button = QPushButton("Start", self)
        self.start_button.setFixedWidth(200)
        self.start_button.clicked.connect(self.start_button_func)
        button_layout.addWidget(self.start_button)
        self.level_button = QPushButton("Level", self)
        self.level_button.setCheckable(True)
        self.level_button.setFixedWidth(200)
        self.level_button.clicked.connect(self.level_button_func)
        button_layout.addWidget(self.level_button)
        self.stop_button = QPushButton("Stop", self)
        self.stop_button.setFixedWidth(200)
        self.stop_button.clicked.connect(self.stop_button_func)
        self.stop_button.clicked.connect(lambda: self.level_button.setChecked(False))  # Uncheck level button when stop is pressed
        self.stop_button.clicked.connect(self.level_button_func)
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
        self.Robot_X_edit = QLineEdit("3.0")
        self.Robot_X_edit.setFixedWidth(120)
        self.Robot_X_L_button = QPushButton("<<", self)
        self.Robot_X_L_button.pressed.connect(lambda: self.move_func(0,-float(self.Robot_X_edit.text())))
        self.Robot_X_L_button.released.connect(self.stop_button_func)
        self.Robot_X_L_button.released.connect(self.level_button_func)
        self.Robot_X_L_button.setFixedWidth(50)
        self.Robot_X_R_button = QPushButton(">>", self)
        self.Robot_X_R_button.pressed.connect(lambda: self.move_func(0,float(self.Robot_X_edit.text())))
        self.Robot_X_R_button.released.connect(self.stop_button_func)
        self.Robot_X_R_button.released.connect(self.level_button_func)
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
        self.Robot_Y_edit = QLineEdit("3.0")
        self.Robot_Y_edit.setFixedWidth(120)
        self.Robot_Y_L_button = QPushButton("<<", self)
        self.Robot_Y_L_button.pressed.connect(lambda: self.move_func(1,-float(self.Robot_Y_edit.text())))
        self.Robot_Y_L_button.released.connect(self.stop_button_func)
        self.Robot_Y_L_button.released.connect(self.level_button_func)
        self.Robot_Y_L_button.setFixedWidth(50)
        self.Robot_Y_R_button = QPushButton(">>", self)
        self.Robot_Y_R_button.pressed.connect(lambda: self.move_func(1,float(self.Robot_Y_edit.text())))
        self.Robot_Y_R_button.released.connect(self.stop_button_func)
        self.Robot_Y_R_button.released.connect(self.level_button_func)
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
        self.Robot_Z_edit = QLineEdit("3.0")
        self.Robot_Z_edit.setFixedWidth(120)
        self.Robot_Z_L_button = QPushButton("<<", self)
        self.Robot_Z_L_button.pressed.connect(lambda: self.move_func(2,-float(self.Robot_Z_edit.text())))
        self.Robot_Z_L_button.released.connect(self.stop_button_func)
        self.Robot_Z_L_button.released.connect(self.level_button_func)
        self.Robot_Z_L_button.setFixedWidth(50)
        self.Robot_Z_R_button = QPushButton(">>", self)
        self.Robot_Z_R_button.pressed.connect(lambda: self.move_func(2,float(self.Robot_Z_edit.text())))
        self.Robot_Z_R_button.released.connect(self.stop_button_func)
        self.Robot_Z_R_button.released.connect(self.level_button_func)
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
        self.Robot_roll_L_button.released.connect(self.level_button_func)
        self.Robot_roll_L_button.setFixedWidth(50)
        self.Robot_roll_R_button = QPushButton(">>", self)
        self.Robot_roll_R_button.pressed.connect(lambda: self.move_func(3,float(self.Robot_roll_edit.text())))
        self.Robot_roll_R_button.released.connect(self.stop_button_func)
        self.Robot_roll_R_button.released.connect(self.level_button_func)
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
        self.Robot_pitch_L_button.released.connect(self.level_button_func)
        self.Robot_pitch_L_button.setFixedWidth(50)
        self.Robot_pitch_R_button = QPushButton(">>", self)
        self.Robot_pitch_R_button.pressed.connect(lambda: self.move_func(4,float(self.Robot_pitch_edit.text())))
        self.Robot_pitch_R_button.released.connect(self.stop_button_func)
        self.Robot_pitch_R_button.released.connect(self.level_button_func)
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
        self.Robot_yaw_edit = QLineEdit("1000.0")
        self.Robot_yaw_edit.setFixedWidth(120)
        self.Robot_yaw_L_button = QPushButton("<<", self)
        self.Robot_yaw_L_button.pressed.connect(lambda: self.move_func(5,-float(self.Robot_yaw_edit.text())))
        self.Robot_yaw_L_button.released.connect(self.stop_button_func)
        self.Robot_yaw_L_button.released.connect(self.level_button_func)
        self.Robot_yaw_L_button.setFixedWidth(50)
        self.Robot_yaw_R_button = QPushButton(">>", self)
        self.Robot_yaw_R_button.pressed.connect(lambda: self.move_func(5,float(self.Robot_yaw_edit.text())))
        self.Robot_yaw_R_button.released.connect(self.stop_button_func)
        self.Robot_yaw_R_button.released.connect(self.level_button_func)
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
        self.Robot_gon_L_button.released.connect(self.level_button_func)
        self.Robot_gon_L_button.setFixedWidth(50)
        self.Robot_gon_R_button = QPushButton(">>", self)
        self.Robot_gon_R_button.pressed.connect(lambda: self.move_func(6,float(self.Robot_gon_edit.text())))
        self.Robot_gon_R_button.released.connect(self.stop_button_func)
        self.Robot_gon_R_button.released.connect(self.level_button_func)
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

        # Edited by Qianhui to add a set_gain button
        gain_layout = QHBoxLayout()
        hbox = QHBoxLayout()
        lbl = QLabel("az (yaw)",self)
        lbl.setStyleSheet("QLabel {font-size: 15px}")
        lbl.setFixedWidth(70)
        self.yaw_edit = QLineEdit("0.0")
        self.yaw_edit.setFixedWidth(70)
        hbox.addWidget(lbl)
        hbox.addWidget(self.yaw_edit)
        hbox.addSpacing(20)
        gain_layout.addLayout(hbox)
        

        hbox = QHBoxLayout()
        lbl = QLabel("alt (el)",self)
        lbl.setStyleSheet("QLabel {font-size: 15px}")
        lbl.setFixedWidth(70)
        self.el_edit = QLineEdit("0.0")
        self.el_edit.setFixedWidth(70)
        hbox.addWidget(lbl)
        hbox.addWidget(self.el_edit)
        hbox.addSpacing(20)
        gain_layout.addLayout(hbox)

        hbox = QHBoxLayout()
        lbl = QLabel("az (yaw) intergal",self)
        lbl.setStyleSheet("QLabel {font-size: 15px}")
        lbl.setFixedWidth(130)
        self.yint_edit = QLineEdit("0.0")
        self.yint_edit.setFixedWidth(70)
        hbox.addWidget(lbl)
        hbox.addWidget(self.yint_edit)
        hbox.addSpacing(20)
        gain_layout.addLayout(hbox)

        hbox = QHBoxLayout()
        lbl = QLabel("alt (el) intergal",self)
        lbl.setStyleSheet("QLabel {font-size: 15px}")
        lbl.setFixedWidth(130)
        self.eint_edit = QLineEdit("0.0")
        self.eint_edit.setFixedWidth(70)
        hbox.addWidget(lbl)
        hbox.addWidget(self.eint_edit)
        hbox.addSpacing(20)
        gain_layout.addLayout(hbox)
        

        # Connect the set_gain button to the server command
        # This will send the alt and az values to the server
        # to set the gain for the robot.
        hbox = QHBoxLayout()
        self.setgain_button = QPushButton("Set Gains", self)
        self.setgain_button.setFixedWidth(150)
        self.setgain_button.clicked.connect(lambda: self.set_gain_func(float(self.yaw_edit.text()), float(self.el_edit.text()), float(self.yint_edit.text()), float(self.eint_edit.text())))
        hbox.addWidget(self.setgain_button)
        hbox.addSpacing(400)
        hbox.addStretch()
        gain_layout.addLayout(hbox)
        self.full_window.addLayout(gain_layout)


        # #Edited by Qianhui to add pitch&roll info showing
        status_layout = QHBoxLayout()
        self.status_timer = QTimer(self)
        self.status_timer.timeout.connect(self.status_button_func)

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

        self.robot = QLabel("Robot: ")
        self.robot.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        hbox.addWidget(self.robot)
        hbox.addSpacing(20)
        self.robot_status = QLabel("NULL")
        self.robot_status.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        hbox.addWidget(self.robot_status)
        hbox.addSpacing(50)

        self.star_track = QLabel("Star track: ")
        self.star_track.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        hbox.addWidget(self.star_track)
        hbox.addSpacing(20)
        self.st_status = QLabel("NULL")
        self.st_status.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        hbox.addWidget(self.st_status)
        hbox.addSpacing(50)

        self.st_status_button = QPushButton("Set ST", self)
        self.st_status_button.setFixedWidth(100)
        self.st_status_button.clicked.connect(lambda: self.set_st_func(int(self.st_edit.text())))
        hbox.addWidget(self.st_status_button)
        self.st_edit = QLineEdit("1")
        self.st_edit.setFixedWidth(70)
        hbox.addWidget(self.st_edit)
        hbox.addSpacing(50)
        hbox.addStretch()
        status_layout.addLayout(hbox)
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
        

    """ Ask for server status """
    def ask_for_status(self):
        if (self.socket.connected):
            self.status_light = "assets/green.svg"
            self.svgWidget.load(self.status_light)
            self.status_text = "Socket Connected"
            self.status_label.setText(self.status_text)

        else:
            self.status_light = "assets/red.svg"
            self.status_text = "Socket Not Connected"
            self.status_label.setText(self.status_text)
            self.svgWidget.load(self.status_light)

    """ UPDATE THESE TO PROPERLY DO THE REQUIRED THINGS! """
        
    """ Request status to the server """
    def status_button_func(self):
        rc_status_list = ["NULL", "IDLE", "Translate", "Resonance", "Track", "Disconnect"]
        st_status_list = [ "IDLE", "Waiting", "Moving", "Tracking"]
        if (self.socket.connected):
            try:
                recv = self.socket.send_command("RC.status")
                recv = json.loads(recv)
                if "roll" in recv:
                    current_roll = recv["roll"]
                else:
                    current_roll = "0.0"
                if "pitch" in recv:
                    current_pitch = recv["pitch"]
                else:
                    current_pitch = "0.0"
                if "loop_status" in recv:
                    loop_sttaus_number = int(recv["loop_status"])
                    current_loop_status = rc_status_list[loop_sttaus_number]
                if "st_status" in recv:
                    st_status_number = int(recv["st_status"])
                    current_st_status = st_status_list[st_status_number]
                self.pitchinfo.setText("{:.2f} ".format(float(current_pitch)))
                self.rollinfo.setText("{:.2f} ".format(float(current_roll)))
                self.robot_status.setText(current_loop_status)
                self.st_status.setText(current_st_status)
            except json.JSONDecodeError:
                print("Error decoding JSON response from server.")
                self.pitchinfo.setText("NULL")
                self.rollinfo.setText("NULL")
                self.robot_status.setText("NULL")
                self.st_status.setText("NULL")
        else:
            self.pitchinfo.setText("NULL")
            self.rollinfo.setText("NULL")
            self.robot_status.setText("NULL")
            self.st_status.setText("NULL")

    """ Set the star tracker status """
    def set_st_func(self, new_st_status):
        if (self.socket.connected):
            if new_st_status in [0, 1, 2, 3]:
                recv = self.socket.send_command("RC.set_st %d"%new_st_status)
            else:
                print("Invalid star tracker status. Please use 0, 1, 2, or 3.")
                return
        
            

    """ Level the robot. Not finished yet! (13th August 2025) """
    def level_button_func(self):
        if self.socket.connected:
            if self.level_button.isChecked():
                self.level_button.setText("Stop Level")
                self.send_to_server("RC.track 1,0,0,0,0,0,0,0")
                print("Sending 'Level' command")
            else:
                self.level_button.setText("Level")
                self.send_to_server("RC.translate 0,0,0,0,0,0,0,0")
                print("Stopping 'Level' command")
        else:
            print("Not connected to server, cannot level the robot.")
            self.level_button.setText("Level")
            self.level_button.setChecked(False)


    """ Stop the robot """
    def stop_button_func(self):
        self.send_to_server("RC.translate 1,0,0,0,0,0,0,0")
        print("Sending 'Stop' command")
        
    """ Disconnect the robot """
    def hard_stop_button_func(self):
        self.send_to_server("RC.disconnect")
        self.status_timer.stop()
        self.set_st_func(0)
        self.status_button_func()
        print("Sending 'Disconnect' command")
        self.level_button.setText("Level")
        self.level_button.setChecked(False)

        
    """ Start the robot """
    def start_button_func(self):
        try:
            self.send_to_server("RC.start")
            print("Sending 'Start' command")
            self.status_timer.start(1000)
        except Exception as e:
            print("Error starting the robot: ", e)



    """ Save the file """
    def save_file(self):
        filename = str(self.file_line_edit.text())
        self.send_to_server("RC.file %s"%filename)
        print("Sending 'Save' command")        

    """ Set the gain for the robot """
    def set_gain_func(self, yaw_gain, el_gain, yaw_integral, el_integral):
        command = ["0","0", "0", "0"] #The integration is set to zero for now.
        command[0] = str(yaw_gain)
        command[1] = str(el_gain)
        command[2] = str(yaw_integral)
        command[3] = str(el_integral)
        str_command = ",".join(command)
        self.send_to_server("RC.set_gains %s"%str_command)
        print("Sending RC.set_gains %s"%str_command + " to the server")

    """ Move the robot along the required axis at a given velocity"""
    def move_func(self,axis,velocity):
        command = ["1","0","0","0","0","0","0","0"]
        command[axis+1] = str(velocity)
        str_command = ",".join(command)
        self.send_to_server("RC.translate %s"%str_command)
        print("moving %s at %s"%(axis,velocity))
        return
    
    """When the keyboard is pressed, move corresponding axis"""
    def keyPressEvent(self, event):
        if event.key() == Qt.Key_A:
            self.move_func(0,-float(self.Robot_X_edit.text()))
        if event.key() == Qt.Key_D:
            self.move_func(0,float(self.Robot_X_edit.text()))
        if event.key() == Qt.Key_W:
            self.move_func(1,-float(self.Robot_Y_edit.text()))
        if event.key() == Qt.Key_S:
            self.move_func(1,float(self.Robot_Y_edit.text()))
        if event.key() == Qt.Key_Down:
            self.move_func(2,-float(self.Robot_Z_edit.text()))
        if event.key() == Qt.Key_Up:
            self.move_func(2,float(self.Robot_Z_edit.text()))
        if event.key() == Qt.Key_Left:
            self.move_func(5,-float(self.Robot_yaw_edit.text()))
        if event.key() == Qt.Key_Right:
            self.move_func(5,float(self.Robot_yaw_edit.text()))
    
    """When the keyboard is released, stop the robot"""
    def keyReleaseEvent(self, event):
        if event.key() in [Qt.Key_A, Qt.Key_D, Qt.Key_W, Qt.Key_S, Qt.Key_Up, Qt.Key_Down, Qt.Key_Left, Qt.Key_Right]:
            self.stop_button_func()
            self.level_button_func()

    def print_status(self):
        if self.socket.connected:
            try:
                recv = self.socket.send_command("RC.status")
                self.response_label.append(recv)
            except json.JSONDecodeError:
                self.response_label.append("*** Error receiving status ***")
        else:
            self.response_label.append("*** Not connected to server ***")