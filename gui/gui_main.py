#!/usr/bin/env python
from __future__ import print_function, division
import sys
import pytomlpp
import collections
import importlib
import faulthandler

faulthandler.enable()

sys.path.insert(0, './classes')
from classes.client_socket import ClientSocket

try:
    from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QHBoxLayout, \
        QVBoxLayout, QGridLayout, QLabel, QTabWidget, QScrollArea, QLineEdit, QTextEdit, QFrame
    from PyQt5.QtCore import QTimer
    from PyQt5.QtGui import QPixmap, QIcon
    from PyQt5.QtSvg import QSvgWidget
except:
    print("Please install PyQt5.")
    raise UserWarning

from qt_material import apply_stylesheet

#Load config file
if len(sys.argv) > 1:
    config = pytomlpp.load(sys.argv[1])
else:
    config = pytomlpp.load("port_setup.toml")

pyxis_config = config["Pyxis"]
config.pop("Pyxis")
config = collections.OrderedDict({k: {key: value for key, value in sorted(config[k].items(), key=lambda x:x[1]["port"])}  for k in ["Navis","Dextra","Sinistra"]})

#Time to automatically update the gui
refresh_time = int(pyxis_config["refresh_time"]*1000) #now in ms

def debug_trace():
  '''Set a tracepoint in the Python debugger that works with Qt.

  This is useful for bugshooting in a Gui environment.'''
  from PyQt5.QtCore import pyqtRemoveInputHook
  from pdb import set_trace
  pyqtRemoveInputHook()
  set_trace()

""" Load a module from a string """
def class_for_name(module_name, class_name):
    # load the module, will raise ImportError if module cannot be loaded
    m = importlib.import_module(module_name)
    # get the class, will raise AttributeError if class cannot be found
    c = getattr(m, class_name)
    return c

import random

""" Random string for debugging """
def random_string(len):

    random_str = ''
    for _ in range(len):
        # Considering only upper and lowercase letters
        random_integer = random.randint(97, 97 + 26 - 1)
        flip_bit = random.randint(0, 1)
        # Convert to lowercase if the flip bit is on
        random_integer = random_integer - 32 if flip_bit == 1 else random_integer
        # Keep appending random characters using chr(x)
        random_str += (chr(random_integer))
    return random_str


