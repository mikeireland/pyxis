"""A small Python implementation of the Pyxis commander socket interface."""

from __future__ import annotations

import inspect
import json
import threading
from collections.abc import Callable
from typing import Any, get_type_hints

import zmq


class commander:
    """Register Python callables and expose them through a Commander REP socket."""

    def __init__(self, socket_number: int):
        self.socket_number = socket_number
        self._commands: dict[str, tuple[Callable[..., Any], str]] = {}
        self._context = zmq.Context()
        self._socket = self._context.socket(zmq.REP)
        self._socket.bind(f"tcp://*:{socket_number}")
        self._running = threading.Event()
        self._running.set()
        self._thread = threading.Thread(target=self.run, daemon=True)

        self.def_("help", self.help, "Get the help message.")
        self.def_("command_names", self.command_names, "Get all command names.")
        self.def_("description", self.description, "Get a command description.")
        self.def_("signature", self.signature, "Get a command signature.")
        self.def_("arguments", self.arguments, "Get command arguments.")
        self.def_("return_type", self.return_type, "Get a command return type.")
        self._thread.start()

    def def_(self, name: str, function: Callable[..., Any], description: str = "") -> commander:
        """Register *function* under *name*, matching ``commander::Module::def``."""
        if name in self._commands:
            raise ValueError(f"Command already registered: {name}")
        if not callable(function):
            raise TypeError("Registered command must be callable")
        self._commands[name] = (function, description)
        return self

    def help(self) -> str:
        return "".join(
            f"{name}: {description}\n"
            for name, (_, description) in self._commands.items()
        )

    def command_names(self) -> list[str]:
        return list(self._commands)

    def description(self, name: str) -> str:
        return self._commands[name][1]

    def signature(self, name: str) -> dict[str, Any]:
        return {
            "arguments": self.arguments(name),
            "return_type": self.return_type(name),
        }

    def arguments(self, name: str) -> list[dict[str, Any]]:
        function = self._commands[name][0]
        signature = inspect.signature(function)
        type_hints = get_type_hints(function)
        arguments = []
        for parameter in signature.parameters.values():
            argument = {
                "name": parameter.name,
                "type": self._type_name(type_hints.get(parameter.name, parameter.annotation)),
            }
            if parameter.default is not inspect.Parameter.empty:
                argument["default_value"] = parameter.default
            arguments.append(argument)
        return arguments

    def return_type(self, name: str) -> str:
        function = self._commands[name][0]
        type_hints = get_type_hints(function)
        return self._type_name(type_hints.get("return", inspect.signature(function).return_annotation))

    def execute(self, name: str, arguments: list[Any]) -> Any:
        try:
            function = self._commands[name][0]
            return function(*arguments)
        except Exception as error:
            return {"error": str(error)}

    def run(self) -> None:
        while self._running.is_set():
            try:
                request = self._socket.recv_string()
                name, separator, payload = request.partition(" ")
                if name == "exit":
                    self._socket.send_string("Exiting!")
                    self._running.clear()
                    continue
                arguments = [] if not separator else json.loads(f"[{payload}]")
                self._socket.send_string(json.dumps(self.execute(name, arguments)))
            except Exception as error:
                self._socket.send_string(f"Error: {error}")
        self._socket.close()
        self._context.term()

    def close(self) -> None:
        """Stop the socket server and release its ZeroMQ resources."""
        if not self._running.is_set():
            return
        self._running.clear()
        context = zmq.Context()
        socket = context.socket(zmq.REQ)
        socket.setsockopt(zmq.LINGER, 0)
        socket.connect(f"tcp://127.0.0.1:{self.socket_number}")
        socket.send_string("exit")
        socket.recv_string()
        socket.close()
        context.term()
        self._thread.join()

    @staticmethod
    def _type_name(annotation: Any) -> str:
        if annotation is inspect.Signature.empty:
            return "void"
        if annotation is None:
            return "None"
        if isinstance(annotation, type):
            return annotation.__name__
        return str(annotation)