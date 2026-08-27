"""Lazy Pirate-style ZeroMQ REQ client."""

import threading

import zmq


class LazyPirateClient:
    def __init__(self, context, endpoint, timeout_ms=1000):
        self.context = context
        self.endpoint = endpoint
        self.timeout_ms = timeout_ms
        self.socket = None
        self.connected = False
        self.lock = threading.Lock()

    def request(self, command):
        with self.lock:
            if self.socket is None:
                self.socket = self.context.socket(zmq.REQ)
                self.socket.setsockopt(zmq.LINGER, 0)
                self.socket.setsockopt(zmq.RCVTIMEO, self.timeout_ms)
                self.socket.connect(self.endpoint)
            try:
                self.socket.send_string(command)
                response = self.socket.recv_string()
                self.connected = True
                return response
            except zmq.ZMQError:
                self._disconnect()
                return None

    def _disconnect(self):
        self.connected = False
        if self.socket is not None:
            self.socket.close()
            self.socket = None

    def status(self):
        with self.lock:
            return self.connected