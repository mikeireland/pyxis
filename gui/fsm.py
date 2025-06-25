#!/home/pyxisuser/miniconda3/bin/python
"""#!/usr/bin/env python"""
"""
A finite state machine that stores the state of the Pyxis servers, and also
is a server itself. 

All server commands are in the class FSM, which owns the dictionary of clients.

Each client is a very simple class with key properties "name", "IP", "port" and 
booleans for status types.
"""
import sys
import pytomlpp
import collections
import zmq
import json
import inspect
import time
from subprocess import call

print("Running with Python:", sys.executable)

sys.path.insert(0, './classes')
from classes.client_socket import ClientSocket

#Load config file
if len(sys.argv) > 1:
    config = pytomlpp.load(sys.argv[1])
else:
    config = pytomlpp.load("port_setup.toml")

#The overall config, that include the FSM_port
pyxis_config = config["Pyxis"]

#The config for all the clients, which give the port.
config.pop("Pyxis")
config = collections.OrderedDict({k: {key: value for key, value in sorted(config[k].items(), key=lambda x:x[1]["port"])}  for k in ["Navis","Dextra","Sinistra"]})

#Edited by Qianhui: define systemctl command dictionary, e.g., NavisRobotControl: pyxis-robot.
systemctl_commands = {
    "NavisRobotControl": "pyxis-robot",
    "NavisDeputyAux": "pyxis-auxillary",
    "DextraCoarseMet": "pyxis-dextra-met",
    "SinistraCoarseMet": "pyxis-sinistra-met",
    "NavisFiberInjection": "pyxis-fiber-injection",
    "NavisStarTracker": "pyxis-fst",
    "NavisScienceCam": "pyxis-science-camera"
}

#Define our client class
class Client:
    def __init__(self, name, IP, port, prefix):
        self.name = name
        self.IP = IP
        self.port = port
        self.prefix = prefix
        self.status = {}  # Dictionary to hold client status
        self.nerrors = 0  # Number of errors encountered
        self.isalive = True #Assume alive until proven otherwise
        self.socket = ClientSocket(IP=IP, Port=port, TIMEOUT=100, logdir="FSMcommand_log")

    def __repr__(self):
        return f"Client(name={self.name}, IP={self.IP}, port={self.port})"

