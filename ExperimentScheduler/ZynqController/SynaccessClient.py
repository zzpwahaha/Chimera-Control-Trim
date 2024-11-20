import socket
import time
import re  # Import the regular expression module

class SynaccessClient:
    def __init__(self, ip_value, port_value):
        self.host = str(ip_value)
        self.port = int(port_value)
        self.sock = None

    def connect(self):
        """Establishes a connection to the specified host and port."""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            print("Connected to {}:{}".format(self.host, self.port))
        except socket.error as e:
            print(f"Socket error: {e}")

    def _to_bytes(self, input_data):
        """Converts the input data to bytes if it is a string.
        Args:
            input_data (str or bytes): The input data to be converted.
        Returns:
            bytes: The input data in bytes format.
        """
        if isinstance(input_data, str):
            return input_data.encode()  # Convert string to bytes
        elif isinstance(input_data, bytes):
            return input_data  # Return bytes as is
        else:
            raise TypeError("Input must be a string or bytes.")

    def query(self, command : bytes, delay=0.5):
        """Sends a command to the server and receives the response.
        Args:
            command (bytes): The command to send to the server.
            delay (float): Time to wait after sending the command before receiving data.
        Returns:
            str: The response received from the server.
        """
        command = self._to_bytes(command)  # Ensure command is in bytes
        if self.sock:
            try:
                _ = self.sock.recv(2048) # clear the existing recv buffer
                self.sock.send(command)
                time.sleep(delay)
                response = self.sock.recv(2048)
                return response.decode()
            except socket.error as e:
                print(f"Socket error during query: {e}")
                return None
        else:
            print("Socket is not connected.")
            return None

    def close(self):
        """Closes the socket connection."""
        if self.sock:
            self.query(b'logout\r')
            self.sock.close()
            self.sock = None
            print("Connection closed")

    def help(self):
        print(self.query(b'help\r'))


    def __del__(self):
        """Destructor to ensure the socket is closed when the object is destroyed."""
        self.close()

    def set_outlet(self, outlet_number, state):
        """Sets the state of the specified outlet.
        Args:
            outlet_number (int): The number of the outlet to control.
            state (int): The state to set (0 for off, 1 for on).
        Returns:
            str: The response received from the server.
        """
        if outlet_number not in (1,2):
            raise ValueError("Outlet must be 1 or 2.")
        if state not in (0, 1):
            raise ValueError("State must be 0 (off) or 1 (on).")
        
        command = f'pset {outlet_number} {state}\r'
        return self.query(command)

    def on(self, outlet_number):
        self.set_outlet(outlet_number, 1)

    def off(self, outlet_number):
        self.set_outlet(outlet_number, 0)

    def reboot(self, outlet_number, wait_time = 2):
        self.off(outlet_number)
        print(f"Outlet #{outlet_number} status: ",self.get_outlet_status(outlet_number))
        time.sleep(wait_time)
        self.on(outlet_number)
        print(f"Outlet #{outlet_number} status: ",self.get_outlet_status(outlet_number))

    def _status(self):
        return self.query(b'pshow\r')
    
    def get_outlet_status(self, outlet_number):
        """Gets the status of the specified outlet.
        Args:
            outlet_number (int): The number of the outlet to check.
        Returns:
            str: The status of the outlet ("ON" or "OFF"), or an error message if the status cannot be determined.
        """
        # Send a command to get the status of all outlets (replace with appropriate command)
        response = self._status()  # Replace 'status\r' with the actual command to get outlet statuses
        
        if response:
            # Use regular expressions to find the status of the specific outlet
            match = re.search(rf'\b{outlet_number}\s*\|\s*\w+\s*\|\s*(ON|OFF)\s*\|', response)
            if match:
                return match.group(1)  # Return "ON" or "OFF"
            else:
                return "Outlet number not found in response."
        else:
            return "No response from server."

# Example usage:
if __name__ == "__main__":
    # Replace with appropriate IP and port values
    client = SynaccessClient("10.10.0.100", 23)
    client.connect()

    client2 = SynaccessClient("10.10.0.101", 23)
    client2.connect()
    # print(client.query('help\r'))
    # print(client.set_outlet(1,1))
    # print(client.get_outlet_status(1))
    client2.off(outlet_number=1)
    time.sleep(1)
    client.reboot(outlet_number=1)
    time.sleep(1)
    client2.on(outlet_number=1)

    # print(client.get_outlet_status(1))



