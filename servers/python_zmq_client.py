import zmq

context = zmq.Context()

#  Socket to talk to server
print("Connecting to hello world server…")
socket = context.socket(zmq.REQ)
socket.connect("tcp://192.168.1.3:4103")

def sendcmd(cmd):
    socket.send_string(cmd)
    return socket.recv()
