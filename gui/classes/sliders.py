# A prelininary attempt to implement two classes that create sliders
# that mimic the behavior of MEL's intSliderGrp and floatSliderGrp. 
# Malcolm Kesson
# 28 July 2020

try:
	from PyQt5.QtCore import *
	from PyQt5.QtGui import *
	from PyQt5.QtWidgets import *
except ImportError:
	from PySide2.QtGui import *
	from PySide2.QtWidgets import *
	from PySide2.QtCore import *
	from PySide2.QtUiTools import *
import math

#______________________________________________________________
class IntSlider(QWidget):
	def __init__(self, title, min, max, def_value, parent_layout, label_width=100):
		super(IntSlider, self).__init__(None)
		label = QLabel()
		label.setText(title)
		label.setFixedWidth(label_width)
	
		self.spinbox = QSpinBox()
		self.spinbox.setRange(min, max)
		self.spinbox.setValue(def_value)
		
		self.slider  = QSlider(Qt.Horizontal)
		self.slider.setRange(min, max)
		self.slider.setValue(def_value)
			
		self.slider.valueChanged.connect(self.update_spinbox)
		self.spinbox.valueChanged.connect(self.update_slider)
		hlayout = QHBoxLayout()	
		hlayout.addWidget(label)
		hlayout.addWidget(self.spinbox)
		hlayout.addWidget(self.slider)
		parent_layout.addLayout(hlayout)
		
	def update_spinbox(self):
		value = self.slider.value()
		self.spinbox.setValue( value )
	def update_slider(self):
		value = self.spinbox.value()
		self.slider.setValue( value )
	def getValue(self):
		return self.spinbox.value() 

#______________________________________________________________


class FloatSlider(QWidget):
	def __init__(self, title, min, max, def_value, parent_layout, label_widt=100, decimal_places=2):
		super(FloatSlider, self).__init__(None)
		self.decimal_conversion = math.pow(10,decimal_places)
		self.min = min
		self.max = max
		
		label = QLabel()
		label.setText(title)
		label.setFixedWidth(label_widt)
		
		self.spinbox = QDoubleSpinBox()
		self.spinbox.setRange(min, max)
		self.spinbox.setValue(def_value)
		self.spinbox.setDecimals(decimal_places)
		
		self.slider  = QSlider(Qt.Horizontal)
		adjusted_max = (max - min) * self.decimal_conversion
		adjusted_max = int(round(adjusted_max))
		self.slider.setRange(0, adjusted_max)
		
		self.slider.setValue( (def_value - min) * self.decimal_conversion)
			
		self.slider.valueChanged.connect(self.update_spinbox)
		self.spinbox.valueChanged.connect(self.update_slider)
		hlayout = QHBoxLayout()	
		hlayout.addWidget(label)
		hlayout.addWidget(self.spinbox)
		hlayout.addWidget(self.slider)
		parent_layout.addLayout(hlayout)
		
	def update_spinbox(self):
		value = self.slider.value()
		value = float(value)/self.decimal_conversion
		self.spinbox.setValue(value + self.min)
		
	def update_slider(self):
		value = self.spinbox.value()
		value = (value - self.min) * self.decimal_conversion
		self.slider.setValue( int(round(value)) )
		
	def getValue(self):
		return self.spinbox.value()
#______________________________________________________________

