import socket
import sys
from time import sleep
from ZynqController.TCPClient import TCPClient


class ZynqTCPClient(TCPClient):
    def __init__(self, host, port, timeout=10):
        super().__init__(host, port, timeout)
        self.connectByteLen = 64
        self.dioByteLen = 28
        self.dacByteLen = 42
        self.ddsByteLen = 46
        super().connect()

    def _sendMessage(self, message : bytes, length):
        messagePad = message.ljust(length, b'\0')
        super().send_message(messagePad)


if __name__ == "__main__":
    HOST = '10.10.0.2'
    PORT = 8080
    client = ZynqTCPClient(HOST, PORT)
    client.send_message("QUIT");
    # sleep(1)