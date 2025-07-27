import zmq

context = zmq.Context()

#  Socket to talk to server
print("Connecting to hello world server…")
socket = context.socket(zmq.REQ)
socket.connect("tcp://192.168.1.5:4300")

def s(cmd):
    socket.send_string(cmd)
    return socket.recv()