""" Main class for Pyxis GUI"""
class PyxisGui(QTabWidget):
    def __init__(self, pyx_IPs,parent=None):

        super(PyxisGui,self).__init__(parent)

        #We'll have tabs for different servers
        self.resize(900, 550)

        self.tab_widgets = {} #Main tabs
        self.sub_tab_widgets = {} #Sub tabs
        self.status_lights = {}
        self.status_texts = {}

        self.FSM_port = pyx_IPs["FSM_port"]
        self.int_IPs = pyx_IPs["Internal"]
        self.ext_IPs = pyx_IPs["External"]

        #Dashboard Tab
        self.tab_widgets["dashboard"] = QWidget()
        self.addTab(self.tab_widgets["dashboard"],"Finite State Machine")

        listBox = QVBoxLayout()
        self.tab_widgets["dashboard"].setLayout(listBox)

        hbox = QHBoxLayout()
        self.fsm_socket = ClientSocket(self.int_IPs["FSM"], self.FSM_port)

        self.connect_fsm_button = QPushButton("Connect to FSM", self)
        self.connect_fsm_button.setFixedWidth(200)
        self.connect_fsm_button.clicked.connect(self.connect_fsm)
        hbox.addWidget(self.connect_fsm_button)

        self.power_button = QPushButton("START SERVERS", self)
        self.power_button.clicked.connect(self.power)
        self.power_button.setCheckable(True)
        self.power_button.setStyleSheet("QPushButton {background-color: #005500;border-color: #005500; color: #ffd740}")
        self.power_button.setFixedWidth(200)
        hbox.addWidget(self.power_button)

        listBox.addLayout(hbox)

        hbox = QHBoxLayout()
        self.dashboard_mainStatus = QLabel("STATUS",self)
        hbox.addWidget(self.dashboard_mainStatus)
        #self.dashboard_mainStatus.setAlignment(Qt.AlignCenter)
        self.dashboard_mainStatus.setStyleSheet("QLabel {font-size: 25px; font-weight: bold; color: #ff7e40}")
        self.dashboard_mainStatus.setFixedHeight(100)
        listBox.addLayout(hbox)

        hbox0 = QHBoxLayout()

        #Make Scrollable
        scroll = QScrollArea(self.tab_widgets["dashboard"])
        hbox0.addWidget(scroll)
        scroll.setWidgetResizable(True)
        scrollContent = QWidget(scroll)
        scrollLayout = QVBoxLayout()
        scrollContent.setLayout(scrollLayout)

        side_input = QVBoxLayout()

        #First, the command entry box
        hbox = QHBoxLayout()
        lbl = QLabel('Command: ', self)
        self.line_edit = QLineEdit("")
        self.line_edit.returnPressed.connect(self.command_enter)
        #Next, the refresh button
        self.refresh_button = QPushButton("Refresh", self)
        self.refresh_button.clicked.connect(self.refresh_click)
        hbox.addWidget(lbl)
        hbox.addWidget(self.line_edit)
        hbox.addWidget(self.refresh_button)
        side_input.addLayout(hbox)

        #Next, the response box
        self.response_label = QTextEdit('[No Server Response Yet]', self)
        self.response_label.setReadOnly(True)
        self.response_label.setStyleSheet("QTextEdit { background-color : black; }")
        self.response_label.setFixedHeight(150)
        side_input.addWidget(self.response_label)

        # Master Refresh button
        self.dashboard_refresh_button = QPushButton("REFRESH", self)
        self.dashboard_refresh_button.clicked.connect(self.refresh_status)
        side_input.addWidget(self.dashboard_refresh_button)

        side_input.addStretch()

        hbox0.addLayout(side_input)

        #For each tab...
        for tab in config:
            self.tab_widgets[tab] = QTabWidget()
            self.sub_tab_widgets[tab] = {}
            self.status_lights[tab] = {}
            self.status_texts[tab] = {}

            title = QLabel(tab, self)
            title.setStyleSheet("font-weight: bold; font-size: 25px; color: #ffd740;")

            scrollLayout.addWidget(title)

            status_grid = QWidget()
            status_grid_layout = QGridLayout()

            i = 0

            #For each sub tab
            for item in config[tab]:

                sub_config = config[tab][item]
                name = sub_config["name"]

                #Load class and create instance, add to tab container
                class_name = sub_config["module_type"]
                widget_module = class_for_name(class_name,class_name)
                self.sub_tab_widgets[tab][name] = widget_module(sub_config,self.int_IPs[tab])
                self.tab_widgets[tab].addTab(self.sub_tab_widgets[tab][name],sub_config["tab_name"])

                #Add status indicator to dashboard
                status_layout = QHBoxLayout()
                self.status_lights[tab][name] = QSvgWidget(self.sub_tab_widgets[tab][name].status_light)
                self.status_lights[tab][name].setFixedSize(25,25)
                self.status_texts[tab][name] = QLabel(self.sub_tab_widgets[tab][name].status_text, self)
                status_layout.addWidget(self.status_lights[tab][name])
                status_layout.addWidget(self.status_texts[tab][name])

                # Port label
                port_label = QLabel(sub_config["tab_name"], self)
                port_label.setStyleSheet("font-weight: bold")

                status_grid_layout.addWidget(port_label,i,0)
                status_grid_layout.addLayout(status_layout,i,1)

                i+=1

            status_grid.setLayout(status_grid_layout)
            scrollLayout.addWidget(status_grid)

            #Add master tab
            self.addTab(self.tab_widgets[tab],tab)

        scroll.setWidget(scrollContent)

        listBox.addLayout(hbox0)

        #Now show everything, and start status timers.
        self.setWindowTitle("Pyxis Server Gui")

        self.stimer = QTimer()

        self.auto_updater()

    """ Function to change all the IPs. Takes in the IP dictionary"""
    def change_IPs(self, new_IPs):
        self.fsm_socket = ClientSocket(new_IPs["FSM"], self.FSM_port)
        for tab in config:
            new_IP = new_IPs[tab]
            for item in self.sub_tab_widgets[tab]:
                self.sub_tab_widgets[tab][item].change_ip(new_IP)


    """ Function to refresh the status of all clients """
    def refresh_status(self):
        tab_index = self.currentIndex()

        if tab_index == 0:
            #ASK TO REFRESH FINITE STATE MACHINE STATUS HERE
            print("NEED TO ASK FOR STATUS TO FSM")
            self.dashboard_mainStatus.setText("PYXIS STATUS: "+random_string(25))

        else:
            tab = list(config.items())[tab_index-1][0]
            subtab_index = self.tab_widgets[tab].currentIndex()
            item = list(config[tab].items())[subtab_index][0]
            sub_config = config[tab][item]
            name = sub_config["name"]
            self.sub_tab_widgets[tab][name].ask_for_status()
            self.status_lights[tab][name].load(self.sub_tab_widgets[tab][name].status_light)
            self.status_texts[tab][name].setText(self.sub_tab_widgets[tab][name].status_text)


    """Function to auto update at a given rate"""
    def auto_updater(self):
        self.refresh_status()
        self.stimer.singleShot(refresh_time, self.auto_updater)
        return

    """ Function to connect to the FSM. CURRENTLY DOES NOTHING """
    def connect_fsm(self):
        return

    """ Button to start or kill all servers. CURRENTLY DOES NOTHING"""
    def power(self):
        if self.power_button.isChecked():
            self.power_button.setText("Kill Servers")
            self.power_button.setStyleSheet("QPushButton {background-color: #550000; border-color: #550000; color: #ffd740}")
        else:
            self.power_button.setText("Start Servers")
            self.power_button.setStyleSheet("QPushButton {background-color: #005500; border-color: #005500; color: #ffd740}")

        return

    """ Send a command to the FSM server """
    def send_to_FSM_server(self, text):
        try:
            response = self.fsm_socket.send_command(text)
        except:
            response = "*** Connection Error ***"
        if type(response)==str:
            self.response_label.append(response)
        elif type(response)==bool:
            if response:
                self.response_label.append("Success!")
            else:
                self.response_label.append("Failure!")
        self.line_edit.setText("")

    """ Parse a command and send it to the FSM server"""
    def command_enter(self):
        self.send_to_FSM_server(str(self.line_edit.text()))

    """What happens when you click the refresh button"""
    def refresh_click(self):
        self.send_to_FSM_server("???")


