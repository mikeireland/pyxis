#!/usr/bin/env python
"""
To test:

import client_socket as cs
cl = cs.ClientSocket(IP="150.203.91.206", Port="4100")
"""
from __future__ import print_function, division
import time
import os

try:
    import zmq
except:
    print("Please install zmq, e.g. with 'pip install --user zmq' if you don't have sudo privliges.")
    raise UserWarning

# Default directory is the GUIcommand_log directory in the directory where the script is run.
# This is where the log files will be saved.
import os

class ClientSocket:
    def __init__(self,IP="127.0.0.1",Port="44010",TIMEOUT=1000, logdir=None):
        """A socket"""
        self.count=0
        self.Port = Port
        self.TIMEOUT = TIMEOUT
        if logdir is None:
            logdir = os.path.dirname(os.path.abspath(__file__)) + "/GUIcommand_log"
            if not os.path.exists(logdir):
                os.makedirs(logdir)
        self.logdir = logdir
        try:
            self.context = zmq.Context()
            self.client = self.context.socket(zmq.REQ)
            self.tcpstring = "tcp://"+IP+":"+Port 
            self.client.connect(self.tcpstring)
            self.client.RCVTIMEO = self.TIMEOUT
            self.connected=True
            print('Socket open to server at {0}'.format(self.tcpstring))
        except:
            print('Could not open socket at {0}'.format(self.tcpstring))
            self.connected=False

    def send_command(self, command):
        """Send a command"""
        #If we aren't connected and the user pressed <Enter>, just try to reconnect
        if (self.connected==False):
            if ((len(command)==0) or (len(command.split(".")[1])==0)):
                try:
                    self.client = self.context.socket(zmq.REQ)
                    self.client.connect(self.tcpstring)
                    self.client.RCVTIMEO = self.TIMEOUT
                    self.client.send_string(command,zmq.NOBLOCK)
                    self.client.recv()
                except:
                    self.count += 1
                    return "Could not receive buffered response - connection still lost ({0:d} times).".format(self.count)
                self.connected=True
                self.log_command("Empty command received, reconnected to server.")
                return "Connection re-established!"
            else:
                self.log_command("Connection lost, but command is not empty. Not reconnecting.")
                return "Connection lost, but command is not empty. Not reconnecting."

        #Send a command to the client.
        try:
            self.client.send_string(command,zmq.NOBLOCK)
            self.log_command(command)
        except:
            self.connected=False
            self.count += 1
            self.log_command("Error sending command, connection lost ({0:d} times)".format(self.count))
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
            self.log_response(response)
            return response
        except:
            self.connected=False
            self.count += 1
            self.log_response("Error receiving response, connection lost ({0:d} times)".format(self.count))
            return 'Error receiving response, connection lost ({0:d} times)\nPress Enter to reconnect.'.format(self.count)
        

        #Edited by Qianhui: log all commands sent to the server with a timestamp
    def log_command(self, command):
        if self.Port.startswith("40"):
            log_file_name = "FSM_log.txt"
        if self.Port.startswith("41"):
            log_file_name = "Navis_log.txt"
        elif self.Port.startswith("42"):
            log_file_name = "Dextra_log.txt"
        elif self.Port.startswith("43"):
            log_file_name = "Sinistra_log.txt"
        with open(self.logdir + "/"+log_file_name, "a") as log_file:
            try:
                log_file.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - Sent: {command}\n")
            except:
                log_file.flush()
            log_file.flush()

    def log_response(self, response):
        """Log the response received from the server."""
        if self.Port.startswith("40"):
            log_file_name = "FSM_log.txt"
        if self.Port.startswith("41"):
            log_file_name = "Navis_log.txt"
        elif self.Port.startswith("42"):
            log_file_name = "Dextra_log.txt"
        elif self.Port.startswith("43"):
            log_file_name = "Sinistra_log.txt"
        with open(self.logdir+ "/"+log_file_name, "a") as log_file:
            if "Image" in response:
                response = "Image received"
            if "Error" in response: #Only log errors responses
                try:
                    log_file.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - Received: {response}\n")
                except:
                    log_file.flush()
            log_file.flush()