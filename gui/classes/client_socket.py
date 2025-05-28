#!/usr/bin/env python
from __future__ import print_function, division
import time

try:
    import zmq
except:
    print("Please install zmq, e.g. with 'pip install --user zmq' if you don't have sudo privliges.")
    raise UserWarning

class ClientSocket:
    def __init__(self,IP="127.0.0.1",Port="44010"):
        """A socket"""
        self.count=0

        try:
            self.context = zmq.Context()
            self.client = self.context.socket(zmq.REQ)
            self.tcpstring = "tcp://"+IP+":"+Port 
            print(self.tcpstring)   
            self.client.connect(self.tcpstring)
            self.client.RCVTIMEO = 5000
            self.connected=True
        except:
            print('ERROR: Could not connect to server. Please check that the server is running and IP is correct.')
            self.connected=False

    def send_command(self, command):
        """Send a command"""
        #If we aren't connected and the user pressed <Enter>, just try to reconnect
        if (self.connected==False) and ((len(command)==0) or (len(command.split(".")[1])==0)):
            try:
                self.client = self.context.socket(zmq.REQ)
                self.client.connect(self.tcpstring)
                self.client.RCVTIMEO = 5000
                self.client.send_string(command,zmq.NOBLOCK)
                self.client.recv()
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
            then = time.time()
            response = self.client.recv_string()
            dt = time.time()-then 
            if (dt > 0.5):
                print(f"Long response time: {dt:.1f}s")
            if response == "success":
                response = True
            elif response == "fail":
                response = False

            return response
        except:
            self.connected=False
            self.count += 1
            return 'Error receiving response, connection lost ({0:d} times)\nPress Enter to reconnect.'.format(self.count)