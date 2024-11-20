import time
import subprocess
import os
from ZynqController.ZynqTCPClient import ZynqTCPClient
from ZynqController.SynaccessClient import SynaccessClient

class ZynqStarter:
    def __init__(self, zynq_host, zynq_port, synaccess_host_zynq, synaccess_host_coil):
        self.zynq_host = zynq_host
        self.zynq_port = zynq_port
        self.synaccess_host_zynq = synaccess_host_zynq
        self.synaccess_host_coil = synaccess_host_coil

    def ping(self, target, timeout=5):
        """Ping a target and return whether it is reachable."""
        try:
            result = subprocess.run(
                ['ping', '-n', '1', target],  # Use '-n' for Windows or '-c' for Unix-based systems
                capture_output=True,
                text=True,
                timeout=timeout
            )
            if result.returncode == 0:
                return True, result.stdout
            else:
                return False, result.stderr
        except subprocess.TimeoutExpired:
            return False, "Ping command timed out"
        except Exception as e:
            return False, str(e)

    def ping_until_success(self, target, total_timeout=120, ping_timeout=1, ping_interval=1):
        """Ping a target repeatedly until successful or until total timeout is reached."""
        start_time = time.time()
        while time.time() - start_time < total_timeout:
            success, output = self.ping(target, timeout=ping_timeout)
            if success:
                return True, output, time.time() - start_time
            print(f"Retrying {target} after {time.time() - start_time:.2f}s")
            time.sleep(ping_interval)
        return False, f"Total timeout of {total_timeout}s reached", time.time() - start_time

    def reboot_zynq(self):
        """Reboot the Zynq device."""
        try:
            syna_zynq = SynaccessClient(self.synaccess_host_zynq, 23)
            syna_zynq.connect()
            syna_zynq.reboot(outlet_number=1)
        except Exception as e:
            print(f"Error rebooting Zynq: {e}")

    def restart_zynq_control(self):
        """Restart the Zynq control and associated systems."""
        try:
            tcp_zynq = ZynqTCPClient(self.zynq_host, self.zynq_port, timeout=1)
            syna_zynq = SynaccessClient(self.synaccess_host_zynq, 23)
            syna_zynq.connect()

            syna_coil = SynaccessClient(self.synaccess_host_coil, 23)
            syna_coil.connect()

            syna_coil.off(outlet_number=1)
            time.sleep(0.5)

            tcp_zynq.send_message("QUIT")
            time.sleep(0.1)
            syna_zynq.reboot(outlet_number=1, wait_time = 5)

            ping_success, ping_output, time_used = self.ping_until_success(self.zynq_host, total_timeout=180)
            if not ping_success:
                print(f"Failed to ping {self.zynq_host}. Restarting Zynq control...")
                del tcp_zynq
                del syna_zynq
                del syna_coil
                self.restart_zynq_control()
                return
            
            if time_used < 4:
                del tcp_zynq
                del syna_zynq
                del syna_coil
                self.restart_zynq_control()
                return

            # Path to the script to run
            script_directory = r"C:/Chimera/Chimera-Cryo/ExperimentScheduler/ZynqController"
            script_name = "SSHClient.py"
            script_path = os.path.join(script_directory, script_name)
            command = f'python "{script_path}"'

            # Open a new Command Prompt window and run the script
            cmd_command = f'cmd.exe /c "start cmd.exe /c {command}"'
            # powershell_command = f'powershell.exe -NoExit -Command "{command}"' # somehow only cmd works
            print(f"Running command: {cmd_command}")
            subprocess.Popen(cmd_command, shell=True)

            time.sleep(1)
            syna_coil.on(outlet_number=1)

        except Exception as e:
            print(f"Error in restart_zynq_control: {e}")

if __name__ == "__main__":
    # Initialize with appropriate values for your setup
    zynq_host = '10.10.0.2'
    zynq_port = 8080
    synaccess_host_zynq = '10.10.0.100'
    synaccess_host_coil = '10.10.0.101'

    controller = ZynqStarter(zynq_host, zynq_port, synaccess_host_zynq, synaccess_host_coil)
    controller.restart_zynq_control()