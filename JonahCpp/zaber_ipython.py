from zaber_motion import Library, Units
from zaber_motion.binary import Connection,BinarySettings

Library.enable_device_db_store()

connection = Connection.open_serial_port("/dev/ttyS2")
device_list = connection.detect_devices()
print("Found {} devices".format(len(device_list)))
device = device_list[0]
settings = device.settings

settings.set(BinarySettings.TARGET_SPEED,1,Units.VELOCITY_MILLIMETRES_PER_SECOND)

def set_speed(device, value):
    settings2 = device.settings
    settings.set(BinarySettings.TARGET_SPEED,value,Units.VELOCITY_MICROMETRES_PER_SECOND)
