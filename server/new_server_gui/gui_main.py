#!/usr/bin/env python
from __future__ import print_function, division
import sys
import pytomlpp
import collections
import importlib

#Import only what we need from PyQt5, or everything from PyQt4. In any case, we'll try
#to keep this back-compatible. Although this floods the namespace somewhat, everything
#starts with a "Q" so there is little chance of getting mixed up.
sys.path.insert(0, './classes')

try:
    from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QHBoxLayout, \
        QVBoxLayout, QGridLayout, QLabel, QTabWidget, QScrollArea
    from PyQt5.QtCore import QTimer
    from PyQt5.QtGui import QPixmap, QIcon
    from PyQt5.QtSvg import QSvgWidget
except:
    print("Please install PyQt5.")
    raise UserWarning

from qt_material import apply_stylesheet

#A hack to allow unicode as a type from python 2 to work in python 3.
#It would be better to typecast everything to str
if (sys.version_info > (3, 0)):
    unicode = str

#Load config file
if len(sys.argv) > 1:
    config = pytomlpp.load(sys.argv[1])
else:
    config = pytomlpp.load("gui_ports_setup.toml")

pyxis_config = config["Pyxis"]
config.pop("Pyxis")
config = collections.OrderedDict({k: config[k] for k in ["Navis","Dextra","Sinistra"]})

#Time to automatically update the gui
refresh_time = pyxis_config["refresh_time"]*1000 #now in ms

def debug_trace():
  '''Set a tracepoint in the Python debugger that works with Qt.

  This is useful for bugshooting in a Gui environment.'''
  try:
    from PyQt4.QtCore import pyqtRemoveInputHook
  except:
    from PyQt5.QtCore import pyqtRemoveInputHook
  from pdb import set_trace
  pyqtRemoveInputHook()
  set_trace()

#Load a module from a string
def class_for_name(module_name, class_name):
    # load the module, will raise ImportError if module cannot be loaded
    m = importlib.import_module(module_name)
    # get the class, will raise AttributeError if class cannot be found
    c = getattr(m, class_name)
    return c


class PyxisGui(QTabWidget):
    def __init__(self, IP='127.0.0.1', parent=None):
        """The Pyxis GUI.

        Parameters
        ----------
        IP: str
            The IP address of the Pyxis server as as string
        """
        super(PyxisGui,self).__init__(parent)

        print("Connecting to IP %s"%IP)

        #We'll have tabs for different servers
        self.resize(900, 550)

        self.tab_widgets = {}
        self.sub_tab_widgets = {}
        self.status_lights = {}
        self.status_texts = {}

        #Dashboard Tab
        self.tab_widgets["dashboard"] = QWidget()
        self.addTab(self.tab_widgets["dashboard"],"Dashboard")

        listBox = QVBoxLayout()
        self.tab_widgets["dashboard"].setLayout(listBox)
        self.dashboard_refresh_button = QPushButton("REFRESH", self)
        self.dashboard_refresh_button.clicked.connect(self.refresh_status)
        listBox.addWidget(self.dashboard_refresh_button)

        #Make Scrollable
        scroll = QScrollArea(self.tab_widgets["dashboard"])
        listBox.addWidget(scroll)
        scroll.setWidgetResizable(True)
        scrollContent = QWidget(scroll)
        scrollLayout = QVBoxLayout()
        scrollContent.setLayout(scrollLayout)

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
                self.sub_tab_widgets[tab][name] = widget_module(sub_config,IP)
                self.tab_widgets[tab].addTab(self.sub_tab_widgets[tab][name],sub_config["tab_name"])

                #Add status indicator to dashboard
                status_layout = QHBoxLayout()
                self.status_lights[tab][name] = QSvgWidget(self.sub_tab_widgets[tab][name].status_light)
                self.status_lights[tab][name].setFixedSize(25,25)
                self.status_texts[tab][name] = QLabel(self.sub_tab_widgets[tab][name].status_text, self)
                status_layout.addWidget(self.status_lights[tab][name])
                status_layout.addWidget(self.status_texts[tab][name])

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

        #Now show everything, and start status timers.
        self.setWindowTitle("Pyxis Server Gui")

        self.stimer = QTimer()

        self.auto_updater()

    #Function to refresh the status of all clients
    def refresh_status(self):
        for tab in config:
            for item in config[tab]:
                sub_config = config[tab][item]
                name = sub_config["name"]
                self.sub_tab_widgets[tab][name].ask_for_status()
                self.status_lights[tab][name].load(self.sub_tab_widgets[tab][name].status_light)
                self.status_texts[tab][name].setText(self.sub_tab_widgets[tab][name].status_text)

    #Function to refresh the camera feeds of each relevant client
    def refresh_camera_feeds(self):
        for tab in config:
            for item in config[tab]:
                sub_config = config[tab][item]
                if sub_config["module_type"] == "CameraWidget":
                    name = sub_config["name"]
                    self.sub_tab_widgets[tab][name].refresh_camera_feed()

    #Function to auto update at a given rate
    def auto_updater(self):
        self.refresh_status()
        self.refresh_camera_feeds()
        self.stimer.singleShot(refresh_time, self.auto_updater)


app = QApplication(sys.argv)
app.setStyle("Fusion")
apply_stylesheet(app, theme='dark_amber.xml')  #Design file

main = QWidget()
main.resize(1150, 750)

#Add logo
logo_wig = QWidget()
hbox = QHBoxLayout(logo_wig)
logo = QLabel()
qpix = QPixmap('assets/Pyxis_logo.png')
qpix = qpix.scaledToWidth(250)
logo.setPixmap(qpix)

ip_connect = QLabel('Connecting to server IP: %s'%pyxis_config["server_ip"])
ip_connect.setStyleSheet("font-weight: bold; color: #ffd740; font-size:14px")

hbox.addWidget(logo)
hbox.addWidget(ip_connect)

pyxis_app = PyxisGui(IP=pyxis_config["server_ip"])

vbox = QVBoxLayout(main)

vbox.addWidget(logo_wig)
vbox.addWidget(pyxis_app)

main.setWindowTitle("Pyxis Control")
app.setWindowIcon(QIcon('assets/telescope.jpg'))

main.show()

sys.exit(app.exec_())
