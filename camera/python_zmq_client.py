import zmq
import cmd

context = zmq.Context()

#  Socket to talk to server
print("Connecting to hello world server…")
socket = context.socket(zmq.REQ)
socket.connect("tcp://192.168.1.4:4001")

#  Do 10 requests, waiting each time for a response
while(1):

    request = input("Server request: ")

    print("Sending request %s …" % request)
    socket.send_string(request)

    #  Get the reply.
    message = socket.recv()
    print("Received reply %s [ %s ]" % (request, message))
