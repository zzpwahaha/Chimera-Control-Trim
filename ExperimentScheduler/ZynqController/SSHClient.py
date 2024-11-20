import paramiko
import time

class SSHClient:
    def __init__(self, hostname, port, username, password):
        """
        Initializes the SSHClient with connection parameters.
        :param hostname: The hostname or IP address of the SSH server.
        :param port: The port number for the SSH connection (default is 22).
        :param username: The username for the SSH connection.
        :param password: The password for the SSH connection.
        """
        self.hostname = hostname
        self.port = port
        self.username = username
        self.password = password
        self.ssh = paramiko.SSHClient()
        self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    
    def connect(self):
        """Establishes the SSH connection."""
        try:
            self.ssh.connect(self.hostname, port=self.port, username=self.username, password=self.password)
        except paramiko.SSHException as e:
            print(f"SSH connection failed: {e}")
            raise

    def execute_chained_commands(self, commands):
        """
        Executes multiple commands in a single shell session, streaming their output.

        :param commands: A list of commands to execute on the remote server.
        """
        if not self.ssh.get_transport().is_active():
            raise RuntimeError("SSH connection is not active. Please connect first.")
        
        # Combine commands into a single command string
        combined_command = ' && '.join(commands)
        print(f"Executing combined command: {combined_command}")
        
        # Execute the combined command
        stdin, stdout, stderr = self.ssh.exec_command(f'bash -c "{combined_command}"')
        
        # Stream the output efficiently
        print("Streaming output:")
        for line in iter(stdout.readline, ""):
            print(line, end='')  # Print each line of output as it arrives
            if line.strip() == "QUIT":
                break
        
        # Check for errors
        error = stderr.read().decode('utf-8')
        if error:
            print(f"Error: {error}")

    def close(self):
        """Closes the SSH connection."""
        self.ssh.close()

    def __del__(self):
        self.close()


if __name__ == "__main__":
    # Example usage
    hostname = "10.10.0.2"
    port = 22
    username = "root"
    password = "root"
    commands = ["cd regalTcp/", "python3 zynq_tcp_server.py"]
    
    ssh_client = SSHClient(hostname, port, username, password)
    try:
        ssh_client.connect()
        ssh_client.execute_chained_commands(commands)
    finally:
        ssh_client.close()

