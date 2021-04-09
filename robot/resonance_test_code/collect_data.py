import serial
from os.path import dirname, abspath
from datetime import datetime

# Configuration variables
# ===============================================
# target: 
#       'Wheel0'   'Wheel1'   'Wheel2'
#       'Linear0'  'Linear1'  'Linear2'
#       'x_axis'   'y_axis'   'z_axis'
#       'pitch'    'roll'     'yaw'

target = 'z_axis'
mode = '-'
# ===============================================

# Choose the path and name for the results file (including the time to prevent overwriting)
dt_string = datetime.now().strftime("%Y-%m-%d_%H%M%S")
filename = dirname(dirname(abspath(__file__))) + '/resonance_test_results/' + target + '_' + dt_string + '.log'

print('Sending sweep test command ...')
with serial.Serial('/dev/ttyACM0', timeout=2) as ser:
    msg = {
        'Wheel0': '0',  'Wheel1': '1',  'Wheel2': '2',
        'Linear0': '3', 'Linear1': '4', 'Linear2': '5',
        'x_axis': 'x',  'y_axis': 'y',  'z_axis': 'z',
        'pitch': 'p',   'roll': 'r',    'yaw': 's',
    }[target]
    ser.write((mode+msg).encode('UTF-  8'))
        
print('Collecting data ...')

# Collect everything from the serial interface (sent from the robot)
# until the done ('&') character is sent.
# Then dump the output in a file.
# The 'plot_data.py' script can parse and process these data.
with open(filename, "w") as file:
    with serial.Serial('/dev/ttyACM0', timeout=5) as ser:
        while True:
            x=ser.read(20000)
            words = x.decode('UTF-  8')
            if '&' in words: #Finished flag. Don't write this.
                file.write(words.replace('&',''))
                break
            file.write(words)
print('Done')
print('Data saved to: ' + filename)
