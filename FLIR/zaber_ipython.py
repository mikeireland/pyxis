from zaber_motion import Library, Units
from zaber_motion.binary import Connection

Library.enable_device_db_store()

connection = Connection.open_serial_port("/dev/ttyS2")
device_list = connection.detect_devices()
print("Found {} devices".format(len(device_list)))
device = device_list[0]
