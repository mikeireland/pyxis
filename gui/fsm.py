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

from enum import Enum
class CoarseMetState(Enum):   
    """Enum for Course Metrolog"""
    RESET=0
    FINDING_LEDS=1
    AQUIRING=2 #LEDs can be seen, but outside 1/4 of beam diameter (5mm)
    TRACKING=3 #LEDs are within 1/4 of beam diameter (5mm)
    STOP = 4  # pause the alignment process for whatever reasons

class StarTrackerState(Enum):
    """Enum for Star Tracker"""
    RESET=0
    SLEW_BLIND=1
    SLEW_CLOSE=2
    MONITORING=3
    STOP = 4

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
        # Here we initialize the states of the FSM
        self.dextra_coarse_met_state = CoarseMetState.STOP #I will wait for the user to start the alignment process
        self.sinistra_coarse_met_state = CoarseMetState.STOP #I will wait for the user to start the alignment process
        self.dextra_star_tracker_state = StarTrackerState.STOP
        self.sinistra_star_tracker_state = StarTrackerState.STOP
        self.navis_star_tracker_state = StarTrackerState.STOP

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
            # if reboot_client_name in systemctl_commands:
            #     if call(["systemctl", "is-active", systemctl_commands[reboot_client_name]]) != 0:
            #         call(["systemctl", "restart", systemctl_commands[reboot_client_name]])
            #         return(f"Rebooting {reboot_client_name}. Please wait...")
            #     else:
            #         return(f"Client {reboot_client_name} is already active, no need to restart.")
            # else:
            #     return(f"No systemctl command defined for {reboot_client_name}. Cannot reboot.")
        else:
            return(f"Client {reboot_client_name} not found.")
         

    def start_pyxis(self):
        """start all pyxis.service"""
        if call(["systemctl", "is-active", "pyxis.service"]) != 0:
            call(["systemctl", "start", "pyxis.service"])
            return("Pyxis service started.")
        else:
            return("Pyxis service is already active, no need to start.")

    def stop_pyxis(self):
        """stop all pyxis.service"""
        if call(["systemctl", "is-active", "pyxis.service"]) == 0:
            call(["systemctl", "stop", "pyxis.service"])
            return("Pyxis service stop request sent.")
            
        else:
            return("Pyxis service is deactived, no need to stop.")
        
    def get_LEDs(self, deputyMet_name):
        """Get the positions of the LEDs on the specific deputy"""
        led_positions = {"LED1": "NULL", "LED2": "NULL"}
        try:
            response = self.clients[deputyMet_name].socket.send_command("CM.getLEDs")
            leds = json.loads(response)
            led_positions["LED1"] = (leds["LED1_x"], leds["LED1_y"])
            led_positions["LED2"] = (leds["LED2_x"], leds["LED2_y"])
        except Exception as e:
            print(f"Error getting LED positions: {e} for deputy {deputyMet_name}")
        return led_positions
    

    def _is_zero(self, vec):
        return all(abs(v) < 1e-6 for v in vec.values())
    
    def _exceed_limits(self, dlt_p):
        MAX_MOVE = 50
        """Check if the misalignment exceeds the limits"""
        if abs(dlt_p["x"]) > MAX_MOVE or abs(dlt_p["y"]) > MAX_MOVE:
            print("Y or Z misalignment exceeds safety limits, please check manually.")
            return True
        else:
            return False
        
    def pupil_aquiring(self, deputyMet_name):
        """Start the pupil alignment process"""
        if deputyMet_name == "DextraCoarseMet":
            state_attr = "dextra_coarse_met_state"
            deputyRC_name = "DextraRobotControl"
        else:
            state_attr = "sinistra_coarse_met_state"
            deputyRC_name = "SinistraRobotControl"

        if self.clients[deputyMet_name].socket.connected:
            response = self.clients[deputyMet_name].socket.send_command("CM.getAlignmentError")
            # Parse the JSON response
            try:
                result = json.loads(response)
                alpha_1 = result["alpha_1"]
                alpha_2 = result["alpha_2"]
                dlt_p = result["dlt_p"]
                dlt_p_x = dlt_p["x"]
                dlt_p_y = dlt_p["y"]
            except Exception as e:
                print(f"Error parsing alignment error response: {e} (response: {response})")
                return False

            if abs(dlt_p_x) < 5 and abs(dlt_p_y) < 5:
                print(f"Alignment is within 5mm in {deputyMet_name}. Coarse Metrlogy state is TRACKING now.")
                setattr(self, state_attr, CoarseMetState.TRACKING)
            else:
                setattr(self, state_attr, CoarseMetState.AQUIRING)

            #check if the misalignment exceeds the limits
            if self._exceed_limits(dlt_p):
                return False
            #check if the calculated misalignment is reasonable
            elif self._is_zero(alpha_1) and self._is_zero(alpha_2) and dlt_p_x == -1 and dlt_p_y == -1:
                print(f"Something went wrong in the image:compute_alignment_error in {deputyMet_name}. Cannot proceed")
                return False
            # check if the misalignment is already zero
            elif self._is_zero(alpha_1) and self._is_zero(alpha_2) and self._is_zero(dlt_p):
                print(f"Alignment is already perfect in {deputyMet_name}. No need to adjust.")
                return True
            #Pass the misalignment to the corresponding robot controller
            else:
                if self.clients[deputyRC_name].socket.connected:
                    cmd = f'RC.receive_AlignmentError {dlt_p_x} {dlt_p_y}'
                    self.clients[deputyRC_name].socket.send_command(cmd)
                    print(f"Sending misalignment to {deputyRC_name}. Delta_p: {dlt_p}")
                    return True
                else:
                    print(f"{deputyRC_name} is not connected, cannot send misalignment.")
                    print("Please ensure this deputy is awake and Pyxis is running. Trying to reconnect now.")
                    self.reconnect(deputyRC_name)
                    return False
        else:
            print(f"{deputyMet_name} is not connected, cannot start pupil alignment process.")
            print("Trying to reconnect to the deputy metrology camera.")
            self.reconnect(deputyMet_name)
            return False


    def stop_CMalign(self, deputy_name):
        """Stop the pupil alignment process of the specified deputy"""
        if deputy_name == "Dextra":
            self.dextra_coarse_met_state = CoarseMetState.STOP
            if self.clients["DextraRobotControl"].socket.connected:
                self.clients["DextraRobotControl"].socket.send_command("RC.stop")
                self.clients["DextraRobotControl"].socket.send_command("RC.disconnect")
        elif deputy_name == "Sinistra":
            self.sinistra_coarse_met_state = CoarseMetState.STOP
            if self.clients["SinistraRobotControl"].socket.connected:
                self.clients["SinistraRobotControl"].socket.send_command("RC.stop")
                self.clients["SinistraRobotControl"].socket.send_command("RC.disconnect")
        else:
            print(f"Unknown deputy name: {deputy_name}. Cannot stop alignment process. Choose Dextra or Sinistra.")
        return None

    def start_CMalign(self, deputy_name):
        """Start the pupil alignment process of the specified deputy"""
        if deputy_name == "Dextra":
            self.dextra_coarse_met_state = CoarseMetState.RESET
        elif deputy_name == "Sinistra":
            self.sinistra_coarse_met_state = CoarseMetState.RESET
        else:
            print(f"Unknown deputy name: {deputy_name}. Cannot start alignment process. Choose Dextra or Sinistra.")
        return None

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
                                print(f"Error checking server {client_name}: {e}, response: {response}")
                        elif client.nerrors < 5:
                            # By convention, sending an empty command will try to reconnect
                            client.socket.send_command("")
                            if client.socket.connected:
                                client.isalive = True
                                client.nerrors = 0
                            else:
                                client.nerrors += 1
                                print(f"Server {client_name} is not responding with status, error count: {client.nerrors}")
                        else:
                            client.isalive = False
                            print(f"Server {client_name} is not responding with status, marking as dead after {client.nerrors} errors.")
                    else:
                        # Here we could implement something to automatically try to restart the client.
                        """Jon has implemented this in the systemctl."""
                        # self.reboot(client)
                        pass
            # # Wait until the next clock tick.
            time.sleep(0.01)  # Sleep for a short time to avoid busy waiting
            # time.sleep(max(0, 0.5 - (time.time() - loop_start)))  # Adjust sleep time to maintain a 10Hz loop

            """Deputy Metrology Alignment Process"""
            #Based on our current state and key status items fromthe servers, we can decide to transition to a different state.cm
            # We can only transition between status if we are connected.
            for CoarseMet in ["DextraCoarseMet", "SinistraCoarseMet"]:
                if CoarseMet == "DextraCoarseMet":
                    state_attr = "dextra_coarse_met_state"
                else:
                    state_attr = "sinistra_coarse_met_state"
                CMstate = getattr(self, state_attr)

                if self.clients[CoarseMet].socket.connected and CMstate != CoarseMetState.STOP:
                    if CMstate == CoarseMetState.RESET:
                    # If the DextraCoarseMet is connected, and the camera is running, we can transition to FINDING_LEDS
                        if self.clients[CoarseMet].status == "Camera Waiting":
                            print(f"{CoarseMet} camera is waiting, please start exposure.")
                        elif "Camera Running" in self.clients[CoarseMet].status:
                            # If the camera is running, we can transition to FINDING_LEDS
                            setattr(self, state_attr, CoarseMetState.FINDING_LEDS)
                            print(f"{CoarseMet} state changed to FINDING_LEDS.")
                        else:
                            #The server is not connected to the camera. The user should connect the camera.
                            print(f"{CoarseMet}  is not connected to camera.")
                    elif CMstate == CoarseMetState.FINDING_LEDS:
                        #We read in the LED positions.
                        ledpositions = self.get_LEDs(CoarseMet)
                        led1 = ledpositions["LED1"]
                        led2 = ledpositions["LED2"]

                        # If we have sensible LED positions, we can transition to AQUIRING
                        if "NULL" not in led1 and "NULL" not in led2:
                            if led1[0]>0 and led1[1]>0 and led2[0]>0 and led2[1]>0:
                                setattr(self, state_attr, CoarseMetState.AQUIRING)
                            else:
                                # If the LED positions are not sensible, we can stay in FINDING_LEDS. Given this 
                                # means we are a long way out, we basically wait for the user to move the robot
                                # appropriately (they will know we are in this state on the FSM tab of the GUI)
                                setattr(self, state_attr, CoarseMetState.RESET)
                                print(f"LEDs in {CoarseMet} are not sensible, staying in FINDING_LEDS state.")
                        else:
                            setattr(self, state_attr, CoarseMetState.RESET)
                            print(f"LEDs in {CoarseMet} are not found, returning to RESET state.")

                    elif CMstate == CoarseMetState.AQUIRING or CMstate == CoarseMetState.TRACKING:
                        # Here we operate the pupil alignment process.
                        result = self.pupil_aquiring(f"{CoarseMet}")
                        #If the alignment process was failed, we reset the state to RESET.
                        if not result:
                            setattr(self, state_attr, CoarseMetState.RESET)
                            print(f"Alignment process failed in {CoarseMet}, returning state to RESET.")
                        else:
                            print(f"{CoarseMet} pupil alignment process is running, state is {CMstate}.")
                    
                    time.sleep(0.5)  # Sleep to avoid busy waiting

                elif CMstate != CoarseMetState.STOP:
                    # If the DextraCoarseMet is not connected, we try to connect to it.
                    print(f"{CoarseMet} is not connected to FSM, trying to reconnect.")
                    self.reconnect(CoarseMet)
                    setattr(self, state_attr, CoarseMetState.RESET)
                    time.sleep(0.5)  # Sleep to avoid busy waiting
                else:
                    pass  # If the state is STOP, we do nothing

            if self.clients["NavisStarTracker"].socket.connected and self.navis_star_tracker_state != StarTrackerState.STOP:
                if self.navis_star_tracker_state == StarTrackerState.RESET:
                    # If the NavisStarTracker is connected, and the camera is running, we can transition to SLEW_BLIND
                    if self.clients["NavisStarTracker"].status == "Camera Waiting":
                        print("Navis Star Tracker camera is waiting, please start exposure.")
                    elif "Camera Running" in self.clients["NavisStarTracker"].status:
                        # If the camera is running, we can transition to SLEW_BLIND
                        self.navis_star_tracker_state = StarTrackerState.SLEW_BLIND
                        print("Navis Star Tracker state changed to SLEW_BLIND.")
                    else:
                        #The server is not connected to the camera. The user should connect the camera.
                        print("Navis Star Tracker is not connected to camera.")
                elif self.navis_star_tracker_state == StarTrackerState.SLEW_BLIND:
                    # We can start the slewing process. I don't know what needs to be implemented for slew_blind.
                    #If the target is close to the current position, we can transition to SLEW_CLOSE
                    pass
                elif self.navis_star_tracker_state == StarTrackerState.SLEW_CLOSE:
                    #I still don't know what needs to be implemented for slew_close.
                    #For Navis star tracker, there is no monitoring mode.
                    pass
            elif self.navis_star_tracker_state != StarTrackerState.STOP:
                # If the NavisStarTracker is not connected, we try to connect to it.
                print("Navis Star Tracker is not connected to FSM, trying to reconnect.")
                self.reconnect("NavisStarTracker")
                self.navis_star_tracker_state = StarTrackerState.RESET
                time.sleep(0.5)
            
            

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