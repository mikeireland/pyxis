#!/usr/bin/env python
from __future__ import print_function, division
import json
from RawWidget import RawWidget

try:
    from PyQt5.QtWidgets import QPushButton, QHBoxLayout, QLabel, QGridLayout
    from PyQt5.QtSvg import QSvgWidget
except:
    print("Please install PyQt5.")
    raise UserWarning


class DepAuxControlWidget(RawWidget):
    def __init__(self, config, IP='127.0.0.1', parent=None):

        super(DepAuxControlWidget,self).__init__(config,IP,parent)

        self.voltage_limit = config["voltage_limit"]
        self.full_window.addSpacing(40)

        # LED controls
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
        self.full_window.addLayout(LED_layout)

        self.full_window.addSpacing(30)

        # Power status
        Power_hbox = QHBoxLayout()
        lbl = QLabel("Power System Controls:",self)
        lbl.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Power_hbox.addWidget(lbl)
        Power_hbox.addStretch()
        self.getPower_button = QPushButton("Get Power", self)
        self.getPower_button.setFixedWidth(200)
        self.getPower_button.clicked.connect(self.getPower_func)
        Power_hbox.addWidget(self.getPower_button)
        Power_hbox.addSpacing(50)
        
        # Current and voltage meters
        Power_layout = QGridLayout()
        Power_layout.setColumnMinimumWidth(0,100)
        Power_layout.setRowMinimumHeight(0, 30)
        Power_layout.setRowMinimumHeight(1, 30)
        Power_layout.setColumnMinimumWidth(1,100)
        Power_layout.setColumnMinimumWidth(2,100)
 
        self.PC = QLabel("PC: ")
        self.PC.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Power_layout.addWidget(self.PC,0,0)
        
        self.PCvoltage = QLabel("0.00 V")
        self.PCvoltage.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Power_layout.addWidget(self.PCvoltage,0,1)
        
        self.PCcurrent = QLabel("0.00 A")
        self.PCcurrent.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Power_layout.addWidget(self.PCcurrent,0,2)

        self.Motor = QLabel("Motor: ")
        self.Motor.setStyleSheet("QLabel {font-size: 20px; font-weight: bold}")
        Power_layout.addWidget(self.Motor,1,0)
        
        self.Motorvoltage = QLabel("0.00 V")
        self.Motorvoltage.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Power_layout.addWidget(self.Motorvoltage,1,1)
        
        self.Motorcurrent = QLabel("0.00 A")
        self.Motorcurrent.setStyleSheet("QLabel {font-size: 20px; font-weight: bold; color: #ffd740}")
        Power_layout.addWidget(self.Motorcurrent,1,2)
        
        Power_hbox.addLayout(Power_layout)
        
        # Complete setup, add status labels and indicators
        self.LED_status_light = 'assets/red.svg'
        self.LED_status_text = 'Voltage getting low'
        self.LED_svgWidget = QSvgWidget(self.LED_status_light)
        self.LED_svgWidget.setFixedSize(20,20)
        self.LED_status_label = QLabel(self.LED_status_text, self)
        Power_hbox.addWidget(self.LED_svgWidget)
        Power_hbox.addWidget(self.LED_status_label)
        Power_hbox.addStretch()
        
        self.full_window.addLayout(Power_hbox)
        self.full_window.addStretch()
        self.ask_for_status()


    """ Ask for server status, updating the power meters as required """
    def ask_for_status(self):

        self.getPower_func()
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

    """ Turn LEDs on """
    def LEDOn_button_func(self):
        self.send_to_server("DA.LEDOn")
        print("Sending 'LEDOn' command")

    """ Turn LEDs off """
    def LEDOff_button_func(self):
        self.send_to_server("DA.LEDOff")
        print("Sending 'LEDOff' command")

    """ Get and set the power meters """
    def getPower_func(self):
        power_str = self.send_to_server_with_response("DA.reqpower")
        print("Sending 'getPower' command")
        try:
            power_dict = json.loads(power_str)
        except:
            return
        self.PCvoltage.setText("{:.2f} V".format(power_dict["PC_voltage"]/1000))
        self.PCcurrent.setText("{:.2f} A".format(power_dict["PC_current"]/1000))
        self.Motorvoltage.setText("{:.2f} V".format(power_dict["Motor_voltage"]/1000))
        self.Motorcurrent.setText("{:.2f} A".format(power_dict["Motor_current"]/1000))
        
        if power_dict["PC_voltage"]/1000 > self.voltage_limit:
            self.LED_status_light = "assets/green.svg"
            self.LED_svgWidget.load(self.LED_status_light)
            self.LED_status_text = "Voltage good"
            self.LED_status_label.setText(self.LED_status_text)
        else:
            self.LED_status_light = "assets/red.svg"
            self.LED_status_text = "Voltage getting low"
            self.LED_status_label.setText(self.LED_status_text)
            self.LED_svgWidget.load(self.LED_status_light)            
