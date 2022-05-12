#!/usr/bin/env python
from __future__ import print_function, division
import sys
import socket
import struct
import numpy as np

try:
    import zmq
except:
    print("Please install zmq, e.g. with 'pip install --user zmq' if you don't have sudo privliges.")
    raise UserWarning

#Some constants. Data types for Ian's communication protocol, and the servers
#we'll be commecting to.
DTYPES = {1:float, 2:int, 3:str, 4:bool, 5:"floatimg", 6:"intimg"}

class ClientSocket:
    def __init__(self,IP="127.0.0.1",Port="44010"):
        """A socket
        """
        ADS = (IP,Port)
        self.count=0
        try:
            self.context = zmq.Context()
            self.client = self.context.socket(zmq.REQ)
            tcpstring = "tcp://"+IP+":"+Port
            self.client.connect(tcpstring)
            self.client.RCVTIMEO = 20000
            self.connected=True
        except:
            print('ERROR: Could not connect to server. Please check that the server is running and IP is correct.')
            self.connected=False

    def send_command(self, command):
        """Send a command"""

        #If we aren't connected and the user pressed <Enter>, just try to reconnect
        if (self.connected==False) and (len(command)==0):
            try:
                response = self.client.recv()
            except:
                self.count += 1
                return "Could not receive buffered response - connection still lost ({0:d} times).".format(self.count)
            self.connected=True
            return "Connection re-established!"

        #Send a command to the client.
        try:
            self.client.send_string(command,zmq.NOBLOCK)
        except:
            self.connected=False
            self.count += 1
            return 'Error sending command, connection lost ({0:d} times).'.format(self.count)

        #Receive the response
        try:
            response = self.client.recv()
        except:
            self.connected=False
            self.count += 1
            return 'Error receiving response, connection lost ({0:d} times)\nPress Enter to reconnect.'.format(self.count)
        try:
            self.connected=True
            #Lets see what data type we have, and support all relevant ones.
            if len(response) > 4:
                data_type = struct.unpack("<I", response[:4])[0]
            if DTYPES[data_type]==str:
                str_response = response[4:].decode(encoding='utf-8')
                return str_response
            if DTYPES[data_type]==bool:
                bool_response = struct.unpack("<I", response[4:8])
                return bool(bool_response)
            elif DTYPES[data_type]=="intimg":
                #For an integer image, data starts with the number of rows and
                #columns, the time of exposure then the exposure time (in s)
                if len(response) > 28:
                    rows_cols = struct.unpack("<II", response[4:12])
                    times = struct.unpack("dd", response[12:28])
                npix = rows_cols[0]*rows_cols[1]
                if len(response) < 28+npix*2:
                    return 'Not enough pixels to unpack!'
                data = struct.unpack("<{:d}H".format(npix), response[28:28+npix*2])
                return [rows_cols, times, np.array(data).reshape(rows_cols)]
            else:
                return 'Unsupported response type'
        except:
            return 'Error parsing response!'
