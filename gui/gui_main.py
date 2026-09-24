#!/usr/bin/env python
from __future__ import print_function, division
import sys, json
import pytomlpp
import collections
import importlib
import faulthandler
import os
# Ensure these are set before any PyQt import so Qt falls back to software GL
os.environ.setdefault("QT_OPENGL", "software")
os.environ.setdefault("LIBGL_ALWAYS_SOFTWARE", "1")
os.environ.setdefault("QT_XCB_GL_INTEGRATION", "none") #Added the above three lines because of OpenGL issues in Ubuntu 24


# import argparse

faulthandler.enable()

sys.path.insert(0, './classes')
from classes.client_socket import ClientSocket

try:
    from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QHBoxLayout, \
        QVBoxLayout, QGridLayout, QLabel, QTabWidget, QScrollArea, QLineEdit, QTextEdit, QFrame, QComboBox
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


class CommandOnlyWidget(QWidget):
    def __init__(self, config, IP, parent=None):
        super(CommandOnlyWidget, self).__init__(parent)

        self.port = config["port"]
        self.prefix = config["prefix"]
        self.socket = ClientSocket(IP=IP, Port=self.port)

        layout = QVBoxLayout(self)
        command_layout = QHBoxLayout()
        command_layout.addWidget(QLabel("Command:", self))
        self.line_edit = QLineEdit(f"{self.prefix}.", self)
        self.line_edit.returnPressed.connect(self.command_enter)
        command_layout.addWidget(self.line_edit)
        layout.addLayout(command_layout)

        self.response_label = QTextEdit("[No Server Response Yet]", self)
        self.response_label.setReadOnly(True)
        self.response_label.setStyleSheet("QTextEdit { background-color : black; }")
        layout.addWidget(self.response_label)

    def change_ip(self, IP):
        self.socket = ClientSocket(IP=IP, Port=self.port)

    def command_enter(self):
        try:
            response = self.socket.send_command(self.line_edit.text())
            self.response_label.append(str(response))
        except Exception as e:
            self.response_label.append(f"Command failed: {str(e)}")
        self.line_edit.setText(f"{self.prefix}.")


