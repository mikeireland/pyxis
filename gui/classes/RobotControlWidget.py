#!/usr/bin/env python
from __future__ import print_function, division
from client_socket import ClientSocket
import random
#Import only what we need from PyQt5, or everything from PyQt4. In any case, we'll try
#to keep this back-compatible. Although this floods the namespace somewhat, everything
#starts with a "Q" so there is little chance of getting mixed up.

try:
    from PyQt5.QtWidgets import QWidget, QPushButton, QHBoxLayout, \
        QVBoxLayout, QLabel, QLineEdit, QTextEdit, QGridLayout
    from PyQt5.QtSvg import QSvgWidget
    from PyQt5.QtCore import Qt
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
        self.line_edit = QLineEdit("RC.")
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

        content_layout = QVBoxLayout()
        lbl = QLabel("Motor Controls (velocity in units of mm/s or rad/s):",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        content_layout.addWidget(lbl)
        content_layout.addSpacing(30)
        config_grid = QGridLayout()
        config_grid.setColumnMinimumWidth(0,150)
        config_grid.setColumnMinimumWidth(1,50)
        config_grid.setColumnMinimumWidth(2,140)
        config_grid.setVerticalSpacing(5)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('X Translation', self)
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
        hbox3.addStretch()
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(20)
        hbox3.addWidget(self.Robot_X_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_X_edit)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_X_R_button)
        hbox3.addStretch()
        config_grid.addLayout(hbox3,0,0)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Y Translation', self)
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
        hbox3.addStretch()
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(20)
        hbox3.addWidget(self.Robot_Y_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_Y_edit)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_Y_R_button)
        hbox3.addStretch()
        config_grid.addLayout(hbox3,1,0)


        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Z Translation', self)
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
        hbox3.addStretch()
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(20)
        hbox3.addWidget(self.Robot_Z_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_Z_edit)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_Z_R_button)
        hbox3.addStretch()
        config_grid.addLayout(hbox3,2,0)

        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Roll ', self)
        lbl1.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
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
        hbox3.addStretch()
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(20)
        hbox3.addWidget(self.Robot_roll_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_roll_edit)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_roll_R_button)
        hbox3.addStretch()
        config_grid.addLayout(hbox3,0,1)


        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Pitch', self)
        lbl1.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
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
        hbox3.addStretch()
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(20)
        hbox3.addWidget(self.Robot_pitch_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_pitch_edit)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_pitch_R_button)
        hbox3.addStretch()
        config_grid.addLayout(hbox3,1,1)


        hbox3 = QHBoxLayout()
        lbl1 = QLabel('Yaw ', self)
        lbl1.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
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
        hbox3.addStretch()
        hbox3.addWidget(lbl1)
        hbox3.addSpacing(20)
        hbox3.addWidget(self.Robot_yaw_L_button)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_yaw_edit)
        hbox3.addSpacing(10)
        hbox3.addWidget(self.Robot_yaw_R_button)
        hbox3.addStretch()
        config_grid.addLayout(hbox3,2,1)

        content_layout.addLayout(config_grid)
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

    def change_ip(self,IP):
        self.socket = ClientSocket(IP=IP, Port=self.port)

    def ask_for_status(self):
        """Ask for status for the server that applies to the current tab (as we can
        only see one server at a time)"""
        #As this is on a continuous timer, only do anything if we are
        #connected
        response = self.socket.send_command("RC.status")
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

    def zero_button_func(self):
        self.send_to_server("RC.zero")
        print("Sending 'Zero' command")

    def level_button_func(self):
        self.send_to_server("RC.level")
        print("Sending 'Level' command")

    def stop_button_func(self):
        #self.send_to_server("RC.stop")
        print("Sending 'Stop' command")


    def move_func(self,axis,velocity):
        print("moving %s at %s"%(axis,velocity))
        return


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
        self.line_edit.setText("RC.")
