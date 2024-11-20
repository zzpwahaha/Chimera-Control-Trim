import socket
from typing import Union

class TCPClient:
    def __init__(self, host, port, timeout=10):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.sock = None

    def connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(self.timeout)  # Set the connection timeout
            self.sock.connect((self.host, self.port))
            print(f"Connected to {self.host}:{self.port}")
        except socket.timeout:
            print(f"Connection timed out after {self.timeout} seconds")
        except Exception as e:
            print(f"Failed to connect: {e}")

    def send_message(self, message : Union[str, bytes]):
        if self.sock:
            try:
                if isinstance(message, str):
                    message = message.encode('utf-8')  # Convert string to bytes
                elif not isinstance(message, bytes):
                    raise TypeError("Message must be a string or bytes")
                
                self.sock.sendall(message)
                print("Message sent.")
            except Exception as e:
                print(f"Failed to send message: {e}")
        else:
            print("Not connected to server.")

    def read_lite(self, bufsize: int = 4096) -> bytes:
        self.sock.settimeout(10)  # Set the timeout in seconds
        if self.sock:
            try:
                data = self.sock.recv(bufsize)
                # return data.decode('utf-8')
                return data
                # data = b""
                # while True:
                #     part = self.sock.recv(bufsize)  # Receive data in chunks
                #     if not part:
                #         break
                #     data += part
                # return data
            except Exception as e:
                print(f"Failed to read data: {e}")
                return ""
        else:
            print("Not connected to server.")
            return ""

    def read_heavy(self, bufsize: int = 4096, timeout=0.5) -> bytes:
        self.sock.settimeout(timeout)  # Set the timeout in seconds
        data = b""
        if self.sock:
            try:
                # data = self.sock.recv(bufsize)
                # # return data.decode('utf-8')
                # return data
                while True:
                    part = self.sock.recv(bufsize)  # Receive data in chunks
                    if not part:
                        break
                    data += part
            except socket.timeout:
                print("Receiving data timed out.")
            except Exception as e:
                print(f"An error occurred: {e}")
            finally:
                self.sock.settimeout(None)  # Reset timeout to default
            return data
        else:
            print("Not connected to server.")
            return ""

    def read(self, bufsize: int = 4096, timeout=0.5) -> bytes:
        if bufsize<=65536:
            return self.read_lite(bufsize)
        else:
            return self.read_heavy(bufsize, timeout)

    def query(self, message: Union[str, bytes], bufsize: int = 4096) -> bytes:
        if self.sock:
            try:
                self.send_message(message)  # Send the message
                response = self.read(bufsize)  # Read the response
                return response
            except Exception as e:
                print(f"Failed to query server: {e}")
                return ""
        else:
            print("Not connected to server.")
            return ""

    def close(self):
        if self.sock:
            self.sock.close()
            print("Connection closed.")

    def __del__(self):
        self.close()

if __name__ == "__main__":
    # host = input("Enter server host (e.g., 'localhost'): ")
    # port = int(input("Enter server port (e.g., 12345): "))
    HOST = 'localhost'
    PORT = 8888
    host, port = HOST, PORT

    client = TCPClient(host, port)
    client.connect()

    try:
        while True:
            message = input("Enter message to send (or 'exit' to quit): ")
            if message.lower() == 'exit':
                break
            print(client.query(message).decode('utf-8'))
    finally:
        client.close()

# Open-Configuration $CryoTweezerLoading\\tweezerloading.Config
# Open-Master-Script $CryoTweezerLoading\\rydberg_420_1013_excitation.mScript
# Start-Experiment $test
# Save-All
# Abort-Experiment $delete