""" Main class for Pyxis GUI"""
class PyxisGui(QTabWidget):
    def __init__(self, pyx_IPs, use_external, parent=None):

        super(PyxisGui,self).__init__(parent)

        #We'll have tabs for different servers
        self.resize(900, 550)

        self.tab_widgets = {} #Main tabs
        self.sub_tab_widgets = {} #Sub tabs
        self.status_lights = {}
        self.status_texts = {}
        self.alive_light = {} #Status lights for each tab
        self.alive_text = {}
        self.operation_statuses = {}

        self.FSM_port = pyx_IPs["FSM_port"]
        self.int_IPs = pyx_IPs["Internal"]
        self.ext_IPs = pyx_IPs["External"]

        #Dashboard Tab
        self.tab_widgets["dashboard"] = QWidget()
        self.addTab(self.tab_widgets["dashboard"],"Finite State Machine")

        listBox = QVBoxLayout()
        self.tab_widgets["dashboard"].setLayout(listBox)

        hbox = QHBoxLayout()
        self.fsm_socket = ClientSocket(self.ext_IPs["FSM"], self.FSM_port, TIMEOUT=5000) #Changed by Qianhui: connect to FSM through external IP

        hbox0 = QHBoxLayout()

        # Status overview occupies the left 35% of the dashboard.
        status_scroll = QScrollArea(self.tab_widgets["dashboard"])
        status_scroll.setWidgetResizable(True)
        status_content = QWidget(status_scroll)
        status_layout = QVBoxLayout(status_content)
        status_scroll.setWidget(status_content)
        hbox0.addWidget(status_scroll, 35)

        dashboard_controls = QVBoxLayout()

        #First, the command entry box
        hbox = QHBoxLayout()
        lbl = QLabel('Command: ', self)
        self.line_edit = QLineEdit("")
        self.line_edit.returnPressed.connect(self.command_enter)
        # #Next, the status button
        self.status_button = QPushButton("Status", self)
        self.status_button.clicked.connect(self.status_click)
        hbox.addWidget(lbl)
        hbox.addWidget(self.line_edit)
        hbox.addWidget(self.status_button)
        dashboard_controls.addLayout(hbox)

        #Next, the response box
        self.response_label = QTextEdit('[No Server Response Yet]', self)
        self.response_label.setReadOnly(True)
        self.response_label.setStyleSheet("QTextEdit { background-color : black; }")
        self.response_label.setFixedHeight(120)
        dashboard_controls.addWidget(self.response_label)

        controls_layout = QHBoxLayout()
        self.connect_fsm_button = QPushButton("Connect to FSM", self)
        self.connect_fsm_button.clicked.connect(self.connect_fsm)
        self.dashboard_refresh_button = QPushButton("REFRESH", self)
        self.dashboard_refresh_button.clicked.connect(self.refresh_status)
        self.start_button = QPushButton("Start", self)
        self.start_button.clicked.connect(lambda: self.send_fsm_command("start_pyxis"))
        self.stop_button = QPushButton("Stop", self)
        self.stop_button.clicked.connect(lambda: self.send_fsm_command("stop_pyxis"))
        self.platform_selector = QComboBox(self)
        self.platform_selector.addItems(["All", "Navis", "Sinistra", "Dextra"])
        self.system_selector = QComboBox(self)
        self.system_selector.addItems(["All", "Star Tracking", "Coarse Metrology", "Fibre Injection", "Fringe Tracking"])
        for control in (self.connect_fsm_button, self.dashboard_refresh_button, self.start_button,
                        self.stop_button, self.platform_selector, self.system_selector):
            controls_layout.addWidget(control)
        dashboard_controls.addLayout(controls_layout)

        operation_grid = QGridLayout()
        operation_label_style = "font-size: 17px"
        operation_grid.addWidget(QLabel(""), 0, 0)
        for column, platform in enumerate(("Navis", "Sinistra", "Dextra"), start=1):
            header = QLabel(platform, self)
            header.setStyleSheet(f"font-weight: bold; {operation_label_style}")
            operation_grid.addWidget(header, 0, column)

        operation_modules = {
            "Star Tracking": ("NavisStarTracker", "SinistraStarTracker", "DextraStarTracker"),
            "Coarse Metrology": (None, "SinistraCoarseMet", "DextraCoarseMet"),
            "Fibre Injection": ("NavisFiberInjection", None, None),
        }
        for row, (operation, module_names) in enumerate(operation_modules.items(), start=1):
            header = QLabel(operation, self)
            header.setStyleSheet(f"font-weight: bold; {operation_label_style}")
            operation_grid.addWidget(header, row, 0)
            for column, module_name in enumerate(module_names, start=1):
                status = QLabel("Unresponsive", self)
                status.setStyleSheet(f"color: #ff7e40; {operation_label_style}")
                operation_grid.addWidget(status, row, column)
                if module_name:
                    self.operation_statuses.setdefault(module_name, []).append(status)
        dashboard_controls.addLayout(operation_grid)

        fringe_layout = QHBoxLayout()
        fringe_label = QLabel("Fringe tracking", self)
        fringe_label.setStyleSheet(operation_label_style)
        fringe_layout.addWidget(fringe_label)
        self.fringe_status = QLabel("Unresponsive", self)
        self.fringe_status.setStyleSheet(f"color: #ff7e40; {operation_label_style}")
        fringe_layout.addWidget(self.fringe_status)
        fringe_layout.addStretch()
        dashboard_controls.addLayout(fringe_layout)
        dashboard_controls.addStretch()

        hbox0.addLayout(dashboard_controls, 65)

        """Edited by Qianhui: read status of individual modules from FSM server"""
        # fsm_status_dict = self.get_status_from_fsm()

        #For each tab...
        for tab in config:
            self.tab_widgets[tab] = QTabWidget()
            if tab == "Navis":
                self.tab_widgets[tab].setUsesScrollButtons(True)
                self.tab_widgets[tab].tabBar().setExpanding(False)
                self.tab_widgets[tab].setStyleSheet("QTabBar::tab { font-size: 10px; padding: 4px 6px; }")
            self.sub_tab_widgets[tab] = {}
            self.status_lights[tab] = {}
            self.status_texts[tab] = {}
            self.alive_light[tab] = {}
            self.alive_text[tab] = {}

            status_heading = QLabel(tab, self)
            status_heading.setStyleSheet("font-weight: bold; font-size: 18px; color: #ffd740;")
            status_layout.addWidget(status_heading)

            #For each sub tab
            for item in config[tab]:
                sub_config = config[tab][item]
                name = sub_config["name"]

                class_name = sub_config.get("module_type")
                if class_name:
                    widget_module = class_for_name(class_name, class_name)
                    if use_external:
                        self.sub_tab_widgets[tab][name] = widget_module(sub_config, self.ext_IPs[tab])
                    else:
                        self.sub_tab_widgets[tab][name] = widget_module(sub_config, self.int_IPs[tab])
                    self.tab_widgets[tab].addTab(self.sub_tab_widgets[tab][name], sub_config["tab_name"])

                    self.status_lights[tab][name] = QSvgWidget(self.sub_tab_widgets[tab][name].status_light)
                    self.status_texts[tab][name] = QLabel("", self)
                elif sub_config["tab_name"] == "Plate Solver":
                    self.sub_tab_widgets[tab][name] = CommandOnlyWidget(sub_config, self.ext_IPs[tab] if use_external else self.int_IPs[tab])
                    self.tab_widgets[tab].addTab(self.sub_tab_widgets[tab][name], sub_config["tab_name"])


                # Each module has one combined FSM status.
                status_row = QHBoxLayout()
                self.alive_light[tab][name] = QSvgWidget("assets/green.svg")
                self.alive_text[tab][name] = QLabel("Unresponsive", self)
                self.alive_light[tab][name].setFixedSize(25,25)
                port_label = QLabel(sub_config["tab_name"], self)
                port_label.setStyleSheet("font-weight: bold")
                status_row.addWidget(port_label)
                status_row.addStretch()
                status_row.addWidget(self.alive_light[tab][name])
                status_row.addWidget(self.alive_text[tab][name])
                reboot_button = QPushButton("Reboot", self)
                reboot_button.clicked.connect(lambda checked, module_name=name: self.reboot_click(module_name))
                status_row.addWidget(reboot_button)
                status_layout.addLayout(status_row)

            #Add master tab
            self.addTab(self.tab_widgets[tab],tab)

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


    """Get the alive status from the FSM server"""
    def get_status_from_fsm(self):
        try:
            fsm_status = self.fsm_socket.send_command("status")
            if not fsm_status:
                self.response_label.append("FSM returned empty response")
                return {}
            fsm_status_str = fsm_status.decode() if isinstance(fsm_status, bytes) else fsm_status
            fsm_status_str = fsm_status_str.replace("True", '"True"').replace("False", '"False"').replace("'", '"')
            fsm_status_dict = json.loads(fsm_status_str)
            return fsm_status_dict
        except Exception as e:
            return ({"error": str(e)})
    

    def get_indicators(self, isalive, connected, name, tab):
        if isalive == "True" and connected == "True":
            self.alive_light[tab][name].load("assets/green.svg")
            self.alive_text[tab][name].setText("Connected")
            status_text = "Connected"
        else:
            self.alive_light[tab][name].load("assets/red.svg")
            self.alive_text[tab][name].setText("Unresponsive")
            status_text = "Unresponsive"

        for status_label in self.operation_statuses.get(name, []):
            status_label.setText(status_text)

    
    def reboot_click(self, name):
        try:
            command = "reboot " + name
            response = self.fsm_socket.send_command(command)
            self.response_label.append(f"Response: {response}")
        except Exception as e:
            self.response_label.append(f"Reboot failed: {str(e)}")

    def send_fsm_command(self, command):
        try:
            response = self.fsm_socket.send_command(command)
            self.response_label.append(f"Response: {response}")
        except Exception as e:
            self.response_label.append(f"Command failed: {str(e)}")


    """ Function to refresh the status of all clients """
    def refresh_status(self):
        tab_index = self.currentIndex()

        if tab_index == 0:
            try:
                fsm_status_dict = self.get_status_from_fsm()
                for tab in config:
                    for item in config[tab]:
                        sub_config = config[tab][item]
                        name = sub_config["name"]
                        self.get_indicators(fsm_status_dict[name]["isalive"], fsm_status_dict[name]["connected"], name, tab)
                fringe_services = [status for name, status in fsm_status_dict.items()
                                   if "fringe" in name.lower()]
                self.fringe_status.setText("Connected" if any(
                    status.get("isalive") == "True" and status.get("connected") == "True"
                    for status in fringe_services) else "Unresponsive")
            except Exception as e:
                self.response_label.append(f"Error refreshing: {str(e)}")

        else:
            tab = list(config.items())[tab_index-1][0]
            subtab_index = self.tab_widgets[tab].currentIndex()
            item = list(config[tab].items())[subtab_index][0]
            sub_config = config[tab][item]
            name = sub_config["name"]
            widget = self.sub_tab_widgets[tab][name]
            if name in self.status_lights[tab]:
                widget.ask_for_status()
                self.status_lights[tab][name].load(widget.status_light)
                self.status_texts[tab][name].setText(widget.status_text)


    # def handle_fsm_status(self, fsm_status_dict):
    #     if "error" in fsm_status_dict:
    #         self.dashboard_mainStatus.setText("PYXIS STATUS: ERROR - " + fsm_status_dict["error"])
    #         return
    #     for tab in config:
    #         for item in config[tab]:
    #             sub_config = config[tab][item]
    #             name = sub_config["name"]
    #             self.get_indicators(fsm_status_dict, name, tab)
    #     self.dashboard_mainStatus.setText("PYXIS STATUS: Refreshed")


    """Function to auto update at a given rate"""
    def auto_updater(self):
        # self.refresh_status()
        # self.stimer.singleShot(refresh_time, self.auto_updater)
        #Edited by Qianhui to prevent the GUI from freezing
        self.stimer.timeout.connect(self.refresh_status)
        self.stimer.start(refresh_time)
        return

    """ Function to connect to the FSM. """#Edited by Qianhui to actually connect to the FSM server
    def connect_fsm(self):
        self.fsm_socket.send_command("")
        return

    """ Button to start or kill all servers. CURRENTLY DOES NOTHING"""
    # def power(self):
    #     if not self.fsm_socket.connected:
    #         self.response_label.append("FSM is not connected. Cannot reboot.")
    #         return
    #     else:
    #         if self.power_button.isChecked():
    #             response = self.fsm_socket.send_command("start_pyxis")
    #             self.response_label.append(f"Response: {response}")
    #             self.power_button.setText("Kill Servers")
    #             self.power_button.setStyleSheet("QPushButton {background-color: #550000; border-color: #550000; color: #ffd740}")
    #         else:
    #             response = self.fsm_socket.send_command("stop_pyxis")
    #             self.response_label.append(f"Response: {response}")
    #             self.power_button.setText("Start Servers")
    #             self.power_button.setStyleSheet("QPushButton {background-color: #005500; border-color: #005500; color: #ffd740}")
    #         return

    """ Send a command to the FSM server """
    # def send_to_FSM_server(self, text):
    #     try:
    #         response = self.fsm_socket.send_command(text)
    #     except:
    #         response = "*** Connection Error ***"
    #     if type(response)==str:
    #         self.response_label.append(response)
    #     elif type(response)==bool:
    #         if response:
    #             self.response_label.append("Success!")
    #         else:
    #             self.response_label.append("Failure!")
    #     self.line_edit.setText("")

    """ Parse a command and send it to the FSM server"""
    def command_enter(self):
        self.fsm_socket.send_command(str(self.line_edit.text()))
        self.line_edit.setText("")

    # """What happens when you click the status button"""
    def status_click(self):
        try:
            fsm_status_dict = self.get_status_from_fsm()
            #print out the status in a readable format
            navis_dict = {k: v for k, v in fsm_status_dict.items() if "Navis" in k or "CoarseMet" in k}
            self.response_label.append("-"*10  + "Navis" + "-"*10)
            self.response_label.append(str(navis_dict))

            dextra_dict = {k: v for k, v in fsm_status_dict.items() if "Dextra" in k and "CoarseMet" not in k}
            self.response_label.append("-"*10  + "Dextra" + "-"*10)
            self.response_label.append(str(dextra_dict))

            sinistra_dict = {k: v for k, v in fsm_status_dict.items() if "Sinistra" in k and "CoarseMet" not in k}
            self.response_label.append("-"*10  + "Sinistra" + "-"*10)
            self.response_label.append(str(sinistra_dict))
        except Exception as e:
            self.response_label.append(f"Error getting status: {str(e)}")


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
qpix = qpix.scaledToWidth(80)
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
# vbox.addWidget(ip_connect)