class FSM:
    """Finite State Machine class that holds the state of all clients"""
    def __init__(self, port):
        self.clients = {}  # Dictionary to hold client objects
        # Open a zmq server socket for the FSM
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REP)
        self.socket.bind(f"tcp://*:{port}")
        #Set the command dictionary for the FSM, using all methods of the FSM class
        #that do not start with '_'
        self.command_dict = {}
        for name, method in inspect.getmembers(self, predicate=inspect.ismethod):
            if not name.startswith('_'):
                self.command_dict[name] = method

    def _add_client(self, name, IP, port, prefix=""):
        """Add a new client to the FSM"""
        self.clients[name] = Client(name, IP, port, prefix)

    def _remove_client(self, name):
        """Remove a client from the FSM"""
        if name in self.clients:
            del self.clients[name]
    
    def help(self):
        """Return a dictionary of available commands and their descriptions"""
        help_dict = {}
        for command, func in self.command_dict.items():
            help_dict[command] = func.__doc__ or "No description available"
        return help_dict
    
    def quit(self):
        """Quit the FSM server"""  
        self.keepgoing = False
    
    def status(self):
        """Return the operational status of all clients"""
        status_dict = {}
        for client_name, client in self.clients.items():
            status_dict[client_name] = {
                "isalive": client.isalive,
                "connected": client.socket.connected
            }
        return status_dict
    
    def reconnect(self, client_name):
        """Reconnect a specific client by name"""
        try: 
            client_name = str(json.loads(client_name))
        except json.JSONDecodeError:
            return "Invalid client name format. Please provide a valid JSON string."
        if client_name in self.clients:
            client = self.clients[client_name]
            # Set nerrors to 0, and the _run loop will reconnect.
            client.isalive = True
            client.nerrors = 0
        else:
            return f"Client {client_name} not found."

    """Reboot a specific server by name"""
    def reboot(self, reboot_client_name):
        if reboot_client_name in fsm.clients:
            reboot_client = fsm.clients[reboot_client_name]
            reboot_client.isalive = True  # Reset the client's alive status, assuming reboot was successful
            reboot_client.socket.connected = True  # Reset the connection status
            reboot_client.nerrors = 0
            # Here we can implement the actual reboot command, e.g., using systemctl
            if reboot_client_name in systemctl_commands:
                if call(["systemctl", "is-active", systemctl_commands[reboot_client_name]]) != 0:
                    call(["systemctl", "restart", systemctl_commands[reboot_client_name]])
                    return(f"Rebooting {reboot_client_name}. Please wait...")
                else:
                    return(f"Client {reboot_client_name} is already active, no need to restart.")
            else:
                return(f"No systemctl command defined for {reboot_client_name}. Cannot reboot.")
        else:
            return(f"Client {reboot_client_name} not found.")
         

    #start all pyxis.service
    def start_pyxis(self):
        if call(["systemctl", "is-active", "pyxis.service"]) != 0:
            call(["systemctl", "start", "pyxis.service"])
            return("Pyxis service started.")
        else:
            return("Pyxis service is already active, no need to start.")

    #stop all pyxis.service
    def stop_pyxis(self):
        if call(["systemctl", "is-active", "pyxis.service"]) == 0:
            call(["systemctl", "stop", "pyxis.service"])
            return("Pyxis service stop request sent.")
            
        else:
            return("Pyxis service is deactived, no need to stop.")
    
    def _run(self):
        """Run the FSM server, listening for commands, and checking on clients
        one at a time """
        self.keepgoing = True
        last_status_check = time.time()
        while self.keepgoing:
            # Record the start time, as we want to run this at 2 Hz maximum.
            # loop_start = time.time()
            
            # Check for incoming commands from the FSM socket
            # We use zmq.NOBLOCK to avoid blocking the loop if no command is received.
            try:
                message = self.socket.recv_string(flags=zmq.NOBLOCK)
                print(f"Received command: {message}")
                command, *args = message.split()
                if command in self.command_dict:
                    response = self.command_dict[command](*args)
                    self.socket.send_string(str(response))
                else:
                    self.socket.send_string(f"Unknown command: {command}")
            except zmq.Again:
                pass
            except:
                print("Error processing command, sending error response.")
                self.socket.send_string("Error processing command")
            
            #Now check the status of one client at a time
            if time.time() - last_status_check > 0.5:
                for client_name, client in self.clients.items():
                    if client.isalive:
                        if client.socket.connected:
                            try:
                                #Check if the client is alive by sending a status command.
                                #We expect a json structure as a response.
                                #The client_socket will handle the connection and disconnection.
                                response = client.socket.send_command(client.prefix + ".status")
                                client.status = json.loads(response) 
                            except Exception as e:
                                print(f"Error checking client {client_name}: {e}, response: {response}")
                        elif client.nerrors < 5:
                            # By convention, sending an empty command will try to reconnect
                            client.socket.send_command("")
                            if client.socket.connected:
                                client.isalive = True
                                client.nerrors = 0
                            else:
                                client.nerrors += 1
                                print(f"Client {client_name} is not responding with status, error count: {client.nerrors}")
                        else:
                            client.isalive = False
                            print(f"Client {client_name} is not responding with status, marking as dead after {client.nerrors} errors.")
                    else:
                        # Here we could implement something to automatically try to restart the client.
                        """Jon has implemented this in the systemctl."""
                        # self.reboot(client)
                        pass
            # # Wait until the next clock tick.
            time.sleep(0.01)  # Sleep for a short time to avoid busy waiting
            # time.sleep(max(0, 0.5 - (time.time() - loop_start)))  # Adjust sleep time to maintain a 10Hz loop
            

#Initialise the FSM with the port from the config
fsm = FSM(pyxis_config['IP']['FSM_port'])

if pyxis_config["IP"]["UseExternal"]:
    use_external = True 
else:
    use_external = False    
for robot in config:
    #For each sub tab
    for item in config[robot]:
        sub_config = config[robot][item]
        if use_external:
            IP = pyxis_config["IP"]["External"]
        else:
            IP = pyxis_config["IP"][robot]
        fsm._add_client(sub_config["name"], IP, sub_config["port"], prefix = sub_config["prefix"])

if __name__ == "__main__":
    # Print the FSM clients for debugging
    for client_name, client in fsm.clients.items():
        print(f"Client: {client_name}, IP: {client.IP}, Port: {client.port}, Alive: {client.isalive}, Connected: {client.socket.connected}")
    
    # # Example of how to access a specific client
    # if "NavisRobotControl" in fsm.clients:
    #     navis_robot_control = fsm.clients["NavisRobotControl"]
    #     print(f"Navis Robot Control - IP: {navis_robot_control.IP}, Port: {navis_robot_control.port}")
        
    # Start the FSM server
    fsm._run()
    
    # After a "quit" command, we close the socket and exit
    fsm.socket.close()
    fsm.context.term()