# Start application
app = QApplication(sys.argv)
app.setStyle("Fusion")
apply_stylesheet(app, theme='dark_amber_JH.xml')  #Design file

main = QWidget()
main.resize(1200, 750)

#Add logo
logo_wig = QWidget()
header = QHBoxLayout(logo_wig)
logo = QLabel()
qpix = QPixmap('assets/Pyxis_logo.png')
qpix = qpix.scaledToWidth(250)
logo.setPixmap(qpix)
header.addWidget(logo)

# MAKE IP DICT
IPs = pyxis_config["IP"]
IP_dict = {}
IP_internal = collections.OrderedDict({k: IPs[k] for k in ["FSM","Navis","Dextra","Sinistra"]})
IP_external = collections.OrderedDict({k: IPs["External"] for k in ["FSM","Navis","Dextra","Sinistra"]})
IP_dict["Internal"] = IP_internal
IP_dict["External"] = IP_external
IP_dict["FSM_port"] = IPs["FSM_port"]

# Add External IP info
ip_frame_ext = QFrame()
vbox = QVBoxLayout()
ip_frame_ext.setLayout(vbox)
ext_IP = pyxis_config["IP"]["External"]
ip_connect = QLabel('External IP: %s'%(ext_IP))
ip_connect.setStyleSheet("font-weight: bold; color: #ffd740; font-size:14px")
vbox.addWidget(ip_connect)

# Add Internal IP info
ip_frame_int = QFrame()
vbox = QVBoxLayout()
ip_frame_int.setLayout(vbox)
IPs = collections.OrderedDict({k: IPs[k] for k in ["FSM","Navis","Dextra","Sinistra"]})
for IP_name in IPs:
	IP = IPs[IP_name]
	ip_connect = QLabel('%s LAN IP: %s'%(IP_name,IP))
	ip_connect.setStyleSheet("font-weight: bold; color: #ffd740; font-size:14px")
	vbox.addWidget(ip_connect)

# Button to switch between IPs
IP_button = QPushButton("Connect to External IP")
IP_button.setFixedWidth(220)
IP_button.setCheckable(True)
IP_button.setStyleSheet("QPushButton {background-color: #000000; border-color: #550000; color: #ffd740}")

header.addWidget(ip_frame_int)
header.addWidget(ip_frame_ext)
header.addSpacing(70)
header.addWidget(IP_button)

ip_frame_ext.hide()

pyxis_app = PyxisGui(pyx_IPs=IP_dict)

""" Function to change between internal and external IPs"""
def change_IP():

    if IP_button.isChecked():
        print("Connecting to External IP")
        IP_button.setStyleSheet("QPushButton {background-color: #550000; border-color: #550000; color: #ffd740}")
        IP_button.setText("Connect to Internal IP")
        ip_frame_int.hide()
        ip_frame_ext.show()
        pyxis_app.change_IPs(IP_dict["External"])


    else:
        print("Connecting to Internal IP")
        IP_button.setStyleSheet("QPushButton {background-color: #550000; border-color: #550000; color: #ffd740}")
        IP_button.setText("Connect to External IP")
        ip_frame_ext.hide()
        ip_frame_int.show()
        pyxis_app.change_IPs(IP_dict["Internal"])

IP_button.clicked.connect(change_IP)
vbox = QVBoxLayout(main)

vbox.addWidget(logo_wig)
vbox.addWidget(pyxis_app)

main.setWindowTitle("Pyxis Control")
app.setWindowIcon(QIcon('assets/telescope.jpg'))

main.show()

sys.exit(app.exec_())