# Add Internal IP info
ip_frame_int = QFrame()
vbox = QVBoxLayout()
ip_frame_int.setLayout(vbox)
IPs = collections.OrderedDict({k: IPs[k] for k in ["FSM","Navis","Dextra","Sinistra"]})
for IP_name in IPs:
	IP = IPs[IP_name]
	ip_connect = QLabel('%s LAN IP: %s'%(IP_name,IP))
	ip_connect.setStyleSheet("font-weight: bold; color: #ffd740; font-size:14px")
	# vbox.addWidget(ip_connect)

# Button to switch between IPs
# if pyxis_config["IP"]["UseExternal"]:
# 	IP_button = QPushButton("Connect to Internal IP")
# 	IP_button.setChecked(True)
# else:
# 	IP_button = QPushButton("Connect to External IP")
# IP_button.setFixedWidth(220)
# IP_button.setCheckable(True)
# IP_button.setStyleSheet("QPushButton {background-color: #000000; border-color: #550000; color: #ffd740}")

# header.addWidget(ip_frame_int)
# header.addWidget(ip_frame_ext)
# header.addSpacing(70)
# header.addWidget(IP_button)

ip_frame_ext.hide()

pyxis_app = PyxisGui(pyx_IPs=IP_dict, use_external=pyxis_config["IP"]["UseExternal"])

""" Function to change between internal and external IPs"""
# def change_IP():

#     if IP_button.isChecked():
#         print("Connecting to External IP")
#         IP_button.setStyleSheet("QPushButton {background-color: #550000; border-color: #550000; color: #ffd740}")
#         IP_button.setText("Connect to Internal IP")
#         ip_frame_int.hide()
#         ip_frame_ext.show()
#         pyxis_app.change_IPs(IP_dict["External"])


#     else:
#         print("Connecting to Internal IP")
#         IP_button.setStyleSheet("QPushButton {background-color: #550000; border-color: #550000; color: #ffd740}")
#         IP_button.setText("Connect to External IP")
#         ip_frame_ext.hide()
#         ip_frame_int.show()
#         pyxis_app.change_IPs(IP_dict["Internal"])

# IP_button.clicked.connect(change_IP)
vbox = QVBoxLayout(main)

vbox.addWidget(logo_wig)
vbox.addWidget(pyxis_app)

main.setWindowTitle("Pyxis Control")
icon_path = os.path.join(os.path.dirname(__file__), 'assets', 'telescope.jpg')
if os.path.exists(icon_path):
    app.setWindowIcon(QIcon(icon_path))

main.show()

sys.exit(app.exec_())
