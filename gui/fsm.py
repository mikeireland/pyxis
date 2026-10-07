#!/home/pyxisuser/miniconda3/bin/python
"""#!/usr/bin/env python"""
"""
A finite state machine that stores the state of the Pyxis servers, and also
is a server itself. 

All server commands are in the class FSM, which owns the dictionary of clients.

Each client is a very simple class with key properties "name", "IP", "port" and 
booleans for status types. 

*** Note that there is a notation error here: the FSM is a client to the pyxis servers,
so usually you would call the array of servers "servers" and the FSM would be a "client" to them. 
But in this code, we call the pyxis servers "clients", which is confusing... ***
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
    def __init__(self, name, IP, port, prefix, n_state_machines):
        self.name = name
        if self.name.startswith("Navis"):
            self.robot = "Navis"
        elif self.name.startswith("Dextra"):
            self.robot = "Dextra"
        elif self.name.startswith("Sinistra"):
            self.robot = "Sinistra"
        self.IP = IP
        self.port = port
        self.prefix = prefix
        self.status = {}  # Dictionary to hold client status
        self.nerrors = 0  # Number of errors encountered
        self.isalive = True #Assume alive until proven otherwise
        self.socket = ClientSocket(IP=IP, Port=port, TIMEOUT=100, logdir="FSMcommand_log")
        self.previous_log = {} # Dictionary to hold previous logs of client's state machine(s)
        self.n_state_machines = n_state_machines
        if self.n_state_machines > 0:
            for i in range(self.n_state_machines):
                self.previous_log[i] = "" # Initialise dictionary with empty logs (1 per state machine)

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
    SOFT_RESET = 0
    HARD_RESET = 1
    IDLE = 2
    READY_TO_SLEW = 3
    SLEW_BLIND = 4
    SLEW_CLOSE = 5
    CENTROIDING = 6
    STOP = 7

# A little messy, and copied from "Globals.h" in the robot control code.
ST_SERVER_STATE = {
    0: StarTrackerState.IDLE,
    1: StarTrackerState.READY_TO_SLEW,
    2: StarTrackerState.SLEW_BLIND,
    3: StarTrackerState.SLEW_CLOSE,
    4: StarTrackerState.CENTROIDING,
    5: StarTrackerState.SOFT_RESET,
}

class FSM:
    """Finite State Machine class that holds the state of all clients"""
    def __init__(self, port):
        self.clients = {}  # Dictionary to hold client objects
        # Open a zmq server socket for the FSM
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REP)
        self.socket.bind(f"tcp://*:{port}")
        self.logdir = "FSMcommand_log"
        #Set the command dictionary for the FSM, using all methods of the FSM class
        #that do not start with '_'
        self.command_dict = {}
        for name, method in inspect.getmembers(self, predicate=inspect.ismethod):
            if not name.startswith('_'):
                self.command_dict[name] = method
        # Here we initialize the states of the FSM
        self.dextra_coarse_met_state = CoarseMetState.STOP #I will wait for the user to start the alignment process
        self.sinistra_coarse_met_state = CoarseMetState.STOP #I will wait for the user to start the alignment process
        self.star_tracker_states = {
            "Dextra": StarTrackerState.STOP,
            "Sinistra": StarTrackerState.STOP,
            "Navis": StarTrackerState.STOP
        }
        self.status_logs = {
            "Navis": "Navis_status_log.txt",
            "Dextra": "Dextra_status_log.txt",
            "Sinistra": "Sinistra_status_log.txt",
            "Event": "FSM_level_events_log.txt"
        }
        self.prev_st_logs = {
            "Navis": "",
            "Dextra": "",
            "Sinistra": "",
        }
        self.event_logs = {}

    def _add_client(self, name, IP, port, prefix="", n_state_machines = 0):
        """Add a new client to the FSM"""
        self.clients[name] = Client(name, IP, port, prefix, n_state_machines)

    def _remove_client(self, name):
        """Remove a client from the FSM"""
        if name in self.clients:
            del self.clients[name]
            
    def _process_status(self, client_name, status):
        """Process the status of a client and update the FSM state accordingly"""
        client = self.clients[client_name]
        robot  = client.robot
        logfile_path = self.status_logs[robot]
        # Update the StarTrackerState unless we are stopped.
        if self.star_tracker_states[robot] != StarTrackerState.STOP:
            if client.prefix == "RC":
                # Update the FSM state based on the robot control status
                server_state = status.get("st_state", 0)
                if server_state >= 2: #If a state that the robot control transitions to itself.
                    if self.star_tracker_states[robot] != StarTrackerState.SOFT_RESET:
                        self.star_tracker_states[robot] = ST_SERVER_STATE.get(server_state, StarTrackerState.STOP)
                        self._log_fsm_status(logfile_path, "ST", self.star_tracker_states[robot], "Updated from RC server")                 
            elif client.prefix == "PS":
                # Update the FSM state based on the plate solver status
                server_state = status.get("state", 0)
                if server_state == 2 or server_state == 3: # Plate Solver process error or disconnection
                    self.star_tracker_states[robot] = StarTrackerState.SOFT_RESET
                    self._log_fsm_status(logfile_path, "ST", self.star_tracker_states[robot], "Error / Disconnect in PS server")
            elif client.prefix == "FST":
                server_state = status.get("status", 0)
                if server_state == 2: # FST camera error
                    self.star_tracker_states[robot] = StarTrackerState.SOFT_RESET
                    self._log_fsm_status(logfile_path, "ST", self.star_tracker_states[robot], "Camera error in FST server")
    
    def _log_fsm_status(self, logfile_path, machine_id, state, description):
        if self.prev_st_logs[robot] != machine_id + str(state) + description:
            self._log_status_helper(logfile_path, "FSM", machine_id, state, description, {time.strftime('%Y-%m-%dT%H:%M:%S')})
            self.prev_st_logs[robot] = machine_id + str(state) + description
    
    def _log_client_status(self, client_name, status):
        """
        Log client state transitions to the relevant robot's state record.
        Each new state transition should be logged only once.
        """
        client = self.clients[client_name]
        logfile_path = self.status_logs[client.robot]
        if client.prefix == "RC":
            RC_ST_index = status.get("st_state", 0) # Default to ST_IDLE
            state_RC_ST = ST_SERVER_STATE(RC_ST_index)
            state_RC_GSS = status.get("loop_status", 1) # Default to ROBOT_IDLE
            if self._update_client_prev_log(client, 0, state_RC_ST):
                self._log_status_helper(logfile_path, "RC", "ST", state_RC_ST)
            if self._update_client_prev_log(client, 1, state_RC_GSS):
                self._log_status_helper(logfile_path, "RC", "GSS", state_RC_GSS)
        elif client.prefix == "PS":
            PS_state = status.get("state", 0) # Default to IDLE
            PS_description = status.get("description", "Default")
            PS_transition = status.get("timestamp", "Default")
            if self._update_client_prev_log(client, 0, PS_state, PS_description, PS_transition):
                self._log_status_helper(logfile_path, "PS", "Main", PS_state, PS_description, PS_transition)
        elif client.prefix == "FST":
            FST_state = status.get("status", 0) # Default to PLATE_SOLVING
            FST_description = status.get("description", "")
            if self._update_client_prev_log(client, 0, FST_state, FST_description):
                self._log_status_helper(logfile_path, "FST", "Main", FST_state, FST_description)

    def _update_client_prev_log(self, client, machine_id, state, description = "", transition_time = ""):
        """
        Check if a client status represents a state transition. Return True if so, False otherwise.
        Update client's previous log with the current state for future comparisons.
        """
        if machine_id in client.previous_log:
            current_log = str(state) + description + transition_time
            if client.previous_log[machine_id] != current_log:
                client.previous_log[machine_id] = current_log
                return True
            else:
                return False
        else:
            self._log_event(self.status_logs[client.robot], "error", "ID-Err", f"Error: Client {client.name} not initialised with state machine id {machine_id}.")
            return False

    def _log_status_helper(self, logfile_path, prefix, machine_id, state, description = "", transition_time = ""):
        """
        Log server state transition to a log_file in the format: 
            Time of logging, [info], time of transition (if unique/provided), server prefix, state machine id,  state, desription
        This is a helper function for _log_status()
        The logfile_path variable is the name of the status log within the log directory.
        """
        with open(self.logdir + "/" + logfile_path, "a") as log_file:
            try:
                if transition_time:
                    log_file.write(f"{time.strftime('%Y-%m-%dT%H:%M:%S')}, [info], {prefix}, {machine_id}, {state}, {description}, {transition_time}\n")
                else:
                    log_file.write(f"{time.strftime('%Y-%m-%dT%H:%M:%S')}, [info], {prefix}, {machine_id}, {state}, {description}, [No transition time]\n")
            except:
                log_file.flush()
            log_file.flush()

    def _log_event(self, logfile_path, label, id, description):
        """
        Log [label] entry for an event to a specified file if this is the first time the event has occurred,
        or if more than 5 seconds have passed since the same event was logged.
        """
        with open(self.logdir + "/" + logfile_path, "a") as log_file:
            try:
                log_file.write(f"{time.strftime('%Y-%m-%dT%H:%M:%S')}, [{label}], {id}, {description}\n")
            except:
                log_file.flush()
            log_file.flush()

    def hello(self, name):
        """A simple test command to check if the FSM is working"""
        return f"Hello, {name}! The FSM is working."
    
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
        MAX_MOVE = 1000 #in unit of mm
        """Check if the misalignment exceeds the limits"""
        if abs(dlt_p["x"]) > MAX_MOVE or abs(dlt_p["y"]) > MAX_MOVE:
            print("Y or Z misalignment exceeds safety limits (1 meter), please check manually.")
            return True
        else:
            return False
        
    def pupil_aquiring(self, deputyMet_name):
        """Start the pupil alignment process by passing misalignment between Coarse Metrology and Robot Control"""
        if deputyMet_name == "DextraCoarseMet":
            state_attr = "dextra_coarse_met_state"
            deputyRC_name = "DextraRobotControl"
            sub_config = config['Navis']['DextraCoarseMet']
            sign = -1 #Dextra CoarseMetCam is rotated 90 degree clockwise
        else:
            state_attr = "sinistra_coarse_met_state"
            deputyRC_name = "SinistraRobotControl"
            sub_config = config['Navis']['SinistraCoarseMet']
            sign = +1 #Sinistra CoarseMetCam is rotated 90 degree counter-clockwise
        beta, gamma, x0, alpha_c = sub_config["beta"], sub_config["gamma"], sub_config["x0"], sub_config["alpha_c"]
        print(f"x0: {x0}, alpha_c: {alpha_c}")
        x0_json = json.dumps({"x": x0[0], "y": x0[1]})
        alpha_c_json = json.dumps({"x": alpha_c[0], "y": alpha_c[1]})
        print(f"x0_json: {x0_json}, alpha_c_json: {alpha_c_json}")

        if self.clients[deputyMet_name].socket.connected:
            cmd = f'CM.getAlignmentError {beta}, {gamma}, {x0_json}, {alpha_c_json}'
            response = self.clients[deputyMet_name].socket.send_command(cmd)
            print("Sending command: "+cmd)
            # Parse the JSON response
            try:
                result = json.loads(response)
                alpha_1 = result["alpha_1"]
                alpha_2 = result["alpha_2"]
                dlt_p = result["dlt_p"]
                v_offset = dlt_p["x"] # vertical offset
                h_offset = dlt_p["y"] # horizontal offset
            except Exception as e:
                print(f"Error parsing alignment error response: {e} (response: {response})")
                return False

            if abs(v_offset) < 5 and abs(h_offset) < 5:
                print(f"Alignment is within 5mm in {deputyMet_name}. Coarse Metrlogy state is TRACKING now.")
                setattr(self, state_attr, CoarseMetState.TRACKING)
            else:
                setattr(self, state_attr, CoarseMetState.AQUIRING)

            #check if the misalignment exceeds the limits
            if self._exceed_limits(dlt_p):
                return False
            #check if the calculated misalignment is reasonable
            elif self._is_zero(alpha_1) and self._is_zero(alpha_2) and v_offset == -1 and h_offset == -1:
                print(f"Something went wrong in the image:compute_alignment_error in {deputyMet_name}. Cannot proceed")
                return False
            # check if the misalignment is already zero
            elif self._is_zero(alpha_1) and self._is_zero(alpha_2) and self._is_zero(dlt_p):
                print(f"Alignment is already perfect in {deputyMet_name}. No need to adjust.")
                return True
            
            #Pass the misalignment to the corresponding robot controller
            else:
                if self.clients[deputyRC_name].socket.connected:
                    cmd = f'RC.receive_AlignmentError {sign * v_offset}, {h_offset}'
                    response = self.clients[deputyRC_name].socket.send_command(cmd)
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
                response = self.clients["DextraRobotControl"].socket.send_command("RC.stop")
        elif deputy_name == "Sinistra":
            self.sinistra_coarse_met_state = CoarseMetState.STOP
            if self.clients["SinistraRobotControl"].socket.connected:
                response = self.clients["SinistraRobotControl"].socket.send_command("RC.stop")
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

    def start_STprocess(self, robot):
        """Start the ST&PO process on the specified robot. Can be used to exit a STOP state."""
        print("Initiating start_STprocess for robot:", robot)
        if robot in ["Navis", "Dextra", "Sinistra"]:
            logfile_path = self.status_logs[robot]
            self.star_tracker_states[robot] = StarTrackerState.SOFT_RESET
            self._log_fsm_status(logfile_path, "ST", self.star_tracker_states[robot], f"Start {robot}")
        else:
            print(f"Unknown platform name: {robot}. Cannot start ST process.")
        return None

    def stop_STprocess(self, robot):
        """Stop the ST&PO process on the specified robot"""
        if robot in ["Navis", "Dextra", "Sinistra"]:
            logfile_path = self.status_logs[robot]
            self.star_tracker_states[robot] = StarTrackerState.STOP
            self._log_fsm_status(logfile_path, "ST", self.star_tracker_states[robot], f"Stop {robot}")
        else: 
            print(f"Unknown platform name: {robot}. Cannot stop ST process.")
            return None

        RC_name = robot + "RobotControl"
        PS_name = robot + "PlateSolver"
        if self.clients[RC_name].socket.connected:
            response = self.clients[RC_name].socket.send_command("RC.stop")        # RC GSS to ROBOT_TRANSLATE
            response = self.clients[RC_name].socket.send_command("RC.set_st 0")    # RC ST to ST_IDLE
        if self.clients[PS_name].socket.connected:
            response = self.clients[PS_name].socket.send_command("PS.set_ps_st 0") # Plate Solver to IDLE

    def _test_logs(self):
        """
        Test update links and client logging functions.
        """
        # Generic log_event (should result in 2 new entries in FSM_level_events; with appropriate timestamps)
        self._log_event(self.status_logs["Event"],"test","Test1","Test single log_event function (1)")
        self._log_event(self.status_logs["Event"],"test","Test2","Test single log_event function (2)")

        # FSM logging
        # Should result in 2 new entries in Dextra_status_log.txt:
        # timestamp, [info], FSM, 0, StarTrackerState.STOP, "Test state logged (STOP)", timestamp
        # timestamp + ~2 seconds, [info], FSM, 1, CoarseMetState.STOP, "Test state logged (STOP)", timestamp + ~2 seconds
        # Should result in 1 new entry in Sinistra_status_log.txt:
        # timestamp + ~3 seconds, [info], FSM, 0, StarTrackerState.STOP, "Test state logged (STOP)", timestamp + ~3 seconds
        self._log_fsm_status(self.status_logs["Dextra"],"ST",StarTrackerState.STOP, "Test state logged (STOP)")
        time.sleep(1)
        self._log_fsm_status(self.status_logs["Dextra"],"ST",StarTrackerState.STOP, "Test state logged (STOP)")
        time.sleep(1)
        self._log_fsm_status(self.status_logs["Dextra"],"CM",CoarseMetState.STOP, "Test state logged (STOP)")
        time.sleep(1)
        self._log_fsm_status(self.status_logs["Sinistra"],"ST",StarTrackerState.STOP, "Test state logged (STOP)")

        # FSM-RC and RC-FSM state update links
        # Should result in 3 logs:
        # timestamp, [info], RC, 0, 0, , [no transition time]
        # timestamp, [info], RC, 1, 2, , [no transition time]
        # timestamp + ~4 seconds, [info], RC, 0, 5, , [no transition time]
        RC_name = "NavisRobotControl"
        RC_client = self.clients[RC_name]
        if RC_client.socket.connected:
            for _ in range(3):
                response = RC_client.socket.send_command("RC.stop")
                time.sleep(1)
                self._process_status(RC_name, self.clients[RC_name].status)
                self._log_client_status(RC_name, self.clients[RC_name].status)
            response = RC_client.socket.send_command("RC.set_st 5") # Set to ST_ERROR
            time.sleep(1)
            self._process_status(RC_name, self.clients[RC_name].status)
            self._log_client_status(RC_name, self.clients[RC_name].status)
            response = RC_client.socket.send_command("RC.stop")
        
        # FSM-PS and PS-FSM state update links
        # Assuming PS is connected, should result in 2 new logs in Navis_status_log.txt:
        # timestamp, [info], PS, 0, 0, , timestamp
        # timestamp + ~4 seconds, [info], PS, 0, 3, , timestamp + ~4 seconds
        PS_name = "NavisPlateSolver"
        PS_client = self.clients[PS_name]
        if PS_client.socket.connected:
            for _ in range(3):
                response = PS_client.socket.send_command("PS.set_ps_st 0")  # Sets PS to IDLE
                time.sleep(1)
                self._process_status(PS_name, self.clients[PS_name].status)
                self._log_client_status(PS_name, self.clients[PS_name].status)
            response = PS_client.socket.send_command("PS.set_ps_st 3")
            time.sleep(1)
            self._process_status(PS_name, self.clients[PS_name].status)
            self._log_client_status(PS_name, self.clients[PS_name].status)

        # FSM-FST and FST-FSM state update links
        FST_name = "NavisStarTracker"
        FST_client = self.clients[FST_name]
        if FST_client.socket.connected:
            for _ in range(3):
                response = FST_client.socket.send_command("FST.switchPlateSolve")
                print(response)
                self._log_client_status(FST_name, self.clients[FST_name].status)
                time.sleep(1)
               

    def _run(self):
        """Run the FSM server, listening for commands, and checking on clients
        one at a time """
        self.keepgoing = True
        check_interval = 0.5  # Check status every 0.5 seconds
        error_threshold = 2  # Number of errors before marking a client as dead
        while self.keepgoing:
            now = time.time()
            # Record the start time, as we want to run this at 2 Hz maximum.
            # loop_start = time.time()
            
            # Check for incoming commands to the FSM server socket, i.e. commands from the GUI.
            # We use zmq.NOBLOCK to avoid blocking the loop if no command is received.
            try:
                message = self.socket.recv_string(flags=zmq.NOBLOCK)
                self._log_event(self.status_logs["Event"],"info","Cmd-Rec",f"Received command: {message}")

                # Handle empty message as a connection ping
                if not message.strip():
                    self.socket.send_string("CONNECT_FSM")
                    continue

                command, *args = message.split()
                if command in self.command_dict:
                    response = self.command_dict[command](*args)
                    self.socket.send_string(str(response))
                else:
                    self.socket.send_string(f"Unknown command: {command}")
            except zmq.Again:
                pass
            except zmq.ZMQError as error:
                error_message = f"ZeroMQ error {error.errno}: {error}"
                print(error_message)
                self._log_event(self.status_logs["Event"], "error", "Cmd-ZMQ-Err", error_message)
                try:
                    self.socket.send_string("Error processing command")
                except zmq.ZMQError as reply_error:
                    reply_error_message = f"ZeroMQ error sending reply {reply_error.errno}: {reply_error}"
                    print(reply_error_message)
                    self._log_event(self.status_logs["Event"], "error", "Cmd-Reply-ZMQ-Err", reply_error_message)
            except Exception as error:
                error_message = f"Error processing command: {error}"
                print(error_message)
                self._log_event(self.status_logs["Event"], "error", "Cmd-Err", error_message)
                self.socket.send_string("Error processing command")
            
            #Now check the status of one client at a time
            for client_name, client in self.clients.items():
                if now - client.last_check_time > check_interval:
                    client.last_check_time = now  # Update the last check time
                    if client.isalive:
                        if client.socket.connected:
                            try:
                                #Check if the client is alive by sending a status command.
                                #We expect a json structure as a response.
                                #The client_socket will handle the connection and disconnection.
                                response = client.socket.send_command(client.prefix + ".status")
                                client.status = json.loads(response)
                                self._log_client_status(client_name, client.status)
                                self._process_status(client_name, client.status)
                            except Exception as e:
                                self._log_event(self.status_logs["Event"], "error", "Proc-Err", f"Error checking server {client_name}: {e}, response: {response}")
                        elif client.nerrors < error_threshold:
                            # By convention, sending an empty command will try to reconnect. Automatically 
                            # reconecting like this is part of the "lazy pirate" pattern.
                            response = client.socket.send_command("")
                            if client.socket.connected:
                                client.isalive = True
                                client.nerrors = 0
                            else:
                                client.nerrors += 1
                                self._log_event(self.status_logs["Event"], "error", "Proc-Err", f"Server {client_name} is not responding with status, error count: {client.nerrors}")
                        else:
                            client.isalive = False
                            self._log_event(self.status_logs["Event"], "error", "Proc-Err", f"Server {client_name} is not responding with status, marking as dead after {client.nerrors} errors.")
                    else:
                        # Here we could implement something to automatically try to restart the client.
                        """Jon has implemented this in the systemctl. !!!TODO """
                        # self.reboot(client)
                        pass
            # # Wait until the next clock tick.
            time.sleep(0.01)  # Sleep for a short time to avoid busy waiting
            # time.sleep(max(0, 0.5 - (time.time() - loop_start)))  # Adjust sleep time to maintain a 10Hz loop

            """Deputy Metrology Alignment Process"""
            #Based on our current state and key status items from the servers, we can decide to transition to a different state.cm
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
                    
                    time.sleep(0.01)  # Sleep to avoid busy waiting

                elif CMstate != CoarseMetState.STOP:
                    # If the DextraCoarseMet is not connected, we try to connect to it.
                    print(f"{CoarseMet} is not connected to FSM, trying to reconnect.")
                    self.reconnect(CoarseMet)
                    setattr(self, state_attr, CoarseMetState.RESET)
                    time.sleep(0.01)  # Sleep to avoid busy waiting
                else:
                    pass  # If the state is STOP, we do nothing
            """ Star Tracker State Transitions """
            # Loop through each robot, and direct the respective robot controller to
            # make the appropriate state transitions based on the FSM state and the server status. 
            for robot in ["Navis","Dextra","Sinistra"]:
                ST_camera = robot + "StarTracker"
                RC_name   = robot + "RobotControl"
                PS_name   = robot + "PlateSolver"
                logfile_path = self.status_logs[robot]
                ST_state  = self.star_tracker_states.get(robot, StarTrackerState.STOP)
                # Relevant robot control and camera must be connected
                # The STOP state would be typically externally triggered by the user.
                if ST_state != StarTrackerState.STOP:
                    # If not any server connected, we must RESET the connection.
                    if not self.clients[RC_name].socket.connected or not self.clients[ST_camera].socket.connected or not self.clients[PS_name].socket.connected:
                        self.star_tracker_states[robot] = StarTrackerState.SOFT_RESET
                        self._log_fsm_status(logfile_path, "ST", self.star_tracker_states[robot], "FSM-Server Disconnection")
                    
                    if ST_state == StarTrackerState.READY_TO_SLEW: # TODO: Should this only happen once? What if overrides RC ST transition to ST_SLEW_BLIND?
                        # Set FST state if dealing with Navis
                        if robot == "Navis":
                            response = self.clients[ST_camera].socket.send_command("FST.switchPlateSolve")
                            if response == "Switched to Plate Solving Mode": # TODO: Is this the only correct scenario?
                                response = self.clients[PS_name].socket.send_command("PS.set_ps_st 1") # Sets PS to RUNNING
                                response = self.clients[RC_name].socket.send_command("RC.track")       # Sets RC GSS to ROBOT_TRACK
                                response = self.clients[RC_name].socket.send_command("RC.set_st 1")    # Sets RC ST to READY_TO_SLEW
                        else:
                            response = self.clients[PS_name].socket.send_command("PS.set_ps_st 1") # Sets PS to RUNNING
                            response = self.clients[RC_name].socket.send_command("RC.track")       # Sets RC GSS to ROBOT_TRACK
                            response = self.clients[RC_name].socket.send_command("RC.set_st 1")    # Sets RC ST to READY_TO_SLEW
                    elif ST_state == StarTrackerState.CENTROIDING:
                        response = self.clients[PS_name].socket.send_command("PS.set_ps_st 0")  # Sets PS to IDLE
                    elif ST_state == StarTrackerState.SOFT_RESET:
                        # If connected to robot, stop all offset correction
                        if self.clients[RC_name].socket.connected:
                            response = self.clients[RC_name].socket.send_command("RC.set_st 0") #ST_IDLE
                            response = self.clients[RC_name].socket.send_command("RC.stop")     #ROBOT_TRANSLATE
                        else: # Otherwise, reconnect to server
                            self.reconnect(RC_name)

                        # If connected to plate solver, stop solving operations
                        if self.clients[PS_name].socket.connected:
                            response = self.clients[PS_name].socket.send_command("PS.set_ps_st 0")
                        else:
                            self.reconnect(PS_name)

                        self.reconnect(ST_camera) # Reconnect ST camera server
                        time.sleep(0.01)  # Sleep to avoid busy waiting (copied from CM logic)

                        # Transition to READY_TO_SLEW if all servers connected 
                        if self.clients[RC_name].socket.connected and self.clients[ST_camera].socket.connected and self.clients[PS_name].socket.connected:
                            self.star_tracker_states[robot] = StarTrackerState.READY_TO_SLEW
                            self._log_fsm_status(logfile_path, "ST", self.star_tracker_states[robot], "FSM-Servers (re)connected")
                        # Otherwise trigger a Hardware RESET
                        else:
                            self.star_tracker_states[robot] = StarTrackerState.HARD_RESET
                            self._log_fsm_status(logfile_path, "ST", self.star_tracker_states[robot], "SOFT_RESET failed. HARD_RESET required.")
                    elif ST_state == StarTrackerState.HARD_RESET:
                        # TODO: Do some hardware reset things
                        self.star_tracker_states[robot] = StarTrackerState.SOFT_RESET
                        self._log_fsm_status(logfile_path, "ST", self.star_tracker_states[robot], "HARD_RESET completed.")
                else: # STOP state
                    # Stop RC motion (if server connection exists)
                    if self.clients[RC_name].socket.connected:
                        response = self.clients[RC_name].socket.send_command("RC.set_st 0") #ST_IDLE
                        response = self.clients[RC_name].socket.send_command("RC.stop")     #ROBOT_TRANSLATE
                    else: # Otherwise, reconnect to server
                        self.reconnect(RC_name)

                    # Stop PS operations (if server connection exists)
                    if self.clients[PS_name].socket.connected:
                        response = self.clients[PS_name].socket.send_command("PS.set_ps_st 0")  # Sets PS to IDLE
                    else:  # Otherwise, reconnect to server
                        self.reconnect(PS_name)

                time.sleep(0.05)
            
#Initialise the FSM with the port from the config
fsm = FSM(pyxis_config['IP']['FSM_port'])

if pyxis_config["IP"]["UseExternal"]:
    use_external = True 
else:
    use_external = False 
# FSM is only run on the local machine so it always connects to other servers via internal IP   
for robot in config:
    #For each sub tab
    for item in config[robot]:
        sub_config = config[robot][item]
        if use_external:
            IP = pyxis_config["IP"]["External"]
        else:
            IP = pyxis_config["IP"][robot]
        fsm._add_client(sub_config["name"], IP, sub_config["port"], prefix = sub_config["prefix"], n_state_machines=sub_config["n_state_machines"])

if __name__ == "__main__":
    # Print the FSM clients for debugging
    for client_name, client in fsm.clients.items():
        print(f"Client: {client_name}, IP: {client.IP}, Port: {client.port}, Alive: {client.isalive}, Connected: {client.socket.connected}")
        client.last_check_time = 0  # Initialize last check time for each client
        client.nerrors = 0  # Initialize error count for each client
    # # Example of how to access a specific client
    # if "NavisRobotControl" in fsm.clients:
    #     navis_robot_control = fsm.clients["NavisRobotControl"]
    #     print(f"Navis Robot Control - IP: {navis_robot_control.IP}, Port: {navis_robot_control.port}")
        
    # Start the FSM server
    fsm._run()

    # Run logging test
    #fsm._test_logs()
    
    # After a "quit" command, we close the socket and exit
    fsm.socket.close()
    fsm.context.term